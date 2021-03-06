/* 
   +----------------------------------------------------------------------+
   | PHP Version 5                                                        |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2004 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.0 of the PHP license,       |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_0.txt.                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: Andi Gutmans <andi@zend.com>                                |
   |          Zeev Suraski <zeev@zend.com>                                |
   +----------------------------------------------------------------------+
*/

/* $Id: datetime.h,v 1.16 2004/01/08 17:32:51 sniper Exp $ */

#ifndef DATETIME_H
#define DATETIME_H

PHP_FUNCTION(time);
PHP_FUNCTION(mktime);
PHP_FUNCTION(gmmktime);
PHP_FUNCTION(date);
PHP_FUNCTION(idate);
PHP_FUNCTION(gmdate);
PHP_FUNCTION(localtime);
PHP_FUNCTION(getdate);
PHP_FUNCTION(checkdate);
#if HAVE_STRFTIME
PHP_FUNCTION(strftime);
PHP_FUNCTION(gmstrftime);
#endif
PHP_FUNCTION(strtotime);

int php_idate(char format, int timestamp, int gm);
extern char *php_std_date(time_t t TSRMLS_DC);
void php_mktime(INTERNAL_FUNCTION_PARAMETERS, int gm);
#if HAVE_STRFTIME
void _php_strftime(INTERNAL_FUNCTION_PARAMETERS, int gm);
#endif

#endif /* DATETIME_H */
