/* vms_fiber_exit.S - the _exit symbol Boost.Context's make_fcontext calls
   (Zend/asm/make_x86_64_sysv_elf_gas.S) if a fiber's entry function returns,
   which PHP never lets happen.  The VMS C RTL exports no symbol of that name,
   and clang maps the C name _exit to the C RTL's own entry point (even
   through an asm label, which then clashed with decc$_exit), so it is
   defined here in assembly and jumps to vms_fiber_exit_c in vms_shims.c
   (DECISIONS D10, PORTING_LOG). */
	.text
	.globl	_exit
	.type	_exit,@function
	.p2align 4
_exit:
	jmp	vms_fiber_exit_c
	.size	_exit, .-_exit
