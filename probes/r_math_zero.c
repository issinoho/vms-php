/* r_math_zero.c - the C RTL's ceil(-0.7) is +0 (C99 F.10.6.1: -0), which
   made PHP's round(-7, -1) 10.  Which other math functions lose the sign of
   a zero result?  Prints "BAD" for each result whose sign is wrong. */
#include <stdio.h>
#include <math.h>

static void chk(const char *what, double got, int want_neg)
{
  double g = got;
  int neg = signbit(g) != 0;
  printf("RESULT %-20s %-4g %s\n", what, got, neg == want_neg ? "ok" : "BAD");
}

int main(void)
{
  volatile double a = -0.7, b = -0.4, c = -0.5, z = -0.0, one = 1.0, m1 = -1.0;
  double ip;

  chk("ceil(-0.7)", ceil(a), 1);
  chk("ceil(-0.0)", ceil(z), 1);
  chk("floor(-0.0)", floor(z), 1);
  chk("trunc(-0.7)", trunc(a), 1);
  chk("round(-0.4)", round(b), 1);
  chk("nearbyint(-0.4)", nearbyint(b), 1);
  chk("rint(-0.4)", rint(b), 1);
  chk("modf(-0.5) frac", modf(c, &ip), 1);
  chk("modf(-0.5) int", ip, 1);
  chk("fmod(-1,1)", fmod(m1, one), 1);
  chk("sqrt(-0.0)", sqrt(z), 1);
  chk("sin(-0.0)", sin(z), 1);
  chk("atan2(-0.0,1)", atan2(z, one), 1);
  chk("-0.0*1", z * one, 1);
  chk("ceilf(-0.7)", (double) ceilf((float) a), 1);
  chk("ceill(-0.7)", (double) ceill((long double) a), 1);
  chk("truncf(-0.7)", (double) truncf((float) a), 1);
  return 0;
}
