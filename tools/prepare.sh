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
# --- PCSI kit inputs (vmsport/kit/MAKE_KIT.COM builds the kit on the node; D25)
step "PCSI kit inputs"
kit=$stage/vmsport/kit
# PHP 8.5.11 with VMS patch level 1 is PCSI V8.5-11E1 (update = third part,
# ECO = our patch level), as the sibling kits do
IFS=. read -r major minor update <<< "$UPSTREAM_VERSION"
pcsiversion="V$major.$minor-${update:-0}E$VMS_PATCH_LEVEL"
kitversion="$UPSTREAM_VERSION-vms$VMS_PATCH_LEVEL"
printf 'KIT_PRODUCER=%s\nKIT_PRODUCT=%s\nPCSI_VERSION=%s\nKIT_VERSION=%s\n' \
    "$KIT_PRODUCER" "$KIT_PRODUCT" "$pcsiversion" "$kitversion" > "$kit/KIT.ENV"
# the CA bundle, and the same certificates as an OpenSSL hashed directory
# (<subject hash>.<n>): openssl.capath, not cafile (PORTING_LOG #57)
ca=$top/cache/$(basename "$CA_BUNDLE_URL")
[ -f "$ca" ] || die "no $ca (tools/fetch.sh)"
cp "$ca" "$kit/CACERT.PEM"
mkdir -p "$kit/CERTS"
awk -v d="$kit/CERTS" '/-----BEGIN CERTIFICATE-----/{n++; f=sprintf("%s/split%03d.pem", d, n)} f{print > f}
                      /-----END CERTIFICATE-----/{close(f); f=""}' "$ca"
for c in "$kit/CERTS"/split*.pem; do
    h=$(openssl x509 -noout -subject_hash -in "$c"); i=0
    while [ -e "$kit/CERTS/$h.$i" ]; do i=$((i+1)); done
    mv "$c" "$kit/CERTS/$h.$i"
done
certfiles=$(cd "$kit/CERTS" && ls | sort | sed 's/^/    file [VMSPHP.ETC.CERTS]/; s/$/ ;/')
subst() {
    sed -e "s/@PRODUCER@/$KIT_PRODUCER/g" -e "s/@BASE@/X86VMS/g" \
        -e "s/@PCSIVERSION@/$pcsiversion/g" -e "s/@VERSION@/$UPSTREAM_VERSION/g" \
        -e "s/@KITVERSION@/$kitversion/g" -e "s/@ARCH@/x86-64/g"
}
lc=$(echo "$KIT_PRODUCT" | tr A-Z a-z)
subst < "$kit/$lc.pcsi\$desc_template" | awk -v files="$certfiles" '$0 == "@CERTFILES@" {print files; next} {print}' \
    > "$kit/PRODUCT-X86VMS.PCSI\$DESC"
subst < "$kit/$lc.pcsi\$text_template" > "$kit/PRODUCT-X86VMS.PCSI\$TEXT"
rm -f "$kit/$lc.pcsi\$desc_template" "$kit/$lc.pcsi\$text_template"
subst < "$kit/readme.vms" > "$kit/README.VMS"; rm -f "$kit/readme.vms"
mv "$kit/news" "$kit/NEWS."
mv "$kit/php-pool.ini-dist" "$kit/PHP-POOL.INI-DIST"
cp "$top/tools/install/php.ini" "$kit/PHP.INI-DIST"
cp "$stage/LICENSE" "$kit/LICENSE."
cp "$stage/vmsport/argon2/LICENSE" "$kit/ARGON2.LICENSE"
cp "$top/docs/APACHE.md" "$kit/APACHE.MD"
for f in 'vmsphp$startup.com' 'vmsphp$setup.com' make_kit.com; do
    u=$(echo "$f" | tr a-z A-Z); mv "$kit/$f" "$kit/$u"
done

step "staged $stage"
