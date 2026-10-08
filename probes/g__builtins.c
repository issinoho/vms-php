/* g__builtins.c - the compiler builtins PHP 8.5's configure checks for
   (Zend/zend_portability.h, Zend/zend_multiply.h, ext/standard): compile and
   link with clang. */
#include <stddef.h>
int main(void)
{
  long a = 3, r; long long b = 5, rr; unsigned u = 7, ur; void *fp;
  int x = __builtin_clz(8u) + __builtin_clzl(8ul) + __builtin_clzll(8ull)
        + __builtin_ctzl(8ul) + __builtin_ctzll(8ull);
  x += __builtin_saddl_overflow(a, a, &r) + __builtin_saddll_overflow(b, b, &rr)
     + __builtin_smull_overflow(a, a, &r) + __builtin_smulll_overflow(b, b, &rr)
     + __builtin_ssubl_overflow(a, a, &r) + __builtin_ssubll_overflow(b, b, &rr)
     + __builtin_usub_overflow(u, u, &ur);
  fp = __builtin_frame_address(0);
  if (__builtin_expect(x == -1, 0)) __builtin_unreachable();
  __builtin_cpu_init();
  x += __builtin_cpu_supports("sse4.2");
  { static void *lbl = &&done; goto *lbl; }   /* computed goto: HYBRID VM */
done:
  return fp == NULL ? x : 0;
}
