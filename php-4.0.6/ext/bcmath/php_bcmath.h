/* 
   +----------------------------------------------------------------------+
   | PHP version 4.0                                                      |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2001 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.02 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available at through the world-wide-web at                           |
   | http://www.php.net/license/2_02.txt.                                 |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: Andi Gutmans <andi@zend.com>                                |
   +----------------------------------------------------------------------+
*/

/* $Id: php_bcmath.h,v 1.8 2001/02/26 06:06:47 andi Exp $ */

#ifndef PHP_BCMATH_H
#define PHP_BCMATH_H

#if WITH_BCMATH

extern zend_module_entry bcmath_module_entry;
#define phpext_bcmath_ptr &bcmath_module_entry

PHP_MINIT_FUNCTION(bcmath);
PHP_MSHUTDOWN_FUNCTION(bcmath);
PHP_RINIT_FUNCTION(bcmath);
PHP_MINFO_FUNCTION(bcmath);

PHP_FUNCTION(bcadd);
PHP_FUNCTION(bcsub);
PHP_FUNCTION(bcmul);
PHP_FUNCTION(bcdiv);
PHP_FUNCTION(bcmod);
PHP_FUNCTION(bcpow);
PHP_FUNCTION(bcsqrt);
PHP_FUNCTION(bccomp);
PHP_FUNCTION(bcscale);

#else

#define phpext_bcmath_ptr NULL

#endif

#endif /* PHP_BCMATH_H */
