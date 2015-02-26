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
   | Author:  Jim Winstead <jimw@php.net>                                 |
   +----------------------------------------------------------------------+
*/

/* $Id: php_filestat.h,v 1.9 2001/02/26 06:07:23 andi Exp $ */

#ifndef PHP_FILESTAT_H
#define PHP_FILESTAT_H

PHP_RINIT_FUNCTION(filestat);
PHP_RSHUTDOWN_FUNCTION(filestat);

PHP_FUNCTION(clearstatcache);
PHP_FUNCTION(fileatime);
PHP_FUNCTION(filectime);
PHP_FUNCTION(filegroup);
PHP_FUNCTION(fileinode);
PHP_FUNCTION(filemtime);
PHP_FUNCTION(fileowner);
PHP_FUNCTION(fileperms);
PHP_FUNCTION(filesize);
PHP_FUNCTION(filetype);
PHP_FUNCTION(is_writable);
PHP_FUNCTION(is_readable);
PHP_FUNCTION(is_executable);
PHP_FUNCTION(is_file);
PHP_FUNCTION(is_dir);
PHP_FUNCTION(is_link);
PHP_FUNCTION(file_exists);
PHP_NAMED_FUNCTION(php_if_stat);
PHP_NAMED_FUNCTION(php_if_lstat);
PHP_FUNCTION(diskfreespace);
PHP_FUNCTION(chown);
PHP_FUNCTION(chgrp);
PHP_FUNCTION(chmod);
PHP_FUNCTION(touch);
PHP_FUNCTION(clearstatcache);

#endif /* PHP_FILESTAT_H */
