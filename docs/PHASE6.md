# Phase 6: 8.5.12-vms2, SQLite and PDO (D28)

**Status:** started (2026-10-22). vms-sqlite's Phase 3 (its `docs/PHASE3.md`).

## Plan

1. Build: `pdo`, `pdo_mysql`, `sqlite3`, `pdo_sqlite` with vms-sqlite's library (D28).
2. The `.phpt` sets of `sqlite3`, `pdo`, `pdo_sqlite` against `cache/refbuild`.
3. A phpBB 3.3 test board on SQLite (own directory, port, pool), `test_phpbb.py` and a load
   run: the acceptance test of vms-sqlite.
4. The kit (8.5.12-vms2), the README, `readme.vms` and `NEWS`, with the user.
