/* r_hostent_r.c - PHP's php_network_gethostbyname() calls the 6-argument
   gethostbyname_r() with a malloc()ed buffer (above 2 GB in LP64), and
   gethostbynamel('localhost') ACCVIOs in system code (PORTING_LOG #50).
   "r_hostent_r 1": that call; "2": the same with a _malloc32() buffer.
   Compiled with PHP's flags. */
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>

void *_malloc32(size_t);

int main(int argc, char **argv)
{
  int c = atoi(argv[1]), herr = 0, r;
  struct hostent hb, *hp = NULL;
  char *buf = c == 2 ? _malloc32(1024) : malloc(1024);
  printf("RESULT case %d: buffer %p, hostent %p\n", c, (void *) buf, (void *) &hb);
  fflush(stdout);
  r = gethostbyname_r("localhost", &hb, buf, 1024, &hp, &herr);
  printf("RESULT   gethostbyname_r %d, hp %p, h_name %s\n", r, (void *) hp, hp ? hp->h_name : "-");
  return 0;
}
