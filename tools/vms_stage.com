$! VMS_STAGE.COM - the staged install tree for the test pool (D15, D22).
$!
$! P1: the build's image directory, e.g. DISK$X:[USER.VMS_PHP.PHP-8_5_11.OBJ_X86_64]
$! P2: the install tree's top directory, e.g. DISK$X:[USER.VMS_PHP.VMSPHP]
$! P3: the PHP.INI for [ETC] (uploaded by tools/stage.sh)
$! P4: the CA bundle for [ETC]CACERT.PEM, openssl.cafile (PORTING_LOG #57)
$! Copies PHP.EXE and PHP_CGI.EXE into [BIN] and PHP.INI into [ETC], creates
$! [TMP] and [TMP.OPCACHE], and checks the SYSTEM logicals the user defined
$! (D22): VMSPHP$ROOT, concealed and rooted at this tree, and USE_ZEND_ALLOC
$! "1". Defining them needs SYSNAM and is left to the user.
$ set noon
$ set process/parse_style=extended
$ say = "write sys$output"
$ obj = p1
$ top = p2 - "]"
$ ini = p3
$ if f$search(top + ".BIN]*.*") .eqs. "" then create/directory 'top'.BIN]
$ if f$search(top + ".ETC]*.*") .eqs. "" then create/directory 'top'.ETC]
$ if f$search(top + ".TMP]*.*") .eqs. "" then create/directory 'top'.TMP]
$! opcache.file_cache for the pool (D19)
$ if f$search(top + ".TMP]OPCACHE.DIR") .eqs. "" then create/directory 'top'.TMP.OPCACHE]
$ copy/nolog 'obj'PHP.EXE 'top'.BIN]PHP.EXE
$ copy/nolog 'obj'PHP_CGI.EXE 'top'.BIN]PHP_CGI.EXE
$ copy/nolog 'ini' 'top'.ETC]PHP.INI
$ if p4 .nes. "" then copy/nolog 'p4' 'top'.ETC]CACERT.PEM
$ purge/nolog 'top'...]*.*
$! the SYSTEM logicals (D22): warn if missing or pointing elsewhere
$ dev = f$getdvi(f$parse(top + "]",,,"DEVICE","NO_CONCEAL"), "FULLDEVNAM")
$ dir = f$parse(top + "]",,,"DIRECTORY","NO_CONCEAL") - "]"
$ want = f$edit(dev + dir + ".]", "UPCASE")
$ have = f$edit(f$trnlnm("VMSPHP$ROOT","LNM$SYSTEM_TABLE"), "UPCASE")
$! the live pool runs an installed copy (D23): then VMSPHP$ROOT points there
$ if have .nes. want then say "STAGE: note: SYSTEM VMSPHP$ROOT is """, have, """, not this tree; ", -
    "install the staged tree there with VMS_INSTALL_TREE.COM (D23)"
$ if f$trnlnm("USE_ZEND_ALLOC","LNM$SYSTEM_TABLE") .nes. "1" then -
    say "STAGE: warning: SYSTEM USE_ZEND_ALLOC is not ""1"" (D22, PORTING_LOG #16)"
$ show logical/full VMSPHP$ROOT
$ show logical/full USE_ZEND_ALLOC
$ directory/size/date VMSPHP$ROOT:[BIN]*.EXE, VMSPHP$ROOT:[ETC]PHP.INI, VMSPHP$ROOT:[ETC]CACERT.PEM
$ define/user sys$error sys$output
$ mcr VMSPHP$ROOT:[BIN]PHP_CGI.EXE -v
$ say "STAGE: done"
$ exit 1
