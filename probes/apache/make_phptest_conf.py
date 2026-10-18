#!/usr/bin/env python3
"""make_phptest_conf.py: a test VSI Apache configuration from VSI's own HTTPD.CONF.

A minimal hand-written httpd.conf started (APACHE$STARTUP START) but its server
exited without logging anything, while VSI's HTTPD.CONF ran the board for years.
So the test configuration is VSI's file with only what the test needs changed:
its port, PID and log files, the document root, a small prefork pool, mod_php
replaced by mod_proxy_fcgi to PHP for OpenVMS's FastCGI pool, and the site's
<VirtualHost> blocks and Include lines dropped.

usage: make_phptest_conf.py <vsi HTTPD.CONF> <docroot (UNIX form)> <port> <fcgi host:port> > out
"""
import re
import sys

src, docroot, port, fcgi = sys.argv[1:5]
out, in_vhost = [], False
for line in open(src, encoding='latin-1').read().splitlines():
    s = line.strip()
    if re.match(r'<VirtualHost\b', s, re.I):
        in_vhost = True
        continue
    if in_vhost:
        if re.match(r'</VirtualHost>', s, re.I):
            in_vhost = False
        continue
    if re.match(r'(Include|IncludeOptional)\s', s, re.I):
        continue
    if re.match(r'LoadModule\s+php_module', s, re.I) or 'x-httpd-php' in s:
        continue
    subs = [
        (r'^Listen\s+\S+', 'Listen %s' % port),
        (r'^ServerAdmin\s+.*', 'ServerAdmin webmaster@localhost'),
        (r'^ServerName\s+.*', 'ServerName localhost'),
        (r'^DocumentRoot\s+"/apache\$root/htdocs"$', 'DocumentRoot "%s"' % docroot),
        (r'^<Directory\s+"/apache\$root/htdocs">$', '<Directory "%s">' % docroot),
        (r'^PidFile\s+.*', 'PidFile logs/phptest.pid'),
        (r'^ErrorLog\s+.*', 'ErrorLog logs/phptest_error_log'),
        (r'^CustomLog\s+logs/access_log\s+(\w+)', r'CustomLog logs/phptest_access_log \1'),
        (r'^StartServers\s+\d+', 'StartServers         2'),
        (r'^MinSpareServers\s+\d+', 'MinSpareServers      2'),
        (r'^MaxSpareServers\s+\d+', 'MaxSpareServers      4'),
        (r'^MaxClients\s+\d+', 'MaxClients           8'),
    ]
    new = s
    for pat, rep in subs:
        if re.match(pat, s, re.I):
            new = re.sub(pat, rep, s, flags=re.I)
            break
    out.append(line if new == s else new)
out += ['',
        '# PHP for OpenVMS over FastCGI: every .php file to the PHP_CGI.EXE -b pool;',
        '# mod_proxy_fcgi passes the script path as SCRIPT_FILENAME, the rest as PATH_INFO',
        'LoadModule proxy_module         modules/mod_proxy.exe',
        'LoadModule proxy_fcgi_module    modules/mod_proxy_fcgi.exe',
        '<FilesMatch "\\.php$">',
        '    SetHandler "proxy:fcgi://%s"' % fcgi,
        '</FilesMatch>',
        '']
sys.stdout.write('\n'.join(out))
