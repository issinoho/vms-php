/* r_fopen_odd.c - openssl tests expect opening "" and writing to a directory
   path to fail (PORTING_LOG #32); on VMS both seem to succeed.  Linked with
   vms_crtl_init.c so PHP's C RTL features are in effect.  Run in a scratch
   directory holding a subdirectory SUBDIR. */
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

static void try_fopen(const char *path, const char *mode)
{
  FILE *f;
  errno = 0;
  f = fopen(path, mode);
  printf("RESULT fopen(\"%s\", \"%s\") %s errno %d\n", path, mode, f ? "OPENED" : "failed", errno);
  if (f) fclose(f);
}

int main(void)
{
  char cwd[512];
  struct stat st;
  int fd;
  getcwd(cwd, sizeof cwd);
  printf("RESULT cwd %s\n", cwd);
  try_fopen("", "r");
  try_fopen("", "w");
  try_fopen("subdir", "r");
  try_fopen("subdir", "w");
  try_fopen("subdir/", "w");
  errno = 0;
  fd = open("subdir", O_WRONLY | O_CREAT | O_TRUNC, 0644);
  printf("RESULT open(\"subdir\", O_WRONLY|O_CREAT) %d errno %d\n", fd, errno);
  if (fd >= 0) close(fd);
  printf("RESULT stat(\"subdir\") %d dir %d\n", stat("subdir", &st), stat("subdir", &st) == 0 && S_ISDIR(st.st_mode));
  printf("RESULT stat(\"\") %d errno %d\n", stat("", &st), errno);
  return 0;
}
