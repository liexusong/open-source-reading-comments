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
   | Authors: Frank M. Kromann frank@frontbase.com>                       |
   +----------------------------------------------------------------------+
 */


/* $Id: php_mssql.h,v 1.12 2001/03/13 16:33:39 fmk Exp $ */

#ifndef PHP_MSSQL_H
#define PHP_MSSQL_H


#if HAVE_MSSQL
#ifdef PHP_WIN32
#define PHP_MSSQL_API __declspec(dllexport)
#else
#define PHP_MSSQL_API
#endif


#define MSSQL_VERSION "7.0"
#include "sqlfront.h"
#include "sqldb.h"

#define coltype(j) dbcoltype(mssql_ptr->link,j)
#define intcol(i) ((int) *(DBINT *) dbdata(mssql_ptr->link,i))
#define smallintcol(i) ((int) *(DBSMALLINT *) dbdata(mssql_ptr->link,i))
#define tinyintcol(i) ((int) *(DBTINYINT *) dbdata(mssql_ptr->link,i))
#define anyintcol(j) (coltype(j)==SQLINT4?intcol(j):(coltype(j)==SQLINT2?smallintcol(j):tinyintcol(j)))
#define charcol(i) ((DBCHAR *) dbdata(mssql_ptr->link,i))
#define floatcol(i) ((float) *(DBFLT8 *) dbdata(mssql_ptr->link,i))

#ifdef ZTS
#include "TSRM.h"
#endif

extern zend_module_entry mssql_module_entry;
#define mssql_module_ptr &mssql_module_entry

extern PHP_MINIT_FUNCTION(mssql);
extern PHP_MSHUTDOWN_FUNCTION(mssql);
extern PHP_RINIT_FUNCTION(mssql);
extern PHP_RSHUTDOWN_FUNCTION(mssql);
PHP_MINFO_FUNCTION(mssql);

PHP_FUNCTION(mssql_connect);
PHP_FUNCTION(mssql_pconnect);
PHP_FUNCTION(mssql_close);
PHP_FUNCTION(mssql_select_db);
PHP_FUNCTION(mssql_query);
PHP_FUNCTION(mssql_fetch_batch);
PHP_FUNCTION(mssql_rows_affected);
PHP_FUNCTION(mssql_free_result);
PHP_FUNCTION(mssql_get_last_message);
PHP_FUNCTION(mssql_num_rows);
PHP_FUNCTION(mssql_num_fields);
PHP_FUNCTION(mssql_fetch_field);
PHP_FUNCTION(mssql_fetch_row);
PHP_FUNCTION(mssql_fetch_array);
PHP_FUNCTION(mssql_fetch_object);
PHP_FUNCTION(mssql_field_length);
PHP_FUNCTION(mssql_field_name);
PHP_FUNCTION(mssql_field_type);
PHP_FUNCTION(mssql_data_seek);
PHP_FUNCTION(mssql_field_seek);
PHP_FUNCTION(mssql_result);
PHP_FUNCTION(mssql_next_result);
PHP_FUNCTION(mssql_min_error_severity);
PHP_FUNCTION(mssql_min_message_severity);

typedef struct mssql_link {
	LOGINREC *login;
	DBPROCESS *link;
	int valid;
} mssql_link;

ZEND_BEGIN_MODULE_GLOBALS(mssql)
	long default_link;
	long num_links,num_persistent;
	long max_links,max_persistent;
	long allow_persistent;
	char *appname;
	char *server_message;
	long min_error_severity, min_message_severity;
	long cfg_min_error_severity, cfg_min_message_severity;
	long compatability_mode, connect_timeout;
	void (*get_column_content)(mssql_link *mssql_ptr,int offset,pval *result,int column_type);
	long textsize, textlimit, batchsize;
	HashTable *resource_list, *resource_plist;
ZEND_END_MODULE_GLOBALS(mssql)

#define MSSQL_ROWS_BLOCK 128

typedef struct mssql_field {
	char *name,*column_source;
	long max_length; 
	int numeric;
	int type;
} mssql_field;

typedef struct mssql_result {
	pval **data;
	mssql_field *fields;
	mssql_link *mssql_ptr;
	int batchsize;
	int lastresult;
	int blocks_initialized;
	int cur_row,cur_field;
	int num_rows,num_fields;
} mssql_result;


#ifdef ZTS
# define MSSQLLS_D		zend_mssql_globals *mssql_globals
# define MSSQLLS_DC		, MSSQLLS_D
# define MSSQLLS_C		mssql_globals
# define MSSQLLS_CC		, MSSQLLS_C
# define MS_SQL_G(v)	(mssql_globals->v)
# define MSSQLLS_FETCH()	zend_mssql_globals *mssql_globals = ts_resource(mssql_globals_id)
#else
# define MSSQLLS_D
# define MSSQLLS_DC
# define MSSQLLS_C
# define MSSQLLS_CC
# define MS_SQL_G(v)	(mssql_globals.v)
# define MSSQLLS_FETCH()
#endif

#else

#define mssql_module_ptr NULL

#endif /* HAVE_MSSQL */

#define phpext_mssql_ptr mssql_module_ptr

#endif /* PHP_MSSQL_H */
