# Phase 2: language and extensions

**Status (2026-10-11):** done, waiting for the user's Phase 2 exit review. Phase 3 has not
started; the user asked to stop before it. Open items are under "Open items" at the end.

The x86-64 node, 2026-10-09 to 2026-10-11. PHP 8.5.11 built with VSI clang (LP64) and patches
0001–0019 plus `overlay/vmsport`, measured against the Linux reference build (`cache/refbuild`:
gcc, the same release, configure options and test files).

## Test runner

`tools/phpt.sh <node> <set>` uploads a prepared set (`tools/phpt.py prepare`) and runs it on the
node with `tools/vms_phpt.com`. For each test that procedure runs the SKIPIF, then the FILE with
`-n -c <test>.ini -d max_execution_time=120`, then CLEAN. It does this without the SYSPRV,
BYPASS, READALL and GRPPRV privileges (D12). `phpt.py compare` judges each output on the host
by run-tests' `EXPECT`/`EXPECTF`/`EXPECTREGEX` rules. The reference results come from
`phpt.py local` with the host build.

Some tests are never run, on either system, and are listed in each set's `UNSUPPORTED.TXT`:
tests that need sections the runner does not support (`--POST--`, `--GET--`, `--CGI--`,
`--STDIN--`, `--ENV--`, `--ARGS--`, ...) and tests of extensions we do not build. `HANGS` in
`phpt.py` is for tests that hang PHP.EXE on the node; it is empty now (PORTING_LOG #49).

The runner itself had two bugs, both fixed and both affecting earlier numbers:
- **Uploads stopped early.** A file name containing a space (#24) and names that are not UTF-8
  (#31) broke the upload, so parts of sets were never sent.
- **Tests skipped wrongly (#47).** A SKIPIF that printed nothing reused the previous test's
  `skip` line, so 552 tests that pass on Linux had never run on VMS. All numbers below come
  from runs after the fix.

## Results

| Set | Tests | VMS pass | VMS fail | Skip | Linux pass | VMS-only failures |
|---|---|---|---|---|---|---|
| zend | 5222 | 5177 | 16 | 29 | 5180 | 3 |
| fibers | 294 | 294 | 0 | 0 | 294 | 0 |
| array | 844 | 833 | 0 | 11 | 836 | 0 |
| strings | 730 | 673 | 2 | 55 | 679 | 1 |
| math | 173 | 162 | 0 | 11 | 162 | 0 |
| file | 890 | 485 | 123 | 282 | 597 | 107 |
| ctype | 48 | 48 | 0 | 0 | 48 | 0 |
| json | 89 | 87 | 0 | 2 | 87 | 0 |
| pcre | 164 | 154 | 0 | 10 | 154 | 0 |
| date | 691 | 676 | 4 | 11 | 677 | 1 |
| mbstring | 412 | 318 | 14 | 80 | 319 | 0 |
| dom | 857 | 838 | 4 | 15 | 843 | 1 |
| xml | 65 | 56 | 0 | 9 | 56 | 0 |
| simplexml | 158 | 158 | 0 | 0 | 158 | 0 |
| session | 243 | 241 | 2 | 0 | 241 | 0 |
| spl | 789 | 783 | 4 | 2 | 787 | 4 |
| random | 69 | 68 | 0 | 1 | 68 | 0 |
| uri | 310 | 310 | 0 | 0 | 310 | 0 |
| openssl | 229 | 163 | 2 | 64 | 219 | 1 |
| zlib | 136 | 128 | 2 | 6 | 130 | 2 |
| **total** | 12413 | 11652 | 173 | 588 | 11845 | 120 |

That is 11,652 passing on VMS against 11,845 on Linux, out of 12,413 run. 120 tests fail only on
VMS, 107 of them in the file set, and all are listed below. The other 73 VMS failures fail on
Linux too. The remaining gap, tests that pass on Linux but are skipped on VMS, comes from
SKIPIF conditions such as no proc_open, `PHP_OS` checks, or a Unix tool the test needs.

"VMS-only failures" are tests that fail on VMS but pass on Linux; every one of them is
explained below. The other failures fail on Linux too: they need things the reference lacks,
such as network access, or test something in development.

## What was fixed

Each entry is a PORTING_LOG number; patches are in `patches/`, our own files in `overlay/vmsport`.

| Area | Fix | PORTING_LOG |
|---|---|---|
| Fibers | stacks in 32-bit space: the C RTL switches to its P1 stack when SP is above 2 GB (0006) | 23 |
| Temporary files | `/sys$scratch`, not `SYS$SCRATCH:` joined with `/` (0007) | 25 |
| Math | rounding functions keep the sign of zero; C99 special values for pow, fmod, atan2, log, hypot, nextafter; correctly rounded sqrt; acosh; pow results beyond the range | 26, 27, 48 |
| Errors | strerror()/strerror_r() with glibc's wording (`vms_strerror.c`) | 28 |
| Environment | putenv() through setenv(): the C RTL keeps the caller's string in its 32-bit environ | 29 |
| Dates | timestamps as zend_long in date()/idate(): the C RTL's time_t is 32-bit unsigned (0008) | 30 |
| OpenSSL | "" and directory paths fail (0009, 0016); SSL3's config path (0010); ASN.1 times as zend_long (0011) | 32–34, 44 |
| exec() | result codes from `$STATUS` (0012, D11) | 35 |
| Files | `.` and `..` in directory streams (0013); fflush() makes writes visible (0014); "w" rewrites in place, one version (0015, D13); mkdir(0777) gets DELETE access, unlink() removes every version and refuses directories, rename() replaces the target's versions (`vms_files.c`) | 36–40, 43 |
| ctype | ASCII classes in the "C" locale (0017) | 46 |
| Resolver | gethostbyname()/gethostbynamel() crashed for any name that resolved (0018, 0019) | 50, 51 |
| Build | clang's temporary files in the build tree, not the login directory | 41 |

## Known VMS differences

These are the remaining VMS-only failures. None of them is in a path phpBB uses, except where
noted under "For Phase 3 and 4".

**File system (the file set, 107 tests).** C RTL and ODS-5 behaviour, not PHP code:

| Difference | Tests |
|---|---|
| no hard links: link() is not implemented in the C RTL | 27 |
| protection model: chmod(f, 0) gives the default protection (0750); a file whose owner has no access cannot be stat()ed by absolute path without privileges; umask and permission tests (#42) | 21 |
| file times: no separate atime; touch() before 1970 (unsigned time_t), and the 1970 hour of British Standard Time | 12 |
| data written to an open file is seen by other opens and stat() only after fflush() or close (#39) | 9 |
| append mode: the position is at EOF from the open, and fseek() cannot go past EOF | 6 |
| rename()/readlink() edge cases (rename onto itself, trailing `/`) | 5 |
| Unix paths and tools: `/etc/passwd`, `/tmp`, popen() and exec() of Unix commands | 5 |
| a path through a directory that does not exist still resolves (`dir/../BADDIR/x`, tempnam in a missing directory) | 4 |
| file names with `*`, a VMS wildcard, cannot be created | 4 |
| flock(LOCK_SH) on a write-only handle: PHP's flock is fcntl(), and POSIX requires read access for a shared lock | 4 |
| proc_open: no fork (D5) | 2 |
| no sparse files (ftruncate to 2 GB) and very large directories (70,000 files) time out | 2 |
| messages and limits: php://fd numbers ≥ 1000, opening a directory, disk_total_space(), ftruncate() to a negative size, select() on files, PATH_MAX | 6 |

**Other sets:**

| Set | Test | Difference |
|---|---|---|
| zend | arginfo_zpp_mismatch | completes, but a stray `:` line follows `===DONE===`: something outside output buffering writes it, probably DCL during one of the shell-related calls (not yet traced) |
| zend | bug80811 | `exec('echo …')`: DCL has no `echo`, so the result code is 1 |
| zend | new_oom | runs `PHP_BINARY … 2>&1` through shell_exec(); DCL cannot run a UNIX path or redirect that way |
| strings | 007 | an over-long file name gives ENOENT, not ENAMETOOLONG |
| date | bug33532 | the C RTL's strftime("%Z") prints `GMT` for UTC |
| dom | XMLDocument_fromString_04 | vms-libxml2 is built without iconv: no Windows-1251 |
| spl | bug51068 | a directory named `*.dir`, the VMS directory file type |
| spl | SplFileObject_fwrite_variation_001/002 | data read back with no flush (#39) |
| spl | SplFileInfo_getInode_basic | compares with `ls -i` |
| zlib | gzencode_variation2 | gzip header OS byte 2 (VMS), not 3 (Unix): correct for VMS |
| zlib | zlib_wrapper_flock_basic | flock(LOCK_SH) on a write-only handle |
| openssl | gh9339 | the test writes the node's path, `/DISK$SYSDUMP/...`, into an OpenSSL config file, whose parser reads `$SYSDUMP` as a variable (#45) |

## Not covered here

- **mysqli** runs in Phase 4 against a database server (D14). In Phase 2 it is covered only by
  building and loading the extension.
- The sets are the plan's: Zend, the standard extension (arrays, strings, math, file),
  json, pcre, date, mbstring, dom/xml/simplexml, session, spl, random, uri, openssl, zlib,
  ctype, plus Zend's fiber and generator tests.

## For Phase 3 and 4

- **Large directories:** a directory with tens of thousands of files is slow on ODS-5, so
  watch the size of phpBB's `cache/` and `store/` directories under load.
- **Version limits:** files PHP rewrites and renames keep one version (D13). Files written by
  other means do not.
- **Unprivileged account:** the pool should run as an unprivileged account. The tests run
  without file-protection privileges (D12, still proposed).
- **Visibility of writes:** data a request writes is visible to others after fflush() or
  close. phpBB closes its cache and session files.
- **exec():** result codes come from `$STATUS`. A command containing `|` or `(` keeps
  pclose()'s value, and a command that ends DCL itself (EXIT) does too.
- **Resolver:** gethostbyname() and gethostbynamel() were broken until patches 0018–0019, so
  check SMTP and any host-name lookups in Phase 4.
- **OpenSSL config files:** a path containing `$` must be escaped (`\$`).

## Open items

- **For the user:**
  - D12 (tests without file-protection privileges) is still proposed.
  - Phase 2 exit review, and the go-ahead for Phase 3.
- **Not yet traced:** the stray `:` after arginfo_zpp_mismatch's `===DONE===`.
- **Possible small fixes:**
  - chmod(f, 0) could map to mode 01000, which gives a true 0000 (#42).
  - flock(LOCK_SH) on a write-only handle could take a write lock.
  Neither affects phpBB.
