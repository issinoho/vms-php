/* r_pclose_status.c - what do pclose() and system() return for DCL
   commands that succeed, fail, or exit with a given VMS status?  For D11's
   mapping of exec()/system()/passthru() results (PORTING_LOG #35). */
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>

static void run(const char *cmd)
{
  FILE *p = popen(cmd, "r");
  char buf[256];
  int r, s;
  if (!p) { printf("RESULT popen failed for %s\n", cmd); return; }
  while (fgets(buf, sizeof buf, p)) ;
  r = pclose(p);
  s = system(cmd);
  printf("RESULT [%s] pclose %d (0x%x) exited %d status %d; system %d (0x%x)\n", cmd, r, r,
         WIFEXITED(r) ? 1 : 0, WEXITSTATUS(r), s, s);
}

int main(void)
{
  run("write sys$output \"x\"");
  run("echo Something");
  run("exit 1");
  run("exit 3");
  run("exit 2");
  run("exit 44");
  run("exit %X10000002");
  run("exit 0");
  return 0;
}
