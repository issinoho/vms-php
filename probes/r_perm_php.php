<?php
/* r_perm_php.php - chmod() and is_readable()/is_writable()/is_executable()
   on OpenVMS (the phpt is_*_basic tests, PORTING_LOG #42).  Run in an empty
   scratch directory, with and without SYSPRV/BYPASS/READALL/GRPPRV. */
touch('p.tmp');
foreach ([0, 0100, 0200, 0400, 0600, 0755, 0644] as $m) {
    clearstatcache();
    $r = @chmod('p.tmp', $m);
    $e = $r ? '' : (error_get_last()['message'] ?? '');
    clearstatcache();
    printf("RESULT chmod %04o: %s %s mode %04o r=%d w=%d x=%d\n", $m, var_export($r, true), $e,
           @fileperms('p.tmp') & 0777, is_readable('p.tmp'), is_writable('p.tmp'), is_executable('p.tmp'));
}
@chmod('p.tmp', 0644); unlink('p.tmp');
