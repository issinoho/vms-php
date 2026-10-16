$! R_MDBG_LINK.COM - [.PHP_PROBE]PHP_CGI_MDBG.EXE: PHP_CGI's objects (the build's
$! own options file) with r_mdbg.obj (allocation tracker) linked first.
$! P1: build tree top, P2: dependency work directory (nodes.conf column 8)
$ set noon
$ set process/parse_style=extended
$ top = p1
$ deps = p2
$ here = f$environment("DEFAULT")
$ @'here'r_cc.com 'top' ../PHP_PROBE/r_mdbg.c ../PHP_PROBE/r_mdbg.obj
$ if .not. $status then exit $status
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
$ link/executable='here'PHP_CGI_MDBG.EXE/map='here'PHP_CGI_MDBG.MAP/full 'here'r_mdbg.obj, [.OBJ_X86_64]PHP_CGI.OPT/options
$ st = $status
$ set default 'here'
$ write sys$output "R_MDBG_LINK: status ", st
$ exit st
