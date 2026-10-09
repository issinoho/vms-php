/* r_math_fix.c - vms_php_compat.h's rounding wrappers (PORTING_LOG #26):
   compiled with the header force-included, as BUILD.COM does.  PHP's
   round(-7, -1): ceil(-0.7) - 1 is -1 (tmp_value2), and copysign(1.0, ceil
   result) must be -1. */
#include <stdio.h>
#include <math.h>

static int sgn(double v) { return signbit(v) ? 1 : 0; }

int main(void)
{
  volatile double a = -0.7, b = -0.4, z = -0.0, p = 2.7;
  double ip;
  printf("RESULT ceil(-0.7) sign %d\n", sgn(ceil(a)));
  printf("RESULT copysign(1,ceil(-0.7)) %g (want -1)\n", copysign(1.0, ceil(a)));
  printf("RESULT trunc %d round %d rint %d nearbyint %d floor(-0) %d (want 1 1 1 1 1)\n",
         sgn(trunc(a)), sgn(round(b)), sgn(rint(b)),
         sgn(nearbyint(b)), sgn(floor(z)));
  modf(-0.5, &ip);
  printf("RESULT modf int sign %d (want 1)\n", sgn(ip));
  printf("RESULT ceil(2.7) %g floor(2.7) %g floor(-2.7) %g round(2.5) %g floorf %g (want 3 2 -3 3 2)\n",
         ceil(p), floor(p), floor(-p), round(2.5), (double) floorf(2.7f));
  return 0;
}
