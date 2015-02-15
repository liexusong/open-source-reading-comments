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
   | Authors: Marcus Boerger <helly@php.net>                              |
   +----------------------------------------------------------------------+
 */

/* $Id: spl_directory.h,v 1.6 2004/01/20 20:59:45 helly Exp $ */

#ifndef SPL_DIRECTORY_H
#define SPL_DIRECTORY_H

#include "php.h"
#include "php_spl.h"

extern zend_class_entry *spl_ce_DirectoryIterator;
extern zend_class_entry *spl_ce_RecursiveDirectoryIterator;

PHP_MINIT_FUNCTION(spl_directory);

typedef struct _spl_ce_dir_object {
	zend_object       std;
	php_stream        *dirp;
	php_stream_dirent entry;
	char              *path;
	char              *path_name;
	int               path_name_len;
	int               index;
} spl_ce_dir_object;

#endif /* SPL_DIRECTORY_H */

/*
 * Local Variables:
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 * vim600: fdm=marker
 * vim: noet sw=4 ts=4
 */
