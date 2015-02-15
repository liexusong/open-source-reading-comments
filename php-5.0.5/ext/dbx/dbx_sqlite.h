/*
   +----------------------------------------------------------------------+
   | PHP Version 5                                                        |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2004 The PHP Group                                |
   +----------------------------------------------------------------------+
   | dbx module version 1.0                                               |
   +----------------------------------------------------------------------+
   | Copyright (c) 2001-2003 Guidance Rotterdam BV                        |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.0 of the PHP license,       |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_0.txt.                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Author : Marc Boeren         <marc@guidance.nl>                      |
   +----------------------------------------------------------------------+
*/

/* $Id: dbx_sqlite.h,v 1.3 2004/01/08 17:32:01 sniper Exp $ */

#ifndef ZEND_DBX_SQLITE_H
#define ZEND_DBX_SQLITE_H

#ifndef INIT_FUNC_ARGS
#include "zend_modules.h"
#endif

#include "php.h"

int dbx_sqlite_connect(zval **rv, zval **host, zval **db, zval **username, zval **password, INTERNAL_FUNCTION_PARAMETERS);
	/* returns connection handle as resource on success or 0 as long on failure */
int dbx_sqlite_pconnect(zval **rv, zval **host, zval **db, zval **username, zval **password, INTERNAL_FUNCTION_PARAMETERS);
	/* returns persistent connection handle as resource on success or 0 as long on failure */
int dbx_sqlite_close(zval **rv, zval **dbx_handle, INTERNAL_FUNCTION_PARAMETERS);
	/* returns 1 as long on success or 0 as long on failure */
int dbx_sqlite_query(zval **rv, zval **dbx_handle, zval **db_name, zval **sql_statement, INTERNAL_FUNCTION_PARAMETERS);
	/* returns 1 as long or a result identifier as resource on success  or 0 as long on failure */
int dbx_sqlite_getcolumncount(zval **rv, zval **result_handle, INTERNAL_FUNCTION_PARAMETERS);
	/* returns column-count as long on success or 0 as long on failure */
int dbx_sqlite_getcolumnname(zval **rv, zval **result_handle, long column_index, INTERNAL_FUNCTION_PARAMETERS);
	/* returns column-name as string on success or 0 as long on failure */
int dbx_sqlite_getcolumntype(zval **rv, zval **result_handle, long column_index, INTERNAL_FUNCTION_PARAMETERS);
	/* returns column-type as string on success or 0 as long on failure */
int dbx_sqlite_getrow(zval **rv, zval **result_handle, long row_number, INTERNAL_FUNCTION_PARAMETERS);
	/* returns array[0..columncount-1] as strings on success or 0 as long on failure */
int dbx_sqlite_error(zval **rv, zval **dbx_handle, INTERNAL_FUNCTION_PARAMETERS);
	/* returns string */
int dbx_sqlite_esc(zval **rv, zval **dbx_handle, zval **string, INTERNAL_FUNCTION_PARAMETERS);
	/* returns escaped string */

#endif /* ZEND_DBX_SQLITE_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */
