#!/usr/bin/env bash
# refbuild.sh - (re)build the Linux reference PHP in cache/refbuild: the same
# release and configure options as the VMS build (upstream.conf), no VMS patches,
# gcc; libxml2 and SQLite from cache/hostlibs.  tools/phpt.sh runs the .phpt sets
# against it (REF=1) to separate VMS failures from upstream ones.  Until 8.5.12-vms2
# it was built by hand the same way (docs/PHASE2.md).
set -euo pipefail

top=$(cd "$(dirname "$0")/.." && pwd)
. "$top/upstream.conf"
name=$UPSTREAM_NAME-$UPSTREAM_VERSION
tarball=$top/cache/$(basename "$UPSTREAM_URL")
hostlibs=$top/cache/hostlibs
ref=$top/cache/refbuild
[ -f "$hostlibs/lib/pkgconfig/sqlite3.pc" ] || { echo "refbuild: run tools/host_sqlite3.sh first" >&2; exit 1; }
work=$(mktemp -d -p "$top/cache" refbuild.XXXX)
tar -xJf "$tarball" -C "$work"
cd "$work/$name"
# shellcheck disable=SC2086
PKG_CONFIG_PATH=$hostlibs/lib/pkgconfig ./configure $PHP_CONFIGURE_OPTIONS > ../configure.log 2>&1 ||
    { echo "refbuild: configure failed ($work/configure.log)" >&2; exit 1; }
make -j"$(nproc)" > ../make.log 2>&1 || { echo "refbuild: make failed ($work/make.log)" >&2; exit 1; }
cd "$top"
rm -rf "$ref.old"
[ -d "$ref" ] && mv "$ref" "$ref.old"
mv "$work/$name" "$ref"
rm -rf "$work" "$ref.old"
echo "refbuild: $ref ($("$ref/sapi/cli/php" -r 'echo PHP_VERSION;'); $("$ref/sapi/cli/php" -m | grep -ciE '^(pdo|pdo_mysql|pdo_sqlite|sqlite3)$') of pdo, pdo_mysql, pdo_sqlite, sqlite3)"
