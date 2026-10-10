<?php
/* r_file_php.php - PHP-level checks behind the phpt file failures
   (PORTING_LOG #36-): fseek past EOF on a stream, flock(), rename() of a
   directory.  Run in an empty scratch directory. */
file_put_contents('seek.tmp', str_repeat('x', 512));
foreach (['r', 'r+', 'a', 'a+'] as $m) {
    $f = fopen('seek.tmp', $m);
    $a = fseek($f, 10, SEEK_END); $p = ftell($f);
    echo "RESULT fseek mode $m: $a ftell $p\n";
    fclose($f);
}
$f = fopen('lock.tmp', 'w');
foreach (['LOCK_SH' => LOCK_SH, 'LOCK_EX' => LOCK_EX, 'LOCK_UN' => LOCK_UN, 'LOCK_EX|NB' => LOCK_EX | LOCK_NB] as $n => $op) {
    $r = @flock($f, $op);
    echo "RESULT flock $n on 'w': ", var_export($r, true), " ", error_get_last()['message'] ?? '', "\n";
}
fclose($f);
$f = fopen('lock.tmp', 'r');
echo "RESULT flock LOCK_SH on 'r': ", var_export(@flock($f, LOCK_SH), true), "\n";
fclose($f);
mkdir('FROM_z');
echo "RESULT rename dir: ", var_export(@rename('FROM_z', 'TO_z'), true), " ", error_get_last()['message'] ?? '', "\n";
@rmdir('FROM_z'); @rmdir('TO_z');
foreach ([0755, 0777] as $m) {
    mkdir('FROM_w', $m);
    echo "RESULT rename dir mode ", decoct($m), ": ", var_export(@rename('FROM_w', 'TO_w'), true), " ", error_get_last()['message'] ?? '', "\n";
    @rmdir('FROM_w'); @rmdir('TO_w');
}
