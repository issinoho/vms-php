$! VMS_INSTALL_TREE.COM - install the staged PHP tree where the live board's pool
$! runs it from (D23). Run by the system manager (needs SYSPRV to create the
$! directories and set their owners); it does not change logical names, it prints
$! the DEFINE/SYSTEM for VMSPHP$ROOT.
$!
$! P1: the staged tree (tools/stage.sh), e.g. DISK$X:[USER.VMS_PHP.VMSPHP]
$! P2: the install tree, default DKA0:[VMSPHP]
$! P3: the pool's account, owner of [TMP.OPCACHE] (default LIGHTTPD)
$! P4: another identifier given write access to [TMP.OPCACHE] (e.g. the
$!     developer's account, for its test pool), optional
$!
$!   [BIN] PHP.EXE, PHP_CGI.EXE      owner SYSTEM, W:RE
$!   [ETC] PHP.INI, CACERT.PEM       owner SYSTEM, W:RE
$!   [ETC.CERTS] hashed CA directory owner SYSTEM, W:RE
$!   [TMP]                           owner SYSTEM, W:RE
$!   [TMP.OPCACHE]                   owner P3, plus an ACL for P4 (D19)
$ set noon
$ say = "write sys$output"
$ src = p1 - "]"
$ dst = p2
$ if dst .eqs. "" then dst = "DKA0:[VMSPHP]"
$ dst = dst - "]"
$ owner = p3
$ if owner .eqs. "" then owner = "LIGHTTPD"
$ if .not. f$privilege("SYSPRV")
$ then
$   say "VMS_INSTALL_TREE: needs SYSPRV (SET PROCESS/PRIVILEGE=SYSPRV)"
$   exit 44
$ endif
$ if f$search(src + ".BIN]PHP_CGI.EXE") .eqs. ""
$ then
$   say "VMS_INSTALL_TREE: no ", src, ".BIN]PHP_CGI.EXE (run tools/stage.sh)"
$   exit 44
$ endif
$ prot = "(s:rwed,o:rwed,g:re,w:re)"
$ dprot = "(s:rwe,o:rwe,g:re,w:re)"
$ create/directory/owner=[SYSTEM]/protection='dprot' 'dst']
$ create/directory/owner=[SYSTEM]/protection='dprot' 'dst'.BIN]
$ create/directory/owner=[SYSTEM]/protection='dprot' 'dst'.ETC]
$ create/directory/owner=[SYSTEM]/protection='dprot' 'dst'.TMP]
$ create/directory/owner=['owner']/protection=(s:rwe,o:rwe,g,w) 'dst'.TMP.OPCACHE]
$ if p4 .nes. ""
$ then
$   set security/acl=((identifier='p4',access=read+write+execute+delete), -
       (identifier='p4',options=default,access=read+write+execute+delete), -
       (identifier='owner',options=default,access=read+write+execute+delete)) -
       'dst'.TMP]OPCACHE.DIR
$ endif
$ copy/nolog 'src'.BIN]PHP.EXE, 'src'.BIN]PHP_CGI.EXE 'dst'.BIN]
$ copy/nolog 'src'.ETC]PHP.INI 'dst'.ETC]
$ if f$search(src + ".ETC]CACERT.PEM") .nes. "" then copy/nolog 'src'.ETC]CACERT.PEM 'dst'.ETC]
$! the hashed CA directory for openssl.capath (PORTING_LOG #57)
$ if f$search(src + ".ETC.CERTS]*.*") .nes. ""
$ then
$   create/directory/owner=[SYSTEM]/protection='dprot' 'dst'.ETC.CERTS]
$   if f$search(dst + ".ETC.CERTS]*.*") .nes. "" then delete/nolog 'dst'.ETC.CERTS]*.*;*
$   copy/nolog 'src'.ETC.CERTS]*.* 'dst'.ETC.CERTS]
$   set file/owner=[SYSTEM] 'dst'.ETC.CERTS]*.*;*
$   set security/protection='prot' 'dst'.ETC.CERTS]*.*;*
$ endif
$ set file/owner=[SYSTEM] 'dst'.BIN]*.*;*, 'dst'.ETC]*.*;*
$! one directory per SET SECURITY: it takes no list of files (%DCL-W-NOLIST)
$ set security/protection='prot' 'dst'.BIN]*.*;*
$ set security/protection='prot' 'dst'.ETC]*.*;*
$ purge/nolog 'dst'.BIN], 'dst'.ETC]
$ directory/security 'dst'...]
$! the rooted, concealed name needs the physical device and directory
$ dev = f$getdvi(f$parse(dst + "]",,,"DEVICE","NO_CONCEAL"), "FULLDEVNAM")
$ dir = f$parse(dst + "]",,,"DIRECTORY","NO_CONCEAL") - "]"
$ say ""
$ say "VMS_INSTALL_TREE: installed in ", dst, "]. To point VMSPHP$ROOT at it:"
$ say "  $ DEFINE/SYSTEM/TRANSLATION_ATTRIBUTES=CONCEALED VMSPHP$ROOT ", dev, dir, ".]"
$ exit 1
