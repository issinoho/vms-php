/* g_m_sockaddr_storage - php_config.h answer (Phase 1) */
#include <sys/socket.h>
int main(void){struct sockaddr_storage s; return (int) sizeof s;}
