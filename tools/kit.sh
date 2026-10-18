#!/usr/bin/env bash
# kit.sh <node> - build on <node>, make the PCSI kit there (vmsport/kit/MAKE_KIT.COM,
# D25), fetch it to out/kits/. Builds from CLEAN first, so the kit matches the
# pushed tree (BUILD.COM tracks no headers); NOCLEAN=1 builds ALL only, when the
# tree was built from CLEAN with the same sources.
set -euo pipefail

top=$(cd "$(dirname "$0")/.." && pwd)
node=${1:?usage: kit.sh <node>}
. "$top/upstream.conf"
remote=$(echo "$UPSTREAM_NAME-$UPSTREAM_VERSION" | tr . _)
REMOTE=$(echo "$remote" | tr a-z A-Z)
read -r _ ARCH _ _ _ WORKDIR _ < <(awk -v n="$node" '$1==n' "$top/tools/nodes.conf")
mkdir -p "$top/out"

if [ "${NOCLEAN:-}" = 1 ]; then
    "$top/tools/build.sh" "$node" ALL > "$top/out/build-$node.log" 2>&1
else
    { "$top/tools/build.sh" "$node" CLEAN && "$top/tools/build.sh" "$node" ALL; } > "$top/out/build-$node.log" 2>&1
fi || { tail -20 "$top/out/build-$node.log"; echo "kit: build failed" >&2; exit 1; }

job=$top/cache/kit-$node.com
printf '$ set noon\n$ @%s.%s.VMSPORT.KIT]MAKE_KIT.COM\n' "${WORKDIR%]}" "$REMOTE" > "$job"
VMS_TIMEOUT=1800 "$top/tools/vms.sh" "$node" run "$job" | tee "$top/out/kit-$node.log"
kit=$(sed -n 's/^MAKE_KIT: kit .*\]\([^;]*\);.*/\1/p' "$top/out/kit-$node.log" | head -1)
[ -n "$kit" ] || { echo "kit: no kit produced (see out/kit-$node.log)" >&2; exit 1; }
mkdir -p "$top/out/kits"
"$top/tools/vms.sh" "$node" get "$remote/KIT_$ARCH/$kit" "$top/out/kits/$kit"
ls -la "$top/out/kits/$kit"
