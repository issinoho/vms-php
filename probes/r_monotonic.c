/* r_monotonic: a clock for hrtime() (PORTING_LOG #58: hrtime() is false; Zend's
   zend_hrtime.h wants _POSIX_TIMERS > 0, _POSIX_MONOTONIC_CLOCK and CLOCK_MONOTONIC,
   and HAVE_CLOCK_GETTIME was undefined by probe g_clock_gettime). What the headers
   say, whether clock_gettime() links and runs for CLOCK_REALTIME and CLOCK_MONOTONIC,
   and its resolution and monotonicity over 1,000,000 reads. */
#include <stdio.h>
#include <time.h>
#include <unistd.h>

int main(void) {
#ifdef _POSIX_TIMERS
  printf("_POSIX_TIMERS %ld\n", (long) _POSIX_TIMERS);
#else
  printf("_POSIX_TIMERS undefined\n");
#endif
#ifdef _POSIX_MONOTONIC_CLOCK
  printf("_POSIX_MONOTONIC_CLOCK %ld\n", (long) _POSIX_MONOTONIC_CLOCK);
#else
  printf("_POSIX_MONOTONIC_CLOCK undefined\n");
#endif
#ifdef CLOCK_MONOTONIC
  printf("CLOCK_MONOTONIC %d\n", (int) CLOCK_MONOTONIC);
  {
    struct timespec a, b, r;
    long i, back = 0;
    int st = clock_gettime(CLOCK_MONOTONIC, &a);
    printf("clock_gettime(MONOTONIC) = %d: %ld.%09ld\n", st, (long) a.tv_sec, a.tv_nsec);
    if (clock_getres(CLOCK_MONOTONIC, &r) == 0)
      printf("clock_getres(MONOTONIC): %ld.%09ld\n", (long) r.tv_sec, r.tv_nsec);
    for (i = 0; i < 1000000; i++) {
      clock_gettime(CLOCK_MONOTONIC, &b);
      if (b.tv_sec < a.tv_sec || (b.tv_sec == a.tv_sec && b.tv_nsec < a.tv_nsec)) back++;
      a = b;
    }
    printf("1000000 reads: %ld went backwards\n", back);
  }
#else
  printf("CLOCK_MONOTONIC undefined\n");
#endif
#ifdef CLOCK_REALTIME
  {
    struct timespec t;
    printf("clock_gettime(REALTIME) = %d\n", clock_gettime(CLOCK_REALTIME, &t));
  }
#endif
  return 0;
}
