<?php
// r_dns.php: PHP's DNS functions on the TCP/IP Services resolver (D21,
// patch 0021). The host names are zend_strings above 2 GB, which the
// resolver cannot take; the patch copies them below.
foreach (['checkdnsrr', 'dns_check_record', 'getmxrr', 'dns_get_mx', 'dns_get_record'] as $f) {
    echo $f, ': ', function_exists($f) ? 'yes' : 'NO', "\n";
}
$host = str_repeat('', 0) . 'gmail.com';           // a fresh zend_string
var_dump(checkdnsrr($host, 'MX'));
var_dump(checkdnsrr('example.com', 'A'));
var_dump(checkdnsrr('no-such-domain.invalid', 'MX'));
var_dump(checkdnsrr(str_repeat('a', 300) . '.com', 'MX'));   // longer than any DNS name
$mx = []; $w = [];
var_dump(getmxrr($host, $mx, $w));
array_multisort($w, $mx);
foreach ($mx as $i => $h) echo "  MX $w[$i] $h\n";
foreach (dns_get_record('example.com', DNS_A | DNS_AAAA | DNS_NS | DNS_TXT) as $r) {
    echo '  ', $r['type'], ' ', $r['ip'] ?? $r['ipv6'] ?? $r['target'] ?? substr($r['txt'] ?? '', 0, 40), "\n";
}
var_dump(dns_get_record('no-such-domain.invalid', DNS_MX));
echo "done\n";
