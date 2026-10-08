/*
  vms_shims.c - C library entry points PHP references that OpenVMS lacks.

  PHP calls these from code it does not compile out on VMS; none of the paths
  is used in our configuration (CGI with PHP_FCGI_CHILDREN=0, no
  proc_open/mail/CLI server workers: DECISIONS D5), but the images must link
  (PORTING_LOG).  Each fails as POSIX says an unsupported call should.
*/
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <netinet/in.h>

/* no fork on VMS: CGI's FastCGI children, the CLI server's workers,
   proc_open(), mail(), opcache's preloading as another user */
int fork(void)
{
  errno = ENOSYS;
  return -1;
}

/* the CGI SAPI's setuid path (never taken: no root on VMS) */
int initgroups(const char *user, gid_t group)
{
  (void) user;
  (void) group;
  errno = ENOSYS;
  return -1;
}

/* strsignal(): a name for the signals the C RTL has, a number otherwise */
char *strsignal(int sig)
{
  static char buf[32];
  switch (sig) {
  case SIGHUP:  return "Hangup";
  case SIGINT:  return "Interrupt";
  case SIGQUIT: return "Quit";
  case SIGILL:  return "Illegal instruction";
  case SIGABRT: return "Aborted";
  case SIGFPE:  return "Floating point exception";
  case SIGKILL: return "Killed";
  case SIGSEGV: return "Segmentation fault";
  case SIGPIPE: return "Broken pipe";
  case SIGALRM: return "Alarm clock";
  case SIGTERM: return "Terminated";
  case SIGUSR1: return "User defined signal 1";
  case SIGUSR2: return "User defined signal 2";
  default:
    snprintf(buf, sizeof buf, "Unknown signal %d", sig);
    return buf;
  }
}

/* <netinet/in6.h> declares these, but neither DECC$SHR nor the TCP/IP
   images define them (as in vms-lighttpd's vms_in6addr.c) */
const struct in6_addr in6addr_any = { { { 0, 0, 0, 0, 0, 0, 0, 0,
                                          0, 0, 0, 0, 0, 0, 0, 0 } } };
const struct in6_addr in6addr_loopback = { { { 0, 0, 0, 0, 0, 0, 0, 0,
                                               0, 0, 0, 0, 0, 0, 0, 1 } } };

/* Boost.Context's make_fcontext calls _exit if a fiber's entry function
   returns; vms_fiber_exit.S defines _exit and jumps here (D10). */
void vms_fiber_exit_c(int status)
{
  exit(status);
}
