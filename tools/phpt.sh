#!/usr/bin/env bash
# phpt.sh <node> <set> - run a prepared .phpt set (tools/phpt.py prepare) on <node>
# with the built PHP.EXE, fetch the results to out/phpt-<set>-<node>.txt and
# judge them (against the host reference, out/phpt-<set>-ref.txt, if present).
#   REF=1 phpt.sh <node> <set>   also run the set with the host reference build
# The set goes to <workdir>.PHPT.<SET>]; files already sent (same content) are
# not sent again.
set -euo pipefail
export LC_ALL=C
top=$(cd "$(dirname "$0")/.." && pwd)
node=${1:?usage: phpt.sh <node> <set>}
set_=${2:?usage: phpt.sh <node> <set>}
. "$top/upstream.conf"
read -r _ _ HOST PORT USER WORKDIR SFTPDIR _ < <(awk -v n="$node" '$1==n' "$top/tools/nodes.conf")
src=$top/cache/phpt/$set_
[ -f "$src/LIST.TXT" ] || { echo "phpt: prepare the set first (tools/phpt.py prepare $set_ ...)" >&2; exit 1; }
mkdir -p "$top/out"

if [ "${REF:-}" = 1 ]; then
    python3 "$top/tools/phpt.py" local "$set_" "$top/cache/refbuild/sapi/cli/php" "$top/out/phpt-$set_-ref.txt"
fi

# upload the set (only files that changed since the last upload)
manifest=$top/cache/pushed-phpt-$set_-$node.sha
touch "$manifest"
# "<sha256>  <path>" per file, the path as raw bytes (sha256sum prints names
# that are not UTF-8 with U+FFFD in place of their bytes, which then named no
# file: PORTING_LOG #31)
(cd "$top/cache/phpt" && python3 -I -c '
import hashlib, os, sys
out = []
for root, dirs, files in os.walk(os.fsencode(sys.argv[1])):
    for f in files:
        p = os.path.join(root, f)
        with open(p, "rb") as fh:
            out.append(hashlib.sha256(fh.read()).hexdigest().encode() + b"  " + p + b"\n")
sys.stdout.buffer.write(b"".join(sorted(out)))' "$set_") > "$manifest.new"
# file names may contain spaces (ext/dom/tests/modern/xml/test foo.xml): the
# path is everything after sha256sum's 64-digit hash and two spaces, and every
# sftp path is quoted (an unquoted one aborted the batch, so the rest of the set
# was never sent)
changed=$(comm -23 <(sort "$manifest.new") <(sort "$manifest") | cut -c67-)
# names that are not UTF-8 (the Windows code-page fixtures in
# ext/standard/tests/file/windows_mb_path, for Windows-only tests) cannot go
# through sftp: they are left out, and out of the manifest
bad=$(printf '%s\n' "$changed" | python3 -I -c '
import sys
for l in sys.stdin.buffer:
    try: l.decode("utf-8")
    except UnicodeDecodeError: sys.stdout.buffer.write(l)')
if [ -n "$bad" ]; then
    echo "phpt: not sending $(printf '%s\n' "$bad" | grep -c .) files whose names are not UTF-8"
    changed=$(comm -23 <(printf '%s\n' "$changed" | sort) <(printf '%s\n' "$bad" | sort))
    awk 'NR==FNR { skip[$0] = 1; next } !(substr($0, 67) in skip)' <(printf '%s\n' "$bad") "$manifest.new" > "$manifest.new2"
    mv "$manifest.new2" "$manifest.new"
fi
batch=$top/cache/phpt-$set_.sftp
{
    echo "cd $SFTPDIR"
    echo "-mkdir phpt"
    (cd "$top/cache/phpt" && find "$set_" -type d) | python3 -I -c '
import sys
for l in sys.stdin.buffer:
    try: sys.stdout.write("-mkdir \"phpt/%s\"\n" % l.decode("utf-8").rstrip("\n"))
    except UnicodeDecodeError: pass'
    while IFS= read -r f; do
        [ -n "$f" ] && printf 'put "%s" "%s"\n' "$top/cache/phpt/$f" "phpt/$f"
    done <<< "$changed"
} > "$batch"
echo "phpt: uploading $(echo "$changed" | grep -c . || true) files"
# record the upload only if sftp finished the batch: otherwise the next run
# would take the unsent files for sent
rc=0
sftp -P "$PORT" -i "${VMS_SSH_KEY:-$HOME/.ssh/vms_ed25519}" -o BatchMode=yes -b "$batch" "$USER@$HOST" \
    2>"$batch.err" >/dev/null || rc=$?
grep -vE '^ *Welcome to|^ *$|^remote mkdir .*Failure' "$batch.err" >&2 || true
if [ "$rc" -ne 0 ]; then
    rm -f "$manifest.new"
    echo "phpt: upload failed; nothing recorded as sent" >&2
    exit 1
fi
mv "$manifest.new" "$manifest"

remote=$(echo "$UPSTREAM_NAME-$UPSTREAM_VERSION" | tr . _ | tr a-z A-Z)
settop="${WORKDIR%]}.PHPT.$(echo "$set_" | tr a-z A-Z)]"
job=$top/cache/phpt-$set_-$node.com
cat > "$job" <<DCL
\$ set noon
\$ @${WORKDIR%]}]VMS_PHPT.COM $settop ${WORKDIR%]}.$remote.OBJ_X86_64]PHP.EXE ${WORKDIR%]}]PHPT_$(echo "$set_" | tr a-z A-Z).TXT
DCL
"$top/tools/vms.sh" "$node" put "$top/tools/vms_phpt.com" >/dev/null
VMS_TIMEOUT=${VMS_TIMEOUT:-28800} "$top/tools/vms.sh" "$node" run "$job" | grep -E 'PHPT-DONE|%' | tail -3
"$top/tools/vms.sh" "$node" get "PHPT_$(echo "$set_" | tr a-z A-Z).TXT" "$top/out/phpt-$set_-$node.txt" >/dev/null
ref=$top/out/phpt-$set_-ref.txt
python3 "$top/tools/phpt.py" compare "$set_" "$top/out/phpt-$set_-$node.txt" $([ -f "$ref" ] && echo "$ref")
