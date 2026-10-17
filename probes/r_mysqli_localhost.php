<?php
// r_mysqli_localhost.php: mysqli to "localhost" (a Unix-domain socket in mysqlnd) against
// "127.0.0.1" (TCP); no credentials needed to see the transport fail (PORTING_LOG #62)
mysqli_report(MYSQLI_REPORT_OFF);
echo "mysqli.default_socket = '", ini_get('mysqli.default_socket'), "'\n";
foreach (['localhost', '127.0.0.1'] as $h) {
    $db = @mysqli_connect($h, 'no_such_user', 'x', '', 3306);
    printf("%-10s errno %d: %s\n", $h, mysqli_connect_errno(), mysqli_connect_error());
}
