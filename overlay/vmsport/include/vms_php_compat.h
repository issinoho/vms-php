/*
  vms_php_compat.h - C11 pieces the VMS C RTL headers lack, for PHP built with
  clang -std=gnu11 (force-included after vms_lp64.h by BUILD.COM).

  max_align_t: C11 <stddef.h> defines it; the C RTL's does not, and
  Zend/zend_portability.h uses it for zend_max_align_t (the Zend memory
  manager's alignment) in C11 mode.  Defined as clang's own <stddef.h> does:
  a type as aligned as long long and long double.
*/
#ifndef VMS_PHP_COMPAT_H
#define VMS_PHP_COMPAT_H

#if defined(__VMS) && defined(__clang__)
#include <stddef.h>

#ifndef __CLANG_MAX_ALIGN_T_DEFINED
#define __CLANG_MAX_ALIGN_T_DEFINED
typedef struct {
  long long __max_align_ll __attribute__((__aligned__(__alignof__(long long))));
  long double __max_align_ld __attribute__((__aligned__(__alignof__(long double))));
} max_align_t;
#endif

/* suseconds_t (php_date.c): not in the C RTL.  tv_usec is int or long
   depending on the C RTL's feature macros, so take its own type. */
#include <sys/time.h>
typedef __typeof__(((struct timeval *) 0)->tv_usec) suseconds_t;

/* timercmp (main/network.c): the BSD macro, missing from <sys/time.h> */
#ifndef timercmp
#define timercmp(a, b, CMP) \
  (((a)->tv_sec == (b)->tv_sec) ? ((a)->tv_usec CMP (b)->tv_usec) \
                                : ((a)->tv_sec CMP (b)->tv_sec))
#endif

/* ext/standard/php_string.h declares localeconv_r(struct lconv *) before
   anything has included <locale.h> on VMS, so the struct would be a new,
   incomplete type in that prototype's scope (string.c: "conflicting types
   for 'localeconv_r'").  Declare it at file scope first. */
#include <locale.h>

/* imaxabs: the C RTL's <inttypes.h> maps it to decc$imaxabs, which DECC$SHR
   does not provide (link: undefined).  intmax_t is long long. */
#include <inttypes.h>
#undef imaxabs
#define imaxabs(j) llabs(j)

#endif /* __VMS && __clang__ */
#endif /* VMS_PHP_COMPAT_H */
