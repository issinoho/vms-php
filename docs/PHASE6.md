# Phase 6: 8.5.12-vms2, SQLite and PDO (D28)

**Status:** built, tested and kitted (2026-10-23): kit V8.5-12E2. Not installed and not released:
both with the user.
vms-sqlite's Phase 3 (its `docs/PHASE3.md`).

## Plan

1. Build: `pdo`, `pdo_mysql`, `sqlite3`, `pdo_sqlite` with vms-sqlite's library (D28).
2. The `.phpt` sets of `sqlite3`, `pdo`, `pdo_sqlite` against `cache/refbuild`.
3. A phpBB 3.3 test board on SQLite (own directory, port, pool), `test_phpbb.py` and a load
   run: the acceptance test of vms-sqlite.
4. The kit (8.5.12-vms2), the README, `readme.vms` and `NEWS`, with the user.

## Results (x86-64, 2026-10-23)

| Test | Result |
|---|---|
| Build CLEAN, then ALL (D28) | 653 objects, 0 failed; `PHP.EXE`, `PHP_CGI.EXE` linked |
| Smoke: `get_loaded_extensions()`, `SQLite3::version()` | sqlite3, PDO, pdo_mysql, pdo_sqlite; SQLite 3.54.1; no `loadExtension`; `journal_mode=WAL` stays `delete` |
| `.phpt` set `sqlite`: ext/sqlite3, ext/pdo (through pdo_sqlite, `ENV.TXT`), ext/pdo_sqlite: 318 tests | 299 pass, 3 fail, 16 skip: exactly as the Linux reference (the same 3 fail there): **0 fail on VMS only** |
| The SQLite board (D29, `tools/p6.sh`): phpbbcli install by the vms2 PHP | installed; viglink disabled |
| `test_phpbb.py` against :18081 | **14 passed, 0 failed** (posting the first topic: 19.8 s) |
| Load: `p6.sh soak 30 4` (vms-lighttpd's soak.sh, SOAK_PHPBB=1) | **2,319 requests, all 200, mean 3.05 s, max 12.4 s**; the MySQL board's baseline with the same run: 1,417, mean 5.0 s (PHASE4) |
| After the load | `integrity_check` ok; no worker crash, restart or PHP error; no server error |

Not run: the `pdo_mysql` set (needs a MySQL server: as mysqli's, D14).

## Notes

- A database file deleted by an application leaves its `<db>-lock` (vms-sqlite D5, D8 (3)):
  to delete a database, delete both.
- The board's pool, server and `VMSPHP2$ROOT` are stopped and removed after the tests;
  `p6.sh logical`, `poolstart`, `start` bring them back (`[.PHPBB6]` and its database stay).

## The kit (D25)

**PCSI product `VMSPHP` V8.5-12E2**, `ISSINOHO-X86VMS-VMSPHP-V0805-12E2-1.PCSI` (48,791,552
bytes; vms1: 44.0 MB), `NOCLEAN=1 tools/kit.sh x86` on the clean-built tree (the images
relinked from the tested build's objects). SHA-256
`4dc7de9bb5ee5abeb4486fe92f83ad134723552e74db6ae48abea295eefa8066`.
- New in `[DOC]`: `SQLITE.NOTICE` (SQLite is in the public domain; the version and vms-sqlite),
  and the licences of the libraries built in since vms1, missing until now: `ZLIB.LICENSE`
  (zlib 1.3.2's `LICENSE`), `PCRE2.LICENSE` (PCRE2 10.49's `LICENCE.md`), `LIBXML2.LICENSE`
  (libxml2 2.15.4's `Copyright`), copied verbatim from the releases pinned in `upstream.conf`
  (the sibling repositories' verified tarballs).
- `NEWS`: the 8.5.12-vms2 entry. `README.VMS` section 2: the extensions, and SQLite's notes for
  administrators (sharing a database between accounts, the `-lock` file, no WAL, no
  `loadExtension`, access by UIC protection); section 4: phpBB on SQLite.
- README.md (status, extensions, SQLite, applications) and APACHE.md updated.

Next, with the user: installing it (`PRODUCT INSTALL VMSPHP`) replaces the live board's PHP,
so it is the user's to do or to ask for, then the board's pool restarted; the GitHub release
`v8.5.12-vms2` (kit, `SHA256SUMS`) and the website entry.
