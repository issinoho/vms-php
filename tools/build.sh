#!/usr/bin/env bash
# build.sh <node> [ALL|CLEAN|LINK] [KEEP_GOING] - push the prepared tree and run
# [.VMSPORT]BUILD.COM on <node>.  The dependency work directory (vms-zlib,
# vms-pcre2 and vms-libxml2 clang trees) is nodes.conf's 8th column.
# The log is printed and saved to out/build-<node>.log.
set -euo pipefail

top=$(cd "$(dirname "$0")/.." && pwd)
node=${1:?usage: build.sh <node> [ALL|CLEAN|LINK] [KEEP_GOING]}
target=${2:-ALL}
keep=${3:-}
. "$top/upstream.conf"
remote=$(echo "$UPSTREAM_NAME-$UPSTREAM_VERSION" | tr . _ | tr a-z A-Z)
read -r _ _ _ _ _ WORKDIR _ DEPSDIR _ < <(awk -v n="$node" '$1==n' "$top/tools/nodes.conf")
[ -n "${DEPSDIR:-}" ] || { echo "build: nodes.conf has no dependency work directory (column 8)" >&2; exit 1; }

"$top/tools/push.sh" "$node"
mkdir -p "$top/out"
job=$top/cache/build-$node.com
cat > "$job" <<DCL
\$ set noon
\$ set process/parse_style=extended
\$ purge/nolog ${WORKDIR%]}.$remote...]*.*
\$ @${WORKDIR%]}.$remote.VMSPORT]BUILD.COM "$target" "$keep" "$DEPSDIR"
DCL
VMS_TIMEOUT=${VMS_BUILD_TIMEOUT:-14400} "$top/tools/vms.sh" "$node" run "$job" | tee "$top/out/build-$node.log"
grep -q 'BUILD: done' "$top/out/build-$node.log"
