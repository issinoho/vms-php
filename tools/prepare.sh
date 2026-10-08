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
