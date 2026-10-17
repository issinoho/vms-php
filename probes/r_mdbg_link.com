$! R_MDBG_LINK.COM - [.PHP_PROBE]PHP_CGI_<P4>.EXE: PHP_CGI's objects (the
$! build's own options file) with probe objects linked first: by default
$! r_mdbg.obj (allocation tracker) as PHP_CGI_MDBG.EXE.
$! P1: build tree top, P2: dependency work directory (nodes.conf column 8)
$! P3: probe sources, comma-separated (default r_mdbg.c), P4: image suffix
$ set noon
$ set process/parse_style=extended
$ top = p1
$ deps = p2
$! a concealed root must name a physical device: resolve rooted logicals
$! such as USER$ROOT first
$ deps = f$getdvi(f$parse(deps,,,"DEVICE","NO_CONCEAL"), "FULLDEVNAM") + -
    f$parse(deps,,,"DIRECTORY","NO_CONCEAL")
$ deps = deps - "][" - ".]["
$ srcs = p3
$ if srcs .eqs. "" then srcs = "r_mdbg.c"
$ img = "PHP_CGI_" + p4
$ if p4 .eqs. "" then img = "PHP_CGI_MDBG"
$ here = f$environment("DEFAULT")
$ objs = ""
$ i = 0
$cl:
$ src = f$element(i, ",", srcs)
$ if src .eqs. "," then goto cd
$ obj = f$parse(src,,,"NAME") + ".obj"
$ @'here'r_cc.com 'top' ../PHP_PROBE/'src' ../PHP_PROBE/'obj'
$ if .not. $status then exit $status
$ objs = objs + here + obj + ","
$ i = i + 1
$ goto cl
$cd:
$ set default 'top'
$ open/read v [.VMSPORT]DEPS.ENV
$dl:
$ read/end=dd v line
$ lname = f$element(0, "=", line)
$ tree = f$element(1, "=", line)
$ root = deps - "]" + "." + tree + ".INSTALL_X86_64_CLANG.]"
$ define/process/translation_attributes=concealed 'lname' 'root'
$ goto dl
$dd:
$ close v
$ define/user sys$error sys$output
$ link/executable='here''img'.EXE/map='here''img'.MAP/full 'objs'[.OBJ_X86_64]PHP_CGI.OPT/options
$ st = $status
$ set default 'here'
$ write sys$output "R_MDBG_LINK: status ", st
$ exit st
