<?php
// r_leak_cgi: run as  php_cgi -T <n> r_leak_cgi.php  (n requests, no FastCGI).
// Logs this process's page-file quota (PAGFILCNT, pagelets) at requests 1,
// 1000 and 3000, through a DCL f$getjpi on our own PID; a counter file keeps
// the request number across requests.
$cf = 'r_leak_cgi.cnt';
$n = (int)@file_get_contents($cf) + 1;
file_put_contents($cf, $n);
if ($n == 1 || $n == 1000 || $n == 3000) {
    $pid = sprintf('%08X', getmypid());
    exec("write sys\$output f\$getjpi(\"$pid\",\"PAGFILCNT\")", $o);
    file_put_contents('r_leak_cgi.log', "$n " . trim(implode(' ', $o)) . "\n", FILE_APPEND);
}
