<?php
// r_db_connect.php: how long mysqli connects to the board's MariaDB take
// (Phase 4: two hard timeouts struck inside phpBB's mysqli_real_connect).
// (microtime: hrtime() is false on VMS, PORTING_LOG #58.) Reads phpBB's config.php for the credentials and prints only timings.
// usage: php r_db_connect.php <phpbb-dir> [n]
$dir = $argv[1];
$n = (int) ($argv[2] ?? 50);
include $dir . '/config.php';
mysqli_report(MYSQLI_REPORT_OFF);
$t = [];
for ($i = 0; $i < $n; $i++) {
    $t0 = microtime(true);
    $db = mysqli_init();
    $ok = mysqli_real_connect($db, $dbhost, $dbuser, $dbpasswd, $dbname, (int) ($dbport ?: 3306));
    $t[] = $ms = (microtime(true) - $t0) * 1e3;
    if (!$ok) { echo "connect $i failed: ", mysqli_connect_error(), "\n"; continue; }
    $q0 = microtime(true);
    mysqli_query($db, 'SELECT 1');
    $qms = (microtime(true) - $q0) * 1e3;
    if ($ms > 500 || $qms > 500) printf("slow: connect %d %.0f ms, query %.0f ms\n", $i, $ms, $qms);
    mysqli_close($db);
}
sort($t);
printf("%d connects: min %.1f ms, median %.1f ms, p95 %.1f ms, max %.1f ms\n",
       $n, $t[0], $t[intdiv($n, 2)], $t[(int) ($n * 0.95)], $t[$n - 1]);
