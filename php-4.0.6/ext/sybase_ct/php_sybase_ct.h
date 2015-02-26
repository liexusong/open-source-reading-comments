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
   | Authors: Zeev Suraski <zeev@zend.com>                                |
   +----------------------------------------------------------------------+
*/

/* $Id: php_sybase_ct.h,v 1.10 2001/02/26 06:07:24 andi Exp $ */

#ifndef PHP_SYBASE_CT_H
#define PHP_SYBASE_CT_H

#if HAVE_SYBASE_CT

#define CTLIB_VERSION CS_VERSION_100

extern zend_module_entry sybase_module_entry;
#define sybase_module_ptr &sybase_module_entry

PHP_MINIT_FUNCTION(sybase);
PHP_MSHUTDOWN_FUNCTION(sybase);
PHP_RINIT_FUNCTION(sybase);
PHP_RSHUTDOWN_FUNCTION(sybase);
PHP_MINFO_FUNCTION(sybase);

PHP_FUNCTION(sybase_connect);
PHP_FUNCTION(sybase_pconnect);
PHP_FUNCTION(sybase_close);
PHP_FUNCTION(sybase_select_db);
PHP_FUNCTION(sybase_query);
PHP_FUNCTION(sybase_free_result);
PHP_FUNCTION(sybase_get_last_message);
PHP_FUNCTION(sybase_num_rows);
PHP_FUNCTION(sybase_num_fields);
PHP_FUNCTION(sybase_fetch_row);
PHP_FUNCTION(sybase_fetch_array);
PHP_FUNCTION(sybase_fetch_object);
PHP_FUNCTION(sybase_data_seek);
PHP_FUNCTION(sybase_result);
PHP_FUNCTION(sybase_affected_rows);
PHP_FUNCTION(sybase_field_seek);
PHP_FUNCTION(sybase_min_client_severity);
PHP_FUNCTION(sybase_min_server_severity);
PHP_FUNCTION(sybase_fetch_field);


#include <ctpublic.h>

ZEND_BEGIN_MODULE_GLOBALS(sybase)
	long default_link;
	long num_links,num_persistent;
	long max_links,max_persistent;
	long allow_persistent;
	char *appname;
	char *hostname;
	char *server_message;
	long min_server_severity, min_client_severity;
	CS_CONTEXT *context;
ZEND_END_MODULE_GLOBALS(sybase)

typedef struct {
	CS_CONNECTION *connection;
	CS_COMMAND *cmd;
	int valid;
	int deadlock;
	int dead;
	long affected_rows;
} sybase_link;

#define SYBASE_ROWS_BLOCK 128

typedef struct {
	char *name,*column_source;
	int max_length, numeric;
	CS_INT type;
} sybase_field;

typedef struct {
	pval **data;
	sybase_field *fields;
	sybase_link *sybase_ptr;
	int cur_row,cur_field;
	int num_rows,num_fields;
} sybase_result;


#ifdef ZTS
# define SybCtLS_D	zend_sybase_globals *sybase_globals
# define SybCtLS_DC	, SybCtLS_D
# define SybCtLS_C	sybase_globals
# define SybCtLS_CC , SybCtLS_C
# define SybCtG(v) (sybase_globals->v)
# define SybCtLS_FETCH()	zend_sybase_globals *sybase_globals = ts_resource(sybase_globals_id)
#else
# define SybCtLS_D
# define SybCtLS_DC
# define SybCtLS_C
# define SybCtLS_CC
# define SybCtG(v) (sybase_globals.v)
# define SybCtLS_FETCH()
#endif

#else

#define sybase_module_ptr NULL

#endif

#define phpext_sybase_ct_ptr sybase_module_ptr

#endif /* PHP_SYBASE_CT_H */
