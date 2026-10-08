# Phase 0: reconnaissance and probes

x86-64 node (OpenVMS E9.2-4, VSI clang 10.0.1, VSI SSL3 3.0.21), 2026-10-08. Raw output:
`docs/probes-x86-clang.txt` (headers, functions, builtins, first runtime run) and
`docs/probes-x86-clang-R.txt` (runtime, final). Probes: `probes/`, run by `tools/probe.sh x86`
(`tools/vms_probe.com`); runtime probes run twice, with C RTL defaults and with the feature
logicals the PHP images will set (EFS charset/case, UNIX report, no version, file sharing,
remove open files, POSIX seek, rename no-inherit). Both runs agree except `getcwd` (VMS
syntax without the logicals).

## Release and host reference

- **PHP 8.5.11** (24 Sep 2026): `php-8.5.11.tar.xz`, SHA-256 `d9be75c0...0360` (php.net's
  release JSON), signature by Volker Dusch (`49D9AF6BC72A80D6691719C8AA23F5BE9C7097D4`).
- Host reference build (WSL, gcc, `--disable-all` + the D3 set except openssl, libxml2
  2.15.4 built under `cache/hostlibs`): 663 objects; `php -m`: Core ctype date dom filter
  hash json lexbor libxml mbstring mysqli mysqlnd pcre random Reflection session SimpleXML SPL
  standard tokenizer uri xml xmlreader xmlwriter Zend OPcache zlib. Largest parts: lexbor 186,
  standard 68, mbstring 39, dom 37, uri 34, opcache 29, bundled PCRE2 29 (not used: D4).
- PHP 8.5 needs C99 (C11 used when present); clang 10 gives C11.
- **libxml2 2.15.4** (GNOME, SHA-256 `98087fd1...8821`, no signature published) for
  vms-libxml2.

## Headers and functions (clang)

Headers: 38 of PHP's 53 checked headers compile. Missing: `cpuid.h`, `immintrin.h`,
`nmmintrin.h`, `tmmintrin.h`, `wmmintrin.h` (no x86 intrinsics headers: the SSE4.2/AVX
string and hash paths stay off), `pty.h`, `syslog.h`, `sys/loadavg.h`, `sys/mount.h`,
`sys/prctl.h`, `sys/select.h`, `sys/statfs.h`, `sys/sysexits.h`, `sys/sysmacros.h`,
`sys/vfs.h`.

Functions (declared by a header and link, G set): 56 of 110. Absent, among those PHP uses:
`flock` (PHP falls back to `fcntl` locks, which work: vms-mariadb PHASE0), `fork`,
`getrandom`, `getentropy`, `arc4random_buf`, `explicit_bzero`, `memmem`, `memrchr`,
`mempcpy`, `strlcpy`/`strlcat`, `reallocarray`, `scandir`/`alphasort`, `timegm`,
`fpclassify` (as a function; the macro path is separate), `fdatasync`, `getloadavg`,
`pthread_getattr_np` and the other stack-address calls, `res_*`/`dn_*` (DNS resolver),
`chroot`, `statfs`, `mremap`, `memfd_create`. `glob` and `getgrnam_r` link but are not
declared (answer: no). `asprintf`/`vasprintf` are present. Compiler builtins: all of PHP's
(`__builtin_*_overflow`, `clz`/`ctz`, `expect`, `unreachable`, `frame_address`,
`cpu_init`/`cpu_supports`) and computed goto compile and link: the HYBRID VM is possible.

## Run time

| Area | Result | Consequence |
|---|---|---|
| Types (clang) | `long`, `size_t`, pointers 8; `off_t` 8; **`time_t` 4**; `st_ino`, `st_dev` 8 | 64-bit PHP; timelib keeps its own 64-bit times |
| C RTL `long` interfaces | `%ld` and `%zu` print **0** for 2^40; `strtol` clamps at 2^31 - 1; `%lld`, `strtoll` right | vms-mariadb D12's forced-include header (`vms_lp64.h`) |
| Timers | no `SIGPROF`/`SIGVTALRM` in `<signal.h>`; `setitimer(ITIMER_REAL)` delivers `SIGALRM` | patch: `max_execution_time` on `ITIMER_REAL`/`SIGALRM` on VMS |
| Non-local jumps | `setjmp`/`longjmp`, `sigsetjmp`/`siglongjmp` (mask 0 and 1) from deep frames: work. **Jumping out of a signal handler ACCVIOs** (either call) | Zend bailouts (fatal errors, `exit()`) are fine; PHP never jumps out of its handlers (the hard timeout writes and exits) |
| Entropy | no `/dev/urandom`; `SYS$GET_ENTROPY` links but is not present at run time (`CALLUNDEFSYM`) | patch: `php_random_bytes` from OpenSSL `RAND_bytes` (SSL3) |
| Fibers | Boost.Context `make/jump_x86_64_sysv_elf_gas.S` assemble with clang; three context switches correct | build them; one undefined symbol, `_exit` (called if a fiber function returns): a shim |
| Memory | anonymous `mmap` `MAP_SHARED` 128 and 512 MB, `MAP_PRIVATE` 64 MB, `mprotect`, partial `munmap`: work (addresses in P0, `0x8000_6000`); `posix_memalign` 2 MB aligned: works | opcache shared memory and the Zend MM's 2 MB chunks |
| Stack | main thread recursed to 64 MB | no stack-limit check needed (PHP has none for VMS anyway) |
| `stat` | `st_ino`/`st_dev` equal across `stat`/`fstat`/`lstat` and stable over `rename`; `.`, `..`, trailing `/`, missing deep paths (ENOENT), names with several dots, `/sys$scratch`, `/SYS$SYSDEVICE/000000`: all correct | realpath cache and opcache validation can trust inodes; the third-party kit's ACCVIO in `stat` is not reproduced by these calls |
| Processes | `popen` + `fgets` work; `pclose` returns 256 for a successful DCL command (VMS status 1); `system("exit 1")` returns 1 | `exec`/`system`/`passthru` work through `popen` but the exit code needs mapping (VMS odd status -> 0) |
| Other | `mkstemp("/sys$scratch/phpXXXXXX")`, `setenv`, `va_copy`, `strptime`, `localtime_r` (BST, +0100 from the VMS time zone rules with `TZ` unset), `setlocale("")` (codeset ASCII), `getaddrinfo` for `localhost` and `127.0.0.1`: work | |

Known from vms-mariadb's Phase 0 (same node and compiler): `fcntl(F_SETLK)`, `poll`,
`socketpair`, non-blocking sockets, IPv6 dual stack; `realpath` returns ENOSYS (PHP has its
own); `open()` of a directory fails; `rename`/`O_TRUNC` onto an existing name leaves the old
version (none in a `/VERSION_LIMIT=1` directory); writes through one descriptor are not
seen through another of the same file in the same process; no `getrlimit`.

## ext/openssl and SSL3's 32-bit `long`

`tools/sysroot.sh x86` fetched SSL3's headers (OpenSSL 3.0.21); ext/openssl's five sources
were preprocessed against them on the host and every call was matched to its prototype.
ext/openssl calls 14 functions with `long` in their signature (plus macros over them):

- **`long` returns** (SSL3 returns 32 bits; clang reads 64, so a negative value arrives as a
  large positive one): `BIO_ctrl` (19 calls, through `BIO_get_mem_ptr`, `BIO_reset`,
  `BIO_get_mem_data`), `SSL_ctrl` / `SSL_CTX_ctrl` (`SSL_set_mode`,
  `SSL_set_tlsext_host_name`, `SSL_CTX_set_min/max_proto_version`, `SSL_CTX_set_tmp_dh`,
  `SSL_get_peer_cert_chain`...), `SSL_CTX_callback_ctrl`, `SSL_get_verify_result`.
- **`long *` results** (SSL3 writes 4 of the 8 bytes): `NCONF_load`, `NCONF_get_number_e`.
- **`unsigned long` returns** (`ERR_get_error`, `ERR_peek_error`, `X509_subject_name_hash`):
  32-bit values, zero-extended correctly (vms-mariadb D9).
- **`long` arguments** (`X509_gmtime_adj`, `X509_set_version`, `ASN1_item_d2i`,
  `RAND_load_file`, `X509_LOOKUP_ctrl`): only the low 32 bits arrive; values are small
  (versions, lengths, seconds up to ~68 years).

Fix (proposed D7): a forced-include header for ext/openssl that sign-extends the `long`
returns and passes `int`-sized temporaries for the two `long *` results.

## Baseline: the current PHP 8.1.23 kit under phpBB

vms-lighttpd's Phase 4 test board on x86 (`serve.sh x86 phpbbsetup`, server on :18080), with
the PHP 8.1.23 `PHP_ROOT` kit in a test pool of 4 on 19100-19103 (`POOL_BASE`; the live
board's pool on 19000-19003 was not touched), loaded with `SOAK_PHPBB=1 tools/soak.sh x86 30 4`
(guest pages: index, forum, topic, search, login form, FAQ, member list), 2026-10-08:

| Measure | PHP 8.1.23 (third-party kit) |
|---|---|
| Requests in 30 min (4 workers) | 1,409 |
| 200 / 500 | 1,405 / **4** |
| Page time, mean / max | 5.0 s / 18.5 s (first pages per process: 1.3-4.2 s warm-up) |
| PHP process crashes | **4**, every 500: `%SYSTEM-F-ACCVIO` at VA `8000000000000000` |
| - in `mysqlnd_conn_data::connect_handshake` (mysqli connect) | 1 |
| - in DECC$SHR `F_FINDFILE`/`F_MISC` under `php_stat` / `file_exists` / phar's interceptor | 3 |
| lighttpd server process | steady (8.0 -> 9.1 MB in the first 10 minutes, then flat) |

So the kit's crash rate under phpBB is about 1 in 350 requests, and both signatures seen on
the live board reproduce on the test board. This run is the acceptance baseline (plan Phase
4): our PHP must serve at least as many requests with no crash, and page times are compared
against it.

## Phase 0 exit

- Release pinned and verified; reference build on the host; probe results above.
- Decisions D0-D6 approved; **D7-D11 proposed** (docs/DECISIONS.md) from these results.
- Next (Phase 1): the vms-zlib clang variant, vms-libxml2 (new repository), the
  configure answers and `php_config.h`, the patches above, `BUILD.COM`.
