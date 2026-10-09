$! VMS_PHPT.COM - run a prepared .phpt set (tools/phpt.py prepare) with PHP.EXE
$!
$! P1: the set's top directory (holds LIST.TXT), e.g. DISK$X:[USER.VMS_PHP.PHPT.JSON]
$! P2: PHP.EXE
$! P3: results file: per test "@@@TEST <dir> <base> <status>", output, "@@@END"
$!     (status: PHP's exit status, or SKIP)
$! Each test runs as  php -n -c <base>.ini <base>.php  in its directory, after
$! <base>.skip.php if it has one (output starting "skip" skips it), and
$! <base>.clean.php after.  PHP's memory manager is forced on: the node's
$! system logical USE_ZEND_ALLOC=0 would switch it off (PORTING_LOG #16).
$ set noon
$ set process/parse_style=extended
$ define/process decc$argv_parse_style enable
$ define/process USE_ZEND_ALLOC "1"
$ php = "$" + p2
$ top = p1
$ res = p3
$ create/fdl="RECORD; FORMAT STREAM_LF;" 'res'
$ open/read lst 'top'LIST.TXT
$ n = 0
$loop:
$ read/end=done lst line
$ line = f$edit(line, "TRIM")
$ if line .eqs. "" then goto loop
$ rel = f$element(0, " ", line)
$ base = f$element(1, " ", line)
$! ext/json/tests -> [.ext.json.tests] under the set's top
$ i = 0
$ vdir = top - "]"
$dir_loop:
$ e = f$element(i, "/", rel)
$ if e .eqs. "/" then goto dir_done
$ vdir = vdir + "." + e
$ i = i + 1
$ goto dir_loop
$dir_done:
$ vdir = vdir + "]"
$ set default 'vdir'
$ status = ""
$ if f$search(base + ".skip.php") .nes. ""
$ then
$   pipe php "-n" "-c" 'base'.ini 'base'.skip.php > phpt_skip.out 2> nl:
$   open/read/error=noskip s phpt_skip.out
$   read/end=skipeof s sline
$skipeof:
$   close s
$   if f$edit(f$extract(0, 4, sline), "LOWERCASE") .eqs. "skip" then status = "SKIP"
$noskip:
$   delete/nolog phpt_skip.out;*
$ endif
$ open/append r 'res'
$ if status .eqs. "SKIP"
$ then
$   write r "@@@TEST ", rel, " ", base, " SKIP"
$   write r "@@@END"
$   close r
$   goto loop
$ endif
$ close r
$ pipe php "-n" "-c" 'base'.ini "-d" "max_execution_time=120" 'base'.php > phpt_run.out 2> phpt_run.err
$ st = $status
$ open/append r 'res'
$ write r "@@@TEST ", rel, " ", base, " ", st
$ close r
$ if f$search("phpt_run.out") .nes. "" then append phpt_run.out 'res'
$ if f$search("phpt_run.err") .nes. "" then append phpt_run.err 'res'
$ open/append r 'res'
$ write r "@@@END"
$ close r
$ if f$search("phpt_run.*") .nes. "" then delete/nolog phpt_run.*;*
$ if f$search(base + ".clean.php") .nes. "" then -
    pipe php "-n" "-c" 'base'.ini 'base'.clean.php > nl: 2> nl:
$ n = n + 1
$ goto loop
$done:
$ close lst
$ write sys$output "PHPT-DONE ", n
