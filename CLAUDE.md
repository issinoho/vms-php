# CLAUDE.md

PHP for OpenVMS x86-64, built natively with VSI's clang and wrapped by the same tooling as
`~/projects/vms-mariadb`, `vms-lighttpd`, `vms-curl` and the other sibling ports. Read
vms-grep's `CLAUDE.md` for the ground rules and the VMS/ssh pitfalls, and vms-mariadb's
`CLAUDE.md` for the clang ones; they all apply. Read the plan
(`docs/PHP_OPENVMS_PLAN.md`) and `docs/DECISIONS.md` before starting work. Work phase by
phase; stop and ask at each phase exit.

**Acceptance:** a drop-in replacement for the phpBB board's PHP (vms-lighttpd's FastCGI pool
runs our `PHP_CGI.EXE -b`; our `PHP.EXE` runs phpBB's CLI), including SMTP mail over TLS,
with no crashes under load.

## Ground rules (from the sibling ports)

- **Never edit `staging/`, `cache/` or `out/`.** Every build starts from the signed release
  tarball pinned in `upstream.conf` (key in `keys/`). Upstream files change only through
  `patches/` (listed in `patches/series`); our own files live in `overlay/`, which may only
  add files.
- **Keep VMS changes minimal and upstreamable:** guard with `#ifdef __VMS`, one fix per patch,
  each with a `Subject:` line and the VMS reason.
- **Use `tools/vms.sh`** for remote work; never raw `ssh host cmd`, never `WAIT` over ssh.
  Run long jobs (probes, builds) with `run_in_background`.
- **Committed files must not contain real node details** (they live in the git-ignored
  `tools/nodes.conf`), nor credentials. Use `<x86-host>`, `<ia64-host>`.
- **Nothing destructive or system-wide on the nodes** without asking: no DELETE/PURGE outside
  our work directories, no installs, no system logical names. The live board (vms-lighttpd
  service on x86) is the user's: read its logs, never touch its files or processes.
- Every build failure gets one entry in `docs/PORTING_LOG.md`; every decision goes in
  `docs/DECISIONS.md` with the alternatives considered.
- Prefer a 20-line probe in `probes/` over speculation. Do not claim something works until a
  test on the node shows it.

## Platform facts that shape the port (details in docs/DECISIONS.md)

- **clang (LP64) for everything** (D2): VSI C on x86-64 is ILP32 with a 32-bit `size_t`,
  which PHP does not support as a 64-bit build. Dependencies must be clang builds too
  (vms-zlib and vms-pcre2 clang variants, vms-libxml2), and VSI SSL3 is used through its
  64-bit-pointer images with an audit of `long`-typed calls (vms-mariadb D9).
- VSI's clang does not define `__GNUC__`; the C RTL's `long` interfaces are 32-bit
  (vms-mariadb D12, forced-include header); `time_t` is 32-bit.
- **No fork:** CGI runs only as `-b` with `PHP_FCGI_CHILDREN=0`; `proc_open`, `pcntl` and
  `mail()` are not available in the first kit.
- C diagnostics go to SYS$ERROR: `define sys$error sys$output` in procedures run by vms.sh.
- **Builds from CLEAN** whenever a header or flag changes: the VMS build tracks neither
  (vms-lighttpd PORTING_LOG #39, vms-mariadb).

## Commands

```sh
tools/fetch.sh                        # download + verify the pinned PHP release into cache/
tools/probe.sh x86                    # Phase 0 probes (clang) -> docs/probes-x86-clang.txt
tools/vms.sh <node> dcl '<cmd>' ...   # run DCL; also run/batch/put/get
tools/prepare.sh; tools/build.sh x86 CLEAN; tools/build.sh x86 ALL   # stage, then build (CLEAN after any header/flag change)
tools/host_sqlite3.sh                 # host SQLite for configure (vms-sqlite's release and options; D28)
tools/refbuild.sh                     # the Linux reference cache/refbuild (same release and options)
python3 tools/phpt.py prepare <set> <ext/x/tests>...; tools/phpt.sh x86 <set>   # .phpt vs the reference
tools/stage.sh x86                    # the staged install tree <workdir>.VMSPHP] (the test pools' PHP)
tools/p6.sh x86 logical|poolstart|start|test|soak|stop|poolstop|nological   # the SQLite test board (D29)
NOCLEAN=1 tools/kit.sh x86            # the PCSI kit -> out/kits/ (without NOCLEAN: CLEAN first)
```

State (2026-10-23): 8.5.12-vms2 (SQLite and PDO, D28) is released and installed on the live node;
the board runs on it. Its record is `docs/PHASE6.md`. SQLite comes from vms-sqlite's install tree
in the dependency directory (its `tools/deploy.sh`, its D10).

## Commits

Commit in logical steps with messages that explain the VMS reason for each change. Don't push
without the user asking.
