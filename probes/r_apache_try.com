$! R_APACHE_TRY.COM - run VSI Apache 2.4 (APACHEWS) in this process, in front of
$! our PHP's FastCGI pool, without starting the system's web server. Needs
$! SYS$MANAGER:APACHE$FIS.COM to have run (system manager): it installs
$! APACHE$APR_SHRP /PROTECTED and the other shareable images and defines their
$! names /SYSTEM/EXEC, which a protected image's activation requires. This
$! procedure only points APACHE$SPECIFIC and APACHE$ROOT, in this process, at the
$! test directory, so all Apache writes stays there.
$! Submit it as a batch job; stop it with STOP/ID.
$! P1: the test directory, e.g. DISK$X:[USER.VMS_PHP.APACHE_TRY] (with [.CONF],
$!     [.HTDOCS], [.LOGS]; httpd.conf.template in [.CONF])
$! P2: Apache's port (18090), P3: the FastCGI pool (127.0.0.1:19100)
$ set noon
$ set process/parse_style=extended
$ top = p1 - "]"
$ port = p2
$ if port .eqs. "" then port = "18090"
$ fcgi = p3
$ if fcgi .eqs. "" then fcgi = "127.0.0.1:19100"
$! the physical device and directory of the test tree
$ sdev = f$getdvi(f$parse(top + "]",,,"DEVICE","NO_CONCEAL"), "FULLDEVNAM")
$ sdir = f$parse(top + "]",,,"DIRECTORY","NO_CONCEAL") - "]"
$ if f$trnlnm("APACHE$APR_SHRP","LNM$SYSTEM_TABLE",,"EXECUTIVE") .eqs. ""
$ then
$   write sys$output "R_APACHE_TRY: Apache's images are not installed (@SYS$MANAGER:APACHE$FIS)"
$   exit 44
$ endif
$ define/process/translation_attributes=(concealed,terminal) APACHE$SPECIFIC 'sdev''sdir'.]
$ define/process APACHE$ROOT APACHE$SPECIFIC,APACHE$COMMON
$ show logical APACHE$ROOT, APACHE$SPECIFIC, APACHE$COMMON
$! UNIX form of the test directory, for the configuration's absolute paths
$ utop = "/" + (sdev - "_" - ":") + "/" + f$extract(1, 999, sdir)
$ul:
$ i = f$locate(".", utop)
$ if i .lt. f$length(utop)
$ then
$   utop = f$extract(0, i, utop) + "/" + f$extract(i + 1, 999, utop)
$   goto ul
$ endif
$! httpd.conf from the template
$ open/read t 'top'.CONF]HTTPD.CONF.TEMPLATE
$ open/write c 'top'.CONF]HTTPD.CONF
$tl:
$ read/end=td t line
$ k = 0
$sub:
$ k = k + 1
$ if k .gt. 3 then goto subd
$ key = f$element(k - 1, ",", "@TOP@,@PORT@,@FCGI@")
$ val = f$element(k - 1, ",", utop + "," + port + "," + fcgi)
$rep:
$ j = f$locate(key, line)
$ if j .lt. f$length(line)
$ then
$   line = f$extract(0, j, line) + val + f$extract(j + f$length(key), 9999, line)
$   goto rep
$ endif
$ goto sub
$subd:
$ write c line
$ goto tl
$td:
$ close t
$ close c
$ convert/fdl="RECORD; FORMAT STREAM_LF;" 'top'.CONF]HTTPD.CONF 'top'.CONF]HTTPD.CONF
$ purge/nolog 'top'.CONF]HTTPD.CONF
$! VSI's own startup in RUN mode: its symbols, the C RTL features of
$! APACHE$SETUP.COM, then "HTTPD -f <config> RUN" (the server in this process)
$ write sys$output "R_APACHE_TRY: APACHE$STARTUP RUN ", utop, "/conf/httpd.conf on port ", port, " -> ", fcgi
$! Run in an ordinary process this stops at "Unable to unlock shared socket
$! device (ap_set_listener())", with no privileges or all of them: VSI's server
$! shares its listener's device in a way that START (as APACHE$WWW) sets up.
$ @SYS$STARTUP:APACHE$STARTUP RUN "''utop'/conf/httpd.conf"
$ write sys$output "R_APACHE_TRY: Apache exited ", $status
$ exit 1
