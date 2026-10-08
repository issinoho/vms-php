/* r_fiber.c - do Boost.Context's x86-64 SysV ELF routines (Zend/asm,
   used by PHP's Fibers) work under the VMS x86-64 calling standard?
   Linked with make_x86_64_sysv_elf_gas.S and jump_x86_64_sysv_elf_gas.S
   assembled by clang (see vms_probe.com).  Switches into a context and back
   three times, as zend_fiber_switch_context does. */
#include <stdio.h>
#include <stdlib.h>

typedef void *fcontext_t;
typedef struct { fcontext_t from; void *data; } transfer_t;
extern fcontext_t make_fcontext(void *sp, size_t size, void (*fn)(transfer_t));
extern transfer_t jump_fcontext(fcontext_t to, void *vp);

static int counter;

static void body(transfer_t t)
{
  for (;;) {
    counter += (int) (size_t) t.data;
    t = jump_fcontext(t.from, (void *) (size_t) counter);
  }
}

int main(void)
{
  size_t size = 256 * 1024;
  char *stack = malloc(size);
  fcontext_t ctx;
  transfer_t t;
  int i;

  ctx = make_fcontext(stack + size, size, body);
  for (i = 1; i <= 3; i++) {
    t = jump_fcontext(ctx, (void *) (size_t) (i * 10));
    ctx = t.from;
    printf("RESULT fiber switch %d: back in main, value=%d\n", i, (int) (size_t) t.data);
  }
  printf("RESULT fiber %s\n", counter == 60 ? "ok" : "wrong");
  return 0;
}
