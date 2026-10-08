/* g_m_struct_flock - php_config.h answer (Phase 1) */
#include <fcntl.h>
int main(void){struct flock f; f.l_type = F_WRLCK; return f.l_type == 0;}
