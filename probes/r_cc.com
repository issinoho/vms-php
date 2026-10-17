$! R_CC.COM - compile a probe with PHP's clang flags (the build tree's
$! [.VMSPORT.BUILD]CLANG.RSP), from the tree's top as BUILD.COM does.
$! P1: the build tree's top, e.g. DISK$X:[USER.VMS_PHP.PHP-8_5_11]
$! P2: source (UNIX name relative to the tree's top), P3: object
$! P4: optional extra clang options, e.g. -S
$ set noon
$ set process/parse_style=extended
$ define/user decc$argv_parse_style enable
$ clang = "$sys$system:clang.exe"
$ top = p1
$ unix = "/" + (f$parse(top,,,"DEVICE","NO_CONCEAL") - ":")
$ dirs = f$parse(top,,,"DIRECTORY","NO_CONCEAL") - "[" - "]" - "<" - ">"
$ dirs = dirs - "000000." - ".000000"
$ i = 0
$u:
$ e = f$element(i, ".", dirs)
$ if e .eqs. "." .or. e .eqs. "" then goto ud
$ unix = unix + "/" + e
$ i = i + 1
$ goto u
$ud:
$ rspunix = unix + "/vmsport/build/clang.rsp"
$ here = f$environment("DEFAULT")
$ set default 'top'
$ define/user sys$error sys$output
$ define/user decc$argv_parse_style enable
$! assembler sources take no C flags (the response file force-includes headers)
$ if f$edit(f$extract(f$length(p2) - 2, 2, p2),"UPCASE") .eqs. ".S"
$ then
$   clang -c 'p2' -o 'p3' 'p4'
$ else
$   clang "@''rspunix'" -c 'p2' -o 'p3' 'p4'
$ endif
$ st = $status
$ set default 'here'
$ exit st
