/* r_hostent.c - gethostbynamel('localhost') ACCVIOs at a sign-extended
   32-bit address in PHP (probe r_dns_empty, PORTING_LOG #50): what does
   gethostbyname() return to LP64 code?  The pointer sizes the headers
   declare and the raw words of h_addr_list.  Compiled with PHP's flags. */
#include <stdio.h>
#include <string.h>
#include <netdb.h>

int main(int argc, char **argv)
{
  const char *name = argc > 1 ? argv[1] : "localhost";
  struct hostent *hp = gethostbyname(name);
  unsigned int *w;
  int i;
  printf("RESULT sizeof(void *) %d, sizeof(hp->h_addr_list) %d, sizeof(*hp->h_addr_list) %d, sizeof(hp->h_name) %d\n",
         (int) sizeof(void *), (int) sizeof(hp->h_addr_list), (int) sizeof(*hp->h_addr_list), (int) sizeof(hp->h_name));
  if (!hp) { printf("RESULT no host\n"); return 0; }
  printf("RESULT hostent at %p, h_name %p \"%s\", h_length %d, h_addr_list %p\n", (void *) hp,
         (void *) hp->h_name, hp->h_name, hp->h_length, (void *) hp->h_addr_list);
  w = (unsigned int *) hp->h_addr_list;
  for (i = 0; i < 6; i++) printf("RESULT   word %d: %08x\n", i, w[i]);
  return 0;
}
