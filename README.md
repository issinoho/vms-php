# vms-php

A port of [PHP](https://www.php.net/) 8.5 to OpenVMS x86-64, built natively with VSI's clang
(64-bit pointers, LP64): the CLI (`PHP.EXE`) and CGI/FastCGI (`PHP_CGI.EXE`), with opcache,
mysqli for [MariaDB](https://github.com/issinoho/vms-mariadb), openssl through VSI's OpenSSL
3.0 kit (SSL3), and Argon2 password hashing. It runs a live [phpBB](https://www.phpbb.com/) 3.3
board behind [lighttpd for OpenVMS](https://github.com/issinoho/vms-lighttpd), as a pool of
FastCGI processes.

**Status:** PHP **8.5.11**, kit **VMSPHP V8.5-11E1** for x86-64.
- PHP's own test suites, against a Linux build of the same release: 11,652 of 12,413 pass on
  VMS, against 11,845 on Linux, and every VMS-only failure is explained (`docs/PHASE2.md`).
- A 60-minute FastCGI soak ran with no crash and flat memory (`docs/PHASE3.md`).
- phpBB's checklist passes 14 of 14, and a load run had no crash where PHP 8.1 had four; the
  board has run on it since 2026-10-17 (`docs/PHASE4.md`).

## Installing

Requirements: OpenVMS x86-64 (built and tested on E9.2-4), VSI TCP/IP Services and VSI SSL3.

```
$ PRODUCT INSTALL VMSPHP /SOURCE=dev:[dir]
$ @VMSPHP$ROOT:[COM]VMSPHP$SETUP          ! php and php_cgi symbols
$ php -v
```

The installation defines `VMSPHP$ROOT` and `USE_ZEND_ALLOC "1"`. Add
`@SYS$STARTUP:VMSPHP$STARTUP` to `SYSTARTUP_VMS.COM`. The kit's `[VMSPHP.DOC]README.VMS` covers
running a FastCGI pool for lighttpd or Apache (`[ETC]PHP-POOL.INI-DIST`), the extensions, and
the differences from PHP on Unix.

## Notes for OpenVMS

- **No fork.** `PHP_CGI.EXE` runs as a FastCGI server (`-b`), one request at a time per
  process (`PHP_FCGI_CHILDREN=0`); the web server spreads requests over several of them.
  `proc_open()`, pcntl and `mail()` are not available: send mail over SMTP.
- **Extensions:** the phpBB set: ctype, dom, filter, json, mbstring, mysqli/mysqlnd, openssl,
  pcre (PCRE2, no JIT), session, SimpleXML, tokenizer, xml/xmlreader/xmlwriter, zlib, opcache.
  There is no gd, intl, iconv, curl, pdo, sockets or posix.
- **Writable directories** (opcache's file cache, sessions, uploads) must be owned by the PHP
  pool's account: PHP checks them with `access()`, which ignores ACLs.
- **`USE_ZEND_ALLOC`:** a system logical `"0"` (some older PHP installs leave one) turns PHP's
  memory manager off. The kit sets `"1"`. VSI's PHP 8.1 crashes with `"1"`, so do not run both
  on one system.
- **Database host:** `127.0.0.1` or `localhost` (both over TCP; VMS has no usable Unix-domain
  sockets).
- **Apache:** VSI's Apache can use this PHP over FastCGI (`mod_proxy_fcgi`); see
  `docs/APACHE.md`. This is not yet verified end to end.

## How this repository works

It stores only the VMS delta over the signed upstream release, the same way as its siblings
([vms-lighttpd](https://github.com/issinoho/vms-lighttpd), [vms-mariadb](https://github.com/issinoho/vms-mariadb), ...):

- `upstream.conf`: the pinned release, its SHA-256 and signing key (`keys/`), the dependencies,
  the CA bundle and Argon2
- `patches/`: 23 changes to upstream files, applied in `patches/series` order, each guarded by
  `#ifdef __VMS` and carrying its VMS reason
- `overlay/`: files we add (the build procedure, VMS helpers and shims, the kit)
- `tools/`: host-side scripts: `fetch.sh`, `prepare.sh`, `build.sh`, `kit.sh`, the `.phpt`
  runner (`phpt.sh`, `phpt.py`), staging and pool helpers, and `vms.sh` for the nodes
- `probes/`: small C and PHP programs that pin down platform behaviour
- `docs/`: the plan, the decisions (`DECISIONS.md`), the porting log (`PORTING_LOG.md`) and the
  results of each phase

Building needs the clang builds of [zlib](https://github.com/issinoho/vms-zlib),
[PCRE2](https://github.com/issinoho/vms-pcre2) and [libxml2](https://github.com/issinoho/vms-libxml2):

```sh
tools/fetch.sh && tools/prepare.sh && tools/build.sh x86 ALL && tools/kit.sh x86
```

Home page for all the ports: [openvms.issinoho.com](https://openvms.issinoho.com).
