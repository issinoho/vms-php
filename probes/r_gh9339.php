<?php
/* r_gh9339.php - why gh9339 gives no "Path for oid_file option must be a
   valid file path" on OpenVMS (PORTING_LOG #45): is the config read, and
   what does expand_filepath() do with a 9,000-character path? */
$cnf = getcwd() . '/gh.cnf';
file_put_contents($cnf, sprintf("oid_file = %s\n[ req ]\ndefault_bits = 1024\n", getcwd() . '/' . str_repeat('a', 9000)));
echo "RESULT cnf size ", filesize($cnf), "\n";
$k = openssl_pkey_new(['config' => $cnf]);
echo "RESULT pkey_new ", $k ? 'key' : 'false', "\n";
while ($e = openssl_error_string()) echo "RESULT err $e\n";
file_put_contents($cnf, "oid_file = " . getcwd() . "/nosuch.oid\n[ req ]\ndefault_bits = 1024\n");
$k = @openssl_pkey_new(['config' => $cnf]);
echo "RESULT short path: ", $k ? 'key' : 'false', " ", error_get_last()['message'] ?? '', "\n";
echo "RESULT realpath long: ", var_export(@realpath(getcwd() . '/' . str_repeat('a', 9000)), true), "\n";
unlink($cnf);
