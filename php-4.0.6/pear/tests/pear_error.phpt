--TEST--
PEAR_Error test
--SKIPIF--
--FILE--
<?php // -*- C++ -*-

// Test for: PEAR.php
// Parts tested: - PEAR_Error class
//               - PEAR::isError static method
// testing PEAR_Error

require_once "PEAR.php";

error_reporting(4095);

class Foo_Error extends PEAR_Error {
    function Foo_Error($message = "unknown error", $code = null,
                       $mode = null, $options = null, $userinfo = null)
    {
        $this->PEAR_Error($message, $code, $mode, $options, $userinfo);
        $this->error_message_prefix = 'Foo_Error prefix';
    }
}

class Test1 extends PEAR {
    function Test1() {
        $this->PEAR("Foo_Error");
    }
    function runtest() {
        return $this->raiseError("test error");
    }
}

function errorhandler(&$obj) {
    print "errorhandler function called, obj=".$obj->toString()."\n";
}

class errorclass {
    function errorhandler(&$obj) {
	print "errorhandler method called, obj=".$obj->toString()."\n";
    }
}

print "specify error class: ";
$obj = new Test1;
$err = $obj->runtest();
print $err->toString() . "\n";

$eo = new errorclass;

print "default PEAR_Error: ";
$err = new PEAR_Error;
print $err->toString() . "\n";
print "Testing it: ";
var_dump(PEAR::isError($err));
print "This is not an error: ";
$str = "not an error";
var_dump(PEAR::isError($str));

print "Now trying a bunch of variations...\n";

print "different message: ";
$err = new PEAR_Error("test error");
print $err->toString() . "\n";

print "different message,code: ";
$err = new PEAR_Error("test error", -42);
print $err->toString() . "\n";

print "mode=print: ";
$err = new PEAR_Error("test error", -42, PEAR_ERROR_PRINT);
print $err->toString() . "\n";

print "mode=callback(function): ";
$err = new PEAR_Error("test error", -42, PEAR_ERROR_CALLBACK, "errorhandler");

print "mode=callback(method): ";
$err = new PEAR_Error("test error", -42, PEAR_ERROR_CALLBACK,
                      array(&$eo, "errorhandler"));

print "mode=print&trigger: ";
$err = new PEAR_Error("test error", -42, PEAR_ERROR_PRINT|PEAR_ERROR_TRIGGER);
print $err->toString() . "\n";

print "mode=trigger: ";
$err = new PEAR_Error("test error", -42, PEAR_ERROR_TRIGGER);
print $err->toString() . "\n";

print "mode=trigger,level=notice: ";
$err = new PEAR_Error("test error", -42, PEAR_ERROR_TRIGGER, E_USER_NOTICE);
print $err->toString() . "\n";

print "mode=trigger,level=warning: ";
$err = new PEAR_Error("test error", -42, PEAR_ERROR_TRIGGER, E_USER_WARNING);
print $err->toString() . "\n";

print "mode=trigger,level=error: ";
$err = new PEAR_Error("test error", -42, PEAR_ERROR_TRIGGER, E_USER_ERROR);
print $err->toString() . "\n";

?>
--GET--
--POST--
--EXPECT--
specify error class: [foo_error: message="test error" code=0 mode=return level=notice prefix="Foo_Error prefix" prepend="" append="" info=""]
default PEAR_Error: [pear_error: message="unknown error" code=0 mode=return level=notice prefix="" prepend="" append="" info=""]
Testing it: bool(true)
This is not an error: bool(false)
Now trying a bunch of variations...
different message: [pear_error: message="test error" code=0 mode=return level=notice prefix="" prepend="" append="" info=""]
different message,code: [pear_error: message="test error" code=-42 mode=return level=notice prefix="" prepend="" append="" info=""]
mode=print: test error[pear_error: message="test error" code=-42 mode=print level=notice prefix="" prepend="" append="" info=""]
mode=callback(function): errorhandler function called, obj=[pear_error: message="test error" code=-42 mode=callback callback=errorhandler prefix="" prepend="" append="" info=""]
mode=callback(method): errorhandler method called, obj=[pear_error: message="test error" code=-42 mode=callback callback=errorclass::errorhandler prefix="" prepend="" append="" info=""]
mode=print&trigger: test error<br>
<b>Notice</b>:  test error in <b>/usr/local/share/php/pear/PEAR.php</b> on line <b>401</b><br>
[pear_error: message="test error" code=-42 mode=print|trigger level=notice prefix="" prepend="" append="" info=""]
mode=trigger: <br>
<b>Notice</b>:  test error in <b>/usr/local/share/php/pear/PEAR.php</b> on line <b>401</b><br>
[pear_error: message="test error" code=-42 mode=trigger level=notice prefix="" prepend="" append="" info=""]
mode=trigger,level=notice: <br>
<b>Notice</b>:  test error in <b>/usr/local/share/php/pear/PEAR.php</b> on line <b>401</b><br>
[pear_error: message="test error" code=-42 mode=trigger level=notice prefix="" prepend="" append="" info=""]
mode=trigger,level=warning: <br>
<b>Warning</b>:  test error in <b>/usr/local/share/php/pear/PEAR.php</b> on line <b>401</b><br>
[pear_error: message="test error" code=-42 mode=trigger level=warning prefix="" prepend="" append="" info=""]
mode=trigger,level=error: <br>
<b>Fatal error</b>:  test error in <b>/usr/local/share/php/pear/PEAR.php</b> on line <b>401</b><br>
