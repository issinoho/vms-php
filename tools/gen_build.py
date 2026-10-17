#!/usr/bin/env python3
"""gen_build.py <hostconf-tree> <stage-tree> <probes-dir> - the VMS build inputs.

From the host configure run (tools/host_configure.sh) and the Phase 0 probe
results, write into the staged tree:

  main/php_config.h          the host's, with every platform answer decided for
                             VMS (headers and functions from the probes, the rest
                             from vmsport/config/php_config.overrides); stops if
                             a host #define is neither probed nor listed
  main/build-defs.h          install paths under /vmsphp$root
  main/internal_functions.c, main/internal_functions_cli.c, Zend/zend_config.h,
  ext/date/lib/timelib_config.h, ext/mbstring/libmbfl/config.h
                             copied: generated, but not platform facts
  vmsport/build/objects.txt  one line per object: <group> <object> <source>
                             [-I/-D flags of that object], groups GLOBAL, CLI,
                             CGI, FASTCGI, BINARY (the host Makefile's lists)
  vmsport/build/includes.txt the global -I list, relative to the tree top, plus
                             the dependency trees
  vmsport/build/config.log   every decision taken for php_config.h
"""
import os, re, shutil, sys

host, stage, probes = sys.argv[1:4]
out_build = os.path.join(stage, 'vmsport', 'build')
os.makedirs(out_build, exist_ok=True)
log = []

# ---------------------------------------------------------------- probes
results = {}
# later files win: the G re-run (probes-x86-clang-G.txt) has the g_m_ probes
for name in ('probes-x86-clang.txt', 'probes-x86-clang-G.txt'):
    p = os.path.join(probes, name)
    if not os.path.exists(p):
        continue
    for line in open(p, errors='replace'):
        m = re.match(r'RESULT (\S+) cc=(\S+) link=(\S+)', line)
        if m:
            n, cc, ln = m.groups()
            ok = cc == '1' and (n.startswith('h_') or ln == '1')
            results[n] = ok

# ------------------------------------------------------------- overrides
overrides = {}
for line in open(os.path.join(stage, 'vmsport', 'config', 'php_config.overrides')):
    line = re.sub(r'\s+#.*$', '', line.strip())     # trailing "  # reason"
    if not line or line.startswith('#'):
        continue
    name, _, value = line.partition(' ')
    overrides[name] = value.strip()

# --------------------------------------------------------- php_config.h
src = open(os.path.join(host, 'main', 'php_config.h')).read().split('\n')
outl, unreviewed = [], []
for line in src:
    m = re.match(r'#define (\w+)(?:\s+(.*))?$', line)
    if not m:
        # a feature the host build leaves out, which VMS has (an override with
        # a value, e.g. HAVE_ARGON2LIB from the compiled-in reference library)
        u = re.match(r'/\* #undef (\w+) \*/$', line)
        if u and overrides.get(u.group(1), '-') not in ('-', '='):
            outl.append('#define %s %s  /* VMS: overrides */' % (u.group(1), overrides[u.group(1)]))
            log.append('%-40s %-8s %s' % (u.group(1), 'value', 'overrides (host: undef)'))
            continue
        outl.append(line)
        continue
    name, value = m.group(1), (m.group(2) or '')
    decision = None
    if name in overrides:
        v = overrides[name]
        decision = 'undef' if v == '-' else ('keep' if v == '=' else 'value:' + v)
        why = 'overrides'
    else:
        hm = re.match(r'HAVE_(\w+)_H$', name)
        fm = re.match(r'HAVE_(?:DECL_)?(\w+)$', name)
        if hm and 'h_' + hm.group(1).lower() + '_h' in results:
            decision = 'keep' if results['h_' + hm.group(1).lower() + '_h'] else 'undef'
            why = 'probe h_%s_h' % hm.group(1).lower()
        elif fm and 'g_' + fm.group(1).lower() in results:
            decision = 'keep' if results['g_' + fm.group(1).lower()] else 'undef'
            why = 'probe g_%s' % fm.group(1).lower()
    if decision is None:
        unreviewed.append(name)
        outl.append(line)
        continue
    if decision == 'keep':
        outl.append(line)
    elif decision == 'undef':
        outl.append('/* #undef %s */  /* VMS: %s */' % (name, why))
    else:
        outl.append('#define %s %s  /* VMS: %s */' % (name, decision[6:], why))
    log.append('%-40s %-8s %s' % (name, decision.split(':')[0], why))
if unreviewed:
    sys.exit('gen_build: host php_config.h defines with no probe and no override '
             '(add them to vmsport/config/php_config.overrides):\n  ' + '\n  '.join(unreviewed))
open(os.path.join(stage, 'main', 'php_config.h'), 'w').write('\n'.join(outl))
open(os.path.join(out_build, 'config.log'), 'w').write('\n'.join(log) + '\n')

# ---------------------------------------------------------- build-defs.h
defs = open(os.path.join(host, 'main', 'build-defs.h')).read()
vms = {
    'CONFIGURE_COMMAND': '"cross-configured on Linux; see vmsport/build/config.log"',
    'PHP_PROG_SENDMAIL': '""',
    'PHP_INCLUDE_PATH': '"."',
    'PHP_EXTENSION_DIR': '"/vmsphp$root/lib/extensions"',
    'PHP_PREFIX': '"/vmsphp$root"',
    'PHP_BINDIR': '"/vmsphp$root/bin"',
    'PHP_SBINDIR': '"/vmsphp$root/bin"',
    'PHP_MANDIR': '"/vmsphp$root/doc"',
    'PHP_LIBDIR': '"/vmsphp$root/lib"',
    'PHP_DATADIR': '"/vmsphp$root/lib"',
    'PHP_SYSCONFDIR': '"/vmsphp$root/etc"',
    'PHP_LOCALSTATEDIR': '"/vmsphp$root/var"',
    'PHP_CONFIG_FILE_PATH': '"/vmsphp$root/etc"',
    'PHP_CONFIG_FILE_SCAN_DIR': '""',
    'PHP_SHLIB_SUFFIX': '"exe"',
}
for k, v in vms.items():
    defs, n = re.subn(r'(#define\s+%s\s+).*' % k, lambda m: m.group(1) + v, defs)
    if n != 1:
        sys.exit('gen_build: build-defs.h has no single %s' % k)
open(os.path.join(stage, 'main', 'build-defs.h'), 'w').write(defs)

for f in ('main/internal_functions.c', 'main/internal_functions_cli.c', 'Zend/zend_config.h',
          'ext/date/lib/timelib_config.h', 'ext/mbstring/libmbfl/config.h'):
    shutil.copy(os.path.join(host, f), os.path.join(stage, f))

# -------------------------------------------------------- object list
mk = open(os.path.join(host, 'Makefile')).read().replace('\\\n', ' ')
def var(name):
    m = re.search(r'^%s\s*=(.*)$' % name, mk, re.M)
    return m.group(1).split() if m else []
prefix = os.path.realpath(host) + '/'
def rel(p):
    p = os.path.realpath(p) + ('/' if p.endswith('/') else '') if p.startswith('/') else p
    if p.rstrip('/') == prefix.rstrip('/'):
        return '.'
    return p[len(prefix):] if p.startswith(prefix) else p
groups = {'GLOBAL': var('PHP_GLOBAL_OBJS'), 'CLI': var('PHP_CLI_OBJS'),
          'CGI': var('PHP_CGI_OBJS'), 'FASTCGI': var('PHP_FASTCGI_OBJS'),
          'BINARY': var('PHP_BINARY_OBJS')}
# internal_functions.c for the CGI image (configure lists only the CLI one)
groups['BINARY_CGI'] = ['main/internal_functions.lo']
rules = {}
for m in re.finditer(r'^(\S+\.lo):\s*(\S+)\n\t(.*)$', mk, re.M):
    obj, srcp, cmd = m.groups()
    cmd = cmd.split('$(CC)', 1)[1].split(' -c ', 1)[0]
    flags = []
    for t in cmd.split():
        if t.startswith('-I'):
            d = rel(t[2:]).rstrip('/') or '.'
            if d not in flags and '-I' + d not in flags:
                flags.append('-I' + d)
        elif t.startswith(('-D', '-U')) and t not in ('-DZEND_SIGNALS',):
            flags.append(t)
    rules[obj] = (rel(srcp), flags)
rules.setdefault('main/internal_functions.lo', ('main/internal_functions.c', ['-Imain']))
def vms_spec(p):
    """ext/json/json_parser.tab.c -> [.ext.json]json_parser^.tab.c (ODS-5: a dot
    that is not the type separator is escaped; BUILD.COM's date checks use it)"""
    parts = p.split('/')
    d, f = parts[:-1], parts[-1]
    stem, dot, typ = f.rpartition('.')
    f = stem.replace('.', '^.') + dot + typ if dot else f
    return ('[.' + '.'.join(x.replace('.', '^.') for x in d) + ']' if d else '[]') + f
lines, seen = [], set()
for g, objs in groups.items():
    for o in objs:
        if o not in rules:
            sys.exit('gen_build: no rule for %s' % o)
        s, fl = rules[o]
        if s.endswith('.c') and not os.path.exists(os.path.join(stage, s)):
            sys.exit('gen_build: %s: no source %s in the staged tree' % (o, s))
        # one flat object name per source, unique: ext/date/lib/astro.lo -> ext_date_lib_astro
        on = o[:-3].replace('/', '_').replace('-', '_')
        if on in seen:
            sys.exit('gen_build: duplicate object name %s' % on)
        seen.add(on)
        # D7: sources that use OpenSSL (directly, or through ext/openssl's
        # header) see SSL3's long-returning calls through sign-extending wrappers
        if s.endswith('.c'):
            text = open(os.path.join(stage, s), errors='replace').read()
            if s.startswith('ext/openssl/') or 'openssl/' in text or 'php_openssl.h' in text:
                fl = fl + ['-include', 'vmsport/include/vms_ssl3_long.h']
        lines.append(' '.join([g, on, s, vms_spec(s)] + fl))
open(os.path.join(out_build, 'objects.txt'), 'w').write('\n'.join(lines) + '\n')

inc = []
for t in var('INCLUDES'):
    d = rel(t[2:]).rstrip('/')
    if '/hostlibs/' in d or d.startswith('/usr/'):
        continue                    # host libxml2 / system headers: replaced below
    inc.append('-I' + (d or '.'))
inc += ['-I/libxml2$root/include', '-I/pcre2$root/include', '-I/zlib$root/include']
inc += ['-Ivmsport/argon2/include']    # D24: compiled in by BUILD.COM
open(os.path.join(out_build, 'includes.txt'), 'w').write('\n'.join(inc) + '\n')

print('gen_build: php_config.h (%d decisions), %d objects (%s)' % (
    len(log), len(lines), ', '.join('%s %d' % (g, len(o)) for g, o in groups.items())))
