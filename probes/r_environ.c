/* r_environ.c - the C RTL's environ as clang (64-bit pointers) sees it.
   PHP declares "extern char **environ" (main/php.h) and walks it for $_ENV
   and $_SERVER; PHP.EXE crashed there with an address whose low 32 bits
   looked like a pointer.  Read the variable and its entries as 32-bit
   values instead. */
#include <stdio.h>
#include <stdint.h>

extern unsigned int environ;     /* the C RTL's char ** is a 32-bit pointer */

int main(void)
{
  unsigned int *e = (unsigned int *) (uintptr_t) environ;
  int i;
  printf("RESULT environ (32-bit) = %08X\n", environ);
  for (i = 0; e && e[i] && i < 4; i++)
    printf("RESULT environ[%d] = %.60s\n", i, (char *) (uintptr_t) e[i]);
  return 0;
}
