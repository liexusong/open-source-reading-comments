--TEST--
Bug #28444 (Cannot access undefined property for object with overloaded property access)
--FILE--
<?php

class Object
{
	public $x;

	function __construct($x)
	{
		$this->x = $x;
	}
}

class Overloaded
{
	public $props = array();
	public $x;

	function __construct($x)
	{
		$this->x = new Object($x);
	}

	function __get($prop)
	{
		echo __METHOD__ . "($prop)\n";
		return $this->props[$prop];
	}

	function __set($prop, $val)
	{
		echo __METHOD__ . "($prop,$val)\n";
		$this->props[$prop] = $val;
	}
}
$y = new Overloaded(2);
var_dump($y->x);
var_dump($y->x->x);
var_dump($y->x->x = 3);
var_dump($y->y = 3);
var_dump($y->y);
var_dump($y->z = new Object(4));
var_dump($y->z->x);
$t = $y->z;
var_dump($t->x = 5);
var_dump($y->z->x = 6);

?>
===DONE===
--EXPECTF--
object(Object)#%d (1) {
  ["x"]=>
  int(2)
}
int(2)
int(3)
Overloaded::__set(y,3)
int(3)
Overloaded::__get(y)
int(3)
Overloaded::__set(z,Object id #3)
object(Object)#%d (1) {
  ["x"]=>
  int(4)
}
Overloaded::__get(z)
int(4)
Overloaded::__get(z)
int(5)
Overloaded::__get(z)
int(6)
===DONE===
