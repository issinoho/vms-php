/* g_m_tm_gmtoff - php_config.h answer (Phase 1) */
#include <time.h>
int main(void){struct tm t; return (int) sizeof t.tm_gmtoff;}
