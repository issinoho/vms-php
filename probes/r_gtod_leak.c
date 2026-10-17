/* r_gtod_leak: does gettimeofday(&tv, NULL) grow P0? PORTING_LOG #53: with
   opcache on, each request grows P0 by ~434 bytes, and the only C RTL calls
   per request that opcache adds are 2 fcntl() and 1 gettimeofday(), the
   latter from sapi_get_request_time() (r_trace). time() as a control.
   usage: r_gtod_leak [n] */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <jpidef.h>
#include <lib$routines.h>

static unsigned long long jpi(int code) {
  unsigned long long v = 0;
  lib$getjpi(&code, 0, 0, &v, 0, 0);
  return v;
}

static void run(const char *what, int n, int gtod) {
  struct timeval tv;
  unsigned long long p0 = jpi(JPI$_FREP0VA), pg = jpi(JPI$_PAGFILCNT);
  int i;
  for (i = 0; i < n; i++) {
    if (gtod) gettimeofday(&tv, NULL);
    else (void) time(0);
  }
  printf("%-13s %6d calls: P0 +%8llu bytes, PAGFILCNT -%6llu pagelets\n", what, n,
         jpi(JPI$_FREP0VA) - p0, pg - jpi(JPI$_PAGFILCNT));
}

int main(int argc, char **argv) {
  int n = argc > 1 ? atoi(argv[1]) : 10000;
  run("time", n, 0);
  run("gettimeofday", n, 1);
  run("gettimeofday", 3 * n, 1);
  return 0;
}
