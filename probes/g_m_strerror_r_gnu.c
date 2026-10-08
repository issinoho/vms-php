/* g_m_strerror_r_gnu - php_config.h answer (Phase 1) */
#include <string.h>
int main(void){char b[64]; char *p = strerror_r(1, b, sizeof b); return p == 0;}
