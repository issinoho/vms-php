/* r_strerror_fix.c - vmsport/vms_strerror.c through vms_php_compat.h
   (PORTING_LOG #28): glibc's wording, strerror_r truncation, and the C RTL's
   own text for errnos outside the table. */
#include <stdio.h>
#include <string.h>
#include <errno.h>

int main(void)
{
  char b[128], small[8];
  int r;
  printf("RESULT ENOENT [%s]\n", strerror(ENOENT));
  printf("RESULT EADDRNOTAVAIL [%s]\n", strerror(EADDRNOTAVAIL));
  r = strerror_r(EACCES, b, sizeof b);
  printf("RESULT strerror_r EACCES %d [%s]\n", r, b);
  r = strerror_r(EACCES, small, sizeof small);
  printf("RESULT strerror_r small %d [%s] (want %d [Permiss])\n", r, small, ERANGE);
#ifdef EVMSERR
  printf("RESULT EVMSERR [%s]\n", strerror(EVMSERR));
#endif
  printf("RESULT 9999 [%s]\n", strerror(9999));
  return 0;
}
