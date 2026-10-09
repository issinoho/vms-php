/* r_tentative.c - a static object declared first without an initializer (a
   C "tentative definition"), used, and defined with an initializer later in
   the file, as ext/opcache/ZendAccelerator.c does with opcache_extension_entry
   (lines 105, 3170, 5120).  Must be one object holding the initializer. */
#include <stdio.h>

struct ext { const char *name; int (*startup)(void); };

static struct ext entry;                 /* tentative */
static const struct ext *registered;

static void register_it(void) { registered = &entry; }

static int start(void) { return 42; }

static struct ext entry = { "opcache", start };   /* the definition */

int main(void)
{
  register_it();
  printf("RESULT tentative same object: %s, name=%s, startup=%s\n",
         registered == &entry ? "yes" : "NO",
         registered->name ? registered->name : "(null)",
         registered->startup ? "set" : "NULL");
  return 0;
}
