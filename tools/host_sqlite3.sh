#!/usr/bin/env bash
# host_sqlite3.sh - build SQLite into cache/hostlibs for the host configure
# (tools/host_configure.sh, DECISIONS D28): the release vms-sqlite builds on VMS
# (upstream.conf SQLITE3_*; fetch.sh verifies it), with vms-sqlite's compile-time
# options that change SQLite's API (no loadable extensions, no WAL), so that
# configure's checks give the answers the VMS library would.  A static library
# and sqlite3.pc; as cache/hostlibs' libxml2.
set -euo pipefail

top=$(cd "$(dirname "$0")/.." && pwd)
. "$top/upstream.conf"
tarball=$top/cache/$(basename "$SQLITE3_URL")
hostlibs=$top/cache/hostlibs
[ -f "$tarball" ] || { echo "host_sqlite3: run tools/fetch.sh first" >&2; exit 1; }
work=$(mktemp -d -p "$top/cache" sqlite3build.XXXX)
tar -xzf "$tarball" -C "$work"
cd "$work"/sqlite-autoconf-*
CFLAGS="-O2 $SQLITE3_HOST_CFLAGS" ./configure --prefix="$hostlibs" --disable-shared \
    > ../configure.log 2>&1 || { echo "host_sqlite3: configure failed ($work/configure.log)" >&2; exit 1; }
make -j4 > ../make.log 2>&1 || { echo "host_sqlite3: make failed ($work/make.log)" >&2; exit 1; }
make install > ../install.log 2>&1
cd "$top"
rm -rf "$work"
echo "host_sqlite3: SQLite $SQLITE3_VERSION in $hostlibs ($(pkg-config --modversion --with-path="$hostlibs/lib/pkgconfig" sqlite3 2>/dev/null || echo '?'))"
