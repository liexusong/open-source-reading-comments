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
   | Authors: Brian Schaffner <brian@tool.net>                            |
   |          Shane Caraveo <shane@caraveo.com>                           |
   |          Zeev Suraski <zeev@zend.com>                                |
   +----------------------------------------------------------------------+
*/

/* $Id: dl.h,v 1.19 2004/01/08 17:32:51 sniper Exp $ */

#ifndef DL_H
#define DL_H

void php_dl(pval *file,int type,pval *return_value TSRMLS_DC);


/* dynamic loading functions */
PHP_FUNCTION(dl);

PHP_MINFO_FUNCTION(dl);

#endif /* DL_H */
