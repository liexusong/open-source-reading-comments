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
   | Authors: Andrei Zmievski <andrei@ispi.net>                           |
   +----------------------------------------------------------------------+
 */
 
/* $Id: php_pcre.h,v 1.26 2001/03/13 05:24:03 andrei Exp $ */

#ifndef PHP_PCRE_H
#define PHP_PCRE_H

#if HAVE_PCRE || HAVE_BUNDLED_PCRE

#if HAVE_BUNDLED_PCRE
#include "pcrelib/pcre.h"
#else
#include "pcre.h"
#endif

#if HAVE_LOCALE_H
#include <locale.h>
#endif

PHP_FUNCTION(preg_match);
PHP_FUNCTION(preg_match_all);
PHP_FUNCTION(preg_replace);
PHP_FUNCTION(preg_replace_callback);
PHP_FUNCTION(preg_split);
PHP_FUNCTION(preg_quote);
PHP_FUNCTION(preg_grep);

extern zend_module_entry pcre_module_entry;
#define pcre_module_ptr &pcre_module_entry

typedef struct {
	pcre *re;
	pcre_extra *extra;
	int preg_options;
#if HAVE_SETLOCALE
	char *locale;
	unsigned const char *tables;
#endif
} pcre_cache_entry;

typedef struct {
	HashTable pcre_cache;
} php_pcre_globals;

#ifdef ZTS
# define PCRE_LS_D 	php_pcre_globals *pcre_globals
# define PCRE_LS_DC , PCRE_LS_D
# define PCRE_LS_C	pcre_globals
# define PCRE_LS_CC	, PCRE_LS_C
# define PCRE_G(v) 	(pcre_globals->v)
# define PCRE_LS_FETCH() php_pcre_globals *pcre_globals = ts_resource(pcre_globals_id);
#else
# define PCRE_LS_D
# define PCRE_LS_DC
# define PCRE_LS_C
# define PCRE_LS_CC
# define PCRE_G(v)	(pcre_globals.v)
# define PCRE_LS_FETCH()
extern ZEND_API php_pcre_globals pcre_globals;
#endif

#else

#define pcre_module_ptr NULL

#endif /* HAVE_PCRE || HAVE_BUNDLED_PCRE */

#define phpext_pcre_ptr pcre_module_ptr

#endif /* PHP_PCRE_H */
