/* r_mkdir_mode.c - make d777 and d775 with mkdir() for DIRECTORY/FULL and a
   DCL RENAME: a directory made with mode 0777 cannot be rename()d (EIO), one
   made with 0775 can, though both come out (RWED,RWED,RE,) under umask 027
   (probes r_file_sem2, r_umask).  Linked with vms_crtl_init.c. */
#include <stdio.h>
#include <sys/stat.h>

int main(void)
{
  printf("RESULT mkdir d777 %d, d775 %d\n", mkdir("d777", 0777), mkdir("d775", 0775));
  return 0;
}
