# Phase 5: kit and release

**Status:** done (2026-10-18). Open items are at the end.

## The kit (D25)

**PCSI product `VMSPHP` V8.5-11E1**, `ISSINOHO-X86VMS-VMSPHP-V0805-11E1-1.PCSI` (43.9 MB).
`tools/kit.sh` builds it (CLEAN by default, `NOCLEAN=1` when the tree is already clean-built),
and `vmsport/kit/MAKE_KIT.COM` packages it on the node.

| In the kit | |
|---|---|
| `[VMSPHP.BIN]` | `PHP.EXE`, `PHP_CGI.EXE` |
| `[VMSPHP.ETC]` | `PHP.INI-DIST` (CLI), `PHP-POOL.INI-DIST` (FastCGI pool, the settings phpBB runs on, under a `VMSPHP_POOL` rooted logical), `CACERT.PEM`, `[.CERTS]` (121 hashed certificates, `openssl.capath`) |
| `[VMSPHP.COM]` | `VMSPHP$SETUP.COM`: the `php` and `php_cgi` symbols |
| `[VMSPHP.DOC]` | `README.VMS`, PHP's `LICENSE`, `ARGON2.LICENSE`, `APACHE.MD`, `NEWS` |
| `SYS$STARTUP` | `VMSPHP$STARTUP.COM`: `VMSPHP$ROOT` (executive mode) and `USE_ZEND_ALLOC "1"`; with `INSTALL`, it also writes `[ETC]PHP.INI` from `PHP.INI-DIST` if there is none |

## The install check (on the live node)

1. **The board's own file cache:** the live pool was moved first to its own
   `[VMSLIGHTTPD_DATA.PHP.OPCACHE]`, owned by LIGHTTPD, so it no longer depends on where PHP is
   installed.
2. **`PRODUCT INSTALL VMSPHP`** installed the kit in `SYS$COMMON:[VMSPHP]`. The node's own
   supervisor-mode `DEFINE/SYSTEM VMSPHP$ROOT` hid the kit's executive-mode definition (#67).
   With that deassigned, and `VMSPHP$STARTUP` in the startup procedure instead, the board ran
   on the kit:
   - the pages, login and the registration CAPTCHA;
   - `php -v` through `VMSPHP$SETUP`;
   - verified HTTPS through the kit's `[ETC.CERTS]`: badssl.com accepted, self-signed and
     wrong-host certificates rejected.

## Releases

- **vms-php:** [v8.5.11-vms1](https://github.com/issinoho/vms-php/releases/tag/v8.5.11-vms1),
  the kit and `SHA256SUMS`. The repository's `README.md` is written.
- **vms-lighttpd:** [v1.4.85-vms3](https://github.com/issinoho/vms-lighttpd/releases/tag/v1.4.85-vms3),
  with the x86-64 and IA64 kits (vms-lighttpd D13):
  - a pool template for this PHP, `[CONF]PHP-VMSPHP.INI`;
  - `VMSLIGHTTPD$CONFIGURE` picks it when it finds `VMSPHP$ROOT`'s kit, creates
    `[PHP.OPCACHE]`, and offers `VMSPHP$ROOT` as the default PHP root;
  - `PHP_POOL.COM` creates `[.OPCACHE]`.

  `test_php.sh` passes 19 of 20 with that template on x86 (the expected gd/pdo_mysql check).

## The website

openvms.issinoho.com is built from vms-grep's `site/`. PHP was added there:
- to `projects.json`, with the release snapshot `releases.json` regenerated (27 releases);
- to vms-grep's `README.md` list and table;
- to the upstream watch (`tools/upstream-watch.json`: anitya 3627, the 8.5 series).

lighttpd's entry moved to vms3, without the preview label. The Pages deploy of vms-grep
5bf7f8c succeeded.

## Also in this phase

- **`docs/APACHE.md`:** PHP behind VSI's Apache over `mod_proxy_fcgi`. It is a draft:
  Apache's modules load, but on this node Apache cannot create a listening socket even with
  VSI's own configuration, which is a question for VSI.

## Open items

- **`DKA0:[VMSPHP]`**, the hand-installed tree from Phase 4, is no longer used and can be
  removed by the user.
- **The live board** still runs lighttpd vms2 with a hand-made pool `PHP.INI` equivalent to
  vms3's template. Upgrading it to vms3 (`PRODUCT INSTALL LIGHTTPD`) keeps its configuration.
- **VSI Apache:** listener sockets on the node (`docs/APACHE.md`).
- **MariaDB's stalls** are being handled in another session
  (`vms-mariadb/docs/INCIDENT_2026-10-17.md`).

## Update: 8.5.12-vms1 (2026-10-21)

PHP 8.5.12 is a security release: CVE-2026-109618 and CVE-2026-109619 (GHSA-w8hw-w9fx-j79w,
GHSA-6wmf-4rv5-p3rp, both critical, remote code execution through state left corrupted by an
out-of-memory or other bailout), fixed in the Zend engine.

- **Pin:** `upstream.conf` re-pinned to 8.5.12. The tarball is signed by another 8.5 release
  manager, so `fetch.sh` now accepts any of the three (#68, D27).
- **Patches:** 0001–0023 apply unchanged (0001 and 0014 at an offset). Upstream changed none of
  their lines. `gen_build.py` took the same 235 `php_config.h` decisions and the same
  627-object list as for 8.5.11.
- **Build:** from CLEAN on x86, 639 compiled, 0 failed.
- **Tests (the sets the fix touches), against a Linux build of 8.5.12:**

  | Set | VMS pass / fail / skip | Linux pass / fail / skip | VMS-only failures |
  |---|---|---|---|
  | zend | 5251 / 16 / 30 | 5253 / 14 / 30 | bug80811, new_oom (both known in 8.5.11) |
  | fibers | 297 / 0 / 0 | 297 / 0 / 0 | none |

  The smoke test's memory exhaustion (`memory_limit=8M`) ends in PHP's fatal error, with no crash.
- **Kit:** `ISSINOHO-X86VMS-VMSPHP-V0805-12E1-1.PCSI`. `PRODUCT INSTALL VMSPHP` upgraded
  V8.5-11E1 on the live node at 20:21. The site's `[ETC]PHP.INI` and both logicals were
  unchanged.
- **The board:** `VMSLIGHTTPD$CONTROL RESTART` at 20:28 (lighttpd down for 3 s). All four
  workers run the kit's new `PHP_CGI.EXE`. The index, a forum, login, search and the CAPTCHA
  image answer 200.
- **Release:** [v8.5.12-vms1](https://github.com/issinoho/vms-php/releases/tag/v8.5.12-vms1),
  the kit and `SHA256SUMS`. The README has the family banner and a notice of the two CVEs.
- **The website:** vms-grep a542625. The PHP card has a note tag naming the CVEs, the README
  row moved to vms1 of 8.5.12, and `releases.json` was regenerated. The Pages deploy succeeded.
- **Not done yet:** the other phpt sets have not been re-run on 8.5.12.

## Update: 8.5.13-vms1 (2026-11-27)

PHP 8.5.13 is a security release: 17 GHSAs, eleven in parts this port builds (Core
CVE-2026-129474 and -129182, CGI -129538, date -129496, DOM -129501, session -126714, -115560
and -129481, standard -128064 and -127960, URI -129438). The other six are in FPM, Phar and
SOAP, which are not built.

- **Pin:** 8.5.13, signed by Volker Dusch. zlib 1.4 (vms-zlib's clang tree) and the CA bundle
  of 2026-11-05.
- **Patches:** 0011 (ASN1 time as `zend_long`) rebased onto upstream's changed
  `openssl_backend_common.c`; the others apply unchanged.
- **Build:** the first ALL failed on `libxml/parser.h` (#69): the node's dependency directory had
  been recreated without the clang trees. After restoring them, from CLEAN on x86: 653
  compiled, 0 failed (639 for 8.5.12), no compiler diagnostics.
- **Tests:** all 24 sets, against a Linux build of 8.5.13: 13,107 tests, 12,287 pass, 191
  fail, 629 skipped; 134 fail on VMS only (106 of them in `file`). Against the sets' last
  VMS runs (8.5.11, and 8.5.12 for zend and fibers), no VMS-only failure went away except two
  that now pass (`file/php_fd_wrapper_04`, `mbstring/mb_ereg_replace_kname_unterminated_nul`),
  and 12 are new, all accepted by the user as explained:

  | Tests | Why they fail on VMS |
  |---|---|
  | date: 9 `oom_*` (`oom_datetime_construct`, `oom_date_modify`, ...), new in 8.5.12 | PHP's output is right, but the exception message holds the 4 MB input string, and the run's output file splits it into 32,765-byte records, so the comparison sees extra newlines. Test harness, not PHP |
  | zlib: `oom_deflate_add`, `oom_gzcompress`, new in 8.5.12 | they fill the heap to 512 MB to reach PHP's own limit; the test account's page-file quota stops the `mmap()` first (`exceeded pagefile quota`, then `Out of memory`). Account quota, not PHP |
  | session: `bug61470` (passed on 8.5.11) | 8.5.13's fix for CVE-2026-126714 makes the files handler check `ftruncate()` before every write (8.5.12 ignored it, and only when the data shrank). The test deletes its own open session file; on VMS `ftruncate()` of that file then fails `ENOENT`, so the write is refused with a warning, where Linux writes into the unlinked file. Either way the data is gone; only the warning is VMS-only. Other session tests (248 pass) and the boards are unaffected |

- **Board:** the SQLite test board (`p6.sh`, its own pool) passed 14 of 14 on the staged 8.5.13.
- **Kit:** `ISSINOHO-X86VMS-VMSPHP-V0805-13E1-1.PCSI`. On the live node `PRODUCT INSTALL
  VMSPHP` upgraded V8.5-12E2 at 12:03, then `VMSLIGHTTPD$CONTROL RESTART`: lighttpd and the
  four workers restarted in under a minute, and the workers took work at once. They run the
  kit's `PHP_CGI.EXE`; `php -v` and `php_cgi -v` report 8.5.13. The board's index, a forum and
  search answer 200 with no SQL or PHP error, and Steward (8443) answers as before.
- **Release:** [v8.5.13-vms1](https://github.com/issinoho/vms-php/releases/tag/v8.5.13-vms1).
  The kit's `NEWS` has no 8.5.13 entry (written after the kit was built); the next kit has it.
