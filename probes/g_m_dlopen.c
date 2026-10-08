/* g_m_dlopen - php_config.h answer (Phase 1) */
#include <dlfcn.h>
int main(void){void *h = dlopen("x", RTLD_LAZY); return h != 0;}
