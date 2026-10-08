/* g_m_strerror_r_posix - php_config.h answer (Phase 1) */
#include <string.h>
int main(void){char b[64]; int r = strerror_r(1, b, sizeof b); return r;}
