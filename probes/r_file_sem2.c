/* r_file_sem2.c - second round of r_file_sem (PORTING_LOG #36-): PHP's
   flock() is fcntl(F_SETLKW) with F_RDLCK for LOCK_SH and F_WRLCK for LOCK_EX
   (ext/standard/flock_compat.c); PHP renames by absolute UNIX path.  Linked
   with vms_crtl_init.c; run in an empty scratch directory. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

static void lock(const char *what, int mode, int cmd, int type)
{
  struct flock fl;
  int fd = open("lock.tmp", mode | O_CREAT, 0644), r;
  memset(&fl, 0, sizeof fl);
  fl.l_type = type; fl.l_whence = SEEK_SET;
  r = fcntl(fd, cmd, &fl);
  printf("RESULT lock %s: %d errno %d (%s)\n", what, r, r ? errno : 0, r ? strerror(errno) : "");
  fl.l_type = F_UNLCK;
  fcntl(fd, F_SETLK, &fl);
  close(fd);
}

int main(void)
{
  char cwd[512], a[600], b[600];
  int r;

  lock("F_SETLKW F_WRLCK O_RDWR", O_RDWR, F_SETLKW, F_WRLCK);
  lock("F_SETLKW F_RDLCK O_RDWR", O_RDWR, F_SETLKW, F_RDLCK);
  lock("F_SETLK F_RDLCK O_RDONLY", O_RDONLY, F_SETLK, F_RDLCK);
  lock("F_SETLKW F_RDLCK O_RDONLY", O_RDONLY, F_SETLKW, F_RDLCK);
  lock("F_SETLK F_WRLCK O_WRONLY", O_WRONLY, F_SETLK, F_WRLCK);

  getcwd(cwd, sizeof cwd);
  printf("RESULT cwd %s\n", cwd);
  sprintf(a, "%s/FROM_x", cwd);
  sprintf(b, "%s/TO_x", cwd);
  mkdir(a, 0755);
  r = rename(a, b);
  printf("RESULT rename(%s, %s): %d errno %d (%s)\n", a, b, r, r ? errno : 0, r ? strerror(errno) : "");
  rmdir(a); rmdir(b);
  mkdir("FROM_y", 0755);
  r = rename("FROM_y", "TO_y");
  printf("RESULT rename(FROM_y, TO_y): %d errno %d (%s)\n", r, r ? errno : 0, r ? strerror(errno) : "");
  rmdir("FROM_y"); rmdir("TO_y");
  {
    static const int modes[] = {0755, 0777, 0700, 0775};
    int i;
    for (i = 0; i < 4; i++) {
      mkdir("FROM_m", modes[i]);
      r = rename("FROM_m", "TO_m");
      printf("RESULT rename dir made with mode %o: %d errno %d (%s)\n", modes[i], r, r ? errno : 0, r ? strerror(errno) : "");
      rmdir("FROM_m"); rmdir("TO_m");
    }
  }
  sprintf(a, "%s/f.tmp", cwd);
  sprintf(b, "%s/g.tmp", cwd);
  close(open(a, O_WRONLY | O_CREAT, 0644));
  r = rename(a, b);
  printf("RESULT rename file absolute: %d errno %d\n", r, r ? errno : 0);
  unlink(b);
  return 0;
}
