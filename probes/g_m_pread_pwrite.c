/* g_m_pread_pwrite - php_config.h answer (Phase 1) */
#include <unistd.h>
int main(void){char b[4]; return (int) pread(0, b, 1, 0) + (int) pwrite(1, b, 0, 0);}
