<?php
/* r_rewrite_php.php - files PHP rewrites keep one version (D13, PORTING_LOG
   #40): file_put_contents() five times, fopen("w") twice, rename() onto an
   existing file twice, then unlink().  Run in an empty scratch directory;
   the procedure lists the versions with DIRECTORY. */
for ($i = 0; $i < 5; $i++) file_put_contents('put.tmp', str_repeat("v$i", 10 - $i));
echo "RESULT put.tmp: ", file_get_contents('put.tmp'), "\n";
for ($i = 0; $i < 2; $i++) { $f = fopen('w.tmp', 'w'); fwrite($f, "w$i"); fclose($f); }
echo "RESULT w.tmp: ", file_get_contents('w.tmp'), "\n";
file_put_contents('target.tmp', 'old');
for ($i = 0; $i < 2; $i++) { file_put_contents('src.tmp', "new$i"); var_dump(rename('src.tmp', 'target.tmp')); }
echo "RESULT target.tmp: ", file_get_contents('target.tmp'), "\n";
file_put_contents('gone.tmp', 'a'); file_put_contents('gone.tmp', 'b');
echo "RESULT unlink: ", var_export(unlink('gone.tmp'), true), " exists ", var_export(file_exists('gone.tmp'), true), "\n";
mkdir('d0777'); echo "RESULT rename dir made with 0777: ", var_export(rename('d0777', 'd0777b'), true), "\n"; rmdir('d0777b');
print_r(scandir('.'));
