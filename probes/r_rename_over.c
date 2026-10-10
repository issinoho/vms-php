/* r_rename_over.c - rename() onto an existing file: replaced, as POSIX
   says, or does the target gain a version (PORTING_LOG #40)?  Linked with
   vms_crtl_init.c; run in an empty scratch directory, then DIRECTORY. */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

static void put(const char *f, const char *s)
{
  int fd = open(f, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  write(fd, s, strlen(s));
  close(fd);
}

int main(void)
{
  char buf[32] = "";
  int fd, n, r;
  put("target.tmp", "old");
  put("new.tmp", "new");
  r = rename("new.tmp", "target.tmp");
  fd = open("target.tmp", O_RDONLY);
  n = read(fd, buf, sizeof buf - 1);
  close(fd);
  if (n > 0) buf[n] = 0;
  printf("RESULT rename onto existing: %d errno %d, target reads \"%s\", new.tmp exists %d\n",
         r, r ? errno : 0, buf, access("new.tmp", F_OK) == 0);
  return 0;
}
