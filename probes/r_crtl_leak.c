/* r_crtl_leak: page-file quota used by 20000 calls of C RTL functions PHP's
   request cycle (with opcache) calls, looking for the ~460 bytes per
   request leak seen in Phase 3. Prints pagelets used per function. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <jpidef.h>
#include <lib$routines.h>
static long left(void) { int code = JPI$_PAGFILCNT; unsigned int v = 0;
  lib$getjpi(&code, 0, 0, &v, 0, 0); return (long) v; }
static char cwd[1024];
static void f_getcwd(void) { getcwd(cwd, sizeof cwd); }
static void f_stat(void) { struct stat s; stat("r_crtl_leak.c", &s); }
static void f_stat_abs(void) { struct stat s; char p[1100]; snprintf(p, sizeof p, "%s/r_crtl_leak.c", cwd); stat(p, &s); }
static void f_lstat_abs(void) { struct stat s; char p[1100]; snprintf(p, sizeof p, "%s/r_crtl_leak.c", cwd); lstat(p, &s); }
static void f_open(void) { struct stat s; int fd = open("r_crtl_leak.c", O_RDONLY); fstat(fd, &s); close(fd); }
static void f_fopen(void) { char b[64]; FILE *f = fopen("r_crtl_leak.c", "r"); fread(b, 1, sizeof b, f); fclose(f); }
static void f_chdir(void) { chdir(cwd); }
static void f_localtime(void) { time_t t = time(NULL); struct tm tm; localtime_r(&t, &tm); }
static void f_getenv(void) { getenv("PHP_FCGI_CHILDREN"); getenv("TMPDIR"); }
static void f_access(void) { access("r_crtl_leak.c", R_OK); }
static struct { const char *n; void (*f)(void); } t[] = {
  {"getcwd", f_getcwd}, {"stat rel", f_stat}, {"stat abs", f_stat_abs},
  {"lstat abs", f_lstat_abs}, {"open/fstat/close", f_open}, {"fopen/fread", f_fopen},
  {"chdir", f_chdir}, {"localtime_r", f_localtime}, {"getenv", f_getenv},
  {"access", f_access}};
int main(void) {
  int i, k;
  getcwd(cwd, sizeof cwd);
  printf("cwd %s\n", cwd);
  for (k = 0; k < (int) (sizeof t / sizeof t[0]); k++) {
    long a;
    t[k].f();                     /* first call: one-time setup */
    a = left();
    for (i = 0; i < 20000; i++) t[k].f();
    printf("%-18s %6ld pagelets per 20000 calls\n", t[k].n, a - left());
  }
  return 0;
}
