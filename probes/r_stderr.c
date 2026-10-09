/* r_stderr.c - stdio's standard streams from clang code built like PHP
   (vms_lp64.h's printf wrappers, -O2).  The C RTL's stdin/stdout/stderr are
   32-bit pointer variables (decc$ga_stdout...), as environ is; a debug
   fprintf(stderr, ...) in PHP's pcre shutdown crashed calling address 0. */
#include <stdio.h>
#include <stdint.h>

int main(void)
{
  printf("RESULT stdout=%p stderr=%p stdin=%p\n", (void *) stdout, (void *) stderr, (void *) stdin);
  fflush(stdout);
  fprintf(stderr, "RESULT fprintf(stderr) works\n");
  fflush(stderr);
  fputs("RESULT fputs(stderr) works\n", stderr);
  fprintf(stdout, "RESULT fprintf(stdout) works\n");
  return 0;
}
