1. PHP怎么调用Zend引擎的
========================
Zend引擎其实是一个脚本语言解析器，他可以把PHP脚本代码编译为opcode(一种中间码)，然后再解析这些opcode。<br />
因为Zend引擎需要SAPI来调用才能执行，所以我们看看CLI模块怎么调用Zend引擎的吧(由于我们分析的Zend引擎，所有有些PHP相关的点就不作细说)。<br /><br />
首先PHP会初始化Zend引擎的环境，代码如下：
```C
...
zend_activate();
...
zend_set_timeout();
...
zend_activate_modules();
...
```

zend_activate()是初始化编译器和解析器，代码如下：
```C
void zend_activate(TSRMLS_D)
{
    init_compiler(TSRMLS_C);
    init_executor(TSRMLS_C);
    startup_scanner(TSRMLS_C);
}
```
init_compiler()主要是初始化编译器用到的一些数据结构，而init_executor()主要是初始化解析器用到的一些数据结构。<br/><br/>

zend_set_timeout()设置PHP的超时时间，Linux系统主要使用setitimer()接口设置。<br />
zend_activate_modules()用于初始化所有Zend模块，主要是调用每个Zend模块的request_startup_func()接口。
