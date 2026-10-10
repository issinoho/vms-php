/* r_versions.c - file versions under PHP's C RTL features (PORTING_LOG #36-).
   unlink() removes only the newest version, and DECC$UNLINK_ALL_VERSIONS does
   not exist here (probe r_file_sem); open(O_TRUNC) on an existing file makes a
   new version, so a file PHP rewrites gains one each time.  Checks:
     a  open(O_CREAT | O_TRUNC) twice: how many versions
     b  open(O_CREAT) without O_TRUNC on an existing file, then ftruncate(0):
        same version?  content right?  a reader holding the file open
     c  unlink() repeated while the name exists: all versions gone
     d  rmdir() of a directory made with mode 0777 (no DELETE: r_mkdir_mode)
   Linked with vms_crtl_init.c; run in an empty scratch directory, then
   DIRECTORY. */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

static void put(const char *f, const char *s, int flags)
{
  int fd = open(f, O_WRONLY | O_CREAT | flags, 0644);
  if (!(flags & O_TRUNC)) ftruncate(fd, 0);
  write(fd, s, strlen(s));
  close(fd);
}

static void show(const char *f)
{
  char buf[64] = "";
  int fd = open(f, O_RDONLY), n = fd < 0 ? -1 : read(fd, buf, sizeof buf - 1);
  if (n > 0) buf[n] = 0;
  printf("RESULT   %s reads %d: \"%s\"\n", f, n, n > 0 ? buf : "");
  if (fd >= 0) close(fd);
}

int main(void)
{
  int i, r, rd;
  char buf[64];

  put("a.tmp", "first", O_TRUNC);
  put("a.tmp", "second", O_TRUNC);
  printf("RESULT a: written twice with O_TRUNC\n");
  show("a.tmp");

  put("b.tmp", "first long content", O_TRUNC);
  rd = open("b.tmp", O_RDONLY);
  put("b.tmp", "second", 0);
  printf("RESULT b: rewritten without O_TRUNC, then ftruncate(0)\n");
  show("b.tmp");
  r = read(rd, buf, sizeof buf - 1);
  printf("RESULT b: reader opened before reads %d\n", r);
  close(rd);
  put("b.tmp", "x", 0);
  show("b.tmp");

  put("c.tmp", "1", O_TRUNC);
  put("c.tmp", "2", O_TRUNC);
  put("c.tmp", "3", O_TRUNC);
  for (i = 0; i < 100 && access("c.tmp", F_OK) == 0; i++)
    unlink("c.tmp");
  printf("RESULT c: %d unlink() calls, exists %d\n", i, access("c.tmp", F_OK) == 0);

  mkdir("d777", 0777);
  r = rmdir("d777");
  printf("RESULT d: rmdir of a 0777 directory %d errno %d (%s)\n", r, r ? errno : 0, r ? strerror(errno) : "");
  return 0;
}
