/* r_asm_mul.c - Zend/zend_multiply.h's x86-64 inline-asm zend_safe_address
   (mulq/adc with "=&a"/"=&d" outputs), used by ecalloc() and safe_emalloc().
   In PHP, ecalloc(1, 24) returned a block still in use: the computed size was
   wrong.  Compare the asm with plain C, and test PHP's other x86-64 asm idiom
   (overflow-checked add with "jo", Zend/zend_operators.h). */
#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>

static size_t asm_safe_address(size_t nmemb, size_t size, size_t offset, bool *overflow)
{
  size_t res;
  unsigned long m_overflow = 0;
  if (offset == 0) {
    res = nmemb;
    __asm__ ("mulq %3\n\t"
             "adc $0,%1"
             : "=&a"(res), "=&d" (m_overflow)
             : "%0"(res), "rm"(size)
             : "cc");
  } else {
    res = nmemb;
    __asm__ ("mulq %3\n\t"
             "add %4,%0\n\t"
             "adc $0,%1"
             : "=&a"(res), "=&d" (m_overflow)
             : "%0"(res), "rm"(size), "rm"(offset)
             : "cc");
  }
  *overflow = m_overflow != 0;
  return m_overflow ? 0 : res;
}

static long asm_add(long a, long b, int *ovf)
{
  long r;
  __asm__ ("movq %2, %0\n\t"
           "addq %3, %0\n\t"
           "seto %b1"
           : "=&r"(r), "=&r"(*ovf)
           : "r"(a), "r"(b)
           : "cc");
  return r;
}

int main(void)
{
  static const size_t cases[][3] = { {1, 24, 0}, {1, 32, 0}, {3, 40, 0}, {7, 9, 5}, {1, 8200, 0},
                                     {(size_t) 1 << 40, 3, 0}, {(size_t) 1 << 63, 4, 0} };
  int i, ovf = 0;
  for (i = 0; i < (int) (sizeof cases / sizeof cases[0]); i++) {
    bool o;
    volatile size_t n = cases[i][0], s = cases[i][1], off = cases[i][2];
    size_t r = asm_safe_address(n, s, off, &o);
    printf("RESULT safe_address(%zu,%zu,%zu) = %zu overflow=%d  (C: %zu)\n", (size_t) n, (size_t) s,
           (size_t) off, r, (int) o, (size_t) (n * s + off));
  }
  {
    volatile long a = 5, b = 7;
    long r = asm_add(a, b, &ovf);
    printf("RESULT asm add 5+7 = %ld ovf=%d\n", r, ovf & 0xff);
  }
  return 0;
}
