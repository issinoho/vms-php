/* r_popen_pipe.c - which PIPE commands work under popen()?  Through patch
   0012's wrapper a pipeline lost the $STATUS line after it, and a PIPE
   subshell "(a | b)" hung, its DCL subprocess waiting in LEF (PORTING_LOG
   #35).  Plain popen, no wrapper: "r_popen_pipe <n>" runs case n, prints its
   output and pclose(), or "hung" after 20 s.  Run each case in a batch job, so
   a hung subprocess goes when the job logs out. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

static const char *cmds[] = {
  "pipe write sys$output \"a\" | search sys$pipe a",
  "pipe write sys$output \"a\" | search sys$pipe a ; write sys$output \"z\"",
  "pipe write sys$output \"a\" ; write sys$output \"z\"",
  "pipe (write sys$output \"a\") ; write sys$output \"z\"",
  "pipe (write sys$output \"a\" | search sys$pipe a)",
  "pipe (write sys$output \"a\" | search sys$pipe a) ; write sys$output \"z\"",
  "pipe write sys$output \"a\" | search sys$pipe a | search sys$pipe a ; write sys$output \"z\"",
  "pipe write sys$output \"a\" | (search sys$pipe a ; write sys$output \"z\")",
  0 };

static void hung(int s) { printf("RESULT   hung\n"); fflush(stdout); _exit(0); }

int main(int argc, char **argv)
{
  int n = atoi(argv[1]), r;
  char buf[256];
  FILE *p;
  printf("RESULT %d [%s]\n", n, cmds[n]);
  signal(SIGALRM, hung);
  alarm(20);
  if (!(p = popen(cmds[n], "r"))) { printf("RESULT   popen failed\n"); return 0; }
  while (fgets(buf, sizeof buf, p)) { buf[strcspn(buf, "\n")] = 0; printf("RESULT   out: %s\n", buf); }
  r = pclose(p);
  printf("RESULT   pclose 0x%x\n", r);
  return 0;
}
