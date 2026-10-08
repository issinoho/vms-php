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

## Build

(in progress)
