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
   | Author:                                                              |
   +----------------------------------------------------------------------+
*/

/* $Id: php_link.h,v 1.11 2004/01/08 17:32:51 sniper Exp $ */

#ifndef PHP_LINK_H
#define PHP_LINK_H

#ifdef HAVE_SYMLINK

PHP_FUNCTION(link);
PHP_FUNCTION(readlink);
PHP_FUNCTION(linkinfo);
PHP_FUNCTION(symlink);

#endif

#endif /* PHP_LINK_H */
