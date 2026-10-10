/* r_exec_status.c - can exec() learn a DCL command's $STATUS through popen?
   pclose() loses it (r_pclose_status, PORTING_LOG #35).  Each command runs
   under three wrappers:
     W1  PIPE cmd ; WRITE SYS$OUTPUT "@@ST=",$STATUS     (status on stdout)
     W2  PIPE cmd ; DEFINE/JOB/NOLOG name "''$STATUS'"   (job logical name)
     W3  PIPE cmd ; DEFINE/JOB/NOLOG name &$STATUS ...  ('' deferred: see output)
   and prints its output lines, pclose()'s value and the job logical name, which
   it then deletes.  "r_exec_status exit <n>" just calls exit(n) (built with
   _POSIX_EXIT, as PHP is), to see how a C program's exit code arrives. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <descrip.h>
#include <lnmdef.h>
#include <psldef.h>
#include <starlet.h>

static const char *cmds[] = {
  "write sys$output \"x\"", "xyzzy", "directory nosuch.xyz", "mcr []r_exec_status exit 0",
  "mcr []r_exec_status exit 3", "mcr []r_exec_status exit 255", "pipe write sys$output \"a\" | search sys$pipe a",
  "write sys$output \"semi ; bar | amp &\"", "exit 3", 0 };

static void run(int w, const char *cmd, int n)
{
  char full[1024], name[64], buf[512];
  FILE *p;
  int r;
  const char *gl;
  struct dsc$descriptor_s tab = {7, DSC$K_DTYPE_T, DSC$K_CLASS_S, "LNM$JOB"}, nd;

  sprintf(name, "PHPX_%x_%d", (unsigned) getpid(), n);
  if (w == 1) sprintf(full, "PIPE %s ; WRITE SYS$OUTPUT \"@@ST=\",$STATUS", cmd);
  else if (w == 2) sprintf(full, "PIPE %s ; DEFINE/JOB/NOLOG %s \"'$STATUS'\"", cmd, name);
  else sprintf(full, "PIPE %s ; DEFINE/JOB/NOLOG %s \"''$STATUS'\"", cmd, name);
  printf("RESULT W%d [%s]\n", w, cmd);
  p = popen(full, "r");
  if (!p) { printf("RESULT   popen failed\n"); return; }
  while (fgets(buf, sizeof buf, p)) { buf[strcspn(buf, "\n")] = 0; printf("RESULT   out: %s\n", buf); }
  r = pclose(p);
  gl = getenv(name);
  printf("RESULT   pclose 0x%x, job logical %s\n", r, gl ? gl : "(none)");
  nd.dsc$w_length = strlen(name); nd.dsc$b_dtype = DSC$K_DTYPE_T; nd.dsc$b_class = DSC$K_CLASS_S;
  nd.dsc$a_pointer = name;
  if (gl) printf("RESULT   dellnm %d\n", sys$dellnm(&tab, &nd, 0));
}

int main(int argc, char **argv)
{
  int w, i, n = 0;
  if (argc > 2 && !strcmp(argv[1], "exit")) exit(atoi(argv[2]));
  for (w = 1; w <= 3; w++)
    for (i = 0; cmds[i]; i++) run(w, cmds[i], n++);
  return 0;
}
