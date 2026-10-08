/* g_m_lchown - php_config.h answer (Phase 1) */
#include <unistd.h>
int main(void){return lchown("x", 0, 0);}
