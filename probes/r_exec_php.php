<?php
/* r_exec_php.php - exec(), system() and passthru() result codes and output on
   OpenVMS with patch 0012 (PORTING_LOG #35).  Run in [.PHP_PROBE], where
   r_exec_status.exe (probe r_exec_status) calls exit(n).  Commands with '|'
   or '(' run unwrapped (probe r_popen_pipe): pclose()'s value. */
$cmds = ['write sys$output "x"', 'write sys$output "a" ; write sys$output "b"', 'xyzzy',
         'directory nosuch.xyz', 'mcr []r_exec_status exit 0', 'mcr []r_exec_status exit 3',
         'mcr []r_exec_status exit 255', 'pipe write sys$output "a" | search sys$pipe a',
         'pipe (write sys$output "a" | search sys$pipe a)', 'pipe write sys$output "a" ; mcr []r_exec_status exit 4',
         '$ write sys$output "semi ; bar | amp &"', 'exit 3', 'type r_exec_status.c'];
foreach ($cmds as $c) {
    $o = [];
    $l = exec($c, $o, $r);
    echo "RESULT exec [$c] r=$r n=", count($o), " last=", json_encode($l), "\n";
    ob_start(); system($c, $r); $s = ob_get_clean();
    echo "RESULT system r=$r len=", strlen($s), " tail=", json_encode(substr($s, -40)), "\n";
    ob_start(); passthru($c, $r); $p = ob_get_clean();
    echo "RESULT passthru r=$r same=", $p === $s ? 1 : 0, " marker=", strpos($p . $s . implode($o), '@@PHP_EXEC') !== false ? 1 : 0, "\n";
}
