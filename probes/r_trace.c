/* r_trace: call counts of every C RTL routine PHP_CGI calls (PORTING_LOG
   #53: the leak is P0 growth, so it is inside the C RTL or below, on a call
   that follows the request count with opcache on). The stubs come from
   r_trace_gen.py; this file resolves the real routines in DECC$SHR on their
   first call and prints the non-zero counts at exit, highest first.
   Not part of the build: linked by hand by r_mdbg_link.com. */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <descrip.h>
#include <libfisdef.h>
#include <lib$routines.h>
#include "r_trace_names.h"

unsigned long long r_trace_cnt[R_TRACE_N];
void *r_trace_real[R_TRACE_N];
static unsigned long long snap[R_TRACE_N];
static int idx[R_TRACE_N];

static void dump(void) {
  int i, j, t;
  for (i = 0; i < R_TRACE_N; i++) { snap[i] = r_trace_cnt[i]; idx[i] = i; }
  for (i = 0; i < R_TRACE_N; i++)
    for (j = i + 1; j < R_TRACE_N; j++)
      if (snap[idx[j]] > snap[idx[i]]) { t = idx[i]; idx[i] = idx[j]; idx[j] = t; }
  for (i = 0; i < R_TRACE_N && snap[idx[i]]; i++)
    fprintf(stderr, "r_trace: %10llu %s\n", snap[idx[i]], r_trace_names[idx[i]]);
}

/* called by the stubs' first-call path; uses no C RTL routine itself, so
   it never re-enters a stub before the one being resolved is done */
void *r_trace_resolve(long i) {
  static int once;
  const char *n = r_trace_names[i];
  unsigned short len = 0;
  int v = 0, st;
  $DESCRIPTOR(img, "DECC$SHR");
  struct dsc$descriptor_s d;
  while (n[len]) len++;
  d.dsc$w_length = len; d.dsc$b_dtype = DSC$K_DTYPE_T;
  d.dsc$b_class = DSC$K_CLASS_S; d.dsc$a_pointer = (char *) n;
  /* a longword, to be sign-extended (probes/r_fis_width.c) */
  st = lib$find_image_symbol(&img, &d, &v, 0, LIB$M_FIS_MIXEDCASE);
  if (!(st & 1)) lib$signal(st);
  r_trace_real[i] = (void *) (intptr_t) v;
  if (!once) { once = 1; atexit(dump); }
  return r_trace_real[i];
}
