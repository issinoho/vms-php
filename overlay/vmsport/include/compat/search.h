/* search.h - main/php_scandir.c includes <search.h> but uses nothing from it
   (its qsort() comes from <stdlib.h>); the VMS C RTL has no such header. */
#ifndef VMS_COMPAT_SEARCH_H
#define VMS_COMPAT_SEARCH_H
#include <stdlib.h>
#endif
