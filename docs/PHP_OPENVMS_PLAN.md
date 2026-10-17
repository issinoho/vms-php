# vms-php: plan for a family port of PHP 8.5 to OpenVMS x86-64

## Context

The phpBB board (vamp.issinoho.com) runs on vms-lighttpd's FastCGI pool with a third-party
PHP 8.1.23 kit (the `PHP_ROOT` build; VSI's own kit is 8.0). That PHP crashes now and then
under phpBB: `%SYSTEM-F-ACCVIO` at VA `8000000000000000` inside the C RTL's
`stat`/file lookup (called through phar → `realpath`/`file_exists`) and in mysqlnd's
connect handshake. Three crashes have been seen, each costing one request. It is a binary
kit we can't fix, and 8.1 is end-of-life.

**Goal:** our own port of the latest PHP, **8.5.x**, in the `issinoho/vms-*` family:
- signed upstream tarball + `patches/` + `overlay/`;
- `tools/vms.sh` for remote work;
- a PCSI kit, docs/DECISIONS.md and docs/PORTING_LOG.md.

**Acceptance:** a drop-in replacement for the board's PHP. vms-lighttpd's pool runs our
`PHP_CGI.EXE -b` and our `PHP.EXE` runs `phpbbcli`. The board works, including email through
SMTP with TLS, with no crashes under load.

**User decisions (2026-10-08):**
- **Version:** PHP 8.5.x. phpBB 3.3.19's INSTALL.html only lists PHP up to 8.3. If 8.5
  breaks phpBB in acceptance, rebuild the same port on 8.4 or 8.3.
- **Scope:** x86-64 only, CGI + CLI SAPIs. IA64 is decided later.
- **GD:** not in the first release, so no libpng. The board keeps a non-image CAPTCHA and no
  thumbnails.
- **Email:** SMTP with TLS, so the openssl extension is required.

## Key design decisions (to record as D0-Dn)

1. **Compiler: clang on x86-64 (LP64), as vms-mariadb D4.**
   - PHP assumes `size_t` and pointers are the same width, and needs C11, GCC builtins and
     `__atomic`. clang gives all of these; VSI C (ILP32, 32-bit `size_t`) does not.
   - It also allows the HYBRID Zend VM, which uses computed goto. The VM's global-register
     variables and the JIT stay off.
   - Reuse from vms-mariadb:
     - the forced-include `vms_lp64.h` for the C RTL's 32-bit `long` interfaces (D12);
     - the clang flags in `clang_common.rsp`;
     - the `vms_crtl_init.c` feature logicals (D6).
2. **Configure: cross-configure on the host, answers from the node.**
   - PHP uses autoconf. Run `./configure --host=x86_64-hp-openvms` in WSL with a
     `config.site` of `ac_cv_*` answers, so it never runs a test program.
   - The answers come from header-aware probes compiled and run with clang on the node.
     This is the idea of vms-mariadb D1/D8, with an autoconf cache in place of a CMake one;
     adapt `tools/sysroot.sh` and `tools/vms_probe.com`.
   - Take `main/php_config.h`, the object list and per-extension flags from the generated
     Makefile. A host script (adapted from vms-mariadb `tools/gen_mms.py`) writes the
     build's source lists and `main/internal_functions*.c`.
   - The release tarball already contains the generated sources (parsers, scanners, Zend VM),
     so no bison/re2c on VMS.
   - Builds run from CLEAN when headers change (lesson from vms-lighttpd PORTING_LOG #39).
3. **Static extensions, no `dl()`:**
   - core set: Core, date, hash, json, pcre, random, Reflection, SPL, standard, opcache
     (opcache is built in and always on in 8.5);
   - for phpBB: ctype, dom, filter, libxml, mbstring (`--disable-mbregex`, so no
     oniguruma), mysqli + mysqlnd, openssl, session, SimpleXML, tokenizer, xml, xmlreader,
     xmlwriter, zlib.
   - Not built: pdo, phar (it sat on two of the crash paths and phpBB doesn't need it), gd,
     intl, iconv, curl, sockets, posix, pcntl.
4. **Dependencies, all built with clang (LP64):**
   - **zlib:** add a clang build variant to **vms-zlib**, as vms-pcre2 did for vms-mariadb
     D11 (`[.INSTALL_X86_64_CLANG]`).
   - **PCRE2:** vms-pcre2's existing clang tree, not PHP's bundled copy (one PCRE2 port for
     the family; JIT off).
   - **libxml2:** a new sibling repository **vms-libxml2**, with a clang variant first.
   - **OpenSSL:** VSI SSL3's 64-bit images (`SSL3$LIBSSL_SHR`, `SSL3$LIBCRYPTO_SHR`), with an
     audit of the `long`-typed calls `ext/openssl` makes (vms-mariadb D9 method). Building our
     own OpenSSL is the fallback.
5. **Process model: no fork.**
   - CGI runs as `-b` with `PHP_FCGI_CHILDREN=0`, as the lighttpd pool already runs it.
   - `proc_open`, `pcntl` and `mail()` are unsupported in the first kit and disabled in the
     ini; phpBB uses SMTP.
   - `exec`/`system`/`popen` go through the C RTL's `popen` if the probes show it works.
     Otherwise they are disabled too.
6. **Naming:** PCSI product **PHP** (producer ISSINOHO, so it can't be confused with VSI's
   kit) and logical `VMSPHP$ROOT`.
   - Images: `[BIN]PHP.EXE` and `[BIN]PHP_CGI.EXE`. Settings in `[ETC]PHP.INI`.
   - Startup: `VMSPHP$STARTUP.COM`. Kit name: `ISSINOHO-X86VMS-PHP-V0805-nnE1-1`.
   - The lighttpd pool already takes the PHP root logical as a parameter
     (`vmslighttpd_php_root`), so switching is one setting.

## Expected VMS work (patches, `#ifdef __VMS`, one fix each)

Each item gets a Phase 0 probe first:
- **Execution timeouts:** `max_execution_time` uses `setitimer(ITIMER_PROF)`/`SIGPROF`. Use
  `ITIMER_REAL`/`SIGALRM` if PROF is missing.
- **Stack:** the stack-limit check (`zend_call_stack`) needs a VMS implementation (`$GETJPI`
  or the thread attributes) or `zend.max_allowed_stack_size=-1`. Check the main stack depth
  is enough for s9e's recursion.
- **Entropy:** `random_bytes` and session IDs need an entropy source. Use OpenSSL
  `RAND_bytes` (or `SYS$GET_ENTROPY` if present); there is no `/dev/urandom` or `getrandom`.
- **Fibers:** the Boost.Context x86-64 assembler routines (SysV, ELF), or a stub that throws
  "Fibers not supported". phpBB doesn't use fibers.
- **File semantics** (vms-mariadb D6/D10, vms-lighttpd patches 0008/0011):
  - `stat` gives correct `st_ino`/`st_dev`, as the realpath cache and opcache's validation
    need;
  - `rename` onto an existing file makes a new version, and phpBB's cache writes this way.
    Fix with `/VERSION_LIMIT=1` on `cache/` and `store/`, set by a setup procedure, or a
    patch to the plain-files rename;
  - `st_size` versus bytes read for files that aren't Stream_LF (scripts must be Stream_LF,
    as already documented);
  - `flock`/`fcntl` locks for sessions and the opcache lock file;
  - anonymous shared `mmap` for opcache. Each process has its own; `opcache.file_cache`
    may cut the 10-18 s first-request compile per process.
- **Paths:** UNIX-form file names only; the `php.ini` search path, `include_path` and
  `sys_temp_dir`/`upload_tmp_dir` defaults go under `VMSPHP$ROOT` and `SYS$SCRATCH`.
- **C RTL details:** `sigsetjmp`/`siglongjmp` (Zend bailout), `va_copy`, `getaddrinfo`,
  `poll`, `utime`, `umask`, time zones (`date.timezone` set in `PHP.INI`).

## Progress

| Phase | State | Report |
|---|---|---|
| 0 reconnaissance and probes | done | docs/PHASE0.md |
| 1 dependencies and the build | done | docs/PHASE1.md |
| 2 language and extensions | done; signed off by the user (2026-10-15) | docs/PHASE2.md |
| 3 CGI/FastCGI with lighttpd | done; signed off by the user (2026-10-17) | docs/PHASE3.md |
| 4 phpBB acceptance | test-board checks done (2026-10-17); waiting for the user's review; the board switch-over not started | docs/PHASE4.md |
| 5 kit and release | not started | |

For a new session:
- **Read first:** docs/PHASE4.md (results, the switch-over prerequisites and open items),
  PORTING_LOG #55–#59, and D20–D21.
- **Commits:** everything is committed locally; nothing is pushed.
- **The node's build tree** holds the latest build, patches 0001–0021; `tools/stage.sh x86`
  copies it into the staged `VMSPHP$ROOT` tree the test pool runs. After a change to a
  header or a flag, build with `tools/build.sh x86 CLEAN` and then `ALL` (CLAUDE.md).
- **Test runs:** `tools/phpt.sh x86 <set>`, compared with `out/phpt-<set>-ref.txt`. Tests that
  leave files behind can break the next run. For a fresh upload, delete the set's tree under the
  work directory's `PHPT` directory on the node, then `cache/pushed-phpt-<set>-x86.sha`.

## Phases (stop and report at each exit)

**Phase 0: reconnaissance and probes** (x86, clang):
- **Pin and reference build:**
  - pin PHP 8.5.x: tarball, SHA-256 and the release manager's GPG key;
  - configure and build it on WSL with the target extension set, as a reference: object
    list, `php_config.h`, the expected-results baseline for the tests;
  - fetch the signed release into `cache/`.
- **Probes** (`probes/*.c`, built with clang on the node): the list above. Also the
  `long` audit of SSL3 for ext/openssl, and clang 10.0.1's C11 support against PHP 8.5's
  minimum.
- **Baseline of the current PHP 8.1.23** on the test board: page times, first-request
  compile time, and a load run to see how often it crashes.
- **Check the board:**
  - its email settings (SMTP host, port, TLS);
  - its CAPTCHA plugin;
  - that no extension it uses needs GD, intl or curl.
- **Exit:** PHASE0.md and the decisions D0-Dn, approved.

**Phase 1: dependencies and the build:**
- the vms-zlib clang variant; vms-libxml2 (new repo, clang; its own tests pass);
- `config.site` answers, `php_config.h`, the source lists, `BUILD.COM`;
- `PHP.EXE -v`, `-m` (all listed extensions) and `-i` run on x86.

**Phase 2: language and extensions:**
- **Test runner:** run-tests.php needs `proc_open`, so a host runner `tools/phpt.sh` sends
  the `.phpt` files and runs `PHP.EXE` once per test through a DCL procedure on the node.
  It compares the results on the host with run-tests' `EXPECT`/`EXPECTF`/`EXPECTREGEX`
  rules.
- **Test sets:** `Zend/tests`, plus the tests of standard (strings, arrays, file, math),
  json, pcre, date, mbstring, dom/xml/simplexml, session, mysqli (against vms-mariadb),
  openssl and zlib.
- **Target:** the same pass list as the WSL reference, minus documented VMS skips (fork,
  posix, signals, dl).

**Phase 3: CGI/FastCGI with lighttpd:**
- vms-lighttpd's test pool on `VMSPHP$ROOT` runs `tools/test_php.sh`: 19 checks, including
  the backend kill and the proxy check;
- opcache on, `opcache.file_cache` tried;
- a 60-minute soak (`SOAK_PHP=1 tools/soak.sh`) with no ACCVIO and flat quotas.

**Phase 4: phpBB acceptance:**
- the Phase 4 test board (lighttpd `tools/serve.sh x86 phpbbsetup`) on our PHP:
  `tools/test_phpbb.py` 14/14;
- the ACP pages by hand;
- `phpbbcli.php` (`db:migrate --safe-mode`, `cache:purge`, cron) with `PHP.EXE`;
- an SMTP-with-TLS test email through `stream_socket_enable_crypto`;
- a phpBB load run of at least as many requests as the 8.1.23 baseline, with no crashes;
- page times compared with 8.1.23.
- Then, with the user, switch the board: set `vmslighttpd_php_root` to `VMSPHP$ROOT` and
  RESTART. Rollback is setting it back.
- If phpBB fails on 8.5, re-pin to 8.4 or 8.3 and repeat Phases 1-4.

**Phase 5: kit and release:**
- PCSI kit PHP for x86-64:
  - images, `PHP.INI` templates, `VMSPHP$STARTUP.COM` (logicals, `php`/`php_cgi` symbols),
    `readme.vms`;
  - the install check run with approval.
- vms-lighttpd `vms3`: `VMSLIGHTTPD$CONFIGURE` and `PHP_POOL` recognise `VMSPHP$ROOT`, with a
  pool `PHP.INI` template for our PHP.
- GitHub release and the website entry, as for vms-lighttpd.

## Repository (WSL `~/projects/vms-php`, copied from vms-mariadb/vms-lighttpd tooling)

```
upstream.conf  keys/  patches/series  overlay/{main,Zend,vmsport/{build.com,
vms_crtl_init.c,include/vms_lp64.h,kit/,tests/}}  probes/  docs/  tools/{vms.sh,fetch.sh,
prepare.sh,push.sh,build.sh,kit.sh,host_configure.sh,gen_build.py,phpt.sh,installcheck.sh}
```

Ground rules as in the siblings' CLAUDE.md:
- don't edit `staging/`, `cache/` or `out/`;
- node details only in the git-ignored `nodes.conf`; no secrets committed;
- nothing system-wide without asking;
- one PORTING_LOG entry per failure.

## Verification

- **Each phase exit**, from WSL against `<x86-host>`:
  - `tools/build.sh x86 CLEAN` then `ALL` finishes with no undefined symbols;
  - `PHP.EXE -m` lists exactly the planned extensions.
- **Phase 2:** the `.phpt` pass rate matches the WSL reference build, apart from listed and
  explained skips.
- **Phase 3:** vms-lighttpd `test_php.sh` 19/19 with the pool on `VMSPHP$ROOT`; a 60-minute
  soak with no ACCVIO in the `LTPHP_*.LOG` files.
- **Phase 4:** `test_phpbb.py` 14/14, the SMTP TLS mail sent, a phpBB load run with no
  crashes, and the board switched with the user's approval.
- **Phase 5:** the install check passes. A failure counts as fixed only when the test
  passes again on the node.
