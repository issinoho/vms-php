/* r_ctor.c - can a clang C program set C RTL features before main() with
   __attribute__((constructor))?  The LIB$INITIALIZE psect entry used so far
   (vms-mariadb's vms_crtl_init.c pattern) is linked but never called on
   x86-64 (probe r_libinit: feature value 0, getcwd in VMS syntax). */
#include <stdio.h>
#include <unistd.h>
#include <unixlib.h>

static int ran;

__attribute__((constructor)) static void set_features(void)
{
  int idx = decc$feature_get_index("DECC$FILENAME_UNIX_REPORT");
  if (idx >= 0) decc$feature_set_value(idx, 1, 1);
  ran = 1;
}

int main(void)
{
  char buf[256];
  int idx = decc$feature_get_index("DECC$FILENAME_UNIX_REPORT");
  printf("RESULT constructor ran %d, feature value %d\n", ran, decc$feature_get_value(idx, 1));
  printf("RESULT getcwd %s\n", getcwd(buf, sizeof buf));
  return 0;
}
