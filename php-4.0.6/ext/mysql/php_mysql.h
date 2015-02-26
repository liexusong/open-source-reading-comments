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


/* $Id: php_mysql.h,v 1.21 2001/05/05 01:42:15 zeev Exp $ */

#ifndef PHP_MYSQL_H
#define PHP_MYSQL_H

#ifdef PHP_WIN32
#define PHP_MYSQL_API __declspec(dllexport)
#else
#define PHP_MYSQL_API
#endif

#if HAVE_MYSQL

#ifdef ZTS
#include "TSRM.h"
#endif

extern zend_module_entry mysql_module_entry;

#define mysql_module_ptr &mysql_module_entry

extern PHP_MINIT_FUNCTION(mysql);
extern PHP_RINIT_FUNCTION(mysql);
extern PHP_MSHUTDOWN_FUNCTION(mysql);
extern PHP_RSHUTDOWN_FUNCTION(mysql);
PHP_MINFO_FUNCTION(mysql);

PHP_FUNCTION(mysql_connect);
PHP_FUNCTION(mysql_pconnect);
PHP_FUNCTION(mysql_close);
PHP_FUNCTION(mysql_select_db);
PHP_FUNCTION(mysql_create_db);
PHP_FUNCTION(mysql_drop_db);
PHP_FUNCTION(mysql_query);
PHP_FUNCTION(mysql_unbuffered_query);
PHP_FUNCTION(mysql_db_query);
PHP_FUNCTION(mysql_list_dbs);
PHP_FUNCTION(mysql_list_tables);
PHP_FUNCTION(mysql_list_fields);
PHP_FUNCTION(mysql_error);
PHP_FUNCTION(mysql_errno);
PHP_FUNCTION(mysql_affected_rows);
PHP_FUNCTION(mysql_insert_id);
PHP_FUNCTION(mysql_result);
PHP_FUNCTION(mysql_num_rows);
PHP_FUNCTION(mysql_num_fields);
PHP_FUNCTION(mysql_fetch_row);
PHP_FUNCTION(mysql_fetch_array);
PHP_FUNCTION(mysql_fetch_assoc);
PHP_FUNCTION(mysql_fetch_object);
PHP_FUNCTION(mysql_data_seek);
PHP_FUNCTION(mysql_fetch_lengths);
PHP_FUNCTION(mysql_fetch_field);
PHP_FUNCTION(mysql_field_seek);
PHP_FUNCTION(mysql_free_result);
PHP_FUNCTION(mysql_field_name);
PHP_FUNCTION(mysql_field_table);
PHP_FUNCTION(mysql_field_len);
PHP_FUNCTION(mysql_field_type);
PHP_FUNCTION(mysql_field_flags);
PHP_FUNCTION(mysql_escape_string);
PHP_FUNCTION(mysql_get_client_info);
PHP_FUNCTION(mysql_get_host_info);
PHP_FUNCTION(mysql_get_proto_info);
PHP_FUNCTION(mysql_get_server_info);

ZEND_BEGIN_MODULE_GLOBALS(mysql)
	long default_link;
	long num_links,num_persistent;
	long max_links,max_persistent;
	long allow_persistent;
	long default_port;
	char *default_host, *default_user, *default_password;
	char *default_socket;
	char *connect_error;
	long connect_errno;
ZEND_END_MODULE_GLOBALS(mysql)

#ifdef ZTS
# define MySLS_D	zend_mysql_globals *mysql_globals
# define MySLS_DC	, MySLS_D
# define MySLS_C	mysql_globals
# define MySLS_CC , MySLS_C
# define MySG(v) (mysql_globals->v)
# define MySLS_FETCH()	zend_mysql_globals *mysql_globals = ts_resource(mysql_globals_id)
#else
# define MySLS_D
# define MySLS_DC
# define MySLS_C
# define MySLS_CC
# define MySG(v) (mysql_globals.v)
# define MySLS_FETCH()
#endif


#else

#define mysql_module_ptr NULL

#endif

#define phpext_mysql_ptr mysql_module_ptr

#endif /* PHP_MYSQL_H */
