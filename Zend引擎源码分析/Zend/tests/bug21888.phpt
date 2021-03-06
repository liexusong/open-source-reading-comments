--TEST--
Bug #21888 (protected property and protected method of the same name)
--SKIPIF--
<?php 
	if (version_compare(zend_version(), '2.0.0-dev', '<')) die('skip ZendEngine 2 is needed'); 
?>
--FILE--
<?php
class mom {

  protected $prot = "protected property\n";

  protected function prot() {
    print "protected method\n";
  } 
}

class child extends mom {
  
  public function callMom() {
    $this->prot();
  }
  
  public function viewMom() {
    print $this->prot;
  }
  
}

$c = new child();
$c->callMom();
$c->viewMom();
?>
--EXPECT--
protected method
protected property
--EXPECT--
protected method
protected property
