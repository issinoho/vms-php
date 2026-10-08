/* g_m_getifaddrs - php_config.h answer (Phase 1) */
#include <ifaddrs.h>
int main(void){struct ifaddrs *a; return getifaddrs(&a);}
