/* g_m_alloca_builtin - php_config.h answer (Phase 1) */
int main(void){ char *p = __builtin_alloca(64); p[0] = 0; return p[0]; }
