/* r_fiber_stack.c - which fiber bodies raise %SYSTEM-F-STKOVF?  Every phpt
   test that starts a Fiber dies with STKOVF in a system-space routine, RSP on
   the fiber's stack.  r_fiber (a body that calls nothing, malloc'd stack)
   works.  Run with one argument, a variant (one image per variant: STKOVF
   ends it):
     1 malloc'd stack, empty body          2 malloc'd stack, body calls printf
     3 mmap'd stack as zend_fibers.c (guard page), empty body
     4 mmap'd stack, body calls printf     5 mmap'd stack, body with a 64 KB frame
     6 mmap'd stack, body calls malloc/free
     7 mmap'd stack, body calls write(1)  8 mmap'd stack, body calls SYS$GETTIM
     9 stack carved from the main stack (alloca), body calls printf
    10 mmap'd stack, body calls sprintf (no I/O)
    11 _malloc32 stack (32-bit address space), body calls sprintf
    12 _malloc32 stack, body calls printf
    13 as patch 0006: _malloc32, page-aligned, mprotect guard page, printf, then
       the guard restored and freed (x50, so freed stacks are reused) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <alloca.h>
#include <starlet.h>

void *_malloc32(size_t);

typedef void *fcontext_t;
typedef struct { fcontext_t from; void *data; } transfer_t;
extern fcontext_t make_fcontext(void *sp, size_t size, void (*fn)(transfer_t));
extern transfer_t jump_fcontext(fcontext_t to, void *vp);

static int variant;

static int big_frame(int n)
{
  volatile char buf[64 * 1024];
  memset((char *) buf, n, sizeof buf);
  return buf[n] + buf[sizeof buf - 1];
}

static void body(transfer_t t)
{
  int r = 0;
  if (variant == 7) r = write(1, "RESULT in fiber: write works\n", 29);
  if (variant == 8) { unsigned __int64 q; r = sys$gettim((void *) &q); }
  if (variant == 10 || variant == 11 || variant == 13) { char b[64]; r = sprintf(b, "%d %s", 42, "x"); }
  if (variant == 2 || variant == 4 || variant == 9 || variant == 12) r = printf("RESULT in fiber: printf works\n");
  if (variant == 5) r = big_frame(1);
  if (variant == 6) { void *p = malloc(1000); r = p != 0; free(p); }
  jump_fcontext(t.from, (void *) (size_t) (r + 1));
}

int main(int argc, char **argv)
{
  size_t size = 2 * 1024 * 1024, page = 8192;
  char *stack;
  transfer_t t;

  variant = argc > 1 ? atoi(argv[1]) : 1;
  if (variant == 9) {
    size = 512 * 1024;
    stack = alloca(size);
  } else if (variant == 13) {
    int i;
    for (i = 0; i < 50; i++) {
      char *raw = _malloc32(size + 2 * page), *g;
      if (!raw) { printf("RESULT variant 13 _malloc32 failed at %d\n", i); return 2; }
      g = (char *) (((size_t) raw + page - 1) & ~(page - 1));
      if (mprotect(g, page, PROT_NONE) < 0) { perror("mprotect"); return 2; }
      stack = g + page;
      t = jump_fcontext(make_fcontext(stack + size, size, body), 0);
      if (mprotect(g, page, PROT_READ | PROT_WRITE) < 0) { perror("mprotect restore"); return 2; }
      free(raw);
    }
    printf("RESULT variant 13 ok, 50 cycles, last stack %p\n", (void *) stack);
    return 0;
  } else if (variant >= 11) {
    stack = _malloc32(size);
  } else if (variant <= 2) {
    stack = malloc(size);
  } else {
    char *p = mmap(NULL, size + page, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (p == MAP_FAILED) { perror("mmap"); return 2; }
    if (mprotect(p, page, PROT_NONE) < 0) perror("mprotect");
    stack = p + page;
  }
  printf("RESULT variant %d stack %p-%p main sp near %p\n", variant, (void *) stack,
         (void *) (stack + size), (void *) &t);
  t = jump_fcontext(make_fcontext(stack + size, size, body), 0);
  printf("RESULT variant %d ok (%d)\n", variant, (int) (size_t) t.data);
  return 0;
}
