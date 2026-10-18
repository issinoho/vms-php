$! VMSPHP$STARTUP.COM - PHP for OpenVMS at system startup (vms-php D22, D25)
$!
$! Usage (as SYSTEM):
$!   @SYS$STARTUP:VMSPHP$STARTUP            from SYSTARTUP_VMS.COM: define
$!                                          VMSPHP$ROOT and USE_ZEND_ALLOC
$!   @SYS$STARTUP:VMSPHP$STARTUP INSTALL    the same, and [ETC]PHP.INI from
$!                                          PHP.INI-DIST if there is none (PCSI)
$!   @SYS$STARTUP:VMSPHP$STARTUP REMOVE     deassign both (PCSI, at removal)
$!
$! USE_ZEND_ALLOC "1" keeps PHP's own memory manager on: a SYSTEM logical "0",
$! left by an earlier PHP install, turns it off for every PHP process (the C
$! RTL's getenv() reads logical names). VSI's PHP 8.1 needs "0" (it crashes in
$! the Zend memory manager otherwise), so the two cannot share a system.
$!
$ set noon
$ say = "write sys$output"
$ op = f$edit(p1, "UPCASE,TRIM")
$ if op .eqs. "REMOVE" then goto remove
$! This procedure sits in <destination>[SYS$STARTUP]; the product is in
$! <destination>[VMSPHP]: DKA0:[SYS0.SYSCOMMON.SYS$STARTUP] -> DKA0:[SYS0.SYSCOMMON.VMSPHP.]
$ proc = f$environment("PROCEDURE")
$ dev = f$getdvi(f$parse(proc,,,"DEVICE","NO_CONCEAL"), "FULLDEVNAM")
$ dir = f$edit(f$parse(proc,,,"DIRECTORY","NO_CONCEAL"), "UPCASE") - "]["
$ root = dir - "SYS$STARTUP]" + "VMSPHP.]"
$ if root .eqs. dir + "VMSPHP.]"
$ then
$   write sys$error "VMSPHP$STARTUP: expected to be in a [SYS$STARTUP] directory, not ''dir'"
$   exit 44
$ endif
$ root = dev + (root - ".000000")
$ define/system/executive_mode/translation_attributes=concealed VMSPHP$ROOT 'root'
$ define/system USE_ZEND_ALLOC "1"
$ if f$search("VMSPHP$ROOT:[BIN]PHP_CGI.EXE") .eqs. ""
$ then
$   say "VMSPHP$STARTUP: no VMSPHP$ROOT:[BIN]PHP_CGI.EXE (root ", root, ")"
$   exit 44
$ endif
$ if op .nes. "INSTALL" then exit 1
$!
$! PCSI installs only the -DIST files: the site's PHP.INI survives upgrades
$ if f$search("VMSPHP$ROOT:[ETC]PHP.INI") .eqs. ""
$ then
$   copy/nolog VMSPHP$ROOT:[ETC]PHP.INI-DIST VMSPHP$ROOT:[ETC]PHP.INI
$   set security/protection=(s:rwed,o:rwed,g:re,w:re) VMSPHP$ROOT:[ETC]PHP.INI
$   say "VMSPHP$STARTUP: wrote VMSPHP$ROOT:[ETC]PHP.INI from PHP.INI-DIST"
$ endif
$ exit 1
$!
$remove:
$ if f$trnlnm("VMSPHP$ROOT","LNM$SYSTEM_TABLE") .nes. "" then -
     deassign/system/executive_mode VMSPHP$ROOT
$ if f$trnlnm("USE_ZEND_ALLOC","LNM$SYSTEM_TABLE") .eqs. "1" then -
     deassign/system USE_ZEND_ALLOC
$ exit 1
