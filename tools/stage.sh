#!/usr/bin/env bash
# stage.sh <node> - the staged install tree <workdir>.VMSPHP] for the Phase 3
# test pool: the built PHP.EXE and PHP_CGI.EXE in [BIN], tools/install/php.ini
# as [ETC]PHP.INI, and a check of the SYSTEM logicals VMSPHP$ROOT and
# USE_ZEND_ALLOC "1" that point the pool at it (D22; the user defines them).
# Run after tools/build.sh; the images come from the build tree's [.OBJ_X86_64].
set -euo pipefail
top=$(cd "$(dirname "$0")/.." && pwd)
node=${1:?usage: stage.sh <node>}
. "$top/upstream.conf"
remote=$(echo "$UPSTREAM_NAME-$UPSTREAM_VERSION" | tr . _ | tr a-z A-Z)
read -r _ _ _ _ _ WORKDIR _ < <(awk -v n="$node" '$1==n' "$top/tools/nodes.conf")
"$top/tools/vms.sh" "$node" put "$top/tools/install/php.ini" -- VMSPHP_STAGE
"$top/tools/vms.sh" "$node" run "$top/tools/vms_stage.com" \
    "${WORKDIR%]}.$remote.OBJ_X86_64]" "${WORKDIR%]}.VMSPHP]" "${WORKDIR%]}.VMSPHP_STAGE]php.ini"
