<?php
/* r_perm_seq.php - is_readable_basic's sequence: chmod() through modes 0..8
   by absolute path, is_readable() after each (PORTING_LOG #42).  Run
   unprivileged in an empty scratch directory. */
$f = getcwd() . '/s.tmp';
touch($f);
for ($m = 0; $m <= 8; $m++) {
    $r = @chmod($f, $m);
    $e = $r ? '' : error_get_last()['message'];
    clearstatcache();
    printf("RESULT mode %04o: chmod %s %s, r=%d exists=%d\n", $m, var_export($r, true), $e, is_readable($f), file_exists($f));
}
@chmod($f, 0644); @unlink($f);
