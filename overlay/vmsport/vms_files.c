/* vms_files.c - mkdir() and unlink() with POSIX results on OpenVMS.

   mkdir(): given mode 0777, the C RTL gives the directory the default
   protection that CREATE/DIRECTORY would, without DELETE access, so it cannot
   be renamed (rename() fails with EIO, RENAME with NOPRIV; probes
   r_file_sem2, r_mkdir_mode).  PHP's mkdir() and phpBB pass 0777.  POSIX's
   result is 0777 less the umask, so that is the mode passed (PORTING_LOG #36).

   unlink(): the C RTL deletes only the newest version, so a file PHP wrote
   more than once still exists after unlink() (probe r_file_sem; there is no
   DECC$UNLINK_ALL_VERSIONS in this C RTL).  POSIX removes the name: delete
   versions until none is left (probe r_versions; PORTING_LOG #37).

   rename(): onto an existing file, the C RTL gives the target a new version
   and keeps the old one (probe r_rename_over), so a file replaced by
   rename() gains a version each time.  POSIX replaces it: remove the
   target's versions first, then rename (PORTING_LOG #40).  The target is
   briefly absent, where POSIX's rename() is atomic.

   vms_php_compat.h maps all three here. */
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#undef mkdir
#undef unlink
#undef rename

int vms_php_mkdir(const char *path, mode_t mode, ...)
{
  if ((mode & 07777) == 0777)
    {
      mode_t mask = umask(0);
      umask(mask);
      mode = (mode & ~07777) | (0777 & ~mask);
    }
  return mkdir(path, mode);
}

int vms_php_unlink(const char *path)
{
  int r = unlink(path), i;

  /* the older versions; the C RTL allows 32767 */
  for (i = 0; r == 0 && i < 32767 && access(path, F_OK) == 0; i++)
    if (unlink(path) != 0)
      break;
  return r;
}

int vms_php_rename(const char *from, const char *to)
{
  struct stat sf, st;

  if (stat(from, &sf) == 0 && !S_ISDIR(sf.st_mode) && stat(to, &st) == 0
      && !S_ISDIR(st.st_mode) && !(sf.st_dev == st.st_dev && sf.st_ino == st.st_ino))
    vms_php_unlink(to);
  return rename(from, to);
}
