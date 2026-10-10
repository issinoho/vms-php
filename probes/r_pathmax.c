/* r_pathmax.c - PATH_MAX and MAXPATHLEN as the C RTL defines them (PHP's
   MAXPATHLEN comes from them; gh9339 expects a 9,000-character path to be
   rejected). */
#include <stdio.h>
#include <limits.h>
#include <sys/param.h>
int main(void)
{
#ifdef PATH_MAX
  printf("RESULT PATH_MAX %d\n", (int) PATH_MAX);
#else
  printf("RESULT PATH_MAX undefined\n");
#endif
#ifdef MAXPATHLEN
  printf("RESULT MAXPATHLEN %d\n", (int) MAXPATHLEN);
#else
  printf("RESULT MAXPATHLEN undefined\n");
#endif
  return 0;
}
