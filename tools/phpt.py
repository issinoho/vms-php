#!/usr/bin/env python3
"""phpt.py - run PHP's .phpt tests against PHP for OpenVMS (Phase 2).

run-tests.php cannot run on VMS (it starts every test with proc_open, and
there is no fork), so this does run-tests' job in three steps:

  phpt.py prepare <set> <dir>...   from the staged tree: copy each test
        directory (fixtures included) to cache/phpt/<set>/, and for every test
        write <base>.php (--FILE--), <base>.skip.php (--SKIPIF--),
        <base>.clean.php (--CLEAN--) and <base>.ini (run-tests' standard
        settings plus --INI--), and LIST.TXT for vms_phpt.com.  Tests whose
        sections need a web server or a driver (--POST--, --GET--, --CGI--,
        --STDIN--, --ENV--, --ARGS--, --PHPDBG--...) or an extension we do not
        build are recorded as unsupported.
  phpt.py compare <set> <results> [ref-results]
        judge every output against --EXPECT--/--EXPECTF--/--EXPECTREGEX--
        (run-tests' rules), print a summary, and, with the Linux reference
        results, list what fails only on VMS.
  phpt.py local <set> <php> <results>
        run the prepared set with a local PHP (the host reference build) and
        write results in the same format as vms_phpt.com.

Results file: for every test "@@@TEST <dir> <base> <status>" (status: the
exit status, or SKIP), then its output, then "@@@END".
"""
import os, re, shutil, subprocess, sys

top = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
version = re.search(r'^UPSTREAM_VERSION=(\S+)', open(os.path.join(top, 'upstream.conf')).read(),
                    re.M).group(1)
stage = os.path.join(top, 'staging', 'php-' + version)

INI = ['output_handler=', 'open_basedir=', 'disable_functions=', 'output_buffering=Off',
       'error_reporting=32767', 'fatal_error_backtraces=Off', 'display_errors=1',
       'display_startup_errors=1', 'log_errors=0', 'html_errors=0', 'track_errors=0',
       'report_zend_debug=0', 'docref_root=', 'docref_ext=.html', 'error_prepend_string=',
       'error_append_string=', 'auto_prepend_file=', 'auto_append_file=',
       'ignore_repeated_errors=0', 'precision=14', 'serialize_precision=-1', 'memory_limit=128M',
       'opcache.fast_shutdown=0', 'opcache.file_update_protection=0', 'opcache.revalidate_freq=0',
       'opcache.protect_memory=1', 'zend.assertions=1', 'zend.exception_ignore_args=0',
       'zend.exception_string_param_max_len=15', 'short_open_tag=0', 'date.timezone=UTC']

BUILT = {'core', 'ctype', 'date', 'dom', 'filter', 'hash', 'json', 'lexbor', 'libxml', 'mbstring',
         'mysqli', 'mysqlnd', 'openssl', 'pcre', 'random', 'reflection', 'session', 'simplexml',
         'spl', 'standard', 'tokenizer', 'uri', 'xml', 'xmlreader', 'xmlwriter', 'opcache', 'zlib',
         'zend opcache', 'pdo', 'pdo_mysql', 'pdo_sqlite', 'sqlite3'}
UNSUPPORTED = {'POST', 'POST_RAW', 'PUT', 'GET', 'COOKIE', 'CGI', 'STDIN', 'ENV', 'ARGS', 'PHPDBG',
               'CAPTURE_STDIO', 'GZIP_POST', 'DEFLATE_POST', 'REQUEST', 'HEADERS', 'EXPECTHEADERS',
               'FILE_EXTERNAL', 'EXPECT_EXTERNAL', 'EXPECTF_EXTERNAL', 'REDIRECTTEST', 'FILEEOF',
               'XFAIL'}


def sections(path):
    secs, cur = {}, None
    with open(path, 'rb') as f:
        for raw in f.read().decode('latin-1').split('\n'):
            m = re.match(r'^--([_A-Z]+)--\s*$', raw)
            if m:
                cur = m.group(1)
                secs[cur] = []
                continue
            if cur:
                secs[cur].append(raw)
    return {k: '\n'.join(v) for k, v in secs.items()}


# Tests that hang PHP.EXE on the node, not run until the cause is fixed (each
# with its PORTING_LOG entry).  vms_phpt.com has no per-test timeout: run-tests
# kills a stuck test from outside, and a DCL watchdog would need WAIT.
# (arginfo_zpp_mismatch was here until patches 0018-0019 fixed its crash, #49-#51.)
HANGS = {
}


def prepare(setname, dirs):
    out = os.path.join(top, 'cache', 'phpt', setname)
    shutil.rmtree(out, ignore_errors=True)
    listing, unsupported, n = [], [], 0
    for d in dirs:
        src = os.path.join(stage, d)
        for root, _, files in os.walk(src):
            rel = os.path.relpath(root, stage)
            dst = os.path.join(out, rel)
            os.makedirs(dst, exist_ok=True)
            for fn in files:
                if not fn.endswith('.phpt'):
                    shutil.copy(os.path.join(root, fn), dst)
            for fn in sorted(f for f in files if f.endswith('.phpt')):
                n += 1
                s = sections(os.path.join(root, fn))
                base = fn[:-5]
                why = sorted(set(s) & UNSUPPORTED)
                exts = [e.strip().lower() for e in s.get('EXTENSIONS', '').split() if e.strip()]
                missing = [e for e in exts if e not in BUILT]
                if '%s/%s' % (rel, base) in HANGS:
                    unsupported.append('%s/%s hangs: %s' % (rel, base, HANGS['%s/%s' % (rel, base)]))
                    continue
                if why or missing or 'FILE' not in s:
                    unsupported.append('%s/%s %s' % (rel, base, ' '.join(why) or
                                                     ('ext ' + ','.join(missing) if missing else 'no FILE')))
                    continue
                shutil.copy(os.path.join(root, fn), dst)
                for sec, suffix in (('FILE', '.php'), ('SKIPIF', '.skip.php'), ('CLEAN', '.clean.php')):
                    if sec in s:
                        with open(os.path.join(dst, base + suffix), 'wb') as f:
                            f.write(s[sec].encode('latin-1') + b'\n')
                ini = list(INI) + [l.strip() for l in s.get('INI', '').split('\n')
                                   if l.strip() and not l.strip().startswith(';')]
                with open(os.path.join(dst, base + '.ini'), 'w') as f:
                    f.write('\n'.join(ini) + '\n')
                listing.append('%s %s' % (rel, base))
    with open(os.path.join(out, 'LIST.TXT'), 'w') as f:
        f.write('\n'.join(listing) + '\n')
    with open(os.path.join(out, 'UNSUPPORTED.TXT'), 'w') as f:
        f.write('\n'.join(unsupported) + '\n')
    # ext/pdo/tests run once per driver: run-tests.php does it through the driver's
    # common.phpt (--REDIRECTTEST--), which sets PDOTEST_DSN and REDIR_TEST_DIR. With
    # pdo_sqlite's tests in the set, ENV.TXT gives every test the same environment,
    # as pdo_sqlite's common.phpt does ("@" is the set's top directory; each runner
    # makes it absolute, vms_phpt.com in VMS syntax).
    env = []
    if any(d.rstrip('/') == 'ext/pdo/tests' for d in dirs) and \
       any(d.rstrip('/') == 'ext/pdo_sqlite/tests' for d in dirs):
        env = ['PDOTEST_DSN sqlite::memory:', 'REDIR_TEST_DIR @ext/pdo/tests/']
    if env:
        with open(os.path.join(out, 'ENV.TXT'), 'w') as f:
            f.write('\n'.join(env) + '\n')
    print('phpt: %s: %d tests, %d to run, %d unsupported here' % (setname, n, len(listing),
                                                                    len(unsupported)))


def expectf_to_regex(wanted):
    wanted = wanted.replace('\r\n', '\n')
    out, i = '', 0
    while i < len(wanted):
        s = wanted.find('%r', i)
        e = wanted.find('%r', s + 2) if s >= 0 else -1
        if s < 0 or e < 0:
            out += re.escape(wanted[i:])
            break
        out += re.escape(wanted[i:s]) + '(' + wanted[s + 2:e] + ')'
        i = e + 2
    # re.escape escapes '%' as '%' (Python 3.7+ leaves it alone)
    rep = {'%e': '/', '%s': r'[^\r\n]+', '%S': r'[^\r\n]*', '%a': r'.+?', '%A': r'.*?',
           '%w': r'\s*', '%i': r'[+-]?\d+', '%d': r'\d+', '%x': r'[0-9a-fA-F]+',
           '%f': r'[+-]?(?:\d+|(?=\.\d))(?:\.\d+)?(?:[Ee][+-]?\d+)?', '%c': '.', '%0': r'\x00'}
    return re.sub(r'%[esSaAwidxfc0]', lambda m: rep[m.group(0)], out)


def judge(test_path, output):
    s = sections(test_path)
    out = output.replace('\r\n', '\n').strip()
    if 'EXPECT' in s:
        return out == s['EXPECT'].replace('\r\n', '\n').strip()
    if 'EXPECTF' in s:
        rx = expectf_to_regex(s['EXPECTF'].strip())
        return re.fullmatch(rx, out, re.S) is not None
    if 'EXPECTREGEX' in s:
        return re.fullmatch(s['EXPECTREGEX'].strip(), out, re.S) is not None
    return False


def read_results(path):
    res, cur, buf = {}, None, []
    for line in open(path, 'rb').read().decode('latin-1').split('\n'):
        if line.startswith('@@@TEST '):
            _, d, b, st = line.split(' ', 3)
            cur, buf = (d.replace('\\', '/'), b, st.strip()), []
        elif line.startswith('@@@END') and cur:
            res['%s/%s' % (cur[0], cur[1])] = (cur[2], '\n'.join(buf))
            cur = None
        elif cur:
            buf.append(line)
    return res


def compare(setname, results, ref=None):
    base = os.path.join(top, 'cache', 'phpt', setname)
    got = read_results(results)
    refres = read_results(ref) if ref else {}
    tally = {'PASS': 0, 'FAIL': 0, 'SKIP': 0, 'NOTRUN': 0}
    fails, onlyvms = [], []
    for key in open(os.path.join(base, 'LIST.TXT')).read().split('\n'):
        if not key.strip():
            continue
        d, b = key.split(' ')
        k = '%s/%s' % (d, b)
        if k not in got:
            tally['NOTRUN'] += 1
            continue
        st, out = got[k]
        if st == 'SKIP':
            tally['SKIP'] += 1
            continue
        ok = judge(os.path.join(base, d, b + '.phpt'), out)
        tally['PASS' if ok else 'FAIL'] += 1
        if not ok:
            fails.append(k)
            if refres:
                rst, rout = refres.get(k, ('?', ''))
                if rst != 'SKIP' and judge(os.path.join(base, d, b + '.phpt'), rout):
                    onlyvms.append(k)
    print('phpt %s: %s' % (setname, ', '.join('%s %d' % kv for kv in tally.items())))
    with open(results + '.fail', 'w') as f:
        f.write('\n'.join(fails) + '\n')
    if refres:
        with open(results + '.onlyvms', 'w') as f:
            f.write('\n'.join(onlyvms) + '\n')
        print('phpt %s: %d fail on VMS but pass on Linux (%s.onlyvms)' % (setname, len(onlyvms), results))


def set_env(base):
    """The environment of a set's tests: ours plus its ENV.TXT (see prepare)."""
    env = dict(os.environ)
    envfile = os.path.join(base, 'ENV.TXT')
    if os.path.exists(envfile):
        for line in open(envfile).read().split('\n'):
            if line.strip():
                k, v = line.split(' ', 1)
                env[k] = base + '/' + v[1:] if v.startswith('@') else v
    return env


def local(setname, php, results):
    base = os.path.join(top, 'cache', 'phpt', setname)
    env = set_env(base)
    with open(results, 'w', encoding='latin-1') as out:
        for key in open(os.path.join(base, 'LIST.TXT')).read().split('\n'):
            if not key.strip():
                continue
            d, b = key.split(' ')
            wd = os.path.join(base, d)
            ini = b + '.ini'
            st = None
            if os.path.exists(os.path.join(wd, b + '.skip.php')):
                r = subprocess.run([php, '-n', '-c', ini, b + '.skip.php'], cwd=wd, capture_output=True,
                                   timeout=60, env=env)
                if r.stdout.decode('latin-1').strip().lower().startswith('skip'):
                    st = 'SKIP'
            text = ''
            if st is None:
                try:
                    r = subprocess.run([php, '-n', '-c', ini, b + '.php'], cwd=wd, capture_output=True,
                                       timeout=120, env=env)
                    st, text = str(r.returncode), (r.stdout + r.stderr).decode('latin-1')
                except subprocess.TimeoutExpired:
                    st, text = 'TIMEOUT', ''
                if os.path.exists(os.path.join(wd, b + '.clean.php')):
                    subprocess.run([php, '-n', '-c', ini, b + '.clean.php'], cwd=wd, capture_output=True,
                                   timeout=60, env=env)
            out.write('@@@TEST %s %s %s\n%s\n@@@END\n' % (d, b, st, text))


if __name__ == '__main__':
    cmd = sys.argv[1]
    if cmd == 'prepare':
        prepare(sys.argv[2], sys.argv[3:])
    elif cmd == 'compare':
        compare(sys.argv[2], sys.argv[3], sys.argv[4] if len(sys.argv) > 4 else None)
    elif cmd == 'local':
        local(sys.argv[2], sys.argv[3], sys.argv[4])
    else:
        sys.exit(__doc__)
