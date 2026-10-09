/* r_heap.c - the C RTL heap as PHP's clang code uses it (Phase 1).
   lexbor (uri ext) and timelib crash on corrupted pointers at
   0x8000000000000000 after malloc/calloc/realloc/free; PHP's own Zend MM
   works.  Where does each call put memory, and do realloc/free cope with
   blocks from the other calls?  Build with BUILD.COM's flags (-O2,
   vms_lp64.h) to see what PHP sees. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define SHOW(what, p) printf("RESULT %-14s %p%s\n", what, (void *) (p), \
                             ((uintptr_t) (p) >> 32) ? " (64-bit)" : " (32-bit)")

int main(void)
{
  char *m = malloc(100), *c = calloc(10, 10), *r = realloc(NULL, 100), *s = strdup("x");
  void *a = NULL;
  int i, bad = 0;

  posix_memalign(&a, 4096, 100);
  SHOW("malloc", m); SHOW("calloc", c); SHOW("realloc(NULL)", r); SHOW("strdup", s);
  SHOW("posix_memalign", a);
  memset(m, 'm', 100);
  m = realloc(m, 100000);
  SHOW("realloc grow", m);
  for (i = 0; i < 100; i++) if (m[i] != 'm') bad++;
  printf("RESULT realloc keeps data: %s\n", bad ? "NO" : "yes");
  memset(c, 'c', 100);
  c = realloc(c, 50000);
  SHOW("realloc calloc", c);
  printf("RESULT realloc of calloc keeps data: %s\n", c[99] == 'c' ? "yes" : "NO");
  free(m); free(c); free(r); free(s); free(a);

  /* a lexbor-like pool: chunks linked through a pointer at the start */
  {
    struct chunk { struct chunk *prev; char *data; } *head = NULL, *ch;
    for (i = 0; i < 1000; i++) {
      ch = calloc(1, sizeof *ch);
      ch->data = malloc(4096 + i);
      memset(ch->data, i, 4096);
      ch->prev = head;
      head = ch;
      if (i % 100 == 0) head->data = realloc(head->data, 65536);
    }
    for (i = 0; head; i++) {
      ch = head->prev;
      free(head->data);
      free(head);
      head = ch;
    }
    printf("RESULT pool of 1000 chunks freed: %d\n", i);
  }
  return 0;
}
