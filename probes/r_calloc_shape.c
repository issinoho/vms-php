/* r_calloc_shape.c - the "allocate, memset/bzero if not NULL, return the
   pointer" shape of MariaDB's ma_calloc_root (libmariadb/mariadb_rpl.c),
   new_ma_field_extension and THD::calloc (sql/sql_class.h), compiled with
   VSI clang at -O2: does it become a tail call to OTS$FILL, whose return
   value is not the pointer (vms-php PORTING_LOG #17)?  Run: compare. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

static char arena[4096];
static size_t used;

__attribute__((noinline)) void *alloc_root(size_t len)
{
  void *p = arena + used;
  used += (len + 15) & ~(size_t) 15;
  return p;
}

__attribute__((noinline)) void *calloc_root_memset(size_t len)
{
  void *p;
  if ((p = alloc_root(len)))
    memset(p, 0, len);
  return p;
}

__attribute__((noinline)) void *calloc_root_bzero(size_t len)
{
  void *p;
  if ((p = alloc_root(len)))
    bzero(p, len);
  return p;
}

int main(void)
{
  void *a = calloc_root_memset(40), *b = calloc_root_bzero(40);
  printf("RESULT memset shape: got %p, expected %p: %s\n", a, (void *) arena,
         a == (void *) arena ? "ok" : "WRONG");
  printf("RESULT bzero shape:  got %p, expected %p: %s\n", b, (void *) (arena + 48),
         b == (void *) (arena + 48) ? "ok" : "WRONG");
  return 0;
}
