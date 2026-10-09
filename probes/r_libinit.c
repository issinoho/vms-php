/* r_libinit.c - does vmsport/vms_crtl_init.c's LIB$INITIALIZE routine run and
   set the C RTL features?  Linked with VMS_CRTL_INIT.OBJ from the PHP build:
   PHP.EXE reported getcwd() in VMS syntax, so DECC$FILENAME_UNIX_REPORT was
   not in effect. */
#include <stdio.h>
#include <unistd.h>
#include <unixlib.h>

int main(void)
{
  char buf[256];
  int idx = decc$feature_get_index("DECC$FILENAME_UNIX_REPORT");
  printf("RESULT feature index %d value %d\n", idx, idx >= 0 ? decc$feature_get_value(idx, 1) : -1);
  printf("RESULT getcwd %s\n", getcwd(buf, sizeof buf));
  return 0;
}
