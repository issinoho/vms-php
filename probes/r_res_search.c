/* r_res_search: the DNS resolver for checkdnsrr()/getmxrr()/dns_get_record()
   (Phase 4: phpBB's email check calls checkdnsrr(), undefined in our build).
   resolv.h maps res_search, dn_expand and dn_skipname to TCPIP$ routines
   exported by SYS$SHARE:TCPIP$ACCESS_SHR, not DECC$SHR, so the Phase 0 probes
   (DECC$SHR only) found none. Link with that image; query MX records with the
   answer buffer on the stack and from malloc (above 2 GB in LP64, which broke
   gethostbyname_r, PORTING_LOG #51), and parse them as ext/standard/dns.c does.
   usage: r_res_search [domain] */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>
#include <netdb.h>

static void mx(const char *what, const char *dom, unsigned char *ans, int size) {
  int n = res_search(dom, ns_c_in, ns_t_mx, ans, size), qd, an, i, len;
  unsigned char *cp, *end;
  char name[1025];
  printf("%-6s buffer %016llX: res_search(%s) = %d\n", what,
         (unsigned long long) (uintptr_t) ans, dom, n);
  if (n < 0) { printf("       h_errno %d\n", h_errno); return; }
  qd = (ans[4] << 8) | ans[5];
  an = (ans[6] << 8) | ans[7];
  cp = ans + 12; end = ans + n;
  while (qd-- > 0) {
    if ((len = dn_skipname(cp, end)) < 0) { printf("       dn_skipname failed\n"); return; }
    cp += len + 4;                         /* type, class */
  }
  for (i = 0; i < an && cp < end; i++) {
    int type, rdlen, pref;
    if ((len = dn_skipname(cp, end)) < 0) { printf("       dn_skipname failed\n"); return; }
    cp += len;
    type = (cp[0] << 8) | cp[1];
    rdlen = (cp[8] << 8) | cp[9];
    cp += 10;
    if (type == ns_t_mx) {
      pref = (cp[0] << 8) | cp[1];
      if (dn_expand(ans, end, cp + 2, name, sizeof name) < 0) { printf("       dn_expand failed\n"); return; }
      printf("       MX %3d %s\n", pref, name);
    }
    cp += rdlen;
  }
}

int main(int argc, char **argv) {
  const char *dom = argc > 1 ? argv[1] : "gmail.com";
  unsigned char stackbuf[8192];
  unsigned char *heap = malloc(8192);
  if (res_init() != 0) { printf("res_init failed\n"); return 1; }
  char *hdom = strdup(dom);                /* the name above 2 GB, as a zend_string's is */
  char *hname = malloc(1025);
  int n;
  mx("stack", dom, stackbuf, sizeof stackbuf);
  mx("stack", "no-such-domain.invalid", stackbuf, sizeof stackbuf);
  printf("name at %016llX:\n", (unsigned long long) (uintptr_t) hdom);
  mx("stack", hdom, stackbuf, sizeof stackbuf);
  /* dn_expand into a buffer above 2 GB: the first answer's owner name */
  n = res_search(dom, ns_c_in, ns_t_mx, stackbuf, sizeof stackbuf);
  if (n > 12) {
    int len = dn_skipname(stackbuf + 12, stackbuf + n);
    printf("dn_expand into %016llX: ", (unsigned long long) (uintptr_t) hname);
    fflush(stdout);
    printf("%d %s\n", dn_expand(stackbuf, stackbuf + n, stackbuf + 12 + len + 4, hname, 1025), hname);
  }
  printf("answer buffer above 2 GB (last: expected to fail):\n");
  fflush(stdout);
  mx("malloc", dom, heap, 8192);
  free(heap);
  return 0;
}
