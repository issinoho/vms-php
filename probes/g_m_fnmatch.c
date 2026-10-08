/* g_m_fnmatch - php_config.h answer (Phase 1) */
#include <fnmatch.h>
int main(void){return fnmatch("*.php", "a.php", 0);}
