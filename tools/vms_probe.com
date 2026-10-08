$! VMS_PROBE.COM - compile, link and run the Phase 0 probes in this directory
$! with clang (x86-64, LP64), as PHP will be built (docs/DECISIONS.md D2).
$! P1: compiler - CLANG (the only one used here; kept for tools/probe.sh).
$! P2: sets to run, any of H F G R (default all).
$! H_*.C  header compiles?         F_*.C  autoconf-style link test (no header)
$! G_*.C  declared by a header and links? (G__BUILTINS.C: compiler builtins)
$! R_*.C  runtime probes, run twice: C RTL defaults, then with the feature
$!        logicals a PHP process will set (process table only).  R_FIBER is
$!        linked with Boost.Context's x86-64 SysV ELF routines from Zend/asm.
$ set noon
$ set process/parse_style=extended
$! Without this the CRTL lower-cases clang's argv (-D_LARGEFILE became -d_largefile).
$ define/process decc$argv_parse_style enable
$ say = "write sys$output"
$ proc = f$environment("PROCEDURE")
$ set default 'f$parse(proc,,,"DEVICE")''f$parse(proc,,,"DIRECTORY")'
$ clang :== $sys$system:clang.exe
$ ccmd = "clang -std=gnu11 -D_LARGEFILE -D_USE_STD_STAT -c"
$ cout = "-o probe_tmp.obj"
$ say "PROBE compiler CLANG: ", ccmd
$ sets = f$edit(p2, "UPCASE")
$ if sets .eqs. "" then sets = "HFGR"
$ if f$locate("H", sets) .lt. f$length(sets) then gosub compile_set_h
$ if f$locate("F", sets) .lt. f$length(sets) then gosub compile_set_f
$ if f$locate("G", sets) .lt. f$length(sets) then gosub compile_set_g
$ if f$locate("R", sets) .lt. f$length(sets) then gosub runtime
$ if f$search("probe_tmp.*") .nes. "" then delete/nolog probe_tmp.*;*
$ say "PROBE-DONE"
$ exit
$!
$compile_set_h:
$ pat = "H_*.C"
$ goto compile_set
$compile_set_f:
$ pat = "F_*.C"
$ goto compile_set
$compile_set_g:
$ pat = "G_*.C"
$compile_set:
$ f = f$search(pat, 1)
$ if f .eqs. "" then return
$ n = f$edit(f$parse(f,,,"NAME"), "LOWERCASE")
$ if f$search("probe_tmp.obj") .nes. "" then delete/nolog probe_tmp.obj;*
$ define/user sys$output probe_cc.lis
$ define/user sys$error probe_cc.lis
$ 'ccmd' 'n'.c 'cout'
$ csev = $severity
$ lsev = "-"
$ if f$search("probe_tmp.obj") .eqs. "" then csev = 2
$ if csev .ne. 2 .and. csev .ne. 4 .and. f$extract(0,2,n) .nes. "h_"
$ then
$   define/user sys$output probe_ln.lis
$   define/user sys$error probe_ln.lis
$   link/exe=probe_tmp.exe probe_tmp.obj
$   lsev = $severity
$ endif
$ msg = ""
$ open/read/error=nomsg in probe_cc.lis
$msgloop:
$ read/end=msgend in line
$ if f$length(msg) .ge. 200 then goto msgloop
$ if f$locate("error:", line) .lt. f$length(line) then msg = msg + " " + f$extract(0,160,line)
$ goto msgloop
$msgend:
$ close in
$nomsg:
$ if f$search("probe_ln.lis") .nes. "" .and. lsev .nes. "-"
$ then
$   open/read/error=nolmsg in probe_ln.lis
$lmsgloop:
$   read/end=lmsgend in line
$   if f$extract(0,1,line) .eqs. "%" .and. f$length(msg) .lt. 200 then msg = msg + " " + f$element(0,",",line)
$   goto lmsgloop
$lmsgend:
$   close in
$nolmsg:
$ endif
$ say "RESULT ", n, " cc=", csev, " link=", lsev, msg
$ if f$search("probe_cc.lis") .nes. "" then delete/nolog probe_cc.lis;*
$ if f$search("probe_ln.lis") .nes. "" then delete/nolog probe_ln.lis;*
$ goto compile_set
$!
$runtime:
$ f = f$search("R_*.C", 2)
$ if f .eqs. "" then return
$ n = f$edit(f$parse(f,,,"NAME"), "LOWERCASE")
$ if f$search("probe_tmp.obj") .nes. "" then delete/nolog probe_tmp.obj;*
$ if f$search("''n'.exe") .nes. "" then delete/nolog 'n'.exe;*
$ define/user sys$error sys$output
$ 'ccmd' 'n'.c 'cout'
$ if f$search("probe_tmp.obj") .eqs. ""
$ then
$   say "--- ", n, " did not compile"
$   goto runtime
$ endif
$ extra = ""
$ if n .eqs. "r_fiber"
$ then
$   define/user sys$error sys$output
$   clang -c make_x86_64_sysv_elf_gas.S -o fiber_make.obj
$   say "RESULT fiber_asm make cc=", $severity
$   define/user sys$error sys$output
$   clang -c jump_x86_64_sysv_elf_gas.S -o fiber_jump.obj
$   say "RESULT fiber_asm jump cc=", $severity
$   extra = ",fiber_make.obj,fiber_jump.obj"
$ endif
$ define/user sys$error sys$output
$ link/exe='n'.exe probe_tmp.obj'extra'
$ if f$search("''n'.exe") .eqs. ""
$ then
$   say "--- ", n, " did not link"
$   goto runtime
$ endif
$ say "--- ", n, " (CRTL defaults)"
$ gosub clean_io
$ define/user sys$error sys$output
$ run 'n'.exe
$ say "--- ", n, " (feature logicals)"
$ gosub clean_io
$ define/process decc$efs_charset enable
$ define/process decc$efs_case_preserve enable
$ define/process decc$filename_unix_report enable
$ define/process decc$filename_unix_no_version enable
$ define/process decc$readdir_dropdotnotype enable
$ define/process decc$file_sharing enable
$ define/process decc$allow_remove_open_files enable
$ define/process decc$posix_seek_stream_file enable
$ define/process decc$rename_no_inherit enable
$ define/user sys$error sys$output
$ run 'n'.exe
$ deassign/process decc$efs_charset
$ deassign/process decc$efs_case_preserve
$ deassign/process decc$filename_unix_report
$ deassign/process decc$filename_unix_no_version
$ deassign/process decc$readdir_dropdotnotype
$ deassign/process decc$file_sharing
$ deassign/process decc$allow_remove_open_files
$ deassign/process decc$posix_seek_stream_file
$ deassign/process decc$rename_no_inherit
$ gosub clean_io
$ goto runtime
$!
$clean_io:
$ if f$search("[.ioprobe...]*.*") .nes. "" then delete/nolog [.ioprobe...]*.*;*
$ if f$search("ioprobe.dir") .nes. "" then set security/protection=(o:rwed) ioprobe.dir;*
$ if f$search("ioprobe.dir") .nes. "" then delete/nolog ioprobe.dir;*
$ return
