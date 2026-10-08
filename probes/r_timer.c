/* r_timer.c - does setitimer() deliver its signal on VMS?
   PHP's max_execution_time uses setitimer(ITIMER_PROF) and SIGPROF
   (Zend/zend_execute_API.c zend_set_timeout_ex); ITIMER_REAL/SIGALRM is
   the alternative.  Each timer gets 0.3 s while the process burns CPU for up
   to 3 s of wall time. */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <setjmp.h>
#include <sys/time.h>
#include <time.h>

static volatile sig_atomic_t fired;
static void on_sig(int s) { fired = s; }

static void try_timer(const char *name, int which, int sig)
{
  struct sigaction sa;
  struct itimerval t;
  time_t start;
  volatile unsigned long spin = 0;
  int rc;

  memset(&sa, 0, sizeof sa);
  sa.sa_handler = on_sig;
  sigemptyset(&sa.sa_mask);
  rc = sigaction(sig, &sa, NULL);
  fired = 0;
  memset(&t, 0, sizeof t);
  t.it_value.tv_usec = 300000;
  errno = 0;
  rc = setitimer(which, &t, NULL);
  printf("RESULT timer_%s setitimer=%d errno=%d\n", name, rc, rc ? errno : 0);
  if (rc) return;
  start = time(NULL);
  while (!fired && time(NULL) - start < 3) spin++;
  printf("RESULT timer_%s fired=%s after %ld s\n", name, fired ? "yes" : "no", (long) (time(NULL) - start));
  memset(&t, 0, sizeof t);
  setitimer(which, &t, NULL);
}

static sigjmp_buf jb;
static void on_alrm_jump(int s) { siglongjmp(jb, s); }

int main(void)
{
  struct sigaction sa;
  sigset_t cur;

  try_timer("prof", ITIMER_PROF, SIGPROF);
  try_timer("virtual", ITIMER_VIRTUAL, SIGVTALRM);
  try_timer("real", ITIMER_REAL, SIGALRM);

  /* zend_bailout()-style escape: siglongjmp out of a handler, mask restored? */
  memset(&sa, 0, sizeof sa);
  sa.sa_handler = on_alrm_jump;
  sigemptyset(&sa.sa_mask);
  sigaction(SIGALRM, &sa, NULL);
  if (sigsetjmp(jb, 1) == 0) {
    raise(SIGALRM);
    printf("RESULT siglongjmp_from_handler no-jump\n");
  } else {
    sigprocmask(SIG_BLOCK, NULL, &cur);
    printf("RESULT siglongjmp_from_handler yes, SIGALRM blocked after=%d\n", sigismember(&cur, SIGALRM));
  }
  return 0;
}
