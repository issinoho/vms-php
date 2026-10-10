/* r_acosh.c - is fdlibm's acosh (e_acosh.c), on the C RTL's log and log1p,
   closer to the correctly rounded result than the C RTL's acosh?  The five
   inputs where libm_check.py found the C RTL 1 ulp from glibc. */
#include <stdio.h>
#include <math.h>

static double fd_acosh(double x)
{
  double t;
  if (x < 1) return (x - x) / (x - x);
  if (x >= 0x1p28) return isinf(x) ? x : log(x) + 0.69314718055994530942;
  if (x == 1) return 0.0;
  if (x > 2) { t = x * x; return log(2.0 * x - 1.0 / (x + sqrt(t - 1.0))); }
  t = x - 1.0;
  return log1p(t + sqrt(2.0 * t + t * t));
}

int main(void)
{
  static const double in[] = {2.0, 23.45, 100.0, 1.0000001, 3.141592653589793};
  int i;
  for (i = 0; i < 5; i++) {
    volatile double x = in[i];
    printf("RESULT acosh(%.17g) crtl %.17g fdlibm %.17g\n", in[i], acosh(x), fd_acosh(x));
  }
  return 0;
}
