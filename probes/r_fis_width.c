/* r_fis_width: does lib$find_image_symbol give a usable 64-bit procedure
   value for DECC$SHR's decc$_malloc64? r_mdbg crashed calling it (fetch from
   0x88037790). Compare with the address the linker binds, and see how many
   bytes of the value argument are written (pre-filled with 0xAA). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <descrip.h>
#include <libfisdef.h>
#include <lib$routines.h>
#include <dlfcn.h>

extern void *malloc_ref(size_t) __asm__("decc$_malloc64");

int main(void) {
  $DESCRIPTOR(img, "DECC$SHR");
  $DESCRIPTOR(name, "decc$_malloc64");
  uint64_t v = 0xAAAAAAAAAAAAAAAAull;
  int st = lib$find_image_symbol(&img, &name, &v, 0, LIB$M_FIS_MIXEDCASE);
  void *h = dlopen("DECC$SHR", RTLD_NOW);
  void *d = h ? dlsym(h, "decc$_malloc64") : 0;
  printf("linker  %016llX\n", (unsigned long long) (uintptr_t) malloc_ref);
  printf("fis     %016llX  status %08X\n", (unsigned long long) v, st);
  printf("dlsym   %016llX  (%s)\n", (unsigned long long) (uintptr_t) d, h ? "ok" : dlerror());
  return 0;
}
