/* r_chmod0.c - chmod(f, 0) gives mode 0750 on OpenVMS (the C RTL's default
   protection, probe r_perm_php): which call gives no access at all, and can
   the owner chmod a file it has no access to?  Run without SYSPRV, BYPASS,
   READALL, GRPPRV, in an empty scratch directory.  PORTING_LOG #42. */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

static void try(mode_t m)
{
  struct stat sb;
  int r = chmod("c.tmp", m), e = errno;
  stat("c.tmp", &sb);
  printf("RESULT chmod %05o: %d errno %d -> mode %04o, access R %d\n", (unsigned) m, r, r ? e : 0,
         (unsigned) sb.st_mode & 07777, access("c.tmp", R_OK) == 0);
  r = chmod("c.tmp", 0644);
  printf("RESULT   then chmod 0644: %d errno %d (%s)\n", r, r ? errno : 0, r ? strerror(errno) : "");
}

int main(void)
{
  close(open("c.tmp", O_WRONLY | O_CREAT, 0644));
  try(0); try(01000); try(02000); try(04000); try(0001); try(0003);
  unlink("c.tmp");
  return 0;
}
