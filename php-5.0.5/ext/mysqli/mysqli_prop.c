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
  | Author: Georg Richter <georg@php.net>                                |
  +----------------------------------------------------------------------+

  $Id: mysqli_prop.c,v 1.14.2.6 2005/03/30 13:52:39 jorton Exp $ 
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <signal.h>

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_mysqli.h"

#define CHECK_OBJECT() \
	if (!obj->valid) { \
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Property access is not allowed yet. Call the default constructor of the object first"); \
		ZVAL_NULL(*retval); \
		return SUCCESS; \
	} \



#define MYSQLI_GET_MYSQL() \
MYSQL *p; \
ALLOC_ZVAL(*retval);\
CHECK_OBJECT(); \
p = (MYSQL *)((MY_MYSQL *)((MYSQLI_RESOURCE *)(obj->ptr))->ptr)->mysql;

#define MYSQLI_GET_RESULT() \
MYSQL_RES *p; \
ALLOC_ZVAL(*retval);\
CHECK_OBJECT(); \
p = (MYSQL_RES *)((MYSQLI_RESOURCE *)(obj->ptr))->ptr

#define MYSQLI_GET_STMT() \
MYSQL_STMT *p; \
ALLOC_ZVAL(*retval);\
CHECK_OBJECT(); \
p = (MYSQL_STMT *)((MY_STMT *)((MYSQLI_RESOURCE *)(obj->ptr))->ptr)->stmt

#define MYSQLI_MAP_PROPERTY_FUNC_LONG( __func, __int_func, __get_type, __ret_type)\
static int __func(mysqli_object *obj, zval **retval TSRMLS_DC) \
{\
	__ret_type l;\
	__get_type;\
	if (!p) {\
		ZVAL_NULL(*retval);\
	} else {\
		l = (__ret_type)__int_func(p);\
		if (l < LONG_MAX) {\
			ZVAL_LONG(*retval, l);\
		} else { \
			char ret[40]; \
			sprintf(ret, "%llu", (my_ulonglong)l); \
			ZVAL_STRING(*retval, ret, 1); \
		} \
	}\
	return SUCCESS;\
}

#define MYSQLI_MAP_PROPERTY_FUNC_STRING(__func, __int_func, __get_type)\
static int __func(mysqli_object *obj, zval **retval TSRMLS_DC)\
{\
	char *c;\
	__get_type;\
	if (!p) {\
		ZVAL_NULL(*retval);\
	} else {\
		c = (char *)__int_func(p);\
		if (!c) {\
			ZVAL_NULL(*retval);\
		} else {\
			ZVAL_STRING(*retval, c, 1);\
		}\
	}\
	return SUCCESS;\
}

/* {{{ property link_client_version_read */
static int link_client_version_read(mysqli_object *obj, zval **retval TSRMLS_DC)
{
	ALLOC_ZVAL(*retval);
	ZVAL_LONG(*retval, MYSQL_VERSION_ID);
	return SUCCESS;
}
/* }}} */

/* {{{ property link_client_info_read */
static int link_client_info_read(mysqli_object *obj, zval **retval TSRMLS_DC)
{
	ALLOC_ZVAL(*retval);
	ZVAL_STRING(*retval, MYSQL_SERVER_VERSION, 1);
	return SUCCESS;
}
/* }}} */

/* {{{ property link_connect_errno_read */
static int link_connect_errno_read(mysqli_object *obj, zval **retval TSRMLS_DC)
{
	ALLOC_ZVAL(*retval);
	ZVAL_LONG(*retval, (long)MyG(error_no));
	return SUCCESS;
}
/* }}} */

/* {{{ property link_connect_error_read */
static int link_connect_error_read(mysqli_object *obj, zval **retval TSRMLS_DC)
{
	ALLOC_ZVAL(*retval);
	ZVAL_STRING(*retval, MyG(error_msg), 1);
	return SUCCESS;
}
/* }}} */

/* {{{ property link_affected_rows_read */
static int link_affected_rows_read(mysqli_object *obj, zval **retval TSRMLS_DC)
{
	MY_MYSQL *mysql;
	my_ulonglong rc;

	ALLOC_ZVAL(*retval); 
	CHECK_OBJECT();

 	mysql = (MY_MYSQL *)((MYSQLI_RESOURCE *)(obj->ptr))->ptr;
	rc = mysql_affected_rows(mysql->mysql);

	if (rc == (my_ulonglong)-1) {
		ZVAL_LONG(*retval, -1);
		return SUCCESS;
	} 

	if (rc < LONG_MAX) {
		ZVAL_LONG(*retval, rc);
	} else {
		char ret[40];
		sprintf(ret, "%llu", (my_ulonglong) rc);
		ZVAL_STRING(*retval, ret, 1);
	}

	return SUCCESS;
}
/* }}} */

/* link properties */
MYSQLI_MAP_PROPERTY_FUNC_LONG(link_errno_read, mysql_errno, MYSQLI_GET_MYSQL(), ulong);
MYSQLI_MAP_PROPERTY_FUNC_STRING(link_error_read, mysql_error, MYSQLI_GET_MYSQL());
MYSQLI_MAP_PROPERTY_FUNC_LONG(link_field_count_read, mysql_field_count, MYSQLI_GET_MYSQL(), ulong);
MYSQLI_MAP_PROPERTY_FUNC_STRING(link_host_info_read, mysql_get_host_info, MYSQLI_GET_MYSQL());
MYSQLI_MAP_PROPERTY_FUNC_STRING(link_info_read, mysql_info, MYSQLI_GET_MYSQL());
MYSQLI_MAP_PROPERTY_FUNC_LONG(link_insert_id_read, mysql_insert_id, MYSQLI_GET_MYSQL(), my_ulonglong);
MYSQLI_MAP_PROPERTY_FUNC_LONG(link_protocol_version_read, mysql_get_proto_info, MYSQLI_GET_MYSQL(), ulong);
MYSQLI_MAP_PROPERTY_FUNC_STRING(link_server_info_read, mysql_get_server_info, MYSQLI_GET_MYSQL());
MYSQLI_MAP_PROPERTY_FUNC_LONG(link_server_version_read, mysql_get_server_version, MYSQLI_GET_MYSQL(), ulong);
MYSQLI_MAP_PROPERTY_FUNC_STRING(link_sqlstate_read, mysql_sqlstate, MYSQLI_GET_MYSQL());
MYSQLI_MAP_PROPERTY_FUNC_LONG(link_thread_id_read, mysql_thread_id, MYSQLI_GET_MYSQL(), ulong);
MYSQLI_MAP_PROPERTY_FUNC_LONG(link_warning_count_read, mysql_warning_count, MYSQLI_GET_MYSQL(), ulong);
/* result properties */

/* {{{ property result_type_read */
static int result_type_read(mysqli_object *obj, zval **retval TSRMLS_DC)
{
	MYSQL_RES *p;

	ALLOC_ZVAL(*retval); 
	CHECK_OBJECT();

 	p = (MYSQL_RES *)((MYSQLI_RESOURCE *)(obj->ptr))->ptr;

	if (!p) {
		ZVAL_NULL(*retval);
	} else {
		ZVAL_LONG(*retval, (p->data) ? MYSQLI_STORE_RESULT : MYSQLI_USE_RESULT);
	}
	return SUCCESS;
}
/* }}} */

/* {{{ property result_lengths_read */
static int result_lengths_read(mysqli_object *obj, zval **retval TSRMLS_DC)
{
	MYSQL_RES *p;

	ALLOC_ZVAL(*retval); 
	CHECK_OBJECT();

	p = (MYSQL_RES *)((MYSQLI_RESOURCE *)(obj->ptr))->ptr;

	if (!p || !p->field_count) {
		ZVAL_NULL(*retval);
	} else {
		ulong i;
		zval *l;
	
		array_init(*retval);

		for (i=0; i < p->field_count; i++) {
			MAKE_STD_ZVAL(l);
			ZVAL_LONG(l, p->lengths[i]);
			add_index_zval(*retval, i, l);
		}	
	}
	return SUCCESS;
}
/* }}} */


MYSQLI_MAP_PROPERTY_FUNC_LONG(result_current_field_read, mysql_field_tell, MYSQLI_GET_RESULT(), ulong);
MYSQLI_MAP_PROPERTY_FUNC_LONG(result_field_count_read, mysql_num_fields, MYSQLI_GET_RESULT(), ulong);
MYSQLI_MAP_PROPERTY_FUNC_LONG(result_num_rows_read, mysql_num_rows, MYSQLI_GET_RESULT(), my_ulonglong);

/* statement properties */

/* {{{ property stmt_id_read */
static int stmt_id_read(mysqli_object *obj, zval **retval TSRMLS_DC)
{
	MY_STMT *p;

	ALLOC_ZVAL(*retval); 
	CHECK_OBJECT();

 	p = (MY_STMT*)((MYSQLI_RESOURCE *)(obj->ptr))->ptr;

	if (!p) {
		ZVAL_NULL(*retval);
	} else {
		ZVAL_LONG(*retval, p->stmt->stmt_id);
	}
	return SUCCESS;
}
/* }}} */

/* {{{ property stmt_affected_rows_read */
static int stmt_affected_rows_read(mysqli_object *obj, zval **retval TSRMLS_DC)
{
	MY_STMT *stmt;
	my_ulonglong rc;

	ALLOC_ZVAL(*retval); 
	CHECK_OBJECT();

 	stmt = (MY_STMT *)((MYSQLI_RESOURCE *)(obj->ptr))->ptr;
	rc = mysql_stmt_affected_rows(stmt->stmt);

	if (rc == (my_ulonglong)-1) {
		ZVAL_LONG(*retval, -1);
		return SUCCESS;
	} 

	if (rc < LONG_MAX) {
		ZVAL_LONG(*retval, rc);
	} else {
		char ret[40];
		sprintf(ret, "%llu", (my_ulonglong) rc);
		ZVAL_STRING(*retval, ret, 1);
	}

	return SUCCESS;
}
/* }}} */

MYSQLI_MAP_PROPERTY_FUNC_LONG(stmt_insert_id_read, mysql_stmt_insert_id, MYSQLI_GET_STMT(), my_ulonglong);
MYSQLI_MAP_PROPERTY_FUNC_LONG(stmt_num_rows_read, mysql_stmt_num_rows, MYSQLI_GET_STMT(), my_ulonglong);
MYSQLI_MAP_PROPERTY_FUNC_LONG(stmt_param_count_read, mysql_stmt_param_count, MYSQLI_GET_STMT(), ulong);
MYSQLI_MAP_PROPERTY_FUNC_LONG(stmt_field_count_read, mysql_stmt_field_count, MYSQLI_GET_STMT(), ulong);
MYSQLI_MAP_PROPERTY_FUNC_LONG(stmt_errno_read, mysql_stmt_errno, MYSQLI_GET_STMT(), ulong);
MYSQLI_MAP_PROPERTY_FUNC_STRING(stmt_error_read, mysql_stmt_error, MYSQLI_GET_STMT());
MYSQLI_MAP_PROPERTY_FUNC_STRING(stmt_sqlstate_read, mysql_stmt_sqlstate, MYSQLI_GET_STMT());

mysqli_property_entry mysqli_link_property_entries[] = {
	{"affected_rows", link_affected_rows_read, NULL},
	{"client_info", link_client_info_read, NULL},
	{"client_version", link_client_version_read, NULL},
	{"connect_errno", link_connect_errno_read, NULL},
	{"connect_error", link_connect_error_read, NULL},
	{"errno", link_errno_read, NULL},
	{"error", link_error_read, NULL},
	{"field_count", link_field_count_read, NULL},
	{"host_info", link_host_info_read, NULL},
	{"info", link_info_read, NULL},
	{"insert_id", link_insert_id_read, NULL},
	{"server_info", link_server_info_read, NULL},
	{"server_version", link_server_version_read, NULL},
	{"sqlstate", link_sqlstate_read, NULL},
	{"protocol_version", link_protocol_version_read, NULL},
	{"thread_id", link_thread_id_read, NULL},
	{"warning_count", link_warning_count_read, NULL},
	{NULL, NULL, NULL}	
};

mysqli_property_entry mysqli_result_property_entries[] = {
	{"current_field", result_current_field_read, NULL},
	{"field_count", result_field_count_read, NULL},
	{"lengths", result_lengths_read, NULL},
	{"num_rows", result_num_rows_read, NULL},
	{"type", result_type_read, NULL},
	{NULL, NULL, NULL}
};

mysqli_property_entry mysqli_stmt_property_entries[] = {
	{"affected_rows", stmt_affected_rows_read, NULL},
	{"insert_id", stmt_insert_id_read, NULL},
	{"num_rows", stmt_num_rows_read, NULL},
	{"param_count", stmt_param_count_read, NULL},
	{"field_count", stmt_field_count_read, NULL},
	{"errno", stmt_errno_read, NULL},
	{"error", stmt_error_read, NULL},
	{"sqlstate", stmt_sqlstate_read, NULL},
	{"id", stmt_id_read, NULL},
	{NULL, NULL, NULL}
};

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
