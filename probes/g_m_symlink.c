/* g_m_symlink - php_config.h answer (Phase 1) */
#include <unistd.h>
int main(void){return symlink("a", "b");}
