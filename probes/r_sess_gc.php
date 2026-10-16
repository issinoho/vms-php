<?php
// r_sess_gc: cost of session GC (ps_files_cleanup_dir) in a large ODS-5
// directory. Creates N sess_* files, ages half of them, then times
// readdir alone, readdir + stat, and the unlink of the aged half.
// Usage: php r_sess_gc.php <dir> [N]
$dir = rtrim($argv[1], '/'); $n = (int)($argv[2] ?? 10000);
@mkdir($dir, 0700);
$t = microtime(true);
for ($i = 0; $i < $n; $i++) {
    $f = sprintf('%s/sess_%026x', $dir, crc32("x$i") * 1000003 + $i);
    file_put_contents($f, 'a|i:1;');
    if ($i % 2) touch($f, time() - 3000);
}
printf("create %d: %.1f s\n", $n, microtime(true) - $t);
$t = microtime(true); $c = 0;
$d = opendir($dir); while (($e = readdir($d)) !== false) $c++; closedir($d);
printf("readdir %d entries: %.1f s\n", $c, microtime(true) - $t);
$t = microtime(true); $old = [];
$d = opendir($dir);
while (($e = readdir($d)) !== false) {
    if (strncmp($e, 'sess_', 5)) continue;
    $s = stat("$dir/$e");
    if ($s['mtime'] < time() - 1440) $old[] = "$dir/$e";
}
closedir($d);
printf("readdir+stat: %.1f s, %d expired\n", microtime(true) - $t, count($old));
$t = microtime(true);
foreach ($old as $i => $f) {
    unlink($f);
    if ($i == 499) printf("  first 500 unlinks: %.1f s\n", microtime(true) - $t);
}
printf("unlink %d: %.1f s\n", count($old), microtime(true) - $t);
$t = microtime(true);
foreach (glob("$dir/sess_*") as $f) unlink($f);
printf("cleanup: %.1f s\n", microtime(true) - $t);
