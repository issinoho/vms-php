#!/usr/bin/env bash
# prepare.sh - build a VMS-ready PHP source tree in staging/<name>-<version>/
#
#   1. fetch + verify the upstream tarball (fetch.sh)
#   2. extract it, apply patches/series, lay overlay/ over the top
#   3. host_configure.sh (configure on this host for the VMS extension set,
#      cache/hostconf/), then gen_build.py: the VMS php_config.h and
#      build-defs.h, the platform-neutral generated files, and the object list
#      vmsport/build/objects.txt that [.VMSPORT]BUILD.COM compiles
#
# Nothing in staging/ is ever edited by hand: fix things in patches/ or overlay/.
set -euo pipefail

top=$(cd "$(dirname "$0")/.." && pwd)
. "$top/upstream.conf"
name=$UPSTREAM_NAME-$UPSTREAM_VERSION
tarball=$top/cache/$(basename "$UPSTREAM_URL")
stage=$top/staging/$name

step() { echo "prepare: $*"; }
die() { echo "prepare: error: $*" >&2; exit 1; }

"$top/tools/fetch.sh" >/dev/null

step "extracting $name"
rm -rf "$stage"; mkdir -p "$top/staging"
tar -xJf "$tarball" -C "$top/staging"
[ -d "$stage" ] || die "tarball did not unpack to $stage"

while read -r p; do
    case $p in ''|'#'*) continue ;; esac
    step "patch $p"
    patch -d "$stage" -p1 -s --no-backup-if-mismatch -F0 < "$top/patches/$p" ||
        die "patch $p does not apply cleanly"
done < "$top/patches/series"

# overlay/ may only add files; changes to upstream files belong in patches/.
(cd "$top/overlay" && find . -type f) | while read -r f; do
    [ -e "$stage/$f" ] && die "overlay/$f would replace an upstream file; use a patch"
    true
done
cp -a "$top/overlay/." "$stage/"

# Argon2's reference library (D24): its sources under vmsport/argon2, compiled by
# BUILD.COM with PHP's flags. Only the library: not opt.c (SSE), bench, genkat, run.
step "argon2 $ARGON2_VERSION"
a2=$top/cache/phc-winner-argon2-$ARGON2_VERSION.tar.gz
[ -f "$a2" ] || die "no $a2 (tools/fetch.sh)"
a2tmp=$(mktemp -d)
tar -xzf "$a2" -C "$a2tmp"
a2src=$a2tmp/phc-winner-argon2-$ARGON2_VERSION
mkdir -p "$stage/vmsport/argon2/include" "$stage/vmsport/argon2/src/blake2"
cp "$a2src/include/argon2.h" "$stage/vmsport/argon2/include/"
for f in argon2.c core.c core.h encoding.c encoding.h ref.c thread.c thread.h test.c; do
    cp "$a2src/src/$f" "$stage/vmsport/argon2/src/"
done
cp "$a2src/src/blake2/"*.h "$a2src/src/blake2/blake2b.c" "$stage/vmsport/argon2/src/blake2/"
cp "$a2src/LICENSE" "$stage/vmsport/argon2/"
rm -rf "$a2tmp"

# The host configure depends only on the pristine release and the options.
stamp=$top/cache/hostconf/.options
if [ "${RECONFIGURE:-}" = 1 ] || [ ! -f "$stamp" ] ||
   [ "$(cat "$stamp")" != "$UPSTREAM_SHA256 $PHP_CONFIGURE_OPTIONS" ]; then
    step "host configure"
    "$top/tools/host_configure.sh"
    echo "$UPSTREAM_SHA256 $PHP_CONFIGURE_OPTIONS" > "$stamp"
fi
python3 "$top/tools/gen_build.py" "$top/cache/hostconf/$name" "$stage" "$top/docs"

printf 'VERSION=%s\nKIT_VERSION=%s-vms%s\n' "$UPSTREAM_VERSION" "$UPSTREAM_VERSION" \
    "$VMS_PATCH_LEVEL" > "$stage/vmsport/version.env"
# the dependencies' trees for BUILD.COM: <logical name>=<tree in the deps work directory>
printf 'ZLIB$ROOT=%s\nPCRE2$ROOT=%s\nLIBXML2$ROOT=%s\n' "$ZLIB_TREE" "$PCRE2_TREE" "$LIBXML2_TREE" \
    > "$stage/vmsport/deps.env"
step "staged $stage"
