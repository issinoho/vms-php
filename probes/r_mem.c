/* r_mem.c - memory PHP's opcache and allocator need: anonymous shared and
   private mmap of opcache-sized regions, mprotect, and where they land
   (64-bit addresses?); posix_memalign for the Zend MM's 2 MB chunks. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/mman.h>

static void try_map(const char *name, size_t len, int flags)
{
  void *p;
  errno = 0;
  p = mmap(NULL, len, PROT_READ | PROT_WRITE, flags, -1, 0);
  if (p == MAP_FAILED) {
    printf("RESULT mmap_%s %lu MB failed errno=%d %s\n", name, (unsigned long) (len >> 20), errno, strerror(errno));
    return;
  }
  memset(p, 0x5a, len);
  printf("RESULT mmap_%s %lu MB ok at %p, last byte %x\n", name, (unsigned long) (len >> 20), p, ((unsigned char *) p)[len - 1]);
  errno = 0;
  printf("RESULT mprotect_%s read-only rc=%d errno=%d\n", name, mprotect(p, 8192, PROT_READ), errno);
  mprotect(p, 8192, PROT_READ | PROT_WRITE);
  printf("RESULT munmap_%s rc=%d\n", name, munmap(p, len));
}

int main(void)
{
  void *a = NULL;
  int rc;
  printf("RESULT pagesize %ld\n", sysconf(_SC_PAGESIZE));
  try_map("shared_anon", 128u << 20, MAP_SHARED | MAP_ANON);
  try_map("private_anon", 64u << 20, MAP_PRIVATE | MAP_ANON);
  try_map("shared_anon_big", 512u << 20, MAP_SHARED | MAP_ANON);
  rc = posix_memalign(&a, 2u << 20, 2u << 20);
  printf("RESULT posix_memalign 2MB/2MB rc=%d at %p aligned=%d\n", rc, a,
         a ? (int) (((unsigned long long) (size_t) a & ((2u << 20) - 1)) == 0) : 0);
  /* Zend MM fallback: mmap 4 MB and trim to a 2 MB boundary with munmap */
  a = mmap(NULL, 4u << 20, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
  if (a != MAP_FAILED) {
    size_t off = (size_t) a & ((2u << 20) - 1);
    size_t lead = off ? (2u << 20) - off : 0;
    errno = 0;
    rc = lead ? munmap(a, lead) : 0;
    printf("RESULT munmap_partial_head lead=%lu rc=%d errno=%d\n", (unsigned long) lead, rc, errno);
  }
  return 0;
}
