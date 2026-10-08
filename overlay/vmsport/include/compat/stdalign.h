/* stdalign.h - C11's <stdalign.h>, which the VMS C RTL lacks (xxhash.h
   includes it in C11 mode).  The keywords are clang's. */
#ifndef VMS_COMPAT_STDALIGN_H
#define VMS_COMPAT_STDALIGN_H
#define alignas _Alignas
#define alignof _Alignof
#define __alignas_is_defined 1
#define __alignof_is_defined 1
#endif
