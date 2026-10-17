$! VMS_POOL_UNPRIV.COM - start vms-lighttpd's test pool with no privileges (Phase 4).
$!
$! The live board's pool runs as LIGHTTPD, an unprivileged account; ours inherits our
$! account's privileges (SYSPRV, BYPASS, READALL...). This writes the same worker
$! procedures as vms-lighttpd's PHP_POOL.COM START (same names, stop file and quotas),
$! with one line more: SET PROCESS/PRIVILEGE=(NOALL,NETMBX,TMPMBX). PHP_POOL.COM's
$! STATUS and STOP work on these workers unchanged.
$! P1: the lighttpd tree's top, P2: count (4), P3: first port (19100),
$! P4: pool directory relative to P1 ([.T3POOL]), P5: PHP root logical (VMSPHP$ROOT)
$ set noon
$ set process/parse_style=extended
$ say = "write sys$output"
$ set default 'p1'
$ count = 4
$ if p2 .nes. "" then count = f$integer(p2)
$ base = 19100
$ if p3 .nes. "" then base = f$integer(p3)
$ pool = p4
$ if pool .eqs. "" then pool = "[.T3POOL]"
$ root = p5
$ if root .eqs. "" then root = "VMSPHP$ROOT"
$! a rooted directory (USER$ROOT) comes back as [ROOT.][DIR]: merge, as PHP_POOL.COM does
$ pooldir = f$parse(pool,,,"DEVICE","NO_CONCEAL") + f$parse(pool,,,"DIRECTORY","NO_CONCEAL")
$ i = f$locate("][", pooldir)
$ if i .lt. f$length(pooldir) then pooldir = f$extract(0, i - 1, pooldir) + "." + f$extract(i + 2, 999, pooldir)
$ pooldir = pooldir - "]"
$ logs = pooldir + ".LOGS]"
$ stopfile = logs + "POOL.STOP"
$ if f$search(stopfile) .nes. "" then delete/nolog 'stopfile';*
$ cgiexe = root + ":[BIN]PHP_CGI.EXE"
$! UNIX form of the pool directory, as PHP_POOL.COM writes it
$ dev = f$parse(pool,,,"DEVICE","NO_CONCEAL") - ":"
$ dir = f$parse(pool,,,"DIRECTORY","NO_CONCEAL")
$ i = f$locate(".][", dir)
$ if i .lt. f$length(dir) then dir = f$extract(0, i, dir) + "." + f$extract(i + 3, 999, dir)
$ dir = f$extract(1, f$length(dir) - 2, dir) - "000000."
$ upool = "/" + dev + "/" + dir
$ul:
$ i = f$locate(".", upool)
$ if i .lt. f$length(upool)
$ then
$   upool = f$extract(0, i, upool) + "/" + f$extract(i + 1, 999, upool)
$   goto ul
$ endif
$ n = 0
$start_loop:
$ if n .ge. count then exit 1
$ port = base + n
$ n = n + 1
$ name = "LTPHP_" + f$string(port)
$ com = logs + name + ".COM"
$ open/write w 'com'
$ write w "$ set noon"
$ write w "$ set process/privilege=(noall,netmbx,tmpmbx)"
$ write w "$ write sys$output """, name, " privileges "", f$getjpi("""",""CURPRIV"")"
$ write w "$ set process/parse_style=extended"
$ write w "$ define/process PHP_FCGI_CHILDREN ""0"""
$ write w "$ define/process PHP_FCGI_MAX_REQUESTS ""0"""
$ write w "$ define/process TMPDIR """, upool, "/tmp"""
$ write w "$ cgi = ""$", cgiexe, """"
$ write w "$loop:"
$ write w "$ if f$search(""", stopfile, """) .nes. """" then exit"
$ write w "$ write sys$output """, name, " start "", f$time()"
$ write w "$ cgi ""-c"" """, upool, "/php.ini"" ""-b"" ""127.0.0.1:", port, """"
$ write w "$ write sys$output """, name, " exit "", $status, "" "", f$time()"
$ write w "$ if f$search(""", stopfile, """) .nes. """" then exit"
$ write w "$ wait 00:00:01"
$ write w "$ goto loop"
$ close w
$ run/detached sys$system:loginout.exe /input='com' -
      /output='logs''name'.LOG /error='logs''name'.LOG -
      /process_name='name' -
      /buffer_limit=200000 /file_limit=200 /io_buffered=200 /io_direct=200 -
      /queue_limit=64 /enqueue_limit=4000 /ast_limit=300 /subprocess_limit=4 -
      /page_file=2000000 /working_set=8192 /maximum_working_set=32768 /extent=131072
$ say "VMS_POOL_UNPRIV: started ", name, " on 127.0.0.1:", port
$ goto start_loop
