--TEST--
Error message handling (with ZendOptimizer)
--SKIPIF--
<?php
extension_loaded("Zend Optimizer") or die("skip Zend Optimizer is not loaded");
?>
--FILE--
<?php
// If this test fails ask the developers of run-test.php
//
// We check the general ini settings which affect error handling
// and than verify if a message is given by a division by zero.
// EXPECTF is used here since the error format may change but ut 
// should always contain 'Division by zero'.
var_dump(ini_get('display_errors'));
var_dump(ini_get('error_reporting'));
var_dump(ini_get('log_errors'));
var_dump(ini_get('track_errors'));
ini_set('display_errors', 0);
var_dump(ini_get('display_errors'));
var_dump($php_errormsg);
$error = 1 / 0;
var_dump($php_errormsg);
?>
--EXPECTF--
%s: %sivision by zero in %s on line %d
string(1) "1"
string(4) "4095"
string(1) "0"
string(1) "1"
string(1) "0"
string(%d) "%sivision by zer%s"
string(%d) "%sivision by zer%s"
