<?php
// r_https_timing.php: where the ~8.5 s of an HTTPS request from PHP goes (Phase 4:
// file_get_contents("https://...") with verification took 8.5 s per request).
// Times DNS (gethostbynamel, getaddrinfo via stream), TCP connect, the TLS
// handshake and the request, separately. usage: php r_https_timing.php [host]
$host = $argv[1] ?? 'version.phpbb.com';
$t = microtime(true);
$lap = function ($what) use (&$t) { $n = microtime(true); printf("%-34s %7.3f s\n", $what, $n - $t); $t = $n; };
$ips = gethostbynamel($host);
$lap('gethostbynamel ' . implode(',', $ips ?: []));
$ip = $ips[0] ?? $host;
$s = @stream_socket_client("tcp://$ip:443", $en, $es, 30);
$lap("tcp connect to $ip:443" . ($s ? '' : " FAILED $es"));
if ($s) {
    stream_context_set_option($s, 'ssl', 'peer_name', $host);
    stream_context_set_option($s, 'ssl', 'verify_peer', true);
    $ok = @stream_socket_enable_crypto($s, true, STREAM_CRYPTO_METHOD_TLS_CLIENT);
    $lap('tls handshake (verify, cafile)' . ($ok ? '' : ' FAILED'));
    if ($ok) {
        fwrite($s, "HEAD / HTTP/1.1\r\nHost: $host\r\nConnection: close\r\n\r\n");
        $first = fgets($s);
        $lap('request, first line: ' . trim($first));
    }
    fclose($s);
}
$s = @stream_socket_client("tcp://$host:443", $en, $es, 30);
$lap("tcp connect by name (getaddrinfo)" . ($s ? '' : " FAILED $es"));
if ($s) fclose($s);
$r = @file_get_contents("https://$host/", false, stream_context_create(['http' => ['method' => 'HEAD', 'timeout' => 30]]));
$lap('file_get_contents https (whole)');
