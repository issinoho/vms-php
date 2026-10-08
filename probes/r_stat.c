/* r_stat.c - stat() as PHP's realpath cache, opcache (validate_timestamps) and
   file_exists() use it: st_ino/st_dev stable across stat/fstat/lstat and a
   rename, errno for missing paths, "." and ".." components, many dots,
   directories, and paths of the shape phpBB uses.  The third-party PHP 8.1
   kit crashed (ACCVIO) inside the C RTL's stat/file lookup under phpBB. */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

static void st(const char *path)
{
  struct stat s;
  int rc;
  errno = 0;
  rc = stat(path, &s);
  if (rc)
    printf("RESULT stat \"%s\" rc=%d errno=%d %s\n", path, rc, errno, strerror(errno));
  else
    printf("RESULT stat \"%s\" ok ino=%llu dev=%llu mode=%o size=%lld\n", path,
           (unsigned long long) s.st_ino, (unsigned long long) s.st_dev, (unsigned) s.st_mode,
           (long long) s.st_size);
}

int main(void)
{
  struct stat a, b, c;
  int fd;
  char cwd[512];

  printf("RESULT sizeof st_ino=%d st_dev=%d off_t=%d time_t=%d\n", (int) sizeof a.st_ino,
         (int) sizeof a.st_dev, (int) sizeof(off_t), (int) sizeof(time_t));
  mkdir("ioprobe", 0755);
  fd = open("ioprobe/a.php", O_CREAT | O_WRONLY | O_TRUNC, 0644);
  write(fd, "<?php echo 1;\n", 14);
  fstat(fd, &b);
  close(fd);
  stat("ioprobe/a.php", &a);
  lstat("ioprobe/a.php", &c);
  printf("RESULT ino stat=%llu fstat=%llu lstat=%llu same=%d\n", (unsigned long long) a.st_ino,
         (unsigned long long) b.st_ino, (unsigned long long) c.st_ino,
         a.st_ino == b.st_ino && a.st_ino == c.st_ino && a.st_dev == b.st_dev);
  rename("ioprobe/a.php", "ioprobe/b.php");
  stat("ioprobe/b.php", &b);
  printf("RESULT ino after rename same=%d\n", a.st_ino == b.st_ino);
  getcwd(cwd, sizeof cwd);
  printf("RESULT getcwd %s\n", cwd);
  st("ioprobe/b.php");
  st("ioprobe/a.php");
  st("ioprobe");
  st("ioprobe/");
  st("ioprobe/./b.php");
  st("ioprobe/../ioprobe/b.php");
  st("ioprobe/missing/deeper/x.php");
  st("ioprobe/b.php/x");
  st("ioprobe/a.b.c.d.php");
  st("ioprobe/vendor/symfony/polyfill-mbstring/bootstrap80.php");
  st(".");
  st("..");
  st("/sys$scratch");
  st("/SYS$SYSDEVICE/000000");
  st("");
  return 0;
}
