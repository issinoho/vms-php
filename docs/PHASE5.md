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
- **Not done yet:** the GitHub release v8.5.12-vms1 and the website entry. The other phpt sets
  have not been re-run on 8.5.12.
