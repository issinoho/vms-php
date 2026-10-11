<?php
/* r_mtime.php - file times against time() on OpenVMS (PORTING_LOG #48):
   touch($f, 100) reads back as 3700 in the phpt touch tests, an hour off
   (BST, UTC+1).  Is it stat() or utime() that is off, and for every file?
   Run in an empty scratch directory. */
$f = getcwd() . '/m.tmp';
$t0 = time();
file_put_contents($f, 'x');
clearstatcache();
echo "RESULT new file: filemtime - time() = ", filemtime($f) - $t0, "\n";
foreach ([100, 1000000000, 1780000000, 1795000000, time()] as $t) {
    touch($f, $t, $t + 7);
    clearstatcache();
    $s = stat($f);
    printf("RESULT touch(%d, atime +7): mtime %+d atime %+d (%s)\n", $t, $s['mtime'] - $t, $s['atime'] - $t, gmdate('Y-m-d H:i', $t));
}
echo "RESULT date default ", date_default_timezone_get(), " TZ env [", getenv('TZ'), "] SYS\$TIMEZONE_NAME [", getenv('SYS$TIMEZONE_NAME'), "] diff [", getenv('SYS$TIMEZONE_DIFFERENTIAL'), "]\n";
unlink($f);
