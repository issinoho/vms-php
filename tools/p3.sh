#!/usr/bin/env bash
# p3.sh <node> setup|start|stop|poolstart|poolstop|poolstatus|test|log - Phase 3:
# vms-lighttpd's test server and test pool ($LIGHTTPD_DIR/tools/serve.sh,
# test_php.sh) on our PHP, VMSPHP$ROOT (tools/stage.sh first).
#
#   setup  serve.sh phpsetup with PHP_ROOT_NAME=VMSPHP$ROOT, then
#          [.T3POOL]PHP.INI without the template's zend_extension and
#          extension_dir lines: they load the PHP 8.1 kit's opcache, and ours
#          is built in (D17)
#   test   test_php.sh; its "phpBB extensions loaded" check fails on gd and
#          pdo_mysql, which D3 does not build (D18)
# The rest go to serve.sh unchanged.
set -euo pipefail
node=${1:?usage: p3.sh <node> setup|start|stop|poolstart|poolstop|poolstatus|test|log}
op=${2:?usage: p3.sh <node> setup|start|stop|poolstart|poolstop|poolstatus|test|log}
lt=${LIGHTTPD_DIR:-$HOME/projects/vms-lighttpd}
export PHP_ROOT_NAME='VMSPHP$ROOT'
. "$lt/upstream.conf"
read -r _ _ _ _ _ WORKDIR _ < <(awk -v n="$node" '$1==n' "$lt/tools/nodes.conf")
tree="${WORKDIR%]}.$(echo "$UPSTREAM_NAME-$UPSTREAM_VERSION" | tr . _ | tr a-z A-Z)]"

case $op in
setup)
    "$lt/tools/serve.sh" "$node" phpsetup
    "$lt/tools/vms.sh" "$node" dcl "set default $tree" \
        'search/match=nor/output=[.T3POOL]PHP.TMP [.T3POOL]PHP.INI "zend_extension","extension_dir"' \
        'delete/nolog [.T3POOL]PHP.INI;*' \
        'convert/fdl=[.T2]STMLF.FDL [.T3POOL]PHP.TMP [.T3POOL]PHP.INI' \
        'delete/nolog [.T3POOL]PHP.TMP;*' \
        'search [.T3POOL]PHP.INI "extension","opcache.enable"'
    ;;
test) exec "$lt/tools/test_php.sh" "$node" ;;
*) exec "$lt/tools/serve.sh" "$node" "$op" ;;
esac
