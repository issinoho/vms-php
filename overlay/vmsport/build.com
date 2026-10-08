$! BUILD.COM - build PHP for OpenVMS x86-64 with VSI clang (LP64)
$!
$! Usage:  @[.VMSPORT]BUILD [ALL|CLEAN|LINK] [KEEP_GOING] [deps]
$!   ALL         compile what is out of date, then link
$!   CLEAN       delete the objects and images
$!   LINK        link only
$!   KEEP_GOING  compile every source even after failures (all errors in
$!               one run); the build still fails at the end
$!   deps        the work directory holding the dependencies' trees (vms-zlib,
$!               vms-pcre2, vms-libxml2), e.g. DISK$X:[USER.VMS_LIGHTTPD];
$!               tools/build.sh passes it from nodes.conf (column 8)
$!
$! Inputs (tools/prepare.sh, gen_build.py): [.VMSPORT.BUILD]OBJECTS.TXT (one
$! line per object: group, object name, source, its -I/-D flags) and
$! INCLUDES.TXT.  A source is compiled when its object is missing or older;
$! headers and flags are not tracked, so CLEAN after changing either.
$! Outputs: [.OBJ_X86_64]PHP.EXE (CLI), PHP_CGI.EXE (CGI/FastCGI).
$!
$ status = 44
$ set noon
$ on control_y then goto done
$ saved_default = f$environment("DEFAULT")
$ proc = f$environment("PROCEDURE")
$ set default 'f$parse(proc,,,"DEVICE")''f$parse(proc,,,"DIRECTORY")'
$ set default [-]
$ top = f$environment("DEFAULT")
$ arch = f$edit(f$getsyi("ARCH_NAME"), "UPCASE")
$ if arch .nes. "X86_64"
$ then
$   write sys$error "BUILD: x86-64 only (clang)"
$   goto done
$ endif
$ target = f$edit(p1, "UPCASE")
$ if target .eqs. "" then target = "ALL"
$ keep_going = f$edit(p2, "UPCASE") .eqs. "KEEP_GOING"
$ obj = "[.OBJ_X86_64]"
$ write sys$output "BUILD: ''target' for ''arch' (clang) in ''top'"
$ if target .eqs. "CLEAN"
$ then
$   if f$search("''obj'*.*;*") .nes. "" then delete/nolog 'obj'*.*;*
$   status = 1
$   goto finish
$ endif
$ if f$search("OBJ_X86_64.DIR") .eqs. "" then create/directory 'obj'
$!
$! --- the dependencies (DECISIONS D4): concealed rooted logical names, and
$! <openssl/x.h> -> VSI SSL3's flat include directory
$ if f$trnlnm("SSL3$INCLUDE") .eqs. ""
$ then
$   write sys$error "BUILD: VSI's SSL3 kit (SSL3$INCLUDE) is not installed"
$   goto done
$ endif
$ define/process openssl ssl3$include:
$ deps = p3
$ if deps .eqs. ""
$ then
$   write sys$error "BUILD: no dependency work directory (P3)"
$   goto done
$ endif
$ open/read v [.VMSPORT]DEPS.ENV
$deps_loop:
$ read/end=deps_done v line
$ lname = f$element(0, "=", line)
$ tree = f$element(1, "=", line)
$ root = deps - "]" + "." + tree + ".INSTALL_X86_64_CLANG.]"
$ define/process/translation_attributes=concealed 'lname' 'root'
$ if f$search("''lname':[LIB]*.OLB") .eqs. "" then -
    write sys$error "BUILD: ''lname' (''root') has no library: build the dependency first"
$ goto deps_loop
$deps_done:
$ close v
$!
$ clang :== $sys$system:clang.exe
$! keep the case of clang's arguments (-D...), and show its diagnostics
$ set process/parse_style=extended
$ define/process decc$argv_parse_style enable
$! let clang open UNIX names with more than one dot (json_parser.tab.c)
$ define/process decc$efs_charset enable
$!
$! --- the common flags, in a response file: clang finds one only by an
$! absolute UNIX path (vms-mariadb).  The family's flags (vms-mariadb
$! clang_common.rsp): __GNUC__ (VSI's clang does not define it), 64-bit argv,
$! shortened names over 31 characters (as VSI C libraries such as SSL3 export
$! them), vms_lp64.h for the C RTL's 32-bit long interfaces (D11).
$ unix = "/" + (f$parse(top,,,"DEVICE","NO_CONCEAL") - ":")
$ dirs = f$parse(top,,,"DIRECTORY","NO_CONCEAL") - "[" - "]" - "<" - ">"
$ dirs = dirs - "000000." - ".000000"
$ i = 0
$unix_loop:
$ e = f$element(i, ".", dirs)
$ if e .eqs. "." .or. e .eqs. "" then goto unix_done
$ unix = unix + "/" + e
$ i = i + 1
$ goto unix_loop
$unix_done:
$ rsp = "[.VMSPORT.BUILD]CLANG.RSP"
$ create/fdl="RECORD; FORMAT STREAM_LF;" 'rsp'
$ open/append r 'rsp'
$ write r "-std=gnu11 -O2 -w -fno-common -ffp-contract=off"
$ write r "-names2=shortened -pointer-size=argv64"
$ write r "-D__GNUC__=4 -D__GNUC_MINOR__=2 -D__GNUC_PATCHLEVEL__=1"
$ write r "-D_LARGEFILE -D_USE_STD_STAT -D_POSIX_EXIT -D_PTHREAD_EXC_INCL_CLEAN -DNDEBUG"
$! clang predefines __SSE2__, but VSI ships no x86 intrinsics headers
$! (<emmintrin.h>): PHP's SSE2 paths are left out; floating point still
$! uses the SSE registers (the calling standard is unchanged)
$ write r "-U__SSE2__"
$ write r "-include vmsport/include/vms_lp64.h"
$ write r "-include vmsport/include/vms_php_compat.h"
$! standard headers the C RTL lacks: stdalign.h, search.h, syslog.h
$ write r "-Ivmsport/include/compat"
$ open/read inc [.VMSPORT.BUILD]INCLUDES.TXT
$inc_loop:
$ read/end=inc_done inc line
$ write r line
$ goto inc_loop
$inc_done:
$ close inc
$ close r
$ purge/nolog 'rsp'
$ rspunix = unix + "/vmsport/build/clang.rsp"
$ if target .eqs. "LINK" then goto link
$!
$! --- compile
$ failed = 0
$ ncomp = 0
$ open/read lst [.VMSPORT.BUILD]OBJECTS.TXT
$compile_loop:
$ read/end=compile_done lst line
$ grp = f$element(0, " ", line)
$ name = f$element(1, " ", line)
$ src = f$element(2, " ", line)
$ vsrc = f$element(3, " ", line)
$ flags = f$extract(f$length(grp) + f$length(name) + f$length(src) + f$length(vsrc) + 4, 2048, line)
$ gosub compile_one
$ if failed .gt. 0 .and. .not. keep_going then goto compile_done
$ goto compile_loop
$compile_done:
$ close lst
$ src = "vmsport/vms_crtl_init.c"
$ vsrc = "[.VMSPORT]VMS_CRTL_INIT.C"
$ name = "vms_crtl_init"
$ flags = ""
$ gosub compile_one
$ src = "vmsport/vms_fiber_exit.S"
$ vsrc = "[.VMSPORT]VMS_FIBER_EXIT.S"
$ name = "vms_fiber_exit"
$ gosub compile_one
$ src = "vmsport/vms_shims.c"
$ vsrc = "[.VMSPORT]VMS_SHIMS.C"
$ name = "vms_shims"
$ gosub compile_one
$ src = "vmsport/vms_syslog.c"
$ vsrc = "[.VMSPORT]VMS_SYSLOG.C"
$ name = "vms_syslog"
$ gosub compile_one
$ write sys$output "BUILD: ''ncomp' compiled, ''failed' failed"
$ if failed .gt. 0 then goto done
$!
$! --- link
$link:
$! The GLOBAL objects are linked directly, not from an object library: the
$! librarian names a module after its source file, and PHP has many sources
$! of the same name in different directories (main/streams/filter.c and
$! ext/filter/filter.c, lexbor's state.c files...), so a library keeps only
$! the first of each (%LIBRAR-W-DUPMODULE; PORTING_LOG).
$ lerr = 0
$ image = "PHP"
$ groups = "GLOBAL,CLI,BINARY"
$ gosub link_image
$ image = "PHP_CGI"
$ groups = "GLOBAL,CGI,FASTCGI,BINARY_CGI"
$ gosub link_image
$ if lerr .gt. 0 then goto done
$ status = 1
$finish:
$ write sys$output "BUILD: done"
$done:
$ if f$trnlnm("OPENSSL", "LNM$PROCESS") .nes. "" then deassign/process openssl
$ set default 'saved_default'
$ exit status
$!
$! compile src into obj name, unless the object is newer than the source
$compile_one:
$ o = obj + name + ".OBJ"
$ if f$search(vsrc) .eqs. ""
$ then
$   failed = failed + 1
$   write sys$output "BUILD: FAILED ''src': no ''vsrc'"
$   return
$ endif
$ if f$search(o) .nes. ""
$ then
$   if f$cvtime(f$file_attributes(o, "RDT")) .ges. f$cvtime(f$file_attributes(vsrc, "RDT")) -
        then return
$ endif
$ ncomp = ncomp + 1
$ define/user sys$error sys$output
$! assembler sources (Boost.Context's fiber routines, D10) without the
$! response file: its forced C headers would be read as assembly
$ if f$edit(f$parse(vsrc,,,"TYPE"), "UPCASE") .eqs. ".S"
$ then
$   clang 'flags' -c 'src' -o 'o'
$ else
$   clang "@''rspunix'" 'flags' -c 'src' -o 'o'
$ endif
$ if .not. $status .or. f$search(o) .eqs. ""
$ then
$   failed = failed + 1
$   write sys$output "BUILD: FAILED ''src'"
$ endif
$ return
$!
$! link 'image' from the objects of 'groups', the PHP library and the
$! dependencies
$link_image:
$ opt = obj + image + ".OPT"
$ create/fdl="RECORD; FORMAT STREAM_LF;" 'opt'
$ open/append o 'opt'
$ open/read lst [.VMSPORT.BUILD]OBJECTS.TXT
$lo_loop:
$ read/end=lo_done lst line
$ if f$locate("," + f$element(0, " ", line) + ",", "," + groups + ",") .ge. f$length("," + groups + ",") -
    then goto lo_loop
$ write o obj, f$element(1, " ", line), ".OBJ"
$ goto lo_loop
$lo_done:
$ close lst
$ write o obj, "VMS_CRTL_INIT.OBJ"
$ write o obj, "VMS_FIBER_EXIT.OBJ"
$ write o obj, "VMS_SYSLOG.OBJ"
$ write o obj, "VMS_SHIMS.OBJ"
$ write o "PCRE2$ROOT:[LIB]PCRE2-8.OLB/LIBRARY"
$ write o "LIBXML2$ROOT:[LIB]LIBXML2.OLB/LIBRARY"
$ write o "ZLIB$ROOT:[LIB]LIBZ.OLB/LIBRARY"
$ write o "SYS$SHARE:SSL3$LIBSSL_SHR/SHAREABLE"
$ write o "SYS$SHARE:SSL3$LIBCRYPTO_SHR/SHAREABLE"
$ write o "PSECT_ATTR=LIB$INITIALIZE,CON,REL,GBL,NOSHR,NOEXE,RD,NOWRT"
$ close o
$ write sys$output "BUILD: link ''obj'''image'.EXE"
$ define/user sys$error sys$output
$ link/executable='obj''image'.EXE/map='obj''image'.MAP 'opt'/options
$ if .not. $status
$ then
$   lerr = lerr + 1
$   write sys$output "BUILD: link of ''image' failed"
$ endif
$ return
