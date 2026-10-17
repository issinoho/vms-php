<?php
// r_smtp_ehlo.php: what the node's SMTP server offers (Phase 4 mail check).
// Connects, prints the greeting and the EHLO reply, and if STARTTLS is
// offered, tries it with phpBB's call (stream_socket_enable_crypto). Sends
// QUIT; no mail. usage: php r_smtp_ehlo.php [host] [port]
$host = $argv[1] ?? 'localhost';
$port = (int) ($argv[2] ?? 25);
$s = @stream_socket_client("$host:$port", $errno, $errstr, 20);
if (!$s) { echo "connect $host:$port failed: $errstr ($errno)\n"; exit(1); }
$read = function () use ($s) {
    $out = '';
    while (($l = fgets($s, 1024)) !== false) { $out .= $l; if ($l[3] === ' ') break; }
    return $out;
};
echo "greeting: ", $read();
fwrite($s, "EHLO " . gethostname() . "\r\n");
$ehlo = $read();
echo "EHLO:\n", $ehlo;
if (stripos($ehlo, 'STARTTLS') !== false) {
    fwrite($s, "STARTTLS\r\n");
    echo "STARTTLS: ", $read();
    $ctx = stream_context_set_option($s, 'ssl', 'verify_peer', false);
    $ok = stream_socket_enable_crypto($s, true, STREAM_CRYPTO_METHOD_TLS_CLIENT);
    echo "TLS: ", var_export($ok, true), "\n";
    if ($ok) {
        $m = stream_get_meta_data($s);
        echo "crypto: ", json_encode($m['crypto'] ?? null), "\n";
        fwrite($s, "EHLO " . gethostname() . "\r\n");
        echo "EHLO over TLS:\n", $read();
    }
}
fwrite($s, "QUIT\r\n");
echo "QUIT: ", $read();
