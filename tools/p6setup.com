$! P6SETUP.COM - the SQLite phpBB board's lighttpd configuration and FastCGI
$! pool (8.5.12-vms2, D28, D29), in our work directory, beside vms-lighttpd's
$! Phase 4 board and away from the live one:
$!   [.T6]LIGHTTPD.CONF  HTTP :18081 (no HTTPS), document root [.PHPBB6],
$!                       FastCGI 127.0.0.1:P2..P2+3, vms-lighttpd's phpbb.conf
$!   [.T6POOL]PHP.INI    vms-lighttpd's PHP-VMSPHP.INI template, @POOL@ filled in
$! Run from the work directory.
$! P1: vms-lighttpd's tree top directory (its server, phpbb.conf, the template,
$!     [.T2]STMLF.FDL: read only)
$! P2: the pool's first port (default 19104)
$ set noon
$ set process/parse_style=extended
$ say = "write sys$output"
$ lt = p1
$ base = 19104
$ if p2 .nes. "" then base = f$integer(p2)
$ if f$search("PHPBB6.DIR") .eqs. ""
$ then
$   say "P6SETUP: no [.PHPBB6]: tools/p6.sh upload and install first"
$   exit 44
$ endif
$ if f$search("T6.DIR") .eqs. "" then create/directory [.T6]
$ if f$search("[.T6]LOGS.DIR") .eqs. "" then create/directory [.T6.LOGS]
$ if f$search("[.T6]TMP.DIR") .eqs. "" then create/directory [.T6.TMP]
$ if f$search("T6POOL.DIR") .eqs. "" then create/directory [.T6POOL]
$ fdl = lt - "]" + ".T2]STMLF.FDL"
$ ltconf = lt - "]" + ".VMSPORT.CONF]PHPBB.CONF"
$ ltini = lt - "]" + ".VMSPORT.PHP]PHP-VMSPHP.INI"
$ if f$search(ltconf) .eqs. "" .or. f$search(ltini) .eqs. "" .or. f$search(fdl) .eqs. ""
$ then
$   say "P6SETUP: vms-lighttpd's ", ltconf, ", ", ltini, " or ", fdl, " is missing"
$   exit 44
$ endif
$ call unix "[.T6]"
$ ut6 = upath
$ call unix "[.T6POOL]"
$ upool = upath
$ call unix "[.PHPBB6]"
$ uphpbb = upath
$ copy 'ltconf' [.T6]PHPBB.CONF
$ purge/nolog [.T6]PHPBB.CONF
$ if f$search("[.T6]LIGHTTPD.CONF") .nes. "" then delete/nolog [.T6]LIGHTTPD.CONF;*
$ open/write f [.T6]LIGHTTPD.TMP
$ write f "server.modules = ( ""mod_access"", ""mod_rewrite"", ""mod_accesslog"", ""mod_fastcgi"" )"
$ write f "server.document-root = """, uphpbb, """"
$ write f "server.errorlog      = """, ut6, "/logs/error.log"""
$ write f "accesslog.filename   = """, ut6, "/logs/access.log"""
$ write f "server.upload-dirs   = ( """, ut6, "/tmp"" )"
$ write f "server.port          = 18081"
$ write f "server.bind          = ""0.0.0.0"""
$ write f "mimetype.assign      = ( "".html"" => ""text/html"", "".css"" => ""text/css"", "".js"" => ""text/javascript"","
$ write f "  "".png"" => ""image/png"", "".gif"" => ""image/gif"", "".jpg"" => ""image/jpeg"", "".svg"" => ""image/svg+xml"","
$ write f "  "".ico"" => ""image/x-icon"", "".woff"" => ""font/woff"", "".woff2"" => ""font/woff2"", "".txt"" => ""text/plain"" )"
$ write f "fastcgi.server = ( "".php"" => ("
$ n = 0
$fcgi_loop:
$ sep = ","
$ if n .eq. 3 then sep = ""
$ write f "    ( ""host"" => ""127.0.0.1"", ""port"" => ", base + n, ", ""check-local"" => ""enable"", ""broken-scriptfilename"" => ""enable"" )", sep
$ n = n + 1
$ if n .lt. 4 then goto fcgi_loop
$ write f "  ) )"
$ write f "index-file.names = ( ""index.php"", ""index.html"" )"
$ write f "static-file.exclude-extensions = ( "".php"", "".inc"" )"
$ write f "include """, ut6, "/phpbb.conf"""
$ close f
$ convert/fdl='fdl' [.T6]LIGHTTPD.TMP [.T6]LIGHTTPD.CONF
$ delete/nolog [.T6]LIGHTTPD.TMP;*
$! the pool's PHP.INI: the template with @POOL@ (its sessions, opcache, logs)
$ if f$search("[.T6POOL]PHP.INI") .nes. "" then delete/nolog [.T6POOL]PHP.INI;*
$! (an OPEN that fails under SET NOON makes READ/END loop for ever)
$ open/read/error=t_noopen t 'ltini'
$ open/write o [.T6POOL]PHP.TMP
$t_loop:
$ read/end=t_done t line
$t_sub:
$ i = f$locate("@POOL@", line)
$ if i .lt. f$length(line)
$ then
$   line = f$extract(0, i, line) + upool + f$extract(i + 6, 999, line)
$   goto t_sub
$ endif
$ write o line
$ goto t_loop
$t_done:
$ close t
$ close o
$ convert/fdl='fdl' [.T6POOL]PHP.TMP [.T6POOL]PHP.INI
$ delete/nolog [.T6POOL]PHP.TMP;*
$ if f$search("[.T6POOL]OPCACHE.DIR") .eqs. "" then create/directory [.T6POOL.OPCACHE]
$ say "P6SETUP: phpBB at ", uphpbb, ", pool ", upool, ", ports ", base, "-", base + 3
$ say "P6SETUP-DONE"
$ exit 1
$t_noopen:
$ say "P6SETUP: cannot open ", ltini
$ exit 44
$!
$! upath = UNIX form of directory P1 (rooted [ROOT.][DIR] handled)
$unix: subroutine
$ dev = f$parse(p1,,,"DEVICE","NO_CONCEAL") - ":"
$ dir = f$parse(p1,,,"DIRECTORY","NO_CONCEAL")
$ i = f$locate(".][", dir)
$ if i .lt. f$length(dir) then dir = f$extract(0, i, dir) + "." + f$extract(i + 3, 999, dir)
$ dir = f$extract(1, f$length(dir) - 2, dir) - "000000."
$ u = "/" + dev + "/" + dir
$uloop:
$ i = f$locate(".", u)
$ if i .lt. f$length(u)
$ then
$   u = f$extract(0, i, u) + "/" + f$extract(i + 1, 999, u)
$   goto uloop
$ endif
$ upath == u
$ exit 1
$ endsubroutine
