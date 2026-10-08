/* r_jmp.c - non-local jumps as the Zend engine uses them, with clang:
   zend_bailout() (every fatal error and exit()) is LONGJMP(*EG(bailout))
   = siglongjmp(buf, -1) back to a SETJMP = sigsetjmp(buf, 0) in an outer
   frame; the hard execution timeout does it from inside a signal handler.
   r_timer showed siglongjmp out of a SIGALRM handler ACCVIOs. */
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>

static sigjmp_buf sj;
static jmp_buf j;
static volatile int depth;

static void deep_sig(int n) { volatile char pad[4096]; pad[0] = (char) n; depth = n; if (n) deep_sig(n - 1); else siglongjmp(sj, -1); pad[1] = 0; }
static void deep(int n) { volatile char pad[4096]; pad[0] = (char) n; depth = n; if (n) deep(n - 1); else longjmp(j, 7); pad[1] = 0; }
static void on_alrm_long(int s) { longjmp(j, s); }
static void on_alrm_sig(int s) { siglongjmp(sj, s); }

int main(void)
{
  struct sigaction sa;
  volatile int r;

  r = sigsetjmp(sj, 0);
  if (r == 0) { deep_sig(20); printf("RESULT siglongjmp_plain not taken\n"); }
  else printf("RESULT siglongjmp_plain mask0 ok r=%d\n", r);
  fflush(stdout);

  r = sigsetjmp(sj, 1);
  if (r == 0) { deep_sig(20); }
  else printf("RESULT siglongjmp_plain mask1 ok r=%d\n", r);
  fflush(stdout);

  r = setjmp(j);
  if (r == 0) { deep(20); }
  else printf("RESULT longjmp_plain ok r=%d\n", r);
  fflush(stdout);

  memset(&sa, 0, sizeof sa);
  sigemptyset(&sa.sa_mask);
  sa.sa_handler = on_alrm_long;
  sigaction(SIGALRM, &sa, NULL);
  r = setjmp(j);
  if (r == 0) { raise(SIGALRM); printf("RESULT longjmp_from_handler not taken\n"); }
  else printf("RESULT longjmp_from_handler ok r=%d\n", r);
  fflush(stdout);

  sa.sa_handler = on_alrm_sig;
  sigaction(SIGALRM, &sa, NULL);
  r = sigsetjmp(sj, 0);
  if (r == 0) { raise(SIGALRM); printf("RESULT siglongjmp_from_handler mask0 not taken\n"); }
  else printf("RESULT siglongjmp_from_handler mask0 ok r=%d\n", r);
  fflush(stdout);
  return 0;
}
