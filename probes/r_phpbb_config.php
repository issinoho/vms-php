<?php
// r_phpbb_config.php: phpBB's config table in the database against its
// cached copy (Phase 4: config:get cookie_domain "does not exist" after the
// MariaDB stall during a cache rebuild, PORTING_LOG #59). Read-only; prints
// row counts and whether the named keys exist, no values.
// usage: php r_phpbb_config.php <phpbb-dir> [key...]
$dir = $argv[1];
$keys = array_slice($argv, 2) ?: ['cookie_domain', 'cookie_name', 'cookie_path', 'cookie_secure', 'server_name'];
include $dir . '/config.php';
mysqli_report(MYSQLI_REPORT_OFF);
$db = mysqli_connect($dbhost, $dbuser, $dbpasswd, $dbname, (int) ($dbport ?: 3306));
if (!$db) { echo "connect failed: ", mysqli_connect_error(), "\n"; exit(1); }
$t = $table_prefix . 'config';
echo "db rows: ", mysqli_query($db, "SELECT COUNT(*) FROM $t")->fetch_row()[0], "\n";
foreach ($keys as $k) {
    $r = mysqli_query($db, "SELECT COUNT(*) FROM $t WHERE config_name = '" . mysqli_real_escape_string($db, $k) . "'")->fetch_row()[0];
    echo "db   $k: ", $r ? 'present' : 'MISSING', "\n";
}
// phpBB's file cache of the config table
foreach (glob($dir . '/cache/production/data_global.php') ?: [] as $f) {
    echo "cache file: ", basename($f), " ", filesize($f), " bytes, ", date('H:i:s', filemtime($f)), "\n";
    $src = file_get_contents($f);
    echo "cache has 'config' entry: ", strpos($src, "'config'") !== false ? 'yes' : 'no', "\n";
    foreach ($keys as $k) echo "cache $k: ", strpos($src, "'$k'") !== false ? 'present' : 'MISSING', "\n";
}
