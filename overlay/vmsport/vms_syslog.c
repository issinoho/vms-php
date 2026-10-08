/*
  vms_syslog.c - openlog/syslog/closelog for PHP on OpenVMS, which has no
  syslog (vmsport/include/compat/syslog.h).  Each message goes to stderr as
  one line, "<ident>[pid]: <message>", if its priority is in the mask; in the
  vms-lighttpd PHP pool stderr is the process's log file.
*/
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include "syslog.h"

static char vms_syslog_ident[64] = "php";
static int vms_syslog_option;
static int vms_syslog_mask = 0xff;

void openlog(const char *ident, int option, int facility)
{
  (void) facility;
  if (ident) {
    strncpy(vms_syslog_ident, ident, sizeof vms_syslog_ident - 1);
    vms_syslog_ident[sizeof vms_syslog_ident - 1] = 0;
  }
  vms_syslog_option = option;
}

void vsyslog(int priority, const char *format, va_list ap)
{
  char msg[2048];
  if (!(vms_syslog_mask & LOG_MASK(LOG_PRI(priority))))
    return;
  vsnprintf(msg, sizeof msg, format, ap);
  if (vms_syslog_option & LOG_PID)
    fprintf(stderr, "%s[%d]: %s\n", vms_syslog_ident, (int) getpid(), msg);
  else
    fprintf(stderr, "%s: %s\n", vms_syslog_ident, msg);
  fflush(stderr);
}

void syslog(int priority, const char *format, ...)
{
  va_list ap;
  va_start(ap, format);
  vsyslog(priority, format, ap);
  va_end(ap);
}

void closelog(void)
{
}

int setlogmask(int mask)
{
  int old = vms_syslog_mask;
  if (mask)
    vms_syslog_mask = mask;
  return old;
}
