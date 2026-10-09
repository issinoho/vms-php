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

/* The C RTL's rounding functions lose the sign of a zero result: ceil(-0.7),
   trunc(-0.7), round(-0.4), rint, nearbyint, floor(-0.0) and modf's integral
   part are +0, not -0 (C99 F.10.6; probe r_math_zero).  PHP's round() takes
   the sign of ceil()'s result, so round(-7, -1) was 10 (PORTING_LOG #26).
   These functions return a value with the sign of x, so copysign(f(x), x)
   is right for every x. */
#include <math.h>
static inline double vms_php_ceil(double x) { return copysign((ceil)(x), x); }
static inline double vms_php_floor(double x) { return copysign((floor)(x), x); }
static inline float vms_php_floorf(float x) { return copysignf((floorf)(x), x); }
static inline double vms_php_trunc(double x) { return copysign((trunc)(x), x); }
static inline double vms_php_round(double x) { return copysign((round)(x), x); }
static inline double vms_php_rint(double x) { return copysign((rint)(x), x); }
static inline double vms_php_nearbyint(double x) { return copysign((nearbyint)(x), x); }
static inline double vms_php_modf(double x, double *ip)
{
  double r = (modf)(x, ip);
  *ip = copysign(*ip, x);
  return r;
}
#define ceil(x) vms_php_ceil(x)
#define floor(x) vms_php_floor(x)
#define floorf(x) vms_php_floorf(x)
#define trunc(x) vms_php_trunc(x)
#define round(x) vms_php_round(x)
#define rint(x) vms_php_rint(x)
#define nearbyint(x) vms_php_nearbyint(x)
#define modf(x, ip) vms_php_modf(x, ip)

#endif /* __VMS && __clang__ */
#endif /* VMS_PHP_COMPAT_H */
