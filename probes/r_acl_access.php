<?php
// r_acl_access.php: does access() honour an ACL? opcache.file_cache is refused when
// stat + access(R_OK|W_OK|X_OK) fails (PORTING_LOG #61). Run without privileges.
$d = '/vmsphp$root/tmp/opcache';
var_dump(is_dir($d), is_readable($d), is_writable($d));
$f = $d . '/r_acl_probe.txt';
var_dump(@file_put_contents($f, "x"));          // an actual write through the ACL
@unlink($f);
