#!/usr/bin/env bash
# p6.sh <node> <op> - the SQLite phpBB board for 8.5.12-vms2 (D28, D29; vms-sqlite's
# acceptance test), in our work directory, beside vms-lighttpd's Phase 4 board
# (:18080, pool 19100-19103) and away from the live board (pool 19000-19003):
#   [.PHPBB6]          phpBB (vms-lighttpd's pin), its database outside the
#                      document root: [.PHPBB6DATA]PHPBB.SQLITE3
#   [.T6], [.T6POOL]   lighttpd's configuration (HTTP :18081, no HTTPS) and the
#                      FastCGI pool LTPHP_19104-19107 (tools/p6setup.com)
#   PHP                the staged tree (tools/stage.sh), through the GROUP
#                      logical VMSPHP2$ROOT: the pool's detached workers inherit
#                      no process logicals, and the SYSTEM VMSPHP$ROOT is the
#                      installed kit the live board runs (D23, D29)
# The server image, P2SERVER.COM, PHP_POOL.COM, phpbb.conf and the ini template
# are vms-lighttpd's built tree's, used read only.  Ops:
#   logical | nological   define / deassign VMSPHP2$ROOT
#   upload                phpBB's files into [.PHPBB6] (a new copy)
#   install               phpbbcli.php install on SQLite; the admin password is
#                         generated into cache/phpbb6.secrets (git-ignored); the
#                         installer's config is deleted after; install/ renamed;
#                         viglink disabled
#   setup                 tools/p6setup.com
#   poolstart|poolstop|poolstatus
#   start | stop          the server, batch job LTSQLT (entry: cache/serve6-<node>.entry)
#   test                  vms-lighttpd's test_phpbb.py against :18081
#   soak [min] [workers]  vms-lighttpd's soak.sh (SOAK_PHPBB=1) against :18081
#   log                   the server's job log and lighttpd's logs into out/p6/
set -euo pipefail
top=$(cd "$(dirname "$0")/.." && pwd)
node=${1:?usage: p6.sh <node> <op>}
op=${2:?usage: p6.sh <node> <op>}
lt=${LIGHTTPD_DIR:-$HOME/projects/vms-lighttpd}
read -r _ _ HOST _ _ WORKDIR _ < <(awk -v n="$node" '$1==n' "$top/tools/nodes.conf")
LTWORK=$(awk -v n="$node" '$1==n {print $6}' "$lt/tools/nodes.conf")
LTNAME=$(. "$lt/upstream.conf"; echo "$UPSTREAM_NAME-$UPSTREAM_VERSION" | tr . _ | tr a-z A-Z)
lttree="${LTWORK%]}.$LTNAME]"
entry_file=$top/cache/serve6-$node.entry
secrets=$top/cache/phpbb6.secrets
base=19104
url="http://$HOST:18081"
vms() { "$top/tools/vms.sh" "$node" "$@"; }
# UNIX form of a directory under the work directory: <sub> = "PHPBB6DATA" etc.
unix_of() { echo "${WORKDIR%]}.$1]" | sed 's/^\([^:]*\):\[/\/\1\//; s/\]$//; s/\./\//g'; }

case $op in
logical)
    # concealed roots must name a physical device (vms-php PORTING_LOG #54)
    vms dcl "dev = f\$getdvi(f\$parse(\"$WORKDIR\",,,\"DEVICE\",\"NO_CONCEAL\"), \"FULLDEVNAM\")" \
        "dir = f\$parse(\"$WORKDIR\",,,\"DIRECTORY\",\"NO_CONCEAL\") - \"][\" - \".][\" - \"]\"" \
        "define/group/translation_attributes=concealed VMSPHP2\$ROOT 'dev''dir'.VMSPHP.]" \
        'show logical/full VMSPHP2$ROOT' 'directory/size VMSPHP2$ROOT:[BIN]*.EXE'
    ;;
nological)
    vms dcl 'deassign/group VMSPHP2$ROOT' 'show logical VMSPHP2$ROOT'
    ;;
upload)
    . "$lt/upstream.conf"
    zip=$top/cache/$(basename "$PHPBB_URL")
    [ -f "$zip" ] || curl -fsSL -o "$zip" "$PHPBB_URL"
    echo "$PHPBB_SHA256  $zip" | sha256sum -c --quiet - || { echo "p6: phpBB SHA-256 mismatch" >&2; exit 1; }
    tmp=$(mktemp -d -p "$top/cache" phpbb6.XXXX)
    unzip -q "$zip" -d "$tmp"
    mv "$tmp/phpBB3" "$tmp/PHPBB6"
    if vms dcl 'if f$search("PHPBB6.DIR") .nes. "" then write sys$output "P6-EXISTS"' | grep -q P6-EXISTS; then
        rm -rf "$tmp"; echo "p6: [.PHPBB6] exists: not uploading over it" >&2; exit 1
    fi
    vms putr "$tmp/PHPBB6" PHPBB6
    rm -rf "$tmp"
    vms dcl 'directory/total [.PHPBB6...]'
    ;;
install)
    [ -f "$secrets" ] || { printf 'PHPBB_ADMIN_PASSWORD=%s\n' "$(python3 -c 'import secrets; print(secrets.token_urlsafe(15))')" > "$secrets"; chmod 600 "$secrets"; }
    pw=$(sed -n 's/^PHPBB_ADMIN_PASSWORD=//p' "$secrets")
    cfg=$(mktemp -d)/p6install.yml
    cat > "$cfg" <<EOF
installer:
    admin:
        name: admin
        password: $pw
        email: admin@example.org
    board:
        lang: en
        name: vms-sqlite test board
        description: phpBB on SQLite on OpenVMS (PHP 8.5.12-vms2)
    database:
        dbms: sqlite3
        dbhost: $(unix_of PHPBB6DATA)/phpbb.sqlite3
        dbport: ~
        dbuser: ~
        dbpasswd: ~
        dbname: ~
        table_prefix: phpbb_
    email:
        enabled: false
        smtp_delivery : ~
        smtp_host: ~
        smtp_port: ~
        smtp_auth: ~
        smtp_user: ~
        smtp_pass: ~
    server:
        cookie_secure: false
        server_protocol: http://
        force_server_vars: false
        server_name: $HOST
        server_port: 18081
        script_path: /
    extensions: []
EOF
    # the config holds the password: an owner-only directory, deleted after
    vms dcl 'if f$search("P6PRIV.DIR") .eqs. "" then create/directory/protection=(S:RWED,O:RWED,G,W) [.P6PRIV]' \
        'if f$search("PHPBB6DATA.DIR") .eqs. "" then create/directory [.PHPBB6DATA]' >/dev/null
    vms put "$cfg" -- P6PRIV >/dev/null
    rm -rf "$(dirname "$cfg")"
    # COLUMNS/LINES: Symfony's console otherwise calls proc_open (vms-lighttpd #24)
    vms dcl 'define/process COLUMNS "80"' 'define/process LINES "24"' 'set default [.PHPBB6]' \
        'php = "$VMSPHP2$ROOT:[BIN]PHP.EXE"' \
        "php \"-c\" \"/vmsphp2\$root/etc/php.ini\" \"install/phpbbcli.php\" \"install\" \"$(unix_of P6PRIV)/p6install.yml\"" \
        'set default [-]' 'delete/nolog [.P6PRIV]P6INSTALL.YML;*' \
        'set security/protection=(O:RWED) P6PRIV.DIR' 'delete/nolog P6PRIV.DIR;*' \
        'if f$search("[.PHPBB6]INSTALL.DIR") .nes. "" then rename [.PHPBB6]INSTALL.DIR [.PHPBB6]INSTALL_DONE.DIR' \
        'set default [.PHPBB6]' \
        'php "-c" "/vmsphp2$root/etc/php.ini" "bin/phpbbcli.php" "extension:disable" "phpbb/viglink"' \
        'set default [-]' 'directory/size/date [.PHPBB6DATA]'
    ;;
setup)
    vms put "$top/tools/p6setup.com" >/dev/null
    vms dcl "@P6SETUP.COM $lttree $base"
    ;;
poolstart|poolstop|poolstatus)
    vms dcl "@${lttree%]}.VMSPORT.PHP]PHP_POOL.COM ${op#pool} 4 $base ${WORKDIR%]}.T6POOL] VMSPHP2\$ROOT" |
        grep -E 'PHP_POOL|%' || true
    ;;
start)
    out=$(vms dcl 'if f$search("[.T6.LOGS]*.*") .nes. "" then delete/nolog [.T6.LOGS]*.*;*' \
          "submit/noprint/log_file=${WORKDIR}P6SERVER.LOG/name=LTSQLT/parameters=(\"$lttree\",\"${WORKDIR%]}.T6]LIGHTTPD.CONF\") ${lttree%]}.VMSPORT.TESTS]P2SERVER.COM")
    echo "$out"
    entry=$(echo "$out" | grep -oE 'entry [0-9]+' | awk '{print $2}')
    [ -n "$entry" ] || { echo "p6: no entry number" >&2; exit 1; }
    echo "$entry" > "$entry_file"
    for _ in $(seq 1 30); do
        curl -s -o /dev/null --max-time 3 "$url/" && { echo "p6: up on :18081 (entry $entry)"; exit 0; }
        sleep 2
    done
    echo "p6: not answering on :18081 after 60 s" >&2; exit 1
    ;;
stop)
    [ -f "$entry_file" ] || { echo "p6: no entry recorded" >&2; exit 1; }
    vms dcl "delete/entry=$(cat "$entry_file")" || true
    rm -f "$entry_file"
    ;;
test)
    PHPBB_URL=$url PHPBB_SECRETS=$secrets exec python3 "$lt/tools/test_phpbb.py" "$node"
    ;;
soak)
    SOAK_PHPBB=1 SOAK_PORT=18081 SOAK_TLS_PORT= SOAK_ENTRY=$entry_file exec "$lt/tools/soak.sh" "$node" "${3:-30}" "${4:-4}"
    ;;
log)
    mkdir -p "$top/out/p6"
    vms get P6SERVER.LOG "$top/out/p6/p6server.log" || true
    vms get T6/LOGS/error.log "$top/out/p6/error.log" || true
    vms get T6/LOGS/access.log "$top/out/p6/access.log" || true
    ls -la "$top/out/p6"
    ;;
*) echo "p6: unknown op $op" >&2; exit 2 ;;
esac
