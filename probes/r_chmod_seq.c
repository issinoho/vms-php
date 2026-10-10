/* r_chmod_seq.c - after chmod(f, 1) then chmod(f, 2), PHP's is_readable_basic
   file is gone (probe r_perm_seq).  Which C RTL call removes it?  Each pair
   on a fresh file: chmod(a), then chmod(b), existence checked after each
   (stat and access F_OK).  Run with and without privileges.  PORTING_LOG #42. */
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

static int ex(const char *f) { struct stat sb; return stat(f, &sb) == 0; }

static void pair(mode_t a, mode_t b)
{
  int r1, r2, e1, e2;
  close(open("q.tmp", O_WRONLY | O_CREAT, 0644));
  r1 = chmod("q.tmp", a); e1 = ex("q.tmp");
  r2 = chmod("q.tmp", b); e2 = ex("q.tmp");
  printf("RESULT chmod %04o: %d exists %d; chmod %04o: %d errno %d exists %d\n",
         (unsigned) a, r1, e1, (unsigned) b, r2, r2 ? errno : 0, e2);
  chmod("q.tmp", 0644);
  while (ex("q.tmp") && unlink("q.tmp") == 0) ;
}

int main(void)
{
  pair(0001, 0002); pair(0000, 0002); pair(0644, 0002); pair(0001, 0644); pair(0002, 0644);
  pair(0200, 0002); pair(0400, 0200);
  return 0;
}
