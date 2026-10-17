/* r_fcntl_leak3: which lock/unlock shapes grow P0 (r_fcntl_leak2: a read
   lock on byte 1 released by a whole-file unlock, opcache's per-request
   pair, costs ~440 bytes of P0 each time; PORTING_LOG #53). Candidate fixes
   for accel_unlock_all() are the 1-byte unlocks of bytes 0, 1 and 2, held
   or not. usage: r_fcntl_leak3 [n] */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <jpidef.h>
#include <lib$routines.h>
#undef unlink

static unsigned long long p0(void) {
  int code = JPI$_FREP0VA;
  unsigned long long v = 0;
  lib$getjpi(&code, 0, 0, &v, 0, 0);
  return v;
}
static int fd;
static void lk(int type, int start, int len) {
  struct flock l = {0};
  l.l_whence = SEEK_SET; l.l_type = type; l.l_start = start; l.l_len = len;
  if (fcntl(fd, F_SETLK, &l) == -1) { perror("fcntl"); exit(1); }
}

static void shape(int s) {
  switch (s) {
  case 0: lk(F_RDLCK, 1, 1); lk(F_UNLCK, 1, 1); break;
  case 1: lk(F_RDLCK, 1, 1); lk(F_UNLCK, 0, 0); break;
  case 2: lk(F_RDLCK, 1, 1); lk(F_UNLCK, 0, 1); lk(F_UNLCK, 1, 1); lk(F_UNLCK, 2, 1); break;
  case 3: lk(F_UNLCK, 0, 0); break;
  case 4: lk(F_UNLCK, 1, 1); break;
  case 5: lk(F_WRLCK, 0, 1); lk(F_UNLCK, 0, 0); break;
  case 6: lk(F_RDLCK, 1, 1); lk(F_UNLCK, 0, 3); break;
  }
}
static const char *what[] = {
  "lock 1, unlock 1",
  "lock 1, unlock all",
  "lock 1, unlock 0, 1, 2",
  "unlock all (none held)",
  "unlock 1 (none held)",
  "wrlock 0, unlock all",
  "lock 1, unlock 0-2",
};

int main(int argc, char **argv) {
  int n = argc > 1 ? atoi(argv[1]) : 10000, s, i;
  char name[] = "r_fcntl3_XXXXXX";
  fd = mkstemp(name);
  if (fd < 0) { perror("mkstemp"); return 1; }
  for (s = 0; s < 7; s++) {
    unsigned long long a = p0();
    for (i = 0; i < n; i++) shape(s);
    printf("%-24s %6d: P0 +%9llu bytes (%6.1f each)\n", what[s], n,
           p0() - a, (double) (p0() - a) / n);
  }
  close(fd); unlink(name);
  return 0;
}
