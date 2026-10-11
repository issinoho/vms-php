/* r_pow_big.c - pow() overflow: PHP's pow(4294967294, 65) is 0 on OpenVMS,
   INF on Linux (pow_basiclong_64bit, PORTING_LOG #48).  The C RTL's pow and
   vms_php_pow (vms_php_compat.h, #27) on overflowing and underflowing
   arguments, and PHP's integer-power path (squaring with doubles). */
#include <stdio.h>
#include <math.h>

static void show(double x, double y)
{
  printf("RESULT pow(%.17g, %g): C RTL %g, vms_php_pow %g\n", x, y, (pow)(x, y), vms_php_pow(x, y));
}

int main(void)
{
  volatile double a = 4294967294.0, m;
  int i;
  show(4294967294.0, 65); show(4294967294.0, 64); show(4294967294.0, 32); show(2, 1100); show(2, 1023);
  show(10, 400); show(1e300, 2); show(-4294967294.0, 65); show(0.5, 1100); show(4294967294.0, -65); show(4294967294.0, -64); show(-4294967294.0, 64);
  show(0.5, -2100); show(-0.5, 2101); show(-2.5, 0.5); show(1e-300, -8); show(3, 0.5);
  m = a;
  for (i = 0; i < 7; i++) { m = m * m; printf("RESULT square %d: %g\n", i, m); }
  return 0;
}
