$! VMS_STAGE.COM - the staged install tree for the Phase 3 test pool (D15).
$!
$! P1: the build's image directory, e.g. DISK$X:[USER.VMS_PHP.PHP-8_5_11.OBJ_X86_64]
$! P2: the install tree's top directory, e.g. DISK$X:[USER.VMS_PHP.VMSPHP]
$! P3: the PHP.INI for [ETC] (uploaded by tools/stage.sh)
$! Copies PHP.EXE and PHP_CGI.EXE into [BIN] and PHP.INI into [ETC], creates
$! [TMP], and defines VMSPHP$ROOT as a concealed rooted GROUP logical for the
$! tree, and USE_ZEND_ALLOC "1" as a GROUP logical: PHP_POOL.COM's detached
$! workers inherit no process or job logicals (D15, D16).
$ set noon
$ set process/parse_style=extended
$ say = "write sys$output"
$ obj = p1
$ top = p2 - "]"
$ ini = p3
$ if f$search(top + ".BIN]*.*") .eqs. "" then create/directory 'top'.BIN]
$ if f$search(top + ".ETC]*.*") .eqs. "" then create/directory 'top'.ETC]
$ if f$search(top + ".TMP]*.*") .eqs. "" then create/directory 'top'.TMP]
$ copy/nolog 'obj'PHP.EXE 'top'.BIN]PHP.EXE
$ copy/nolog 'obj'PHP_CGI.EXE 'top'.BIN]PHP_CGI.EXE
$ copy/nolog 'ini' 'top'.ETC]PHP.INI
$ purge/nolog 'top'...]*.*
$! a rooted logical needs the physical device name
$ dev = f$getdvi(f$parse(top + "]",,,"DEVICE","NO_CONCEAL"), "FULLDEVNAM")
$ dir = f$parse(top + "]",,,"DIRECTORY","NO_CONCEAL") - "]"
$ define/group/translation_attributes=concealed VMSPHP$ROOT 'dev''dir'.]
$! the node's SYSTEM logical USE_ZEND_ALLOC "0" would turn PHP's memory
$! manager off in the workers too (PORTING_LOG #16, D16)
$ define/group USE_ZEND_ALLOC "1"
$ show logical/full VMSPHP$ROOT
$ show logical/full USE_ZEND_ALLOC
$ directory/size/date VMSPHP$ROOT:[BIN]*.EXE, VMSPHP$ROOT:[ETC]PHP.INI
$ define/user sys$error sys$output
$ mcr VMSPHP$ROOT:[BIN]PHP_CGI.EXE -v
$ say "STAGE: done"
$ exit 1
