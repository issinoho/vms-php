#!/usr/bin/env bash
# stage.sh <node> - the staged install tree <workdir>.VMSPHP] for the Phase 3
# test pool: the built PHP.EXE and PHP_CGI.EXE in [BIN], tools/install/php.ini
# as [ETC]PHP.INI, the pinned CA bundle as [ETC]CACERT.PEM and as
# the hashed directory [ETC.CERTS] (openssl.capath), and a check of the SYSTEM logicals VMSPHP$ROOT and
# USE_ZEND_ALLOC "1" that point the pool at it (D22; the user defines them).
# Run after tools/build.sh; the images come from the build tree's [.OBJ_X86_64].
set -euo pipefail
top=$(cd "$(dirname "$0")/.." && pwd)
node=${1:?usage: stage.sh <node>}
. "$top/upstream.conf"
remote=$(echo "$UPSTREAM_NAME-$UPSTREAM_VERSION" | tr . _ | tr a-z A-Z)
read -r _ _ _ _ _ WORKDIR _ < <(awk -v n="$node" '$1==n' "$top/tools/nodes.conf")
ca=$top/cache/$(basename "$CA_BUNDLE_URL")
[ -f "$ca" ] || { echo "stage: run tools/fetch.sh first (CA bundle)" >&2; exit 1; }
# the CA bundle as an OpenSSL hashed directory (<subject hash>.<n>): with
# openssl.cafile, SSL3's OpenSSL 3.0 parses all ~120 certificates on every
# handshake (~54 ms each, 6.5 s); with openssl.capath it reads the one it needs
certs=$top/cache/cacerts-hashed; rm -rf "$certs"; mkdir -p "$certs"
awk -v d="$certs" '/-----BEGIN CERTIFICATE-----/{n++; f=sprintf("%s/split%03d.pem", d, n)} f{print > f}
                  /-----END CERTIFICATE-----/{close(f); f=""}' "$ca"
for c in "$certs"/split*.pem; do
    h=$(openssl x509 -noout -subject_hash -in "$c"); i=0
    while [ -e "$certs/$h.$i" ]; do i=$((i+1)); done
    mv "$c" "$certs/$h.$i"
done
"$top/tools/vms.sh" "$node" put "$top/tools/install/php.ini" "$ca" -- VMSPHP_STAGE
"$top/tools/vms.sh" "$node" dcl 'if f$search("[.VMSPHP_STAGE.CERTS]*.*") .nes. "" then delete/nolog [.VMSPHP_STAGE.CERTS]*.*;*' >/dev/null
"$top/tools/vms.sh" "$node" putdir "$certs" VMSPHP_STAGE/CERTS
"$top/tools/vms.sh" "$node" run "$top/tools/vms_stage.com" \
    "${WORKDIR%]}.$remote.OBJ_X86_64]" "${WORKDIR%]}.VMSPHP]" "${WORKDIR%]}.VMSPHP_STAGE]php.ini" \
    "${WORKDIR%]}.VMSPHP_STAGE]$(basename "$ca")" "${WORKDIR%]}.VMSPHP_STAGE.CERTS]"
