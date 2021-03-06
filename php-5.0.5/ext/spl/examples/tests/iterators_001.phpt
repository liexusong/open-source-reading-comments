--TEST--
SPL: EmptyIterator
--FILE--
<?php

require_once('examples.inc');

echo "===EmptyIterator===\n";

foreach(new LimitIterator(new EmptyIterator(), 0, 3) as $key => $val)
{
	echo "$key=>$val\n";
}

?>
===DONE===
<?php exit(0);
--EXPECTF--
===EmptyIterator===
===DONE===
