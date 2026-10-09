#!/usr/bin/env python3
"""layout_check.py <dir> - compare struct layouts across PHP's sources.

<dir> holds one clang -fdump-record-layouts-simple dump per source
(BUILD.COM LAYOUT, fetched by tools/layout.sh).  A named struct whose size,
alignment or field offsets differ between two sources is printed with the
sources on each side: code in one would read or write the other's memory at
the wrong offsets.  Anonymous records are compared through their parents.
"""
import os, re, sys, collections

d = sys.argv[1]
layouts = collections.defaultdict(lambda: collections.defaultdict(list))
for f in sorted(os.listdir(d)):
    txt = open(os.path.join(d, f), errors='replace').read()
    cur, body = None, []
    def flush():
        if cur and 'anonymous' not in cur:
            layouts[cur]['\n'.join(body)].append(f)
    for line in txt.split('\n'):
        m = re.match(r'Type: (.*)', line.strip())
        if m:
            flush()
            cur, body = re.sub(r';\d+', '', m.group(1)), []
            continue
        if cur and re.match(r'\s*(Size|DataSize|Alignment|FieldOffsets)', line):
            body.append(line.strip())
    flush()

bad = 0
for name in sorted(layouts):
    variants = layouts[name]
    if len(variants) > 1:
        bad += 1
        print('MISMATCH', name)
        for body, files in variants.items():
            print('   ', body.replace('\n', '  '), ' <-', ', '.join(files[:4]),
                  '(+%d)' % (len(files) - 4) if len(files) > 4 else '')
print('layout_check: %d structs, %d with differing layouts' % (len(layouts), bad))
sys.exit(1 if bad else 0)
