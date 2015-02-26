--TEST--
DB_Error/DB_Warning test
--SKIPIF--
<?php if (!include("DB.php")) print "skip"; ?>
--FILE--
<?php // -*- C++ -*-

// Test for: DB.php
// Parts tested: DB_Error, DB_Warning

require_once "DB.php";

print "testing different error codes...\n";
$e = new DB_Error(); print $e->toString()."\n";
$e = new DB_Error("test error"); print $e->toString()."\n";
$e = new DB_Error(DB_OK); print $e->toString()."\n";
$e = new DB_Error(DB_ERROR); print $e->toString()."\n";
$e = new DB_Error(DB_ERROR_SYNTAX); print $e->toString()."\n";
$e = new DB_Error(DB_ERROR_DIVZERO); print $e->toString()."\n";
$e = new DB_Warning(); print $e->toString()."\n";
$e = new DB_Warning("test warning"); print $e->toString()."\n";
$e = new DB_Warning(DB_WARNING_READ_ONLY); print $e->toString()."\n";

print "testing different error modes...\n";
$e = new DB_Error(DB_ERROR, PEAR_ERROR_PRINT); print $e->toString()."\n";
$e = new DB_Error(DB_ERROR_SYNTAX, PEAR_ERROR_TRIGGER);

print "testing different error serverities...\n";
$e = new DB_Error(DB_ERROR_SYNTAX, PEAR_ERROR_TRIGGER, E_USER_NOTICE);
$e = new DB_Error(DB_ERROR_SYNTAX, PEAR_ERROR_TRIGGER, E_USER_WARNING);
$e = new DB_Error(DB_ERROR_SYNTAX, PEAR_ERROR_TRIGGER, E_USER_ERROR);

?>
--GET--
--POST--
--EXPECT--
testing different error codes...
[db_error: message="DB Error: unknown error" code=-1 mode=return level=notice prefix="" prepend="" append="" info=""]
[db_error: message="DB Error: test error" code=-1 mode=return level=notice prefix="" prepend="" append="" info=""]
[db_error: message="DB Error: no error" code=0 mode=return level=notice prefix="" prepend="" append="" info=""]
[db_error: message="DB Error: unknown error" code=-1 mode=return level=notice prefix="" prepend="" append="" info=""]
[db_error: message="DB Error: syntax error" code=-2 mode=return level=notice prefix="" prepend="" append="" info=""]
[db_error: message="DB Error: division by zero" code=-13 mode=return level=notice prefix="" prepend="" append="" info=""]
[db_warning: message="DB Warning: unknown warning" code=-1000 mode=return level=notice prefix="" prepend="" append="" info=""]
[db_warning: message="DB Warning: test warning" code=0 mode=return level=notice prefix="" prepend="" append="" info=""]
[db_warning: message="DB Warning: read only" code=-1001 mode=return level=notice prefix="" prepend="" append="" info=""]
testing different error modes...
DB Error: unknown error[db_error: message="DB Error: unknown error" code=-1 mode=print level=notice prefix="" prepend="" append="" info=""]
<br>
<b>Notice</b>:  DB Error: syntax error in <b>/usr/local/share/php/pear/PEAR.php</b> on line <b>401</b><br>
testing different error serverities...
<br>
<b>Notice</b>:  DB Error: syntax error in <b>/usr/local/share/php/pear/PEAR.php</b> on line <b>401</b><br>
<br>
<b>Warning</b>:  DB Error: syntax error in <b>/usr/local/share/php/pear/PEAR.php</b> on line <b>401</b><br>
<br>
<b>Fatal error</b>:  DB Error: syntax error in <b>/usr/local/share/php/pear/PEAR.php</b> on line <b>401</b><br>
