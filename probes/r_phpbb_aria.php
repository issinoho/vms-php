<?php
// r_phpbb_aria.php: convert the test board's phpBB tables to Aria (Phase 4,
// PORTING_LOG #60: phpBB creates tables with the server's default engine,
// MyISAM on this server today, and a killed MariaDB left all 64 MyISAM
// tables empty; the board's earlier database, phpbb3319, is Aria). Uses
// phpBB's config.php; prints engine counts, no data.
// usage: php r_phpbb_aria.php <phpbb-dir> [--dry-run]
$dir = $argv[1];
$dry = in_array('--dry-run', $argv, true);
include $dir . '/config.php';
mysqli_report(MYSQLI_REPORT_OFF);
$db = mysqli_connect($dbhost, $dbuser, $dbpasswd, $dbname, (int) ($dbport ?: 3306));
if (!$db) { echo "connect failed: ", mysqli_connect_error(), "\n"; exit(1); }
$aria = false;
foreach (mysqli_query($db, 'SHOW ENGINES')->fetch_all(MYSQLI_ASSOC) as $e) {
    if (strcasecmp($e['Engine'], 'Aria') === 0) $aria = in_array($e['Support'], ['YES', 'DEFAULT'], true);
    if ($e['Support'] === 'DEFAULT') echo "server default engine: {$e['Engine']}\n";
}
if (!$aria) { echo "Aria not available\n"; exit(1); }
$count = function () use ($db, $table_prefix) {
    $c = [];
    foreach (mysqli_query($db, "SHOW TABLE STATUS LIKE '{$table_prefix}%'")->fetch_all(MYSQLI_ASSOC) as $t)
        $c[$t['Engine']][] = $t['Name'];
    return $c;
};
$before = $count();
foreach ($before as $e => $ts) echo "before: ", count($ts), " $e\n";
$failed = 0;
foreach ($before as $e => $ts) {
    if (strcasecmp($e, 'Aria') === 0) continue;
    foreach ($ts as $t) {
        if ($dry) { echo "would convert $t\n"; continue; }
        if (!mysqli_query($db, "ALTER TABLE `$t` ENGINE=Aria")) { $failed++; echo "$t: ", mysqli_error($db), "\n"; }
    }
}
foreach ($count() as $e => $ts) echo "after: ", count($ts), " $e\n";
echo "failed: $failed\n";
