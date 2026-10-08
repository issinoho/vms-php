#!/usr/bin/env bash
# push.sh <node> - upload the prepared tree (staging/) to <node>'s work directory.
#
# Uploads what the VMS build needs into <workdir>/<name>-<version with dots as
# underscores>, [.PHP-8_5_11]: the sources and headers of main, Zend, TSRM, the
# CLI and CGI SAPIs and the extensions we build (DECISIONS D3), and vmsport/.
# Left behind: tests, extensions we do not build, PHP's bundled PCRE2 (D4) and
# the opcache JIT.  Only files whose content changed since the last push to the
# node are sent (cache/pushed-*.sha); build.sh purges old versions on VMS.
set -euo pipefail
export LC_ALL=C   # sort and comm must agree on collation

top=$(cd "$(dirname "$0")/.." && pwd)
node=${1:?usage: push.sh <node>}
. "$top/upstream.conf"
name=$UPSTREAM_NAME-$UPSTREAM_VERSION
stage=$top/staging/$name
remote=$(echo "$name" | tr . _)
[ -f "$stage/vmsport/build/objects.txt" ] || { echo "push: run tools/prepare.sh first" >&2; exit 1; }

exts="date libxml openssl pcre zlib ctype dom filter hash json lexbor mbstring mysqli
mysqlnd opcache random reflection session simplexml spl standard tokenizer uri xml
xmlreader xmlwriter"
dirs="main Zend TSRM sapi/cli sapi/cgi"
for e in $exts; do dirs="$dirs ext/$e"; done

pack=$top/cache/push-$name-$node
rm -rf "$pack"; mkdir -p "$pack/$remote"
( cd "$stage"
  # shellcheck disable=SC2086
  find $dirs \( -path '*/tests' -o -path 'ext/pcre/pcre2lib' -o -path 'ext/opcache/jit' \) -prune -o \
       -type f \( -name '*.c' -o -name '*.h' -o -name '*.S' -o -name '*.inc' -o -name '*.def' -o -name '*.macros' \) -print
  find vmsport -type f -print ) > "$pack.files"
( cd "$stage" && tar -cf - -T "$pack.files" ) | ( cd "$pack/$remote" && tar -xf - )

echo "push: -> $node:[.$(echo "$remote" | tr a-z A-Z)] ($(wc -l < "$pack.files") files)"
read -r _ _ HOST PORT USER _ SFTPDIR _ < <(awk -v n="$node" '$1==n' "$top/tools/nodes.conf")
manifest=$top/cache/pushed-$name-$node.sha
[ "${PUSH_ALL:-}" = 1 ] && rm -f "$manifest"
touch "$manifest"
(cd "$pack" && find "$remote" -type f -print0 | sort -z | xargs -0 sha256sum) > "$pack.sha"
changed=$(comm -23 <(awk '{print $2" "$1}' "$pack.sha" | sort) \
                   <(awk '{print $2" "$1}' "$manifest" | sort) | awk '{print $1}')
echo "push: $(echo "$changed" | grep -c . || true) changed of $(wc -l < "$pack.sha") files"
batch=$pack/sftp.batch
{
    echo "cd $SFTPDIR"
    (cd "$pack" && find "$remote" -type d) | sed 's/^/-mkdir /'
    for f in $changed; do echo "put $pack/$f $f"; done
} > "$batch"
sftp -P "$PORT" -i "${VMS_SSH_KEY:-$HOME/.ssh/vms_ed25519}" -o BatchMode=yes -b "$batch" "$USER@$HOST" \
    2>&1 >/dev/null | grep -vE '^ *Welcome to|^ *$|^remote mkdir .*Failure' >&2 || true
cp "$pack.sha" "$manifest"
echo "push: done"
