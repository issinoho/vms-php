/* r_memset_ret.c - does the C RTL's memset() return its destination?
   PHP's _ecalloc is "p = _emalloc(size); memset(p, 0, size); return p;",
   which clang may compile as "return memset(p, 0, size);".  In PHP.EXE
   ecalloc() popped the right block but returned another, page-aligned one.
   Also memcpy/memmove/strcpy, whose results code uses the same way. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
  static const size_t offs[] = { 0, 8, 0x18, 0x78, 0x1f0, 0x1000 + 0x48 };
  char *base = malloc(0x4000);
  int i;
  for (i = 0; i < (int) (sizeof offs / sizeof offs[0]); i++) {
    char *p = base + offs[i];
    volatile size_t n = 24;
    void *r = memset(p, 0, n);
    void *c = memcpy(p, "abcdefghijklmnopqrstuvwxyz", n);
    void *m = memmove(p + 1, p, n);
    char *s = strcpy(p, "x");
    printf("RESULT p=%p memset=%p memcpy=%p memmove(p+1)=%p strcpy=%p %s\n", (void *) p, r, c, m,
           (void *) s, (r == p && c == p && m == p + 1 && s == p) ? "ok" : "WRONG");
  }
  return 0;
}
