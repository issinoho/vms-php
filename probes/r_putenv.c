/* r_putenv.c - date tests that call putenv("TZ=...") ACCVIO in the C RTL at
   a sign-extended P2 address (PORTING_LOG #29).  Does the C RTL's putenv()
   keep a 64-bit string pointer in its 32-bit environ?  Variant (argument):
     1 putenv of a malloc'd (P2) string, then getenv and tzset
     2 the same with a _malloc32 string
     3 setenv("TZ", ...), then getenv and tzset
     4 putenv of a malloc'd string, then unsetenv */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void *_malloc32(size_t);

int main(int argc, char **argv)
{
  int v = argc > 1 ? atoi(argv[1]) : 1;
  char *s = v == 2 ? _malloc32(32) : malloc(32);
  const char *g;
  int r = 0;

  strcpy(s, "TZ=Europe/London");
  printf("RESULT variant %d string at %p\n", v, (void *) s);
  if (v == 3) r = setenv("TZ", "Europe/London", 1);
  else r = putenv(s);
  printf("RESULT variant %d set %d\n", v, r);
  g = getenv("TZ");
  printf("RESULT variant %d getenv [%s]\n", v, g ? g : "(null)");
  tzset();
  printf("RESULT variant %d tzset ok\n", v);
  if (v == 4) printf("RESULT variant 4 unsetenv %d\n", unsetenv("TZ"));
  return 0;
}
