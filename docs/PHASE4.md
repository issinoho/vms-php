# Phase 4: phpBB acceptance

**Status:** the test board's checks are done (2026-10-17); waiting for the user's review.
The board switch-over, which the plan does with the user, has not started. Its prerequisites
and the open items are at the end.

The x86-64 node, 2026-10-17. PHP 8.5.11 built with VSI clang (LP64), patches 0001–0021 plus
`overlay/vmsport`. It runs as vms-lighttpd's FastCGI test pool (`LTPHP_19100`–`19103`, the
Phase 3 pool with the opcache file cache, D19) behind the Phase 4 test server on :18080.
The live board was not touched.

## The test board

vms-lighttpd's Phase 4 environment from 2026-10-08 was gone: the phpBB tree, the `[.T4]`
configuration and `cache/db.secrets`. It was rebuilt like this:

1. phpBB 3.3.19, the version pinned in vms-lighttpd's `upstream.conf`, was fetched and its
   SHA-256 checked.
2. Its 3,893 files were uploaded to `<lighttpd workdir>.PHPBB]` with the new
   `tools/vms.sh putr` (sftp `put -r`).
3. phpBB's CLI installer (`install/phpbbcli.php install`) ran with **our `PHP.EXE`**,
   against a database the user provided (`db.secrets`, git-ignored).
   - The installer's configuration was in an owner-only directory on the node and was deleted
     straight after.
   - As on 2026-10-08, the installer enabled `phpbb/viglink` although the list was empty.
     `extension:disable` turned it off.
4. `serve.sh x86 phpbbsetup` wrote `[.T4]`, and the test server was restarted on it.

### Reinstall after the database loss (#60)

Late on 2026-10-17 the stalled MariaDB server was stopped with a kill. The test database's
tables were MyISAM, the server's default engine, and 64 of its 69 tables came back empty. The
user dropped and recreated it, and the board was installed again the same way. All 69 tables
were then converted to Aria (probe r_phpbb_aria.php), the crash-safe engine the board's
earlier `phpbb3319` database uses. `test_phpbb.py` passed 14 of 14 again, and a 10-minute
load run with the same guest pages and 4 clients served 588 requests, all 200, with a mean of
3.9 s and no worker errors.

## Results

| Plan item | Result |
|---|---|
| Test board on our PHP: `test_phpbb.py` | **14 of 14**, before and after the D21 rebuild |
| ACP by hand | used by the user: email settings saved, a test email sent, the admin's email address changed (after D21) |
| `phpbbcli.php` with `PHP.EXE` | `install`, `db:migrate --safe-mode`, `cache:purge`, `cron:list`, `cron:run` (18 tasks), `extension:disable`/`extension:show`, `config:get`/`config:set` |
| SMTP test email | sent by phpBB's own SMTP code to the node's SMTP server on localhost, and delivered (D20: that server offers no STARTTLS) |
| Load run, at least the 8.1.23 baseline, no crashes | **1,417 requests, all 200, no crash** (8.1.23: 1,409, 4 ACCVIOs) |
| Page times against 8.1.23 | mean 5.0 s (8.1.23: 5.0 s) |

### Load runs

Both used `SOAK_PHPBB=1 soak.sh x86 30 4`, the baseline's settings: guest pages (index,
forum, topic, search, login form, FAQ, member list) for 30 minutes with 4 clients.

| | 8.1.23 (2026-10-08) | First run | Rerun |
|---|---|---|---|
| Requests | 1,409 | 1,475 | **1,417** |
| 200 / errors | 1,405 / 4 × 500 | 1,462 / 2 × 500, 11 timeouts | **1,417 / 0** |
| Mean page time | 5.0 s | 4.8 s | **5.0 s** |
| PHP crashes | 4 ACCVIOs | none | **none** |

- **The first run's errors** were PHP's 60-second hard timeout in phpBB's mysqli connect and
  query while MariaDB stalled (#59). The run overlapped the user's ACP session. The rerun had
  nobody else on the board.
- **Memory:** after warm-up, the workers' page-file quota did not move by a single pagelet
  over three batches of 280 guest pages.

### Cold starts

With MariaDB healthy:
- the first request after `cache:purge` took 19.1 s, the second 1.9 s, then 0.5–1.2 s;
- a freshly started worker's first request, with phpBB's cache intact, took 8.1 s. The
  opcache file cache saves it compiling phpBB again.

The 8.1.23 kit took 10–18 s for a first request.

## What was found and fixed

| Problem | Fix | Ref. |
|---|---|---|
| Changing an email address was an HTTP 500: `checkdnsrr()` undefined. The DNS functions were left out because the resolver is in `TCPIP$IPC_SHR`, not the C RTL | D21, patch 0021: the resolver functions are enabled and linked; host names are copied below 2 GB, because the resolver ACCVIOs on higher addresses | #55, #56 |
| 60-second failures, first read as slow cold starts | none in PHP: MariaDB stopped accepting connections (the user restored it) | #59 |
| `hrtime()` returns `false` | open (phpBB does not use it) | #58 |
| The `network` .phpt set, first run: 47 pass on VMS, 55 on Linux; its 3 DNS tests pass | 4 VMS-only failures, below | #57 |

## For the switch-over (plan: with the user)

The plan's last Phase 4 step sets `vmslighttpd_php_root` to `VMSPHP$ROOT` on the live board
and restarts it. Rollback is setting it back. These need to be settled first:

1. **The live pool's account (LIGHTTPD) must see our PHP:**
   - `VMSPHP$ROOT` and `USE_ZEND_ALLOC` are GROUP logicals in our UIC group (D15, D16).
     LIGHTTPD sees them only if it is in the same group. Otherwise they need another home,
     which is the open USE_ZEND_ALLOC decision from Phase 3.
   - LIGHTTPD must be able to read the staged tree in our work directory, and write
     `[TMP.OPCACHE]` (D19).
   - Or install our tree where the kit will put it.
2. **The pool's PHP.INI for 8.5:** the board's ini is the 8.1 kit's. It loads opcache as a
   `zend_extension` (D17), and has no `opcache.file_cache` (D19) or `openssl.cafile` (#57).
3. **Privileges:** the test pool ran with our account's privileges, while LIGHTTPD is
   unprivileged. phpBB's file writes (`cache/`, `store/`, `files/`) under LIGHTTPD have not
   been tested.
4. **MariaDB:** it stalled twice today (#59), and the kill that ended the second stall
   emptied the test database's MyISAM tables (#60). Until the stalls are understood, one
   during the switch-over would look like a PHP failure. The server's default engine is
   MyISAM, so any database created on it without an engine is not crash-safe.
5. **The logicals:** `VMSPHP$ROOT` and `USE_ZEND_ALLOC` are now SYSTEM logicals (D22). The
   live 8.1 pool moves to Zend's memory manager as its workers restart. Neither definition
   is in the node's startup yet.

## Open items

- **Outgoing HTTPS from PHP** has no CA certificates (#57). phpBB's version check and remote
  avatars use it. The kit's PHP.INI needs `openssl.cafile` or `openssl.capath`.
- **`hrtime()`** has no clock source on VMS (#58).
- **Concurrent session-file reads:** during the Phase 3 tests, eight parallel requests sharing
  one PHP session file logged "Read returned less bytes than requested". The likely cause is
  that data written to an open file is not visible until fsync or close (#39). phpBB keeps
  its sessions in the database.
- **The network set's other VMS-only failures:**
  - `ip2long` accepts some short forms that glibc rejects;
  - AF_UNIX sockets cannot be created under `/tmp`, which VMS does not have.
- **On the node, outside our work directories** (left for the user):
  - SMTP queue entry 514, the first test email, addressed to the installer's
    `admin@example.org`;
  - the lock files and session directories listed in PHASE3.md.
- **vms-steward:** 48 orphaned ssh clients on the host hold 49 SSH sessions on the node.
  Their parent process has exited; they are not vms-php's.
