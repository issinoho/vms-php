/* g_m_socklen_t - php_config.h answer (Phase 1) */
#include <sys/socket.h>
int main(void){socklen_t n = 0; return (int) n;}
