<?php
// r_argon2_cross.php: password hashes made by one PHP verified by another (D24).
// The live board's passwords were hashed with argon2id by the PHP 8.1 kit; our
// build must verify them, and hashes it makes must verify under 8.1 too.
// usage: php r_argon2_cross.php hash <file>     write argon2i, argon2id, 2y hashes
//        php r_argon2_cross.php verify <file>   verify them (and a wrong password)
[$mode, $file] = [$argv[1] ?? '', $argv[2] ?? 'r_argon2_hashes.txt'];
$pw = 'correct horse battery staple';
echo PHP_VERSION, ' algos: ', implode(',', password_algos()), "\n";
if ($mode === 'hash') {
    $out = [];
    foreach (['argon2i' => PASSWORD_ARGON2I, 'argon2id' => PASSWORD_ARGON2ID, '2y' => PASSWORD_BCRYPT] as $n => $a)
        $out[] = "$n " . password_hash($pw, $a);
    // phpBB's own parameters for argon2 (memory 64 MiB, time 4, threads 1: PHP's defaults)
    file_put_contents($file, implode("\n", $out) . "\n");
    echo implode("\n", $out), "\n";
} elseif ($mode === 'verify') {
    foreach (file($file, FILE_IGNORE_NEW_LINES | FILE_SKIP_EMPTY_LINES) as $l) {
        [$n, $h] = explode(' ', $l, 2);
        printf("%-9s right: %s  wrong: %s  info: %s\n", $n,
               var_export(password_verify($pw, $h), true),
               var_export(password_verify($pw . 'x', $h), true),
               json_encode(password_get_info($h)['algoName']));
    }
} else {
    echo "usage: hash|verify <file>\n";
    exit(2);
}
