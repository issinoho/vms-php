$! VMSPHP$SETUP.COM - DCL symbols for PHP for OpenVMS (from LOGIN.COM or SYLOGIN.COM)
$!
$!   $ @VMSPHP$ROOT:[COM]VMSPHP$SETUP
$!   $ php -v
$!   $ php "-r" "echo PHP_VERSION, PHP_EOL;"
$!
$! DCL upper-cases arguments that are not quoted: quote options such as "-r",
$! "-d" and "-T", and anything case-sensitive (ini names, URLs, PHP code).
$ php     :== $VMSPHP$ROOT:[BIN]PHP.EXE
$ php_cgi :== $VMSPHP$ROOT:[BIN]PHP_CGI.EXE
$ exit 1
