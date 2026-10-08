/*
  vms_ssl3_long.h - VSI SSL3's 32-bit 'long' for clang (LP64) callers.

  VSI's SSL3 (OpenSSL 3.0) images were built with a 32-bit 'long'.  Our
  clang code has a 64-bit one, so (docs/PHASE0.md, DECISIONS D7):
    - a 'long' result comes back in the low 32 bits of the register: a
      negative value (an error, -1) reads as a large positive one.  The
      wrappers below sign-extend it.
    - a 'long *' result is written as 4 bytes: the wrappers pass an int.
  'unsigned long' results (ERR_get_error...) are 32-bit values and
  zero-extend correctly; 'long' arguments arrive as their low 32 bits, which
  holds for the small values ext/openssl passes.

  Force-included (clang -include) into ext/openssl's compiles and every other
  one that includes OpenSSL headers (BUILD.COM).  Object-like macros, so
  that the OpenSSL macros built on these functions (BIO_get_mem_ptr,
  SSL_set_mode, SSL_set_tlsext_host_name...) expand to the wrappers too.
*/
#ifndef VMS_SSL3_LONG_H
#define VMS_SSL3_LONG_H

#if defined(__VMS) && defined(__LP64__)

#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/conf.h>

static inline long vms_ssl3_BIO_ctrl(BIO *bp, int cmd, long larg, void *parg)
{ return (long) (int) BIO_ctrl(bp, cmd, larg, parg); }
static inline long vms_ssl3_SSL_ctrl(SSL *ssl, int cmd, long larg, void *parg)
{ return (long) (int) SSL_ctrl(ssl, cmd, larg, parg); }
static inline long vms_ssl3_SSL_CTX_ctrl(SSL_CTX *ctx, int cmd, long larg, void *parg)
{ return (long) (int) SSL_CTX_ctrl(ctx, cmd, larg, parg); }
static inline long vms_ssl3_SSL_CTX_callback_ctrl(SSL_CTX *ctx, int cmd, void (*fp)(void))
{ return (long) (int) SSL_CTX_callback_ctrl(ctx, cmd, fp); }
static inline long vms_ssl3_SSL_get_verify_result(const SSL *ssl)
{ return (long) (int) SSL_get_verify_result(ssl); }

static inline int vms_ssl3_NCONF_load(CONF *conf, const char *file, long *eline)
{
  int line = 0, r = NCONF_load(conf, file, (long *) &line);
  if (eline)
    *eline = line;
  return r;
}
static inline int vms_ssl3_NCONF_get_number_e(const CONF *conf, const char *group,
                                              const char *name, long *result)
{
  int v = 0, r = NCONF_get_number_e(conf, group, name, (long *) &v);
  if (result)
    *result = v;
  return r;
}

#define BIO_ctrl              vms_ssl3_BIO_ctrl
#define SSL_ctrl              vms_ssl3_SSL_ctrl
#define SSL_CTX_ctrl          vms_ssl3_SSL_CTX_ctrl
#define SSL_CTX_callback_ctrl vms_ssl3_SSL_CTX_callback_ctrl
#define SSL_get_verify_result vms_ssl3_SSL_get_verify_result
#define NCONF_load            vms_ssl3_NCONF_load
#define NCONF_get_number_e    vms_ssl3_NCONF_get_number_e

#endif /* __VMS && __LP64__ */
#endif /* VMS_SSL3_LONG_H */
