/*
  vms_crtl_init.c - C RTL features for the PHP images on OpenVMS.

  Linked into PHP.EXE and PHP_CGI.EXE (vmsport/BUILD.COM).  A constructor
  sets these DECC$ features before main() runs, unless the user has defined
  the feature's logical name, which then wins.  They are set inside the
  images rather than as process logical names because they change how every
  C RTL program in the process behaves (vms-mariadb D6).

  Not a LIB$INITIALIZE psect entry (vms-mariadb's pattern): with VSI clang on
  x86-64 such an entry is linked but never called (probes r_libinit, r_ctor;
  PORTING_LOG #22).  clang puts constructors in .init_array, which runs.
*/
#include <stdlib.h>
#include <unixlib.h>

static const struct
{
  const char *name;
  int value;
} vms_features[] =
{
  /* PHP and its scripts use UNIX file names of any shape: dots, mixed case */
  { "DECC$EFS_CHARSET", 1 },
  { "DECC$EFS_CASE_PRESERVE", 1 },
  /* report UNIX names (readdir, getcwd, __FILE__, realpath) without ;version */
  { "DECC$FILENAME_UNIX_REPORT", 1 },
  { "DECC$FILENAME_UNIX_NO_VERSION", 1 },
  { "DECC$READDIR_DROPDOTNOTYPE", 1 },
  /* open a file more than once (sessions, caches, includes) and remove files
     another descriptor still has open, as PHP code expects */
  { "DECC$FILE_SHARING", 1 },
  { "DECC$ALLOW_REMOVE_OPEN_FILES", 1 },
  { "DECC$POSIX_SEEK_STREAM_FILE", 1 },
  { "DECC$RENAME_NO_INHERIT", 1 },
  /* keep the case of command-line arguments under SET PROCESS/PARSE=EXTENDED */
  { "DECC$ARGV_PARSE_STYLE", 1 },
  { NULL, 0 }
};

__attribute__((constructor)) static void vms_crtl_init(void)
{
  int i, idx;
  for (i = 0; vms_features[i].name; i++)
  {
    if (getenv(vms_features[i].name))
      continue;                         /* the user's logical name wins */
    if ((idx = decc$feature_get_index(vms_features[i].name)) >= 0)
      decc$feature_set_value(idx, 1, vms_features[i].value);
  }
}
