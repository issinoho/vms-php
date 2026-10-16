#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
int main(void){void *p=malloc(10);p=realloc(p,20);free(p);p=calloc(1,2);free(p);p=strdup("x");free(p);p=mmap(0,8192,PROT_READ|PROT_WRITE,MAP_ANONYMOUS|MAP_PRIVATE,-1,0);munmap(p,8192);return 0;}
