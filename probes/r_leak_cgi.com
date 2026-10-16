$! R_LEAK_CGI.COM - page-file use of PHP_CGI.EXE -T <n> <script>, sampled
$! from outside every 10 s (PAGFILCNT, pagelets). P1: script, P2: n,
$! P3: a -d setting (quoted: ini names are case-sensitive), P4: N for -n
$ set noon
$ open/write s r_leak_run.com
$ write s "$ define/user sys$output nl:"
$ line = "$ mcr VMSPHP$ROOT:[BIN]PHP_CGI.EXE ""-T"" " + p2
$ if p4 .nes. "" then line = line + " ""-n"""
$ if p3 .nes. "" then line = line + " ""-d"" " + p3
$ write s line, " ", p1
$ close s
$ spawn/nowait/process=RLEAKCGI/output=nl: @r_leak_run.com
$ wait 00:00:03
$loop:
$ ctx = ""
$ x = f$context("PROCESS", ctx, "PRCNAM", "RLEAKCGI", "EQL")
$ pid = f$pid(ctx)
$ if pid .eqs. "" then goto done
$ x = f$context("PROCESS", ctx, "CANCEL")
$ write sys$output f$time(), " ", f$getjpi(pid, "PAGFILCNT"), " cpu ", f$getjpi(pid, "CPUTIM")
$ wait 00:00:10
$ goto loop
$done:
$ delete/nolog r_leak_run.com;*
$ exit 1
