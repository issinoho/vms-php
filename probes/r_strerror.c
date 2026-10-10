/* r_strerror.c - every errno name glibc knows: is it defined on VMS, its
   value, and the C RTL's strerror() text (for the glibc wording table in
   vmsport/vms_strerror.c, PORTING_LOG #28).  Also strerror_r() for one. */
#include <stdio.h>
#include <string.h>
#include <errno.h>

int main(void)
{
#ifdef E2BIG
  printf("RESULT E2BIG %d %s\n", E2BIG, strerror(E2BIG));
#endif
#ifdef EACCES
  printf("RESULT EACCES %d %s\n", EACCES, strerror(EACCES));
#endif
#ifdef EADDRINUSE
  printf("RESULT EADDRINUSE %d %s\n", EADDRINUSE, strerror(EADDRINUSE));
#endif
#ifdef EADDRNOTAVAIL
  printf("RESULT EADDRNOTAVAIL %d %s\n", EADDRNOTAVAIL, strerror(EADDRNOTAVAIL));
#endif
#ifdef EADV
  printf("RESULT EADV %d %s\n", EADV, strerror(EADV));
#endif
#ifdef EAFNOSUPPORT
  printf("RESULT EAFNOSUPPORT %d %s\n", EAFNOSUPPORT, strerror(EAFNOSUPPORT));
#endif
#ifdef EAGAIN
  printf("RESULT EAGAIN %d %s\n", EAGAIN, strerror(EAGAIN));
#endif
#ifdef EALREADY
  printf("RESULT EALREADY %d %s\n", EALREADY, strerror(EALREADY));
#endif
#ifdef EBADE
  printf("RESULT EBADE %d %s\n", EBADE, strerror(EBADE));
#endif
#ifdef EBADF
  printf("RESULT EBADF %d %s\n", EBADF, strerror(EBADF));
#endif
#ifdef EBADFD
  printf("RESULT EBADFD %d %s\n", EBADFD, strerror(EBADFD));
#endif
#ifdef EBADMSG
  printf("RESULT EBADMSG %d %s\n", EBADMSG, strerror(EBADMSG));
#endif
#ifdef EBADR
  printf("RESULT EBADR %d %s\n", EBADR, strerror(EBADR));
#endif
#ifdef EBADRQC
  printf("RESULT EBADRQC %d %s\n", EBADRQC, strerror(EBADRQC));
#endif
#ifdef EBADSLT
  printf("RESULT EBADSLT %d %s\n", EBADSLT, strerror(EBADSLT));
#endif
#ifdef EBFONT
  printf("RESULT EBFONT %d %s\n", EBFONT, strerror(EBFONT));
#endif
#ifdef EBUSY
  printf("RESULT EBUSY %d %s\n", EBUSY, strerror(EBUSY));
#endif
#ifdef ECANCELED
  printf("RESULT ECANCELED %d %s\n", ECANCELED, strerror(ECANCELED));
#endif
#ifdef ECHILD
  printf("RESULT ECHILD %d %s\n", ECHILD, strerror(ECHILD));
#endif
#ifdef ECHRNG
  printf("RESULT ECHRNG %d %s\n", ECHRNG, strerror(ECHRNG));
#endif
#ifdef ECOMM
  printf("RESULT ECOMM %d %s\n", ECOMM, strerror(ECOMM));
#endif
#ifdef ECONNABORTED
  printf("RESULT ECONNABORTED %d %s\n", ECONNABORTED, strerror(ECONNABORTED));
#endif
#ifdef ECONNREFUSED
  printf("RESULT ECONNREFUSED %d %s\n", ECONNREFUSED, strerror(ECONNREFUSED));
#endif
#ifdef ECONNRESET
  printf("RESULT ECONNRESET %d %s\n", ECONNRESET, strerror(ECONNRESET));
#endif
#ifdef EDEADLOCK
  printf("RESULT EDEADLOCK %d %s\n", EDEADLOCK, strerror(EDEADLOCK));
#endif
#ifdef EDESTADDRREQ
  printf("RESULT EDESTADDRREQ %d %s\n", EDESTADDRREQ, strerror(EDESTADDRREQ));
#endif
#ifdef EDOM
  printf("RESULT EDOM %d %s\n", EDOM, strerror(EDOM));
#endif
#ifdef EDOTDOT
  printf("RESULT EDOTDOT %d %s\n", EDOTDOT, strerror(EDOTDOT));
#endif
#ifdef EDQUOT
  printf("RESULT EDQUOT %d %s\n", EDQUOT, strerror(EDQUOT));
#endif
#ifdef EEXIST
  printf("RESULT EEXIST %d %s\n", EEXIST, strerror(EEXIST));
#endif
#ifdef EFAULT
  printf("RESULT EFAULT %d %s\n", EFAULT, strerror(EFAULT));
#endif
#ifdef EFBIG
  printf("RESULT EFBIG %d %s\n", EFBIG, strerror(EFBIG));
#endif
#ifdef EHOSTDOWN
  printf("RESULT EHOSTDOWN %d %s\n", EHOSTDOWN, strerror(EHOSTDOWN));
#endif
#ifdef EHOSTUNREACH
  printf("RESULT EHOSTUNREACH %d %s\n", EHOSTUNREACH, strerror(EHOSTUNREACH));
#endif
#ifdef EHWPOISON
  printf("RESULT EHWPOISON %d %s\n", EHWPOISON, strerror(EHWPOISON));
#endif
#ifdef EIDRM
  printf("RESULT EIDRM %d %s\n", EIDRM, strerror(EIDRM));
#endif
#ifdef EILSEQ
  printf("RESULT EILSEQ %d %s\n", EILSEQ, strerror(EILSEQ));
#endif
#ifdef EINPROGRESS
  printf("RESULT EINPROGRESS %d %s\n", EINPROGRESS, strerror(EINPROGRESS));
#endif
#ifdef EINTR
  printf("RESULT EINTR %d %s\n", EINTR, strerror(EINTR));
#endif
#ifdef EINVAL
  printf("RESULT EINVAL %d %s\n", EINVAL, strerror(EINVAL));
#endif
#ifdef EIO
  printf("RESULT EIO %d %s\n", EIO, strerror(EIO));
#endif
#ifdef EISCONN
  printf("RESULT EISCONN %d %s\n", EISCONN, strerror(EISCONN));
#endif
#ifdef EISDIR
  printf("RESULT EISDIR %d %s\n", EISDIR, strerror(EISDIR));
#endif
#ifdef EISNAM
  printf("RESULT EISNAM %d %s\n", EISNAM, strerror(EISNAM));
#endif
#ifdef EKEYEXPIRED
  printf("RESULT EKEYEXPIRED %d %s\n", EKEYEXPIRED, strerror(EKEYEXPIRED));
#endif
#ifdef EKEYREJECTED
  printf("RESULT EKEYREJECTED %d %s\n", EKEYREJECTED, strerror(EKEYREJECTED));
#endif
#ifdef EKEYREVOKED
  printf("RESULT EKEYREVOKED %d %s\n", EKEYREVOKED, strerror(EKEYREVOKED));
#endif
#ifdef EL2HLT
  printf("RESULT EL2HLT %d %s\n", EL2HLT, strerror(EL2HLT));
#endif
#ifdef EL2NSYNC
  printf("RESULT EL2NSYNC %d %s\n", EL2NSYNC, strerror(EL2NSYNC));
#endif
#ifdef EL3HLT
  printf("RESULT EL3HLT %d %s\n", EL3HLT, strerror(EL3HLT));
#endif
#ifdef EL3RST
  printf("RESULT EL3RST %d %s\n", EL3RST, strerror(EL3RST));
#endif
#ifdef ELIBACC
  printf("RESULT ELIBACC %d %s\n", ELIBACC, strerror(ELIBACC));
#endif
#ifdef ELIBBAD
  printf("RESULT ELIBBAD %d %s\n", ELIBBAD, strerror(ELIBBAD));
#endif
#ifdef ELIBEXEC
  printf("RESULT ELIBEXEC %d %s\n", ELIBEXEC, strerror(ELIBEXEC));
#endif
#ifdef ELIBMAX
  printf("RESULT ELIBMAX %d %s\n", ELIBMAX, strerror(ELIBMAX));
#endif
#ifdef ELIBSCN
  printf("RESULT ELIBSCN %d %s\n", ELIBSCN, strerror(ELIBSCN));
#endif
#ifdef ELNRNG
  printf("RESULT ELNRNG %d %s\n", ELNRNG, strerror(ELNRNG));
#endif
#ifdef ELOOP
  printf("RESULT ELOOP %d %s\n", ELOOP, strerror(ELOOP));
#endif
#ifdef EMEDIUMTYPE
  printf("RESULT EMEDIUMTYPE %d %s\n", EMEDIUMTYPE, strerror(EMEDIUMTYPE));
#endif
#ifdef EMFILE
  printf("RESULT EMFILE %d %s\n", EMFILE, strerror(EMFILE));
#endif
#ifdef EMLINK
  printf("RESULT EMLINK %d %s\n", EMLINK, strerror(EMLINK));
#endif
#ifdef EMSGSIZE
  printf("RESULT EMSGSIZE %d %s\n", EMSGSIZE, strerror(EMSGSIZE));
#endif
#ifdef EMULTIHOP
  printf("RESULT EMULTIHOP %d %s\n", EMULTIHOP, strerror(EMULTIHOP));
#endif
#ifdef ENAMETOOLONG
  printf("RESULT ENAMETOOLONG %d %s\n", ENAMETOOLONG, strerror(ENAMETOOLONG));
#endif
#ifdef ENAVAIL
  printf("RESULT ENAVAIL %d %s\n", ENAVAIL, strerror(ENAVAIL));
#endif
#ifdef ENETDOWN
  printf("RESULT ENETDOWN %d %s\n", ENETDOWN, strerror(ENETDOWN));
#endif
#ifdef ENETRESET
  printf("RESULT ENETRESET %d %s\n", ENETRESET, strerror(ENETRESET));
#endif
#ifdef ENETUNREACH
  printf("RESULT ENETUNREACH %d %s\n", ENETUNREACH, strerror(ENETUNREACH));
#endif
#ifdef ENFILE
  printf("RESULT ENFILE %d %s\n", ENFILE, strerror(ENFILE));
#endif
#ifdef ENOANO
  printf("RESULT ENOANO %d %s\n", ENOANO, strerror(ENOANO));
#endif
#ifdef ENOBUFS
  printf("RESULT ENOBUFS %d %s\n", ENOBUFS, strerror(ENOBUFS));
#endif
#ifdef ENOCSI
  printf("RESULT ENOCSI %d %s\n", ENOCSI, strerror(ENOCSI));
#endif
#ifdef ENODATA
  printf("RESULT ENODATA %d %s\n", ENODATA, strerror(ENODATA));
#endif
#ifdef ENODEV
  printf("RESULT ENODEV %d %s\n", ENODEV, strerror(ENODEV));
#endif
#ifdef ENOENT
  printf("RESULT ENOENT %d %s\n", ENOENT, strerror(ENOENT));
#endif
#ifdef ENOEXEC
  printf("RESULT ENOEXEC %d %s\n", ENOEXEC, strerror(ENOEXEC));
#endif
#ifdef ENOKEY
  printf("RESULT ENOKEY %d %s\n", ENOKEY, strerror(ENOKEY));
#endif
#ifdef ENOLCK
  printf("RESULT ENOLCK %d %s\n", ENOLCK, strerror(ENOLCK));
#endif
#ifdef ENOLINK
  printf("RESULT ENOLINK %d %s\n", ENOLINK, strerror(ENOLINK));
#endif
#ifdef ENOMEDIUM
  printf("RESULT ENOMEDIUM %d %s\n", ENOMEDIUM, strerror(ENOMEDIUM));
#endif
#ifdef ENOMEM
  printf("RESULT ENOMEM %d %s\n", ENOMEM, strerror(ENOMEM));
#endif
#ifdef ENOMSG
  printf("RESULT ENOMSG %d %s\n", ENOMSG, strerror(ENOMSG));
#endif
#ifdef ENONET
  printf("RESULT ENONET %d %s\n", ENONET, strerror(ENONET));
#endif
#ifdef ENOPKG
  printf("RESULT ENOPKG %d %s\n", ENOPKG, strerror(ENOPKG));
#endif
#ifdef ENOPROTOOPT
  printf("RESULT ENOPROTOOPT %d %s\n", ENOPROTOOPT, strerror(ENOPROTOOPT));
#endif
#ifdef ENOSPC
  printf("RESULT ENOSPC %d %s\n", ENOSPC, strerror(ENOSPC));
#endif
#ifdef ENOSR
  printf("RESULT ENOSR %d %s\n", ENOSR, strerror(ENOSR));
#endif
#ifdef ENOSTR
  printf("RESULT ENOSTR %d %s\n", ENOSTR, strerror(ENOSTR));
#endif
#ifdef ENOSYS
  printf("RESULT ENOSYS %d %s\n", ENOSYS, strerror(ENOSYS));
#endif
#ifdef ENOTBLK
  printf("RESULT ENOTBLK %d %s\n", ENOTBLK, strerror(ENOTBLK));
#endif
#ifdef ENOTCONN
  printf("RESULT ENOTCONN %d %s\n", ENOTCONN, strerror(ENOTCONN));
#endif
#ifdef ENOTDIR
  printf("RESULT ENOTDIR %d %s\n", ENOTDIR, strerror(ENOTDIR));
#endif
#ifdef ENOTEMPTY
  printf("RESULT ENOTEMPTY %d %s\n", ENOTEMPTY, strerror(ENOTEMPTY));
#endif
#ifdef ENOTNAM
  printf("RESULT ENOTNAM %d %s\n", ENOTNAM, strerror(ENOTNAM));
#endif
#ifdef ENOTRECOVERABLE
  printf("RESULT ENOTRECOVERABLE %d %s\n", ENOTRECOVERABLE, strerror(ENOTRECOVERABLE));
#endif
#ifdef ENOTSOCK
  printf("RESULT ENOTSOCK %d %s\n", ENOTSOCK, strerror(ENOTSOCK));
#endif
#ifdef ENOTSUP
  printf("RESULT ENOTSUP %d %s\n", ENOTSUP, strerror(ENOTSUP));
#endif
#ifdef ENOTTY
  printf("RESULT ENOTTY %d %s\n", ENOTTY, strerror(ENOTTY));
#endif
#ifdef ENOTUNIQ
  printf("RESULT ENOTUNIQ %d %s\n", ENOTUNIQ, strerror(ENOTUNIQ));
#endif
#ifdef ENXIO
  printf("RESULT ENXIO %d %s\n", ENXIO, strerror(ENXIO));
#endif
#ifdef EOVERFLOW
  printf("RESULT EOVERFLOW %d %s\n", EOVERFLOW, strerror(EOVERFLOW));
#endif
#ifdef EOWNERDEAD
  printf("RESULT EOWNERDEAD %d %s\n", EOWNERDEAD, strerror(EOWNERDEAD));
#endif
#ifdef EPERM
  printf("RESULT EPERM %d %s\n", EPERM, strerror(EPERM));
#endif
#ifdef EPFNOSUPPORT
  printf("RESULT EPFNOSUPPORT %d %s\n", EPFNOSUPPORT, strerror(EPFNOSUPPORT));
#endif
#ifdef EPIPE
  printf("RESULT EPIPE %d %s\n", EPIPE, strerror(EPIPE));
#endif
#ifdef EPROTO
  printf("RESULT EPROTO %d %s\n", EPROTO, strerror(EPROTO));
#endif
#ifdef EPROTONOSUPPORT
  printf("RESULT EPROTONOSUPPORT %d %s\n", EPROTONOSUPPORT, strerror(EPROTONOSUPPORT));
#endif
#ifdef EPROTOTYPE
  printf("RESULT EPROTOTYPE %d %s\n", EPROTOTYPE, strerror(EPROTOTYPE));
#endif
#ifdef ERANGE
  printf("RESULT ERANGE %d %s\n", ERANGE, strerror(ERANGE));
#endif
#ifdef EREMCHG
  printf("RESULT EREMCHG %d %s\n", EREMCHG, strerror(EREMCHG));
#endif
#ifdef EREMOTE
  printf("RESULT EREMOTE %d %s\n", EREMOTE, strerror(EREMOTE));
#endif
#ifdef EREMOTEIO
  printf("RESULT EREMOTEIO %d %s\n", EREMOTEIO, strerror(EREMOTEIO));
#endif
#ifdef ERESTART
  printf("RESULT ERESTART %d %s\n", ERESTART, strerror(ERESTART));
#endif
#ifdef ERFKILL
  printf("RESULT ERFKILL %d %s\n", ERFKILL, strerror(ERFKILL));
#endif
#ifdef EROFS
  printf("RESULT EROFS %d %s\n", EROFS, strerror(EROFS));
#endif
#ifdef ESHUTDOWN
  printf("RESULT ESHUTDOWN %d %s\n", ESHUTDOWN, strerror(ESHUTDOWN));
#endif
#ifdef ESOCKTNOSUPPORT
  printf("RESULT ESOCKTNOSUPPORT %d %s\n", ESOCKTNOSUPPORT, strerror(ESOCKTNOSUPPORT));
#endif
#ifdef ESPIPE
  printf("RESULT ESPIPE %d %s\n", ESPIPE, strerror(ESPIPE));
#endif
#ifdef ESRCH
  printf("RESULT ESRCH %d %s\n", ESRCH, strerror(ESRCH));
#endif
#ifdef ESRMNT
  printf("RESULT ESRMNT %d %s\n", ESRMNT, strerror(ESRMNT));
#endif
#ifdef ESTALE
  printf("RESULT ESTALE %d %s\n", ESTALE, strerror(ESTALE));
#endif
#ifdef ESTRPIPE
  printf("RESULT ESTRPIPE %d %s\n", ESTRPIPE, strerror(ESTRPIPE));
#endif
#ifdef ETIME
  printf("RESULT ETIME %d %s\n", ETIME, strerror(ETIME));
#endif
#ifdef ETIMEDOUT
  printf("RESULT ETIMEDOUT %d %s\n", ETIMEDOUT, strerror(ETIMEDOUT));
#endif
#ifdef ETOOMANYREFS
  printf("RESULT ETOOMANYREFS %d %s\n", ETOOMANYREFS, strerror(ETOOMANYREFS));
#endif
#ifdef ETXTBSY
  printf("RESULT ETXTBSY %d %s\n", ETXTBSY, strerror(ETXTBSY));
#endif
#ifdef EUCLEAN
  printf("RESULT EUCLEAN %d %s\n", EUCLEAN, strerror(EUCLEAN));
#endif
#ifdef EUNATCH
  printf("RESULT EUNATCH %d %s\n", EUNATCH, strerror(EUNATCH));
#endif
#ifdef EUSERS
  printf("RESULT EUSERS %d %s\n", EUSERS, strerror(EUSERS));
#endif
#ifdef EXDEV
  printf("RESULT EXDEV %d %s\n", EXDEV, strerror(EXDEV));
#endif
#ifdef EXFULL
  printf("RESULT EXFULL %d %s\n", EXFULL, strerror(EXFULL));
#endif
  { char b[128]; int r = strerror_r(ENOENT, b, sizeof b); printf("RESULT strerror_r %d %s\n", r, b); }
  printf("RESULT unknown 9999 %s\n", strerror(9999));
  return 0;
}
