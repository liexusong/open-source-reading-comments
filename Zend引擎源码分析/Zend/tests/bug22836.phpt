--TEST--
Bug #22836 (returning references to NULL)
--SKIPIF--
<?php if (version_compare(zend_version(), '2.0.0-dev', '<')) die('skip ZendEngine 2 is needed'); ?>
--FILE--
<?php
function &f()
{
	$x = "foo";
	var_dump($x);
	print "'$x'\n";
	return ($a);
}
for ($i = 0; $i < 8; $i++) {
	$h =& f();
}
?>
--EXPECTF--
string(3) "foo"
'foo'

Notice: Undefined variable: a in %s on line %d

Strict Standards: Only variable references should be returned by reference in %s on line %d
string(3) "foo"
'foo'

Notice: Undefined variable: a in %s on line %d

Strict Standards: Only variable references should be returned by reference in %s on line %d
string(3) "foo"
'foo'

Notice: Undefined variable: a in %s on line %d

Strict Standards: Only variable references should be returned by reference in %s on line %d
string(3) "foo"
'foo'

Notice: Undefined variable: a in %s on line %d

Strict Standards: Only variable references should be returned by reference in %s on line %d
string(3) "foo"
'foo'

Notice: Undefined variable: a in %s on line %d

Strict Standards: Only variable references should be returned by reference in %s on line %d
string(3) "foo"
'foo'

Notice: Undefined variable: a in %s on line %d

Strict Standards: Only variable references should be returned by reference in %s on line %d
string(3) "foo"
'foo'

Notice: Undefined variable: a in %s on line %d

Strict Standards: Only variable references should be returned by reference in %s on line %d
string(3) "foo"
'foo'

Notice: Undefined variable: a in %s on line %d

Strict Standards: Only variable references should be returned by reference in %s on line %d
