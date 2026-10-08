/* g_m_tm_zone - php_config.h answer (Phase 1) */
#include <time.h>
int main(void){struct tm t; return t.tm_zone == 0;}
