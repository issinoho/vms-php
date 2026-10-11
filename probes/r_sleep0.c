/* r_sleep0.c - Zend/tests/arginfo_zpp_mismatch hibernated for 10 hours on
   OpenVMS (no subprocess; PORTING_LOG #49).  It calls every function with
   null arguments: sleep(0), usleep(0), nanosleep(0)?  "r_sleep0 <n>" runs
   case n, with a 10 s alarm in case it never returns. */
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>

static void hung(int s) { printf("RESULT   hung\n"); fflush(stdout); _exit(0); }

int main(int argc, char **argv)
{
  int n = atoi(argv[1]);
  struct timespec ts = {0, 0};
  signal(SIGALRM, hung);
  alarm(10);
  if (n == 0) { printf("RESULT sleep(0): "); fflush(stdout); printf("%u\n", sleep(0)); }
  if (n == 1) { printf("RESULT usleep(0): "); fflush(stdout); printf("%d\n", usleep(0)); }
  if (n == 2) { printf("RESULT nanosleep(0,0): "); fflush(stdout); printf("%d\n", nanosleep(&ts, NULL)); }
  if (n == 3) { printf("RESULT usleep(1): "); fflush(stdout); printf("%d\n", usleep(1)); }
  if (n == 4) { ts.tv_nsec = 1; printf("RESULT nanosleep(0,1): "); fflush(stdout); printf("%d\n", nanosleep(&ts, NULL)); }
  return 0;
}
