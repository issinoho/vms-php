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

/* Special values the C RTL's libm gets wrong (C99 F.10; probes/libm_check.py
   against glibc over 4,400 calls, PORTING_LOG #27): pow(+0, -1) = -INF,
   pow(-0, y < 0) = NAN, pow(+-1, +-INF or NAN), pow(-INF, y) = NAN, the sign of
   an underflowed pow(x < 0, odd y); fmod(x, +-0) = 0; atan2(+-0, +-0) and
   atan2(+-INF, +-INF) = NAN, atan2(-tiny, x > 0) = +0; log, log10, log2(-0) =
   NAN; hypot(+-INF, NAN) = NAN; nextafter(x, y) with x == y.  These handle
   the C99 cases and pass everything else to the C RTL, whose finite results
   are within 1 ulp of glibc's.  The C RTL's sqrt() is not correctly rounded,
   as IEEE 754 requires (sqrt(2.0) is 1 ulp low, and so hypot(1, 1)): it is
   the SSE2 instruction, which is, and hypot the square root of the scaled
   sum of squares, correctly rounded whenever that sum is exact. */
static inline double vms_php_sqrt(double x)
{
  double r;
  __asm__("sqrtsd %1, %0" : "=x" (r) : "x" (x));
  return r;
}
static inline int vms_php_odd_int(double y)
{
  return fabs(y) < 9007199254740992.0 && y == (trunc)(y) && ((long long) y & 1);
}
static inline double vms_php_pow(double x, double y)
{
  if (y == 0 || x == 1) return 1.0;
  if (isnan(x) || isnan(y)) return x + y;
  if (x == 0) {
    if (y < 0) return vms_php_odd_int(y) ? copysign(__builtin_inf(), x) : __builtin_inf();
    return vms_php_odd_int(y) ? x : 0.0;
  }
  if (isinf(y)) {
    if (x == -1) return 1.0;
    return (fabs(x) < 1) == (y < 0) ? __builtin_inf() : 0.0;
  }
  if (isinf(x)) {
    if (x > 0) return y < 0 ? 0.0 : x;
    if (y < 0) return vms_php_odd_int(y) ? -0.0 : 0.0;
    return vms_php_odd_int(y) ? x : -x;
  }
  if (x < 0 && y == (trunc)(y)) {
    double r = (pow)(-x, y);
    return vms_php_odd_int(y) ? -r : r;
  }
  return (pow)(x, y);
}
static inline double vms_php_fmod(double x, double y)
{
  if (isnan(x) || isnan(y)) return x + y;
  if (y == 0 || isinf(x)) return __builtin_nan("");
  return (fmod)(x, y);
}
static inline double vms_php_atan2(double y, double x)
{
  double r;
  if (isnan(x) || isnan(y)) return x + y;
  if (y == 0) return (x > 0 || (x == 0 && !signbit(x))) ? y : copysign(M_PI, y);
  if (isinf(y) && isinf(x)) return copysign(x > 0 ? M_PI / 4 : 3 * M_PI / 4, y);
  r = (atan2)(y, x);
  return r == 0 ? copysign(0.0, y) : r;
}
static inline double vms_php_log(double x) { return x == 0 ? -__builtin_inf() : (log)(x); }
static inline double vms_php_log10(double x) { return x == 0 ? -__builtin_inf() : (log10)(x); }
static inline double vms_php_log2(double x) { return x == 0 ? -__builtin_inf() : (log2)(x); }
static inline double vms_php_hypot(double x, double y)
{
  double a = fabs(x), b = fabs(y), t, s = 1.0;
  if (isinf(a) || isinf(b)) return __builtin_inf();
  if (isnan(a) || isnan(b)) return a + b;
  if (a < b) { t = a; a = b; b = t; }
  /* b < a * 2^-27: (b/a)^2 / 2 is below half an ulp of a */
  if (b <= a * 0x1p-27) return a;
  if (a > 0x1p500) { a *= 0x1p-600; b *= 0x1p-600; s = 0x1p600; }
  else if (a < 0x1p-500) { a *= 0x1p600; b *= 0x1p600; s = 0x1p-600; }
  return s * vms_php_sqrt(a * a + b * b);
}
static inline double vms_php_nextafter(double x, double y)
{
  return x == y ? y : (nextafter)(x, y);
}
/* acosh: fdlibm's algorithm (e_acosh.c) on the C RTL's log and log1p is
   closer to the correctly rounded result than the C RTL's own (probe r_acosh:
   3 of the 5 inputs that were 1 ulp off, among them acosh_variation's 23.45,
   now match; none worse; above 2^28 the C RTL's own is kept). */
static inline double vms_php_acosh(double x)
{
  double t;
  if (isnan(x)) return x + x;
  if (x < 1) return __builtin_nan("");
  if (x >= 0x1p28) return (acosh)(x);   /* the C RTL is right there */
  if (x == 1) return 0.0;
  if (x > 2) { t = x * x; return vms_php_log(2.0 * x - 1.0 / (x + vms_php_sqrt(t - 1.0))); }
  t = x - 1.0;
  return log1p(t + vms_php_sqrt(2.0 * t + t * t));
}
#define sqrt(x) vms_php_sqrt(x)
#define acosh(x) vms_php_acosh(x)
#define pow(x, y) vms_php_pow(x, y)
#define fmod(x, y) vms_php_fmod(x, y)
#define atan2(y, x) vms_php_atan2(y, x)
/* object-like: mysqlnd calls a struct member log() with six arguments, which
   this renames consistently in every file */
#define log vms_php_log
#define log10(x) vms_php_log10(x)
#define log2(x) vms_php_log2(x)
#define hypot(x, y) vms_php_hypot(x, y)
#define nextafter(x, y) vms_php_nextafter(x, y)

/* strerror() and strerror_r() with glibc's wording, not the C RTL's
   ("No such file or directory", not "no such file or directory"):
   vmsport/vms_strerror.c (PORTING_LOG #28). */
#include <string.h>
char *vms_php_strerror(int e);
int vms_php_strerror_r(int e, char *buf, size_t n);
#define strerror(e) vms_php_strerror(e)
#define strerror_r(e, buf, n) vms_php_strerror_r(e, buf, n)

/* putenv() through setenv(): the C RTL's putenv keeps the string in its
   32-bit environ and ACCVIOs on one above 2 GB (vmsport/vms_shims.c,
   PORTING_LOG #29). */
#include <stdlib.h>
int vms_php_putenv(char *s);
#define putenv(s) vms_php_putenv(s)

#endif /* __VMS && __clang__ */
#endif /* VMS_PHP_COMPAT_H */
