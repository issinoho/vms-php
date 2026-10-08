/* g_m_asm_goto - php_config.h answer (Phase 1) */
int main(void){ asm goto ("jmp %l0" :::: out); return 1; out: return 0; }
