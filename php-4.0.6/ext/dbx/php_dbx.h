/*
   +----------------------------------------------------------------------+
   | PHP version 4.0                                                      |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2001 The PHP Group                                |
   +----------------------------------------------------------------------+
   | dbx module version 1.0                                               |
   +----------------------------------------------------------------------+
   | Copyright (c) 2001 Guidance Rotterdam BV                             |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.02 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available at through the world-wide-web at                           |
   | http://www.php.net/license/2_02.txt.                                 |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Author : Marc Boeren         <marc@guidance.nl>                      |
   +----------------------------------------------------------------------+
*/

#ifndef ZEND_PHP_DBX_H
#define ZEND_PHP_DBX_H

#ifndef INIT_FUNC_ARGS
#include "zend_modules.h"
#endif

extern zend_module_entry dbx_module_entry;
#define phpext_dbx_ptr &dbx_module_entry

#ifdef ZEND_WIN32
#define ZEND_DBX_API __declspec(dllexport)
#else
#define ZEND_DBX_API
#endif

ZEND_MINIT_FUNCTION(dbx);
ZEND_MSHUTDOWN_FUNCTION(dbx);
/*/ ZEND_RINIT_FUNCTION(dbx); /*/
/*/ ZEND_RSHUTDOWN_FUNCTION(dbx); /*/

ZEND_MINFO_FUNCTION(dbx);

ZEND_FUNCTION(dbx_connect);
ZEND_FUNCTION(dbx_close);
ZEND_FUNCTION(dbx_query);
ZEND_FUNCTION(dbx_error);

ZEND_FUNCTION(dbx_sort);
ZEND_FUNCTION(dbx_cmp_asc);
ZEND_FUNCTION(dbx_cmp_desc);

/* 
  	Declare any global variables you may need between the BEGIN
	and END macros here:     
*/

/*
ZEND_BEGIN_MODULE_GLOBALS(dbx)
ZEND_END_MODULE_GLOBALS(dbx)
*/

/* In every function that needs to use variables in php_dbx_globals,
   do call dbxLS_FETCH(); after declaring other variables used by
   that function, and always refer to them as dbxG(variable).
   You are encouraged to rename these macros something shorter, see
   examples in any other php module directory.
*/

#ifdef ZTS
#define DBXLS_D	zend_dbx_globals *dbx_globals
#define DBXLS_DC	, DBXLS_D
#define DBXLS_C	dbx_globals
#define DBXLS_CC , DBXLS_C
#define DBXG(v) (dbx_globals->v)
#define DBXLS_FETCH() zend_dbx_globals *dbx_globals = ts_resource(dbx_globals_id)
#else
#define DBXLS_D
#define DBXLS_DC
#define DBXLS_C
#define DBXLS_CC
#define DBXG(v) (dbx_globals.v)
#define DBXLS_FETCH()
#endif

#endif	/* ZEND_PHP_DBX_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */
