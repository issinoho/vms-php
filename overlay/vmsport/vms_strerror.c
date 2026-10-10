/* vms_strerror.c - strerror() and strerror_r() with glibc's wording.

   The C RTL's messages are worded differently ("no such file or directory",
   "can't assign requested address ") from glibc's, which PHP's tests and
   anything that matches error text expect ("No such file or directory").
   vms_php_compat.h maps both functions here (PORTING_LOG #28).  The table
   was generated from probes/r_strerror.c's output on the node: every errno
   the C RTL defines whose text differs from glibc's, with glibc's text.  Any
   other errno, among them EVMSERR and the VMS-only codes, keeps the C RTL's
   message. */
#include <errno.h>
#include <string.h>

#undef strerror
#undef strerror_r

static const struct { int e; const char *text; } vms_php_errtext[] = {
#ifdef E2BIG
  { E2BIG, "Argument list too long" },
#endif
#ifdef EACCES
  { EACCES, "Permission denied" },
#endif
#ifdef EADDRINUSE
  { EADDRINUSE, "Address already in use" },
#endif
#ifdef EADDRNOTAVAIL
  { EADDRNOTAVAIL, "Cannot assign requested address" },
#endif
#ifdef EAFNOSUPPORT
  { EAFNOSUPPORT, "Address family not supported by protocol" },
#endif
#ifdef EAGAIN
  { EAGAIN, "Resource temporarily unavailable" },
#endif
#ifdef EALREADY
  { EALREADY, "Operation already in progress" },
#endif
#ifdef EBADF
  { EBADF, "Bad file descriptor" },
#endif
#ifdef EBADMSG
  { EBADMSG, "Bad message" },
#endif
#ifdef EBUSY
  { EBUSY, "Device or resource busy" },
#endif
#ifdef ECANCELED
  { ECANCELED, "Operation canceled" },
#endif
#ifdef ECHILD
  { ECHILD, "No child processes" },
#endif
#ifdef ECONNABORTED
  { ECONNABORTED, "Software caused connection abort" },
#endif
#ifdef ECONNREFUSED
  { ECONNREFUSED, "Connection refused" },
#endif
#ifdef ECONNRESET
  { ECONNRESET, "Connection reset by peer" },
#endif
#ifdef EDESTADDRREQ
  { EDESTADDRREQ, "Destination address required" },
#endif
#ifdef EDOM
  { EDOM, "Numerical argument out of domain" },
#endif
#ifdef EDQUOT
  { EDQUOT, "Disk quota exceeded" },
#endif
#ifdef EEXIST
  { EEXIST, "File exists" },
#endif
#ifdef EFAULT
  { EFAULT, "Bad address" },
#endif
#ifdef EFBIG
  { EFBIG, "File too large" },
#endif
#ifdef EHOSTDOWN
  { EHOSTDOWN, "Host is down" },
#endif
#ifdef EHOSTUNREACH
  { EHOSTUNREACH, "No route to host" },
#endif
#ifdef EIDRM
  { EIDRM, "Identifier removed" },
#endif
#ifdef EILSEQ
  { EILSEQ, "Invalid or incomplete multibyte or wide character" },
#endif
#ifdef EINPROGRESS
  { EINPROGRESS, "Operation now in progress" },
#endif
#ifdef EINTR
  { EINTR, "Interrupted system call" },
#endif
#ifdef EINVAL
  { EINVAL, "Invalid argument" },
#endif
#ifdef EIO
  { EIO, "Input/output error" },
#endif
#ifdef EISCONN
  { EISCONN, "Transport endpoint is already connected" },
#endif
#ifdef EISDIR
  { EISDIR, "Is a directory" },
#endif
#ifdef ELOOP
  { ELOOP, "Too many levels of symbolic links" },
#endif
#ifdef EMFILE
  { EMFILE, "Too many open files" },
#endif
#ifdef EMLINK
  { EMLINK, "Too many links" },
#endif
#ifdef EMSGSIZE
  { EMSGSIZE, "Message too long" },
#endif
#ifdef ENAMETOOLONG
  { ENAMETOOLONG, "File name too long" },
#endif
#ifdef ENETDOWN
  { ENETDOWN, "Network is down" },
#endif
#ifdef ENETRESET
  { ENETRESET, "Network dropped connection on reset" },
#endif
#ifdef ENETUNREACH
  { ENETUNREACH, "Network is unreachable" },
#endif
#ifdef ENFILE
  { ENFILE, "Too many open files in system" },
#endif
#ifdef ENOBUFS
  { ENOBUFS, "No buffer space available" },
#endif
#ifdef ENODEV
  { ENODEV, "No such device" },
#endif
#ifdef ENOENT
  { ENOENT, "No such file or directory" },
#endif
#ifdef ENOEXEC
  { ENOEXEC, "Exec format error" },
#endif
#ifdef ENOLCK
  { ENOLCK, "No locks available" },
#endif
#ifdef ENOMEM
  { ENOMEM, "Cannot allocate memory" },
#endif
#ifdef ENOMSG
  { ENOMSG, "No message of desired type" },
#endif
#ifdef ENOPROTOOPT
  { ENOPROTOOPT, "Protocol not available" },
#endif
#ifdef ENOSPC
  { ENOSPC, "No space left on device" },
#endif
#ifdef ENOSYS
  { ENOSYS, "Function not implemented" },
#endif
#ifdef ENOTBLK
  { ENOTBLK, "Block device required" },
#endif
#ifdef ENOTCONN
  { ENOTCONN, "Transport endpoint is not connected" },
#endif
#ifdef ENOTDIR
  { ENOTDIR, "Not a directory" },
#endif
#ifdef ENOTEMPTY
  { ENOTEMPTY, "Directory not empty" },
#endif
#ifdef ENOTSOCK
  { ENOTSOCK, "Socket operation on non-socket" },
#endif
#ifdef ENOTSUP
  { ENOTSUP, "Operation not supported" },
#endif
#ifdef ENOTTY
  { ENOTTY, "Inappropriate ioctl for device" },
#endif
#ifdef ENXIO
  { ENXIO, "No such device or address" },
#endif
#ifdef EOVERFLOW
  { EOVERFLOW, "Value too large for defined data type" },
#endif
#ifdef EPERM
  { EPERM, "Operation not permitted" },
#endif
#ifdef EPFNOSUPPORT
  { EPFNOSUPPORT, "Protocol family not supported" },
#endif
#ifdef EPIPE
  { EPIPE, "Broken pipe" },
#endif
#ifdef EPROTONOSUPPORT
  { EPROTONOSUPPORT, "Protocol not supported" },
#endif
#ifdef EPROTOTYPE
  { EPROTOTYPE, "Protocol wrong type for socket" },
#endif
#ifdef ERANGE
  { ERANGE, "Numerical result out of range" },
#endif
#ifdef EREMOTE
  { EREMOTE, "Object is remote" },
#endif
#ifdef EROFS
  { EROFS, "Read-only file system" },
#endif
#ifdef ESHUTDOWN
  { ESHUTDOWN, "Cannot send after transport endpoint shutdown" },
#endif
#ifdef ESOCKTNOSUPPORT
  { ESOCKTNOSUPPORT, "Socket type not supported" },
#endif
#ifdef ESPIPE
  { ESPIPE, "Illegal seek" },
#endif
#ifdef ESRCH
  { ESRCH, "No such process" },
#endif
#ifdef ESTALE
  { ESTALE, "Stale file handle" },
#endif
#ifdef ETIMEDOUT
  { ETIMEDOUT, "Connection timed out" },
#endif
#ifdef ETOOMANYREFS
  { ETOOMANYREFS, "Too many references: cannot splice" },
#endif
#ifdef ETXTBSY
  { ETXTBSY, "Text file busy" },
#endif
#ifdef EUSERS
  { EUSERS, "Too many users" },
#endif
#ifdef EXDEV
  { EXDEV, "Invalid cross-device link" },
#endif
};

static const char *vms_php_lookup(int e)
{
  size_t i;
  for (i = 0; i < sizeof vms_php_errtext / sizeof vms_php_errtext[0]; i++)
    if (vms_php_errtext[i].e == e) return vms_php_errtext[i].text;
  return 0;
}

char *vms_php_strerror(int e)
{
  const char *t = vms_php_lookup(e);
  return t ? (char *) t : strerror(e);
}

int vms_php_strerror_r(int e, char *buf, size_t n)
{
  const char *t = vms_php_lookup(e);
  size_t len;
  if (!t) return strerror_r(e, buf, n);
  if (n == 0) return ERANGE;
  len = strlen(t);
  if (len >= n) {
    memcpy(buf, t, n - 1);
    buf[n - 1] = 0;
    return ERANGE;
  }
  memcpy(buf, t, len + 1);
  return 0;
}
