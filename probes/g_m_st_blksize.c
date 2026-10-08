/* g_m_st_blksize - php_config.h answer (Phase 1) */
#include <sys/stat.h>
int main(void){struct stat s; return (int) sizeof s.st_blksize;}
