<?php
/* r_dns_empty.php - gethostbynamel("") hibernates on OpenVMS (probe
   r_zpp_find; arginfo_zpp_mismatch, PORTING_LOG #49).  Case argv[1], timed;
   run each with -d max_execution_time=20. */
$c = (int) $argv[1];
$t = microtime(true);
$r = match ($c) {
    0 => gethostbyname(''),
    1 => gethostbynamel(''),
    2 => gethostbyname('nosuch.invalid'),
    3 => gethostbynamel('nosuch.invalid'),
    4 => gethostbynamel('localhost'),
    5 => dns_get_record('', DNS_A),
};
printf("RESULT case %d: %s in %.2f s\n", $c, json_encode($r), microtime(true) - $t);
