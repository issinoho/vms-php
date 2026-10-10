/* r_umask.c - the C RTL's umask, and the protection of what PHP creates with
   its default modes (file 0666, directory 0777); a directory made with 0777
   cannot be renamed (EIO, probe r_file_sem2).  Linked with vms_crtl_init.c;
   run in an empty scratch directory, then DIRECTORY/PROTECTION. */
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

int main(void)
{
  mode_t m = umask(0);
  struct stat sb;
  umask(m);
  printf("RESULT umask %03o\n", (unsigned) m);
  close(open("f666.tmp", O_WRONLY | O_CREAT, 0666));
  mkdir("d777", 0777);
  stat("f666.tmp", &sb);
  printf("RESULT file 0666 -> mode %03o\n", (unsigned) sb.st_mode & 0777);
  stat("d777", &sb);
  printf("RESULT dir 0777 -> mode %03o\n", (unsigned) sb.st_mode & 0777);
  return 0;
}
