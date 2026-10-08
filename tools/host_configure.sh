#!/usr/bin/env bash
# host_configure.sh - run PHP's configure on this host for the VMS extension set
# (docs/DECISIONS.md D3, D12) in cache/hostconf/<name>/, as the source of:
#   - the object list and per-object -I/-D flags (Makefile; tools/gen_build.py)
#   - the generated files that are not platform facts (internal_functions*.c,
#     Zend/zend_config.h, timelib_config.h, libmbfl config.h)
#   - main/php_config.h, which tools/gen_build.py turns into the VMS one by
#     overriding the platform answers (docs/PHASE0.md probes,
#     vmsport/config/answers.txt).
# Only configure runs (no make).  libxml2 comes from cache/hostlibs (built from
# the same release as vms-libxml2), OpenSSL and zlib from the host system; PCRE2
# is "external" (vms-pcre2 on VMS) with no JIT, so PHP's bundled copy is not
# in the object list.
set -euo pipefail

top=$(cd "$(dirname "$0")/.." && pwd)
. "$top/upstream.conf"
name=$UPSTREAM_NAME-$UPSTREAM_VERSION
tarball=$top/cache/$(basename "$UPSTREAM_URL")
conf=$top/cache/hostconf
hostlibs=$top/cache/hostlibs

die() { echo "host_configure: error: $*" >&2; exit 1; }
[ -f "$hostlibs/lib/pkgconfig/libxml-2.0.pc" ] ||
    die "no host libxml2 in cache/hostlibs (see docs/PHASE0.md: libxml2 built from the vms-libxml2 release)"

rm -rf "$conf"; mkdir -p "$conf"
tar -xJf "$tarball" -C "$conf"
cd "$conf/$name"
# shellcheck disable=SC2086
PKG_CONFIG_PATH=$hostlibs/lib/pkgconfig ./configure $PHP_CONFIGURE_OPTIONS > ../configure.log 2>&1 ||
    die "configure failed (cache/hostconf/configure.log)"
grep -q '^PHP_GLOBAL_OBJS' Makefile || die "no PHP_GLOBAL_OBJS in the generated Makefile"
echo "host_configure: $conf/$name ($(grep -c '\.lo:' Makefile) object rules)"
