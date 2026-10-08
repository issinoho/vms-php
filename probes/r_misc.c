/* r_misc.c - assorted C RTL behaviour PHP depends on, with clang (LP64):
   type sizes, printf of 64-bit types, time zone data, strptime, locale,
   temporary files, popen/pclose (exec/system/shell_exec), environment,
   va_copy, getaddrinfo for MySQL's 127.0.0.1/localhost, and an entropy source
   for random_bytes (no getrandom/arc4random//dev/urandom). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <time.h>
#include <locale.h>
#include <langinfo.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <limits.h>

static int sum(int n, ...)
{
  va_list ap, ap2;
  int i, s = 0;
  va_start(ap, n);
  va_copy(ap2, ap);
  for (i = 0; i < n; i++) s += va_arg(ap2, int);
  va_end(ap2);
  va_end(ap);
  return s;
}

int main(void)
{
  char buf[256];
  struct tm tm, *ptm;
  time_t now = time(NULL);
  FILE *p;
  int fd, rc;
  struct addrinfo hints, *res;

  printf("RESULT sizes long=%d size_t=%d void*=%d long long=%d LONG_MAX=%lld\n", (int) sizeof(long),
         (int) sizeof(size_t), (int) sizeof(void *), (int) sizeof(long long), (long long) LONG_MAX);
  snprintf(buf, sizeof buf, "%zu %lld %ld %llx", (size_t) 1 << 40, (long long) 1 << 40, (long) 1 << 40,
           0x123456789abcULL);
  printf("RESULT printf64 \"%s\" (want \"1099511627776 1099511627776 1099511627776 123456789abc\")\n", buf);
  printf("RESULT strtoll %lld strtol %ld\n", strtoll("9000000000", NULL, 10), strtol("9000000000", NULL, 10));
  printf("RESULT va_copy sum=%d\n", sum(3, 1, 2, 3));

  tzset();
  ptm = localtime_r(&now, &tm);
  strftime(buf, sizeof buf, "%Y-%m-%d %H:%M:%S %Z %z", &tm);
  printf("RESULT localtime %s TZ=%s\n", ptm ? buf : "fail", getenv("TZ") ? getenv("TZ") : "(unset)");
  memset(&tm, 0, sizeof tm);
  printf("RESULT strptime %s\n", strptime("2026-10-08 12:34:56", "%Y-%m-%d %H:%M:%S", &tm) ? "ok" : "fail");

  printf("RESULT setlocale C=%s \"\"=%s codeset=%s\n", setlocale(LC_ALL, "C") ? "ok" : "fail",
         setlocale(LC_ALL, "") ? "ok" : "fail", nl_langinfo(CODESET));
  setlocale(LC_ALL, "C");

  strcpy(buf, "/sys$scratch/phpXXXXXX");
  errno = 0;
  fd = mkstemp(buf);
  printf("RESULT mkstemp %s fd=%d errno=%d\n", buf, fd, errno);
  if (fd >= 0) { close(fd); unlink(buf); }

  errno = 0;
  p = popen("write sys$output \"popen-ok\"", "r");
  if (p) {
    buf[0] = 0;
    fgets(buf, sizeof buf, p);
    rc = pclose(p);
    buf[strcspn(buf, "\r\n")] = 0;
    printf("RESULT popen read \"%s\" pclose=%d\n", buf, rc);
  } else
    printf("RESULT popen fail errno=%d\n", errno);
  printf("RESULT system rc=%d\n", system("exit 1"));

  printf("RESULT setenv rc=%d getenv=%s\n", setenv("PHP_PROBE", "x1", 1),
         getenv("PHP_PROBE") ? getenv("PHP_PROBE") : "(null)");

  memset(&hints, 0, sizeof hints);
  hints.ai_socktype = SOCK_STREAM;
  rc = getaddrinfo("localhost", "3306", &hints, &res);
  printf("RESULT getaddrinfo localhost rc=%d family=%d\n", rc, rc ? -1 : res->ai_family);
  if (!rc) freeaddrinfo(res);
  rc = getaddrinfo("127.0.0.1", "3306", &hints, &res);
  printf("RESULT getaddrinfo 127.0.0.1 rc=%d\n", rc);
  if (!rc) freeaddrinfo(res);

  errno = 0;
  fd = open("/dev/urandom", O_RDONLY);
  printf("RESULT /dev/urandom fd=%d errno=%d\n", fd, errno);
  return 0;
}
