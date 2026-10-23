# PHP for OpenVMS with VSI's Apache (APACHEWS 2.4)

**Status (2026-10-18): draft, not yet tested end to end.**
- **Verified on the x86 node:**
  - VSI Apache V2.4-62 loads every module this setup uses, `mod_proxy`, `mod_proxy_fcgi` and
    `mod_proxy_balancer` included;
  - it needs the `User` line described below.
- **Not verified:** a request through Apache to PHP. On the test node Apache cannot create its
  listening socket at all, not even with VSI's own `HTTPD.CONF` (moved to another port,
  `mod_php` removed) and `httpd -t`. That is the case whoever runs it: a batch job, an
  interactive session, or `APACHE$WWW` through `APACHE$STARTUP START`. The same PHP pool passes
  every test behind lighttpd (vms-lighttpd `test_php.sh`, phpBB's `test_phpbb.py`).

## How it fits together

VSI's Apache kit bundles PHP 8.1.23 as an Apache module (`MOD_PHP_APACHE-2_0.EXE`, in
`SYS$COMMON:[APACHE.PHP]`). PHP for OpenVMS has no Apache module; it is a FastCGI server,
`PHP_CGI.EXE -b`. Apache passes `.php` requests to it with `mod_proxy_fcgi`, which VSI ships,
the same way lighttpd does.

```
browser -> Apache (APACHE$WWW) --FastCGI/TCP--> PHP_CGI.EXE -b 127.0.0.1:19000..19003
                                                (pool workers, one request at a time each)
```

On OpenVMS each worker serves **one request at a time**: there is no fork, so
`PHP_FCGI_CHILDREN` is 0. Run several workers on consecutive ports, and let
`mod_proxy_balancer` spread requests over them. Plain CGI (`mod_cgi` starting `PHP_CGI.EXE`
per request) also works in principle, but creating a process per request is slow on VMS, and
opcache cannot help.

## Requirements

- **PHP for OpenVMS** installed, e.g. in `DKA0:[VMSPHP]`, with the system logicals:
  - `VMSPHP$ROOT`, concealed and rooted at the tree;
  - `USE_ZEND_ALLOC` = `"1"`. A system `"0"` turns PHP's memory manager off. The 8.1 kit
    needs `"0"`, so the two cannot share this logical. Run PHP 8.1 under Apache, or our PHP,
    not both.
- **VSI Apache V2.4-62**, configured (`APACHE$CONFIG`) and started (`APACHE$STARTUP`), which
  installs its shareable images. It must ship `MOD_PROXY.EXE`, `MOD_PROXY_FCGI.EXE`,
  `MOD_PROXY_BALANCER.EXE`, `MOD_SLOTMEM_SHM.EXE` and `MOD_LBMETHOD_BYREQUESTS.EXE` in
  `APACHE$COMMON:[MODULES]`.

## 1. The PHP pool

Run the workers under the account that owns the PHP application's writable files.
`APACHE$WWW` is the natural choice, because Apache already runs as it. Each worker is a
detached process looping over `PHP_CGI.EXE -b`:

```
$! LTPHP_19000.COM - one PHP FastCGI worker (one per port)
$ set noon
$ define/process PHP_FCGI_CHILDREN "0"
$ define/process PHP_FCGI_MAX_REQUESTS "0"
$ define/process TMPDIR "/apache$specific/php/tmp"
$ cgi = "$VMSPHP$ROOT:[BIN]PHP_CGI.EXE"
$loop:
$ cgi "-c" "/apache$specific/php/php.ini" "-b" "127.0.0.1:19000"
$ wait 00:00:01
$ goto loop
```

Start each worker detached, as `SYSTEM`, with quotas big enough for PHP. These are
vms-lighttpd's `PHP_POOL.COM` values, run with phpBB:

```
$ RUN/DETACHED/UIC=APACHE$WWW SYS$SYSTEM:LOGINOUT.EXE /INPUT=LTPHP_19000.COM -
      /OUTPUT=LTPHP_19000.LOG /PROCESS_NAME=LTPHP_19000 -
      /BUFFER_LIMIT=200000 /FILE_LIMIT=200 /IO_BUFFERED=200 /IO_DIRECT=200 -
      /QUEUE_LIMIT=64 /ENQUEUE_LIMIT=4000 /AST_LIMIT=300 /SUBPROCESS_LIMIT=4 -
      /PAGE_FILE=2000000 /WORKING_SET=8192 /MAXIMUM_WORKING_SET=32768 /EXTENT=131072
```

vms-lighttpd's `PHP_POOL.COM` (`START|STOP|STATUS`, count, first port, pool directory, PHP
root) does all of this for several workers. It works for Apache unchanged:
`@PHP_POOL START 4 19000 <pool dir> VMSPHP$ROOT`.

The workers need no privileges beyond TMPMBX and NETMBX. Phase 4 ran phpBB on such a pool.

## 2. The pool's PHP.INI

Start from the kit's `PHP.INI`. The lines that matter on VMS:

```
; opcache is built in: no zend_extension line
opcache.enable = 1
opcache.file_cache = "/apache$specific/php/opcache"   ; owned by the pool's account
; the pinned CA bundle as a hashed directory; VMS syntax, see Troubleshooting
openssl.capath = "VMSPHP$ROOT:[ETC.CERTS]"
session.save_path = "/apache$specific/php/sessions"
upload_tmp_dir = "/apache$specific/php/tmp"
error_log = "/apache$specific/php/logs/php_errors.log"
max_execution_time = 60
cgi.fix_pathinfo = 1
```

Every directory PHP writes to (`opcache`, `sessions`, `tmp`, `logs`) must be **owned by the
pool's account**, or writable to it through UIC protection. An ACL is not enough: PHP checks
with the C RTL's `access()`, which ignores ACLs. opcache refuses a file cache it cannot
`access()`, and the worker exits at startup (vms-php PORTING_LOG #61).

## 3. Apache's configuration

In `APACHE$ROOT:[CONF]HTTPD.CONF`:

1. **Turn off VSI's PHP module**, so `.php` files are not run by PHP 8.1 or served as text:

   ```
   # LoadModule php_module modules/MOD_PHP_APACHE-2_0.EXE
   # AddType application/x-httpd-php .php .phtml
   # AddType application/x-httpd-php-source .phps
   ```

2. **Load the proxy modules:**

   ```
   LoadModule proxy_module               modules/mod_proxy.exe
   LoadModule proxy_fcgi_module          modules/mod_proxy_fcgi.exe
   LoadModule proxy_balancer_module      modules/mod_proxy_balancer.exe
   LoadModule slotmem_shm_module         modules/mod_slotmem_shm.exe
   LoadModule lbmethod_byrequests_module modules/mod_lbmethod_byrequests.exe
   ```

3. **Send `.php` to the pool:**

   ```
   <Proxy "balancer://php">
       BalancerMember "fcgi://127.0.0.1:19000"
       BalancerMember "fcgi://127.0.0.1:19001"
       BalancerMember "fcgi://127.0.0.1:19002"
       BalancerMember "fcgi://127.0.0.1:19003"
   </Proxy>
   <FilesMatch "\.php$">
       SetHandler "proxy:balancer://php"
   </FilesMatch>
   DirectoryIndex index.php index.html
   ```

   With a single worker, `SetHandler "proxy:fcgi://127.0.0.1:19000"` is enough.
   `mod_proxy_fcgi` sends the script's path as `SCRIPT_FILENAME` and the rest of the URL as
   `PATH_INFO`, in UNIX form (`/apache$root/htdocs/...`), which PHP opens directly.

4. **Keep VSI's `User` line.** VSI's prefork server starts its children under that account. A
   configuration without it fails with `error creating persona for #-1`:

   ```
   <IfModule unixd_module>
       User APACHE$WWW
       Group #-1
   </IfModule>
   ```

Then restart Apache: `@SYS$STARTUP:APACHE$STARTUP RESTART`.

## 4. File access

Apache and PHP both open the scripts: Apache to map the URL, PHP to run them. Both accounts
need read access to the document tree by UIC protection. With the pool running as
`APACHE$WWW`, Apache's own protection is enough. A pool under another account needs group or
world read access on the scripts, and execute on every directory down to them. On the test
node `APACHE$WWW` was in the developer's UIC group.

## 5. phpBB, and moving from VSI's PHP 8.1

Lessons from moving the phpBB board from the 8.1 kit to this PHP (vms-php PHASE4.md):

- **Database host:** use `$dbhost = '127.0.0.1'`. PHP 8.5's mysqli takes `localhost` as a
  Unix-domain socket, which fails on VMS with 2002 "Operation not permitted". Builds with
  patch 0022 use TCP for `localhost` too.
- **Passwords:** the 8.1 kit stored phpBB's passwords as `argon2id`. This PHP verifies
  argon2i, argon2id and bcrypt (D24), so existing accounts keep working.
- **CAPTCHA:** this PHP has no GD. Before switching, set phpBB's CAPTCHA to "Simple image"
  (ACP → Spambot countermeasures); the GD plugins fail with HTTP 500.
- **Extensions:** VSI's 8.1 adds curl, ldap, pgsql, pdo_pgsql and openvms. This PHP builds
  the phpBB set (vms-php D3) and, since 8.5.12-vms2, PDO, pdo_mysql, pdo_sqlite and sqlite3
  (D28): no curl, ldap or PostgreSQL. Check what the application uses (`php -m`).
- **`USE_ZEND_ALLOC`:** set it to `"1"` system-wide for this PHP. The 8.1 kit crashes with it
  (vms-php PORTING_LOG #63).

## Troubleshooting

| Message | Cause | Fix |
|---|---|---|
| `%SYSTEM-F-PROTINSTALL, protected images must be installed` (`APACHE$APR_SHRP`) | Apache's images are not installed | `@SYS$MANAGER:APACHE$FIS`, or Apache's startup |
| `[mpm_prefork:error] … error creating persona for #-1` | no `User` directive | `User APACHE$WWW` / `Group #-1` (step 3) |
| `alloc_listener: failed to get a socket for 0.0.0.0` … `Listen setup failed` (also with `httpd -t`) | Apache cannot create its listening socket. Seen on the test node 2026-10-18 for every port tried (8090, 8092, 18090), as every account, and with VSI's own `HTTPD.CONF`; Apache had served port 80 there until 2026-10-08. Note that `httpd -t` loads modules (`LoadModule`) while reading the file and processes `Listen` only after that, so a `LoadModule` error hides this one | not known; a question for VSI. Not caused by PHP or by this configuration |
| HTTP 500 or 503 with `SQL ERROR [ mysqli ] Operation not permitted [2002]` | `localhost` as database host | `127.0.0.1` (section 5) |
| PHP worker exits at startup: `opcache.file_cache must be a full path of an accessible directory` | the file cache directory is not the pool account's (an ACL does not count) | make the pool's account its owner |
| verified HTTPS from PHP takes ~7 s | `openssl.cafile`: SSL3's OpenSSL parses the whole bundle on every handshake | `openssl.capath = "VMSPHP$ROOT:[ETC.CERTS]"`, in VMS syntax: OpenSSL takes a directory without `]` or `:` for a logical name |

## Test material

These are in the vms-php repository, under `probes/`:
- `apache/make_phptest_conf.py` turns VSI's `HTTPD.CONF` into a test configuration on another
  port, with `mod_php` swapped for `mod_proxy_fcgi`.
- `apache/httpd-phptest.conf` is the minimal hand-written version.
- `r_apache_try.com` runs Apache in a batch job (`APACHE$STARTUP RUN`) with the test
  configuration.
