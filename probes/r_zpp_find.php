<?php
/* r_zpp_find.php - which call hangs Zend/tests/arginfo_zpp_mismatch on
   OpenVMS (PORTING_LOG #49)?  The test's calls, with each function's name
   appended to zpp_find.log before it is called.  Run with
   -d max_execution_time=N in the test's directory (for the .inc). */
require getcwd() . "/arginfo_zpp_mismatch.inc";
function test($function) {
    if (skipFunction($function)) return;
    $name = is_string($function) ? $function : get_class($function[0]) . "::$function[1]";
    file_put_contents(__DIR__ . '/zpp_find.log', "$name\n", FILE_APPEND);
    ob_start();
    for ($n = 0; $n <= 8; $n++) {
        try { @$function(...array_fill(0, $n, null)); } catch (Throwable) {}
    }
    ob_end_clean();
}
foreach (get_defined_functions()["internal"] as $function) test($function);
foreach (get_declared_classes() as $class) {
    try { $obj = (new ReflectionClass($class))->newInstanceWithoutConstructor(); } catch (Throwable) { continue; }
    foreach (get_class_methods($class) as $method) test([$obj, $method]);
}
file_put_contents(__DIR__ . '/zpp_find.log', "===DONE===\n", FILE_APPEND);
