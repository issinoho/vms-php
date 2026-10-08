/* r_entropy.c - is there an OS entropy service for random_bytes()?
   Tries SYS$GET_ENTROPY (VSI OpenVMS V9.2 and later, if present: linking
   fails otherwise, which is the answer too). */
#include <stdio.h>
#include <string.h>

extern int sys$get_entropy(void *buffer, unsigned int length);

int main(void)
{
  unsigned char b1[32], b2[32];
  int s1, s2, i;
  memset(b1, 0, sizeof b1);
  memset(b2, 0, sizeof b2);
  s1 = sys$get_entropy(b1, sizeof b1);
  s2 = sys$get_entropy(b2, sizeof b2);
  printf("RESULT sys$get_entropy status=%%X%08X %%X%08X differ=%d first=", s1, s2,
         memcmp(b1, b2, sizeof b1) != 0);
  for (i = 0; i < 8; i++) printf("%02x", b1[i]);
  printf("\n");
  return 0;
}
