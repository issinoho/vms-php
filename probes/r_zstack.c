/* r_stack.c - how deep can the main thread's stack go?  PHP recurses in
   C for nested arrays, includes and s9e/TextFormatter's DOM work; 8.5 has no
   stack-limit check on VMS (zend_call_stack.c knows no VMS method), so
   running out means a crash.  Prints a line at each milestone; the last one
   printed is the answer (run last: an overflow ends the process). */
#include <stdio.h>
#include <string.h>

static unsigned long reached;

static void down(unsigned long depth_kb, unsigned long limit_kb)
{
  volatile char frame[1024];
  memset((char *) frame, 1, sizeof frame);
  reached = depth_kb;
  if ((depth_kb & (depth_kb - 1)) == 0 && depth_kb >= 1024) {
    printf("RESULT stack reached %lu MB\n", depth_kb / 1024);
    fflush(stdout);
  }
  if (depth_kb < limit_kb) down(depth_kb + 1, limit_kb);
  frame[0] ^= 1;
}

int main(void)
{
  down(1, 64 * 1024);
  printf("RESULT stack ok to %lu MB\n", reached / 1024);
  return 0;
}
