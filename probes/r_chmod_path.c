/* r_chmod_path.c - PHP stops seeing a file after chmod(f, 1), chmod(f, 2) by
   absolute path (probe r_perm_seq), C with a relative name does not
   (r_chmod_seq).  stat(), access(F_OK) and realpath() by absolute path after
   each mode.  Run unprivileged.  PORTING_LOG #42. */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <sys/stat.h>

int main(void)
{
  char cwd[512], f[600], rp[4096];
  static const mode_t modes[] = {0644, 0000, 0001, 0002, 0003, 0004, 0200, 0644};
  int i;
  struct stat sb;
  getcwd(cwd, sizeof cwd);
  sprintf(f, "%s/a.tmp", cwd);
  close(open(f, O_WRONLY | O_CREAT, 0644));
  for (i = 0; i < 8; i++) {
    int c = chmod(f, modes[i]), ce = errno, s, se, a, re;
    char *r;
    s = stat(f, &sb); se = errno;
    a = access(f, F_OK);
    r = realpath(f, rp); re = errno;
    printf("RESULT %04o: chmod %d/%d stat %d/%d mode %04o access %d realpath %s/%d\n", (unsigned) modes[i],
           c, c ? ce : 0, s, s ? se : 0, s ? 0 : (unsigned) sb.st_mode & 07777, a, r ? r : "NULL", r ? 0 : re);
  }
  unlink(f);
  return 0;
}
