/* r_mmtrim.c - the Zend memory manager's chunk allocation on VMS
   (Zend/zend_alloc.c zend_mm_chunk_alloc_int): mmap 2 MB; if it is not 2 MB
   aligned, munmap it, mmap 4 MB - page, munmap the unaligned head and the
   tail, and keep the aligned 2 MB in the middle.  PHP then saw C RTL / Zend
   allocations land on live memory.  Is the kept 2 MB still mapped and
   writable, and does malloc() ever hand out an address inside it? */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/mman.h>

#define CHUNK (2u << 20)

static void *zend_like_chunk(void)
{
  size_t page = (size_t) sysconf(_SC_PAGESIZE), off;
  char *p = mmap(NULL, CHUNK, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
  if (p == MAP_FAILED) return NULL;
  if (((uintptr_t) p & (CHUNK - 1)) == 0) { printf("RESULT first mmap aligned %p\n", p); return p; }
  munmap(p, CHUNK);
  p = mmap(NULL, CHUNK + CHUNK - page, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
  if (p == MAP_FAILED) return NULL;
  off = (uintptr_t) p & (CHUNK - 1);
  printf("RESULT big mmap %p, head trim %lu\n", p, (unsigned long) (off ? CHUNK - off : 0));
  if (off) {
    off = CHUNK - off;
    printf("RESULT munmap head rc=%d\n", munmap(p, off));
    p += off;
    printf("RESULT munmap tail rc=%d\n", munmap(p + CHUNK, CHUNK - off - page));
  } else {
    printf("RESULT munmap tail rc=%d\n", munmap(p + CHUNK, CHUNK - page));
  }
  return p;
}

int main(void)
{
  char *c, *chunks[6];
  int i, j, inside = 0, broken = 0;

  /* several chunks in a row, as the Zend MM asks for more: each must be a
     new address range, and each keeps its own contents */
  for (j = 0; j < 6; j++) {
    chunks[j] = zend_like_chunk();
    printf("RESULT chunk %d at %p\n", j, chunks[j]);
    if (chunks[j]) memset(chunks[j], 0x10 + j, CHUNK);
  }
  for (j = 0; j < 6; j++) {
    for (i = 0; i < 6; i++)
      if (i != j && chunks[i] && chunks[j] && chunks[i] < chunks[j] + CHUNK && chunks[j] < chunks[i] + CHUNK)
        printf("RESULT chunks %d and %d OVERLAP\n", i, j);
    for (i = 0; chunks[j] && i < CHUNK; i += 4096)
      if (((unsigned char *) chunks[j])[i] != 0x10 + j) { broken++; break; }
  }
  printf("RESULT chunks with contents changed: %d\n", broken);
  c = zend_like_chunk();
  printf("RESULT chunk %p\n", c);
  if (!c) return 1;
  memset(c, 0x5a, CHUNK);                      /* still mapped and writable? */
  printf("RESULT chunk writable, last byte %x\n", ((unsigned char *) c)[CHUNK - 1]);
  for (i = 0; i < 20000; i++) {
    char *m = malloc(64 + (i % 4000));
    if (m >= c && m < c + CHUNK) {
      if (inside++ < 3) printf("RESULT malloc %p INSIDE the chunk\n", m);
    }
    memset(m, 0x33, 64);
  }
  for (i = 0; i < CHUNK; i++) if (((unsigned char *) c)[i] != 0x5a) break;
  printf("RESULT malloc blocks inside the chunk: %d; chunk intact: %s\n", inside,
         i == CHUNK ? "yes" : "NO");
  return 0;
}
