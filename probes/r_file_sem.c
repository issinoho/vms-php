/* r_file_sem.c - the C RTL file semantics behind the phpt file and spl
   failures (PORTING_LOG #36-), with PHP's C RTL features (linked with
   vms_crtl_init.c).  Run in an empty scratch directory.
     1 data written with write() before close: stat() size, a second open's
       read, after fsync()
     2 lseek(fd, 10, SEEK_END) on a 512-byte file opened O_RDONLY
     3 unlink() of a file with two versions, then DECC$UNLINK_ALL_VERSIONS
     4 rename() of a directory, empty and not
     5 fcntl(F_SETLK) write lock and unlock, flock()
     6 link() and symlink()
     7 readdir(): "." and ".." */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unixlib.h>

static long size_of(const char *f) { struct stat sb; return stat(f, &sb) ? -1 : (long) sb.st_size; }

static void mkfile(const char *f, int n)
{
  char buf[600];
  int fd = open(f, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  memset(buf, 'x', sizeof buf);
  write(fd, buf, n);
  close(fd);
}

int main(void)
{
  char buf[600];
  int fd, fd2, n;
  long r;
  struct flock fl;
  DIR *d;
  struct dirent *e;

  /* 1 */
  fd = open("vis.tmp", O_WRONLY | O_CREAT | O_TRUNC, 0644);
  write(fd, "Testing ftruncate error conditions \n", 36);
  fd2 = open("vis.tmp", O_RDONLY);
  n = read(fd2, buf, sizeof buf);
  printf("RESULT 1 open: stat size %ld, second open reads %d\n", size_of("vis.tmp"), n);
  close(fd2);
  r = fsync(fd);
  fd2 = open("vis.tmp", O_RDONLY);
  n = read(fd2, buf, sizeof buf);
  printf("RESULT 1 fsync %ld: stat size %ld, second open reads %d\n", r, size_of("vis.tmp"), n);
  close(fd2);
  close(fd);
  printf("RESULT 1 closed: stat size %ld\n", size_of("vis.tmp"));

  /* 2 */
  mkfile("seek.tmp", 512);
  fd = open("seek.tmp", O_RDONLY);
  r = lseek(fd, 10, SEEK_END);
  printf("RESULT 2 lseek(10, SEEK_END) on 512 bytes, O_RDONLY: %ld (errno %d)\n", r, r < 0 ? errno : 0);
  close(fd);
  fd = open("seek.tmp", O_RDWR);
  r = lseek(fd, 10, SEEK_END);
  printf("RESULT 2 same, O_RDWR: %ld\n", r);
  close(fd);

  /* 3 */
  mkfile("ver.tmp", 10);
  mkfile("ver.tmp", 20);
  printf("RESULT 3 unlink: %d, then exists %d (size %ld)\n", unlink("ver.tmp"), access("ver.tmp", F_OK) == 0, size_of("ver.tmp"));
  unlink("ver.tmp");
  n = decc$feature_get_index("DECC$UNLINK_ALL_VERSIONS");
  printf("RESULT 3 UNLINK_ALL_VERSIONS index %d, was %d\n", n, decc$feature_get_value(n, 1));
  decc$feature_set_value(n, 1, 1);
  mkfile("ver.tmp", 10);
  mkfile("ver.tmp", 20);
  printf("RESULT 3 with it: unlink %d, then exists %d\n", unlink("ver.tmp"), access("ver.tmp", F_OK) == 0);

  /* 4 */
  mkdir("da", 0755);
  r = rename("da", "db");
  printf("RESULT 4 rename(da, db), empty: %ld errno %d (%s), db is dir %d\n", r, r ? errno : 0, r ? strerror(errno) : "", access("db", F_OK) == 0);
  rmdir("da"); rmdir("db");
  mkdir("dc", 0755);
  mkfile("dc/f.tmp", 5);
  r = rename("dc", "dd");
  printf("RESULT 4 rename(dc, dd), one file: %ld errno %d (%s)\n", r, r ? errno : 0, r ? strerror(errno) : "");
  r = rename("dc/", "dd/");
  printf("RESULT 4 rename(dc/, dd/): %ld errno %d\n", r, r ? errno : 0);
  unlink("dc/f.tmp"); unlink("dd/f.tmp"); rmdir("dc"); rmdir("dd");

  /* 5 */
  fd = open("lock.tmp", O_RDWR | O_CREAT, 0644);
  memset(&fl, 0, sizeof fl);
  fl.l_type = F_WRLCK; fl.l_whence = SEEK_SET;
  r = fcntl(fd, F_SETLK, &fl);
  printf("RESULT 5 fcntl F_SETLK F_WRLCK: %ld errno %d (%s)\n", r, r ? errno : 0, r ? strerror(errno) : "");
  fl.l_type = F_UNLCK;
  r = fcntl(fd, F_SETLK, &fl);
  printf("RESULT 5 fcntl F_UNLCK: %ld errno %d\n", r, r ? errno : 0);
  close(fd);

  /* 6 */
  mkfile("t.tmp", 5);
  r = link("t.tmp", "hard.tmp");
  printf("RESULT 6 link: %ld errno %d (%s)\n", r, r ? errno : 0, r ? strerror(errno) : "");
  r = symlink("t.tmp", "sym.tmp");
  printf("RESULT 6 symlink: %ld errno %d, readlink ok %d\n", r, r ? errno : 0, readlink("sym.tmp", buf, sizeof buf) > 0);
  unlink("sym.tmp"); unlink("hard.tmp");

  /* 7 */
  d = opendir(".");
  while ((e = readdir(d)))
    printf("RESULT 7 readdir: %s\n", e->d_name);
  closedir(d);
  return 0;
}
