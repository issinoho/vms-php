/* r_mdbg: temporary allocation tracker for the Phase 3 leak hunt (~460 bytes
   per request with opcache on). Linked by hand in front of PHP_CGI's objects
   (never part of the build): defines the C RTL's allocator entry points under
   their own names, so PHP's references bind here first, and forwards to
   DECC$SHR's. Each live block remembers its caller; at exit the callers with
   the most live blocks are printed with the address of main, for the map.
   Run PHP_CGI -T 1000 and -T 3000: the leaking caller's count follows n.
   Also counted: the peak of live blocks and bytes (memory held until
   shutdown is freed before the dump), mmap/munmap, and the page-file
   quota (PAGFILCNT) at the first call and at exit. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <descrip.h>
#include <libfisdef.h>
#include <lib$routines.h>
#include <jpidef.h>

typedef void *(*alloc_f)(size_t);
typedef void *(*realloc_f)(void *, size_t);
typedef void *(*calloc_f)(size_t, size_t);
typedef void (*free_f)(void *);
typedef char *(*strdup_f)(const char *);
static alloc_f r_malloc;
static realloc_f r_realloc;
static calloc_f r_calloc;
static free_f r_free;
static strdup_f r_strdup;
typedef void *(*mmap_f)(void *, size_t, int, int, int, long long);
typedef int (*munmap_f)(void *, size_t);
static mmap_f r_mmap;
static munmap_f r_munmap;

#define SLOTS (1u << 20)
static struct { void *p; void *caller; size_t n; } tab[SLOTS];
static int ready, busy;
static unsigned long long nalloc, nfree, live, livebytes, peak, peakbytes;
static unsigned long long nmmap, nmunmap, mapbytes, peakmap;
static unsigned int pgf0;
static unsigned long long va0[3];
static unsigned long long jpi(int code) {
  unsigned long long v = 0;
  lib$getjpi(&code, 0, 0, &v, 0, 0);
  return v;
}
static unsigned int pagfilcnt(void) { return (unsigned int) jpi(JPI$_PAGFILCNT); }
/* first free VA of P0 (grows up), P1 (grows down) and P2 (grows up) */
static void regions(unsigned long long *v) {
  v[0] = jpi(JPI$_FREP0VA); v[1] = jpi(JPI$_FREP1VA); v[2] = jpi(JPI$_P2_FIRST_FREE_VA_64);
}

static size_t h(void *p) { return (size_t) (((uintptr_t) p >> 4) * 2654435761u) & (SLOTS - 1); }
static void add(void *p, size_t n, void *caller) {
  size_t i;
  if (!p || !ready || busy) return;
  nalloc++;
  for (i = h(p); tab[i].p; i = (i + 1) & (SLOTS - 1)) ;
  tab[i].p = p; tab[i].n = n; tab[i].caller = caller;
  live++; livebytes += n;
  if (live > peak) peak = live;
  if (livebytes > peakbytes) peakbytes = livebytes;
}
static void del(void *p) {
  size_t i, j, k;
  if (!p || !ready) return;
  nfree++;
  for (i = h(p); tab[i].p && tab[i].p != p; i = (i + 1) & (SLOTS - 1)) ;
  if (!tab[i].p) return;
  live--; livebytes -= tab[i].n;
  tab[i].p = 0;
  /* linear-probing delete: re-place the rest of the cluster */
  for (j = (i + 1) & (SLOTS - 1); tab[j].p; j = (j + 1) & (SLOTS - 1)) {
    k = h(tab[j].p);
    if ((j > i && (k <= i || k > j)) || (j < i && (k <= i && k > j))) {
      tab[i] = tab[j]; tab[j].p = 0; i = j;
    }
  }
}

void *w_malloc(size_t n) __asm__("decc$_malloc64");
void *w_realloc(void *p, size_t n) __asm__("decc$_realloc64");
void *w_calloc(size_t a, size_t b) __asm__("decc$_calloc64");
void w_free(void *p) __asm__("decc$free");
char *w_strdup(const char *s) __asm__("decc$_strdup64");

static void syms(void);
#define NEED() do { if (!r_free) syms(); } while (0)
void *w_malloc(size_t n) { void *p; NEED(); p = r_malloc(n); add(p, n, __builtin_return_address(0)); return p; }
void *w_calloc(size_t a, size_t b) { void *p; NEED(); p = r_calloc(a, b); add(p, a * b, __builtin_return_address(0)); return p; }
char *w_strdup(const char *s) { char *p; NEED(); p = r_strdup(s); add(p, p ? strlen(p) + 1 : 0, __builtin_return_address(0)); return p; }
void *w_realloc(void *p, size_t n) {
  void *q;
  NEED();
  q = r_realloc(p, n);
  if (q) { if (p) del(p); add(q, n, __builtin_return_address(0)); }
  return q;
}
void w_free(void *p) { NEED(); del(p); r_free(p); }

void *w_mmap(void *a, size_t n, int prot, int fl, int fd, long long off) __asm__("decc$___off64_mmap64");
int w_munmap(void *a, size_t n) __asm__("decc$munmap");
void *w_mmap(void *a, size_t n, int prot, int fl, int fd, long long off) {
  void *p;
  NEED();
  p = r_mmap(a, n, prot, fl, fd, off);
  if (p != (void *) -1) { nmmap++; mapbytes += n; if (mapbytes > peakmap) peakmap = mapbytes; }
  return p;
}
int w_munmap(void *a, size_t n) {
  int r;
  NEED();
  r = r_munmap(a, n);
  if (r == 0) { nmunmap++; mapbytes -= n; }
  return r;
}

static void *sym(const char *name) {
  $DESCRIPTOR(img, "DECC$SHR");
  struct dsc$descriptor_s d = { (unsigned short) strlen(name), DSC$K_DTYPE_T, DSC$K_CLASS_S, (char *) name };
  /* the value comes back as a longword: a procedure value in the 32-bit
     space, to be sign-extended (probes/r_fis_width.c) */
  int v = 0;
  int st = lib$find_image_symbol(&img, &d, &v, 0, LIB$M_FIS_MIXEDCASE);
  if (!(st & 1)) { fprintf(stderr, "r_mdbg: %s: %%X%08X\n", name, st); exit(st); }
  return (void *) (intptr_t) v;
}

static struct agg { void *caller; size_t blocks, bytes; } a[4096];
static void dump(void) {
  size_t i, j, na = 0, total = 0;
  extern int main();
  busy = 1;
  for (i = 0; i < SLOTS; i++) {
    if (!tab[i].p) continue;
    total++;
    for (j = 0; j < na && a[j].caller != tab[i].caller; j++) ;
    if (j == na) { if (na == 4096) continue; a[na].caller = tab[i].caller; na++; }
    a[j].blocks++; a[j].bytes += tab[i].n;
  }
  for (i = 0; i < na; i++)
    for (j = i + 1; j < na; j++)
      if (a[j].blocks > a[i].blocks) { struct agg t = a[i]; a[i] = a[j]; a[j] = t; }
  fprintf(stderr, "r_mdbg: %llu allocs, %llu frees; %llu live blocks, %llu callers; main at %016llX\n",
          nalloc, nfree, (unsigned long long) total, (unsigned long long) na,
          (unsigned long long) (uintptr_t) main);
  fprintf(stderr, "r_mdbg: peak %llu live blocks, %llu bytes; mmap %llu, munmap %llu, "
          "mapped now %llu bytes, peak %llu\n", peak, peakbytes, nmmap, nmunmap, mapbytes, peakmap);
  fprintf(stderr, "r_mdbg: PAGFILCNT first call %u, exit %u: used %d pagelets\n",
          pgf0, pagfilcnt(), (int) (pgf0 - pagfilcnt()));
  {
    unsigned long long v[3];
    regions(v);
    fprintf(stderr, "r_mdbg: P0 end %016llX -> %016llX (+%lld)\n", va0[0], v[0], (long long) (v[0] - va0[0]));
    fprintf(stderr, "r_mdbg: P1 end %016llX -> %016llX (-%lld)\n", va0[1], v[1], (long long) (va0[1] - v[1]));
    fprintf(stderr, "r_mdbg: P2 end %016llX -> %016llX (+%lld)\n", va0[2], v[2], (long long) (v[2] - va0[2]));
  }
  for (i = 0; i < na && i < 25; i++)
    fprintf(stderr, "r_mdbg: caller %016llX  blocks %8llu  bytes %10llu\n",
            (unsigned long long) (uintptr_t) a[i].caller,
            (unsigned long long) a[i].blocks, (unsigned long long) a[i].bytes);
}

/* set up on the first allocator call: a constructor with a priority
   (section .init_array.101) was never run, though plain ones are (#22) */
static void syms(void) {
  r_malloc = (alloc_f) sym("decc$_malloc64");
  r_realloc = (realloc_f) sym("decc$_realloc64");
  r_calloc = (calloc_f) sym("decc$_calloc64");
  r_strdup = (strdup_f) sym("decc$_strdup64");
  r_free = (free_f) sym("decc$free");
  r_mmap = (mmap_f) sym("decc$___off64_mmap64");
  r_munmap = (munmap_f) sym("decc$munmap");
  pgf0 = pagfilcnt();
  regions(va0);
  atexit(dump);
  ready = 1;
}
