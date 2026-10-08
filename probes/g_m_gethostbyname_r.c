/* g_m_gethostbyname_r - php_config.h answer (Phase 1) */
#include <netdb.h>
int main(void){struct hostent h, *r; char b[512]; int e; return gethostbyname_r("localhost", &h, b, sizeof b, &r, &e);}
