--TEST--
SPL: ArrayIterator implementing RecursiveIterator
--SKIPIF--
<?php if (!extension_loaded("spl")) print "skip"; ?>
--FILE--
<?php

class RecursiceArrayIterator extends ArrayIterator implements RecursiveIterator
{
	function hasChildren()
	{
		return is_array($this->current());
	}
	
	function getChildren()
	{
		return new RecursiceArrayIterator($this->current());
	}
}

$array = array(1, 2 => array(21, 22 => array(221, 222), 23 => array(231)), 3);

$dir = new RecursiveIteratorIterator(new RecursiceArrayIterator($array), RIT_LEAVES_ONLY);

foreach ($dir as $file) {
	print "$file\n";
}

?>
===DONE===
<?php exit(0); ?>
--EXPECT--
1
21
221
222
231
3
===DONE===
