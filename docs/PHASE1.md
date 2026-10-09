# Phase 1: dependencies and the build

x86-64 node, 2026-10-08.

## Dependencies (DECISIONS D4), all clang (LP64)

They live in one work directory on the node (the one vms-lighttpd's dependencies already
use; `tools/nodes.conf` column 8), side by side, as concealed rooted logical names during
the build:

| Library | Repository | Tree | Result |
|---|---|---|---|
| zlib 1.3.2 | vms-zlib: new `BUILD ALL CLANG` variant (the 15 library sources with clang against the `zconf.h` MAKE_VMS.COM configures) | `ZLIB-1_3_2.INSTALL_X86_64_CLANG` (`ZLIB$ROOT`) | smoke test 2/2 (clang); VSI C build unchanged, 2/2 |
| PCRE2 10.49 | vms-pcre2: existing clang variant, first built on this node | `PCRE2-10_49.INSTALL_X86_64_CLANG` (`PCRE2$ROOT`) | 21 pass, 2 known `pcre2test`-only failures (as documented there) |
| libxml2 2.15.4 | **vms-libxml2** (new repository, local only so far) | `LIBXML2-2_15_4.INSTALL_X86_64_CLANG` (`LIBXML2$ROOT`) | smoke test 11/11 (upstream's unit tests, xmllint) |
| OpenSSL 3.0 | VSI SSL3, 64-bit images `SSL3$LIBSSL_SHR`, `SSL3$LIBCRYPTO_SHR`; headers through `OPENSSL` -> `SSL3$INCLUDE:` | | D7 header for the `long` calls |

## Configuration

`tools/host_configure.sh` runs PHP's configure on the host with `PHP_CONFIGURE_OPTIONS`
(upstream.conf: the D3 extension set, CLI and CGI, external PCRE2 without JIT, no opcache
JIT): 634 object rules, no bundled PCRE2. `tools/gen_build.py` then writes into the staged
tree:

- `main/php_config.h` for VMS: 233 decisions, each logged in
  `vmsport/build/config.log`. Headers (`HAVE_*_H`) from the header probes, functions
  (`HAVE_*`, `HAVE_DECL_*`) from the declared-and-links probes, everything else from
  `vmsport/config/php_config.overrides` with a reason per line. A host `#define` that is
  neither probed nor listed stops the build. 65 answers differ from Linux (no fork, flock,
  getrandom, glob, fnmatch, strlcpy, memmem, scandir, x86 intrinsics headers, ...;
  `ZEND_SIGNALS`, `ZEND_CHECK_STACK_LIMIT`, `PHP_CAN_SUPPORT_PROC_OPEN`, ifunc/AVX-512
  dispatch, global registers, System V shm off).
- `main/build-defs.h` with paths under `/vmsphp$root` (`PHP_CONFIG_FILE_PATH`
  `/vmsphp$root/etc`).
- The platform-neutral generated files, copied: `main/internal_functions.c`,
  `main/internal_functions_cli.c`, `Zend/zend_config.h`, `ext/date/lib/timelib_config.h`,
  `ext/mbstring/libmbfl/config.h`.
- `vmsport/build/objects.txt`: 627 objects (618 in `PHP.OLB`, the CLI's 5 + 1, the CGI's
  1 + FastCGI 1 + 1), each with its source, VMS file spec and own `-I`/`-D` flags; sources
  that use OpenSSL also get `-include vmsport/include/vms_ssl3_long.h` (D7).

Extra compile probes for the answers (`probes/g_m_*.c`): `struct stat` has `st_blksize`,
`st_blocks`, `st_rdev`; `struct tm` has `tm_gmtoff`, `tm_zone`; `sockaddr_storage`,
`socklen_t`, `struct flock`, `pread`/`pwrite`, `asm goto`, `__builtin_alloca`,
`gethostbyname_r` (6 arguments), `getifaddrs`, `utime`, `lchown`, `symlink` and `dlopen`
are present; `strerror_r` is the POSIX (int) form; there is no `<fnmatch.h>`.

## Patches and overlay so far

| | |
|---|---|
| 0001 | `max_execution_time` on `ITIMER_REAL`/`SIGALRM` (D9) |
| 0002 | `random_bytes()` from OpenSSL `RAND_bytes` (D8) |
| `vmsport/vms_crtl_init.c` | C RTL features through `LIB$INITIALIZE` (EFS, UNIX names, file sharing, ...) |
| `vmsport/vms_fiber_exit.c` | the `_exit` symbol Boost.Context's `make_fcontext` calls (D10) |
| `vmsport/include/vms_lp64.h` | vms-mariadb's header for the C RTL's 32-bit `long` interfaces (D11) |
| `vmsport/include/vms_ssl3_long.h` | SSL3's 32-bit `long` returns and results (D7) |
| `vmsport/BUILD.COM` | compiles `objects.txt` with clang (flags in a response file), links `PHP.EXE` and `PHP_CGI.EXE` |

Added during the build (PORTING_LOG #1-21):

| | |
|---|---|
| 0003 | `environ` read through a 64-bit copy (the C RTL's is a 32-bit pointer to 32-bit pointers) |
| 0004 | the CLI leaves flushing stdout to stdio (each fflush of SYS$OUTPUT ends a record) |
| 0005 | opcache's lock file in `/sys$scratch`, named `ZendSem_XXXXXX` |
| `vmsport/vms_shims.c` | `fork`/`initgroups` (ENOSYS), `strsignal`, `in6addr_any`/`loopback`, `vms_php_environ()` |
| `vmsport/vms_fiber_exit.S` | `_exit` for Boost.Context (assembly: clang maps the C name to `decc$_exit`) |
| `vmsport/vms_syslog.c`, `include/compat/` | syslog to stderr; `stdalign.h`, `search.h`, `syslog.h` |
| `include/vms_php_compat.h` | `max_align_t`, `suseconds_t`, `timercmp`, `imaxabs`, `<locale.h>` first |
| clang flags | `-U__SSE2__` (no intrinsics headers), **`-fno-builtin-memset`** (below) |
| config | `HAVE_LIBDL` kept (else Zend extensions, opcache included, never start); `PHP_WRITE_STDOUT` off |

**The memset miscompile (PORTING_LOG #17).** VSI clang lowers `memset()` to `OTS$FILL`
but its optimiser still assumes the call returns its destination, as memset does: Zend's
`_ecalloc` (`memset(p, 0, n); return p;`) became a tail call to `OTS$FILL` and returned
whatever that left in RAX, a block still in use. Every `ecalloc()` (lexbor, timelib...) then
overwrote other modules' memory: crashes at shutdown in opcache, pcre and lexbor and in
timelib, with pointers like `0x8000000000000000`. Found by instrumenting step by step (a
watch on pcre's context, a log of Zend MM page runs, then `emalloc` against `ecalloc`) and
confirmed in the generated assembly. `-fno-builtin-memset` keeps explicit memset() a real
C RTL call (`decc$_memset64`, which returns its destination). memcpy is unaffected (it
calls `decc$_memcpy64`). The dependencies' sources have no `memset(); return` pattern; any
other clang code in the family may (vms-mariadb).

**The node's `USE_ZEND_ALLOC` (PORTING_LOG #16).** A system logical name `USE_ZEND_ALLOC`
= "0" on the x86 node (from an earlier PHP or Apache installation) silently switches any
PHP that reads it to the system allocator. Our tests define `USE_ZEND_ALLOC` = "1" in the
process; the kit's startup and the pool must do the same.

## Build

`tools/build.sh x86 CLEAN`, then `ALL`: **631 compiled, 0 failed; PHP.EXE and PHP_CGI.EXE
link** with no undefined symbols (2026-10-09; about 50 minutes for a full build).

| Exit check (x86) | Result |
|---|---|
| `PHP.EXE -v` | PHP 8.5.11 (cli) (NTS), with Zend OPcache v8.5.11; status 1 |
| `PHP.EXE -m` | the 27 planned modules: Core ctype date dom filter hash json lexbor libxml mbstring mysqli mysqlnd openssl pcre random Reflection session SimpleXML SPL standard tokenizer uri xml xmlreader xmlwriter Zend OPcache zlib; Zend Modules: Zend OPcache |
| `PHP.EXE -i` | System OpenVMS X86VMS E9.2-4; php.ini path `/vmsphp$root/etc`; OpenSSL 3.0.21 (SSL3); PCRE 10.49; libxml2 2.15.4; mysqlnd 8.5.11; Zend Memory Manager enabled; opcache.enable On |
| a script | `PHP_OS` OpenVMS, `PHP_INT_MAX` 9223372036854775807, `date()` and time zones, `json_encode`, `random_bytes` (OpenSSL), `preg_match`, SimpleXML, 30,000-string allocation check: all right; exit status 1 |
| `PHP_CGI.EXE -v` | PHP 8.5.11 (cgi-fcgi), with Zend OPcache; status 1 |

Open for Phase 2: the test runner and test sets; `opcache.file_cache`'s writev passes
`struct iovec` where the C RTL wants `struct __iovec64` (CHECK warning in
zend_file_cache.c); `exec()` exit codes (D11); the release build without `-g`.
