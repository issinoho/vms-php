/* r_fcntl_leak: does a fcntl() F_SETLK lock/unlock pair use page-file quota?
   opcache takes and drops a read lock on byte 1 of its lock file on every
   request (accel_activate_add/accel_deactivate_sub). Prints PAGFILCNT
   (pagelets left) every 20000 pairs; also a loop of F_GETLK for comparison,
   and opcache's request-end unlock of the whole file (l_start 0, l_len 0:
   accel_unlock_all), alone and after the pair. */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <jpidef.h>
#include <lib$routines.h>
static long left(void) { int code = JPI$_PAGFILCNT; unsigned int v = 0;
  lib$getjpi(&code, 0, 0, &v, 0, 0); return (long) v; }
int main(void) {
  char name[] = "r_fcntl_XXXXXX";
  int fd = mkstemp(name), i;
  struct flock l = {0};
  long start = left();
  if (fd < 0) { perror("mkstemp"); return 1; }
  printf("start %ld\n", start);
  for (i = 1; i <= 100000; i++) {
    l.l_whence = SEEK_SET; l.l_start = 1; l.l_len = 1;
    l.l_type = F_RDLCK; if (fcntl(fd, F_SETLK, &l) == -1) { perror("lock"); return 1; }
    l.l_type = F_UNLCK; if (fcntl(fd, F_SETLK, &l) == -1) { perror("unlock"); return 1; }
    if (i % 20000 == 0) printf("setlk %6d  used %ld pagelets\n", i, start - left());
  }
  start = left();
  for (i = 1; i <= 100000; i++) {
    l.l_whence = SEEK_SET; l.l_start = 2; l.l_len = 1; l.l_type = F_WRLCK;
    if (fcntl(fd, F_GETLK, &l) == -1) { perror("getlk"); return 1; }
    if (i % 50000 == 0) printf("getlk %6d  used %ld pagelets\n", i, start - left());
  }
  start = left();
  for (i = 1; i <= 20000; i++) {
    l.l_whence = SEEK_SET; l.l_start = 0; l.l_len = 0; l.l_type = F_UNLCK;
    if (fcntl(fd, F_SETLK, &l) == -1) { perror("unlock all"); return 1; }
  }
  printf("unlock-all 20000  used %ld pagelets\n", start - left());
  start = left();
  for (i = 1; i <= 20000; i++) {
    l.l_whence = SEEK_SET; l.l_start = 1; l.l_len = 1;
    l.l_type = F_RDLCK; fcntl(fd, F_SETLK, &l);
    l.l_type = F_UNLCK; fcntl(fd, F_SETLK, &l);
    l.l_start = 0; l.l_len = 0; fcntl(fd, F_SETLK, &l);
  }
  printf("pair+unlock-all 20000  used %ld pagelets\n", start - left());
  close(fd); unlink(name);
  return 0;
}
