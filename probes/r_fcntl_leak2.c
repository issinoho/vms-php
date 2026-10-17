/* r_fcntl_leak2: opcache's per-request lock pair (r_trace: 2 fcntl() per
   request with opcache on, PORTING_LOG #53) on its lock file as opcache sets
   it up (zend_shared_alloc.c): mkstemp in /sys$scratch, fchmod 0666,
   FD_CLOEXEC, then unlink() while open. r_fcntl_leak used a plain file and
   saw nothing. Per "request": a read lock on byte 1 (accel_activate_add),
   then an unlock of the whole file (accel_unlock_all). Measures P0 growth
   and PAGFILCNT. usage: r_fcntl_leak2 [n] */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <jpidef.h>
#include <lib$routines.h>
#undef unlink                   /* the C RTL's, not vms_php_unlink */

static unsigned long long jpi(int code) {
  unsigned long long v = 0;
  lib$getjpi(&code, 0, 0, &v, 0, 0);
  return v;
}

static void run(const char *what, int fd, int n) {
  struct flock l = {0};
  unsigned long long p0 = jpi(JPI$_FREP0VA), pg = jpi(JPI$_PAGFILCNT);
  int i;
  for (i = 0; i < n; i++) {
    l.l_whence = SEEK_SET; l.l_type = F_RDLCK; l.l_start = 1; l.l_len = 1;
    if (fcntl(fd, F_SETLK, &l) == -1) { perror("lock"); exit(1); }
    l.l_whence = SEEK_SET; l.l_type = F_UNLCK; l.l_start = 0; l.l_len = 0;
    if (fcntl(fd, F_SETLK, &l) == -1) { perror("unlock all"); exit(1); }
  }
  printf("%-26s %6d: P0 +%8llu bytes (%5.1f/pair), PAGFILCNT -%6llu\n", what, n,
         jpi(JPI$_FREP0VA) - p0, (double) (jpi(JPI$_FREP0VA) - p0) / n,
         pg - jpi(JPI$_PAGFILCNT));
}

int main(int argc, char **argv) {
  int n = argc > 1 ? atoi(argv[1]) : 10000, fd, val;
  char plain[] = "r_fcntl2_XXXXXX";
  char opc[] = "/sys$scratch/ZendSem_XXXXXX";

  fd = mkstemp(plain);
  if (fd < 0) { perror("mkstemp plain"); return 1; }
  run("plain file", fd, n);
  run("plain file", fd, 3 * n);
  close(fd); unlink(plain);

  fd = mkstemp(opc);
  if (fd < 0) { perror("mkstemp sys$scratch"); return 1; }
  if (fchmod(fd, 0666) == -1) perror("fchmod");
  val = fcntl(fd, F_GETFD, 0);
  fcntl(fd, F_SETFD, val | FD_CLOEXEC);
  if (unlink(opc) == -1) perror("unlink");
  run("opcache's (unlinked)", fd, n);
  run("opcache's (unlinked)", fd, 3 * n);
  close(fd);
  return 0;
}
