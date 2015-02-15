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

  $Id: mysqli.c,v 1.43.2.17 2005/06/27 18:22:00 tony2001 Exp $ 
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <signal.h>

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "ext/standard/php_string.h"
#include "php_mysqli.h"
#include "zend_exceptions.h"

#define MYSQLI_STORE_RESULT 0
#define MYSQLI_USE_RESULT 1

ZEND_DECLARE_MODULE_GLOBALS(mysqli)
static zend_object_handlers mysqli_object_handlers;
static HashTable classes;
static HashTable mysqli_link_properties;
static HashTable mysqli_result_properties;
static HashTable mysqli_stmt_properties;
extern void php_mysqli_connect(INTERNAL_FUNCTION_PARAMETERS);

typedef int (*mysqli_read_t)(mysqli_object *obj, zval **retval TSRMLS_DC);
typedef int (*mysqli_write_t)(mysqli_object *obj, zval *newval TSRMLS_DC);

typedef struct _mysqli_prop_handler {
	mysqli_read_t read_func;
	mysqli_write_t write_func;
} mysqli_prop_handler;

/* {{{ php_free_stmt_bind_buffer */
void php_free_stmt_bind_buffer(BIND_BUFFER bbuf, int type)
{
	unsigned int i;

	if (!bbuf.var_cnt) {
		return;
	}

	if (bbuf.is_null) {
		efree(bbuf.is_null);
	}

	for (i=0; i < bbuf.var_cnt; i++) {

		/* free temporary bind buffer */
		if (type == FETCH_RESULT && bbuf.buf[i].val) {
			efree(bbuf.buf[i].val);
		}

		if (bbuf.vars[i]) {
			zval_ptr_dtor(&bbuf.vars[i]);
		}	
	}

	if (bbuf.vars) {
		efree(bbuf.vars);
	}

	if (type == FETCH_RESULT) {
		efree(bbuf.buf);
	}
	bbuf.var_cnt = 0;
	return;
}
/* }}} */

/* {{{ php_clear_stmt_bind */
void php_clear_stmt_bind(MY_STMT *stmt)
{
	if (stmt->stmt) {
		mysql_stmt_close(stmt->stmt);
	}

	php_free_stmt_bind_buffer(stmt->param, FETCH_SIMPLE);
	php_free_stmt_bind_buffer(stmt->result, FETCH_RESULT);

	if (stmt->query) {
		efree(stmt->query);
	}
	efree(stmt);
	return;
}
/* }}} */

/* {{{ php_clear_mysql */
void php_clear_mysql(MY_MYSQL *mysql) {
	if (mysql->li_read) {
		efree(Z_STRVAL_P(mysql->li_read));
		FREE_ZVAL(mysql->li_read);
		mysql->li_read = NULL;
	}
}
/* }}} */

/* {{{ mysqli_objects_free_storage
 */
static void mysqli_objects_free_storage(zend_object *object TSRMLS_DC)
{
	mysqli_object 	*intern = (mysqli_object *)object;
	
	zend_objects_free_object_storage(&(intern->zo) TSRMLS_CC);
}
/* }}} */

/* {{{ mysqli_objects_destroy_object
 */
static void mysqli_objects_destroy_object(void *object, zend_object_handle handle TSRMLS_DC)
{
	mysqli_object 	*intern = (mysqli_object *)object;
	MYSQLI_RESOURCE	*my_res = (MYSQLI_RESOURCE *)intern->ptr;

	zend_objects_destroy_object(object, handle TSRMLS_CC);

	/* link object */
	if (instanceof_function(intern->zo.ce, mysqli_link_class_entry TSRMLS_CC)) {
		if (my_res && my_res->ptr) {
			MY_MYSQL *mysql = (MY_MYSQL *)my_res->ptr;
		
			if (mysql->mysql) {
				mysql_close(mysql->mysql);
			}

			php_clear_mysql(mysql);
			efree(mysql);

			my_res->ptr = NULL;
		}
	} else if (intern->zo.ce == mysqli_stmt_class_entry) { /* stmt object */
		if (my_res && my_res->ptr) {
			php_clear_stmt_bind((MY_STMT *)my_res->ptr);
		}
	} else if (intern->zo.ce == mysqli_result_class_entry) { /* result object */
		if (my_res && my_res->ptr) {
			mysql_free_result(my_res->ptr);
		}
	}
	intern->ptr = NULL;
	my_efree(my_res);
}
/* }}} */

/* {{{ mysqli_read_na */
static int mysqli_read_na(mysqli_object *obj, zval **retval TSRMLS_DC)
{
	*retval = NULL;
	php_error_docref(NULL TSRMLS_CC, E_ERROR, "Cannot read property");
	return FAILURE;
}
/* }}} */

/* {{{ mysqli_write_na */
static int mysqli_write_na(mysqli_object *obj, zval *newval TSRMLS_DC)
{
	php_error_docref(NULL TSRMLS_CC, E_ERROR, "Cannot write property");
	return FAILURE;
}
/* }}} */

/* {{{ mysqli_read_property */
zval *mysqli_read_property(zval *object, zval *member, int type TSRMLS_DC)
{
	zval tmp_member;
	zval *retval;
	mysqli_object *obj;
	mysqli_prop_handler *hnd;
	zend_object_handlers *std_hnd;
	int ret;

	ret = FAILURE;
	obj = (mysqli_object *)zend_objects_get_address(object TSRMLS_CC);

 	if (member->type != IS_STRING) {
		tmp_member = *member;
		zval_copy_ctor(&tmp_member);
		convert_to_string(&tmp_member);
		member = &tmp_member;
	}

	if (obj->prop_handler != NULL) {
		ret = zend_hash_find(obj->prop_handler, Z_STRVAL_P(member), Z_STRLEN_P(member)+1, (void **) &hnd);
	}
	if (ret == SUCCESS) {

		/* check if mysqli object is still valid */
		if (!strcmp(obj->zo.ce->name, "mysqli")) {
			if (!obj->ptr ||
		    	!((MYSQL *)((MY_MYSQL *)((MYSQLI_RESOURCE *)(obj->ptr))->ptr)->mysql)->thread_id) {
				retval = EG(uninitialized_zval_ptr);
				return(retval);
			}
		} else
		/* check if stmt object is still valid */
		if (!strcmp(obj->zo.ce->name, "mysqli_stmt")) {
			if (!obj->ptr ||
		    	!((MYSQL_STMT *)((MY_STMT *)((MYSQLI_RESOURCE *)(obj->ptr))->ptr)->stmt)->mysql) {
				retval = EG(uninitialized_zval_ptr);
				return(retval);
			}
		}

		ret = hnd->read_func(obj, &retval TSRMLS_CC);
		if (ret == SUCCESS) {
			/* ensure we're creating a temporary variable */
			retval->refcount = 0;
		} else {
			retval = EG(uninitialized_zval_ptr);
		}
	} else {
		std_hnd = zend_get_std_object_handlers();
		retval = std_hnd->read_property(object, member, type TSRMLS_CC);
	}

	if (member == &tmp_member) {
		zval_dtor(member);
	}
	return(retval);
}
/* }}} */

/* {{{ mysqli_write_property */
void mysqli_write_property(zval *object, zval *member, zval *value TSRMLS_DC)
{
	zval tmp_member;
	mysqli_object *obj;
	mysqli_prop_handler *hnd;
	zend_object_handlers *std_hnd;
	int ret;

 	if (member->type != IS_STRING) {
		tmp_member = *member;
		zval_copy_ctor(&tmp_member);
		convert_to_string(&tmp_member);
		member = &tmp_member;
	}

	ret = FAILURE;
	obj = (mysqli_object *)zend_objects_get_address(object TSRMLS_CC);

	if (obj->prop_handler != NULL) {
		ret = zend_hash_find((HashTable *)obj->prop_handler, Z_STRVAL_P(member), Z_STRLEN_P(member)+1, (void **) &hnd);
	}
	if (ret == SUCCESS) {
		hnd->write_func(obj, value TSRMLS_CC);
		if (! PZVAL_IS_REF(value) && value->refcount == 0) {
			value->refcount++;
			zval_ptr_dtor(&value);
		}
	} else {
		std_hnd = zend_get_std_object_handlers();
		std_hnd->write_property(object, member, value TSRMLS_CC);
	}

	if (member == &tmp_member) {
		zval_dtor(member);
	}
}
/* }}} */

/* {{{ void mysqli_add_property(HashTable *h, char *pname, mysqli_read_t r_func, mysqli_write_t w_func TSRMLS_DC) */
void mysqli_add_property(HashTable *h, char *pname, mysqli_read_t r_func, mysqli_write_t w_func TSRMLS_DC) {
	mysqli_prop_handler		p;

	p.read_func = (r_func) ? r_func : mysqli_read_na; 
	p.write_func = (w_func) ? w_func : mysqli_write_na;

	zend_hash_add(h, pname, strlen(pname) + 1, &p, sizeof(mysqli_prop_handler), NULL);
}
/* }}} */

static union _zend_function *php_mysqli_constructor_get(zval *object TSRMLS_DC)
{
	mysqli_object *obj = (mysqli_object *)zend_objects_get_address(object TSRMLS_CC);

	if (obj->zo.ce != mysqli_link_class_entry) {
		return obj->zo.ce->constructor;
	} else {
		static zend_internal_function f;

		f.function_name = mysqli_link_class_entry->name;
		f.scope = mysqli_link_class_entry;
		f.arg_info = NULL;
		f.num_args = 0;
		f.fn_flags = 0;

		f.type = ZEND_INTERNAL_FUNCTION;
		f.handler = ZEND_FN(mysqli_connect);
	
		return (union _zend_function*)&f;
	}
}
/* {{{ mysqli_objects_new
 */
PHP_MYSQLI_EXPORT(zend_object_value) mysqli_objects_new(zend_class_entry *class_type TSRMLS_DC)
{
	zend_object_value retval;
	mysqli_object *intern;
	zval *tmp;
	zend_class_entry *mysqli_base_class;

	intern = emalloc(sizeof(mysqli_object));
	memset(intern, 0, sizeof(mysqli_object));
	intern->zo.ce = class_type;
	intern->zo.in_get = 0;
	intern->zo.in_set = 0;
	intern->ptr = NULL;
	intern->valid = 0;
	intern->prop_handler = NULL;

	mysqli_base_class = class_type;
	while (mysqli_base_class->type != ZEND_INTERNAL_CLASS && mysqli_base_class->parent != NULL)
	{
		mysqli_base_class = mysqli_base_class->parent;
	}
	zend_hash_find(&classes, mysqli_base_class->name, mysqli_base_class->name_length + 1, 
					(void **) &intern->prop_handler);

	ALLOC_HASHTABLE(intern->zo.properties);
	zend_hash_init(intern->zo.properties, 0, NULL, ZVAL_PTR_DTOR, 0);
	zend_hash_copy(intern->zo.properties, &class_type->default_properties, (copy_ctor_func_t) zval_add_ref,
					(void *) &tmp, sizeof(zval *));

	retval.handle = zend_objects_store_put(intern, (zend_objects_store_dtor_t) mysqli_objects_destroy_object, (zend_objects_free_object_storage_t) mysqli_objects_free_storage, NULL TSRMLS_CC);
	retval.handlers = &mysqli_object_handlers;

	return retval;
}
/* }}} */
	
/* {{{ mysqli_module_entry
 */
zend_module_entry mysqli_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
	STANDARD_MODULE_HEADER,
#endif
	"mysqli",
	mysqli_functions,
	PHP_MINIT(mysqli),
	PHP_MSHUTDOWN(mysqli),
	PHP_RINIT(mysqli),		/* Replace with NULL if there's nothing to do at request start */
	PHP_RSHUTDOWN(mysqli),	/* Replace with NULL if there's nothing to do at request end */
	PHP_MINFO(mysqli),
#if ZEND_MODULE_API_NO >= 20010901
	"0.1", /* Replace with version number for your extension */
#endif
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_MYSQLI
ZEND_GET_MODULE(mysqli)
#endif

/* {{{ PHP_INI_BEGIN
*/
PHP_INI_BEGIN()
	STD_PHP_INI_ENTRY_EX("mysqli.max_links",			"-1",	PHP_INI_SYSTEM,		OnUpdateLong,		max_links,			zend_mysqli_globals,		mysqli_globals, display_link_numbers)
	STD_PHP_INI_ENTRY("mysqli.default_host",			NULL,	PHP_INI_ALL,		OnUpdateString,		default_host,		zend_mysqli_globals,		mysqli_globals)
	STD_PHP_INI_ENTRY("mysqli.default_user",			NULL,	PHP_INI_ALL,		OnUpdateString,		default_user,		zend_mysqli_globals,		mysqli_globals)
	STD_PHP_INI_ENTRY("mysqli.default_pw",				NULL,	PHP_INI_ALL,		OnUpdateString,		default_pw,			zend_mysqli_globals,		mysqli_globals)
	STD_PHP_INI_ENTRY("mysqli.default_port",			"3306",	PHP_INI_ALL,		OnUpdateLong,		default_port,		zend_mysqli_globals,		mysqli_globals)
	STD_PHP_INI_ENTRY("mysqli.default_socket",			NULL,	PHP_INI_ALL,		OnUpdateStringUnempty,	default_socket,	zend_mysqli_globals,		mysqli_globals)
	STD_PHP_INI_BOOLEAN("mysqli.reconnect",				"0",	PHP_INI_SYSTEM,		OnUpdateLong,		reconnect,			zend_mysqli_globals,		mysqli_globals)
PHP_INI_END()

/* }}} */

/* {{{ php_mysqli_init_globals
 */
static void php_mysqli_init_globals(zend_mysqli_globals *mysqli_globals)
{
	mysqli_globals->num_links = 0;
	mysqli_globals->max_links = 0;
	mysqli_globals->default_port = 0;
	mysqli_globals->default_host = NULL;
	mysqli_globals->default_user = NULL;
	mysqli_globals->default_pw = NULL;
	mysqli_globals->default_socket = NULL;
	mysqli_globals->reconnect = 0;
	mysqli_globals->report_mode = 0;
	mysqli_globals->report_ht = 0;
}
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(mysqli)
{
	zend_class_entry *ce;
	zend_object_handlers *std_hnd = zend_get_std_object_handlers();
	
	ZEND_INIT_MODULE_GLOBALS(mysqli, php_mysqli_init_globals, NULL);
	REGISTER_INI_ENTRIES();

	memcpy(&mysqli_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	mysqli_object_handlers.clone_obj = NULL;
	mysqli_object_handlers.read_property = mysqli_read_property;
	mysqli_object_handlers.write_property = mysqli_write_property;
	mysqli_object_handlers.get_property_ptr_ptr = std_hnd->get_property_ptr_ptr;
	mysqli_object_handlers.get_constructor = php_mysqli_constructor_get;

	zend_hash_init(&classes, 0, NULL, NULL, 1);

	REGISTER_MYSQLI_CLASS_ENTRY("mysqli", mysqli_link_class_entry, mysqli_link_methods);

	ce = mysqli_link_class_entry;
	zend_hash_init(&mysqli_link_properties, 0, NULL, NULL, 1);
	MYSQLI_ADD_PROPERTIES(&mysqli_link_properties, mysqli_link_property_entries);
	zend_hash_add(&classes, ce->name, ce->name_length+1, &mysqli_link_properties, sizeof(mysqli_link_properties), NULL);

	REGISTER_MYSQLI_CLASS_ENTRY("mysqli_result", mysqli_result_class_entry, mysqli_result_methods);
	ce = mysqli_result_class_entry;
	zend_hash_init(&mysqli_result_properties, 0, NULL, NULL, 1);
	MYSQLI_ADD_PROPERTIES(&mysqli_result_properties, mysqli_result_property_entries);
	zend_hash_add(&classes, ce->name, ce->name_length+1, &mysqli_result_properties, sizeof(mysqli_result_properties), NULL);

	REGISTER_MYSQLI_CLASS_ENTRY("mysqli_stmt", mysqli_stmt_class_entry, mysqli_stmt_methods);
	ce = mysqli_stmt_class_entry;
	zend_hash_init(&mysqli_stmt_properties, 0, NULL, NULL, 1);
	MYSQLI_ADD_PROPERTIES(&mysqli_stmt_properties, mysqli_stmt_property_entries);
	zend_hash_add(&classes, ce->name, ce->name_length+1, &mysqli_stmt_properties, sizeof(mysqli_stmt_properties), NULL);
	
	/* mysqli_options */
	REGISTER_LONG_CONSTANT("MYSQLI_READ_DEFAULT_GROUP", MYSQL_READ_DEFAULT_GROUP, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLI_READ_DEFAULT_FILE", MYSQL_READ_DEFAULT_FILE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLI_OPT_CONNECT_TIMEOUT", MYSQL_OPT_CONNECT_TIMEOUT, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLI_OPT_LOCAL_INFILE", MYSQL_OPT_LOCAL_INFILE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLI_INIT_COMMAND", MYSQL_INIT_COMMAND, CONST_CS | CONST_PERSISTENT);

	/* mysqli_real_connect flags */	
	REGISTER_LONG_CONSTANT("MYSQLI_CLIENT_SSL", CLIENT_SSL, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLI_CLIENT_COMPRESS",CLIENT_COMPRESS, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLI_CLIENT_INTERACTIVE", CLIENT_INTERACTIVE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLI_CLIENT_IGNORE_SPACE", CLIENT_IGNORE_SPACE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLI_CLIENT_NO_SCHEMA", CLIENT_NO_SCHEMA, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLI_CLIENT_FOUND_ROWS", CLIENT_FOUND_ROWS, CONST_CS | CONST_PERSISTENT);

	/* for mysqli_query */
	REGISTER_LONG_CONSTANT("MYSQLI_STORE_RESULT", 0, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLI_USE_RESULT", 1, CONST_CS | CONST_PERSISTENT);

	/* for mysqli_fetch_assoc */
	REGISTER_LONG_CONSTANT("MYSQLI_ASSOC", MYSQLI_ASSOC, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLI_NUM", MYSQLI_NUM, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLI_BOTH", MYSQLI_BOTH, CONST_CS | CONST_PERSISTENT);
	
	/* for mysqli_stmt_set_attr */
	REGISTER_LONG_CONSTANT("MYSQLI_STMT_ATTR_UPDATE_MAX_LENGTH", STMT_ATTR_UPDATE_MAX_LENGTH, CONST_CS | CONST_PERSISTENT);

#if MYSQL_VERSION_ID > 50003
	REGISTER_LONG_CONSTANT("MYSQLI_STMT_ATTR_CURSOR_TYPE", STMT_ATTR_CURSOR_TYPE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLI_CURSOR_TYPE_NO_CURSOR", CURSOR_TYPE_NO_CURSOR, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLI_CURSOR_TYPE_READ_ONLY", CURSOR_TYPE_READ_ONLY, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLI_CURSOR_TYPE_FOR_UPDATE", CURSOR_TYPE_FOR_UPDATE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLI_CURSOR_TYPE_SCROLLABLE", CURSOR_TYPE_SCROLLABLE, CONST_CS | CONST_PERSISTENT);
#endif
	
	/* column information */
	REGISTER_LONG_CONSTANT("MYSQLI_NOT_NULL_FLAG", NOT_NULL_FLAG, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLI_PRI_KEY_FLAG", PRI_KEY_FLAG, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLI_UNIQUE_KEY_FLAG", UNIQUE_KEY_FLAG, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLI_MULTIPLE_KEY_FLAG", MULTIPLE_KEY_FLAG, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLI_BLOB_FLAG", BLOB_FLAG, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLI_UNSIGNED_FLAG", UNSIGNED_FLAG, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLI_ZEROFILL_FLAG", ZEROFILL_FLAG, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLI_AUTO_INCREMENT_FLAG", AUTO_INCREMENT_FLAG, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLI_TIMESTAMP_FLAG", TIMESTAMP_FLAG, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLI_SET_FLAG", SET_FLAG, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLI_NUM_FLAG", NUM_FLAG, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLI_PART_KEY_FLAG", PART_KEY_FLAG, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLI_GROUP_FLAG", GROUP_FLAG, CONST_CS | CONST_PERSISTENT);

	REGISTER_LONG_CONSTANT("MYSQLI_TYPE_DECIMAL", FIELD_TYPE_DECIMAL, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLI_TYPE_TINY", FIELD_TYPE_TINY, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLI_TYPE_SHORT", FIELD_TYPE_SHORT, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLI_TYPE_LONG", FIELD_TYPE_LONG, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLI_TYPE_FLOAT", FIELD_TYPE_FLOAT, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLI_TYPE_DOUBLE", FIELD_TYPE_DOUBLE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLI_TYPE_NULL", FIELD_TYPE_NULL, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLI_TYPE_TIMESTAMP", FIELD_TYPE_TIMESTAMP, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLI_TYPE_LONGLONG", FIELD_TYPE_LONGLONG, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLI_TYPE_INT24", FIELD_TYPE_INT24, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLI_TYPE_DATE", FIELD_TYPE_DATE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLI_TYPE_TIME", FIELD_TYPE_TIME, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLI_TYPE_DATETIME", FIELD_TYPE_DATETIME	, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLI_TYPE_YEAR", FIELD_TYPE_YEAR, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLI_TYPE_NEWDATE", FIELD_TYPE_NEWDATE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLI_TYPE_ENUM", FIELD_TYPE_ENUM, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLI_TYPE_SET", FIELD_TYPE_SET, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLI_TYPE_TINY_BLOB", FIELD_TYPE_TINY_BLOB, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLI_TYPE_MEDIUM_BLOB", FIELD_TYPE_MEDIUM_BLOB, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLI_TYPE_LONG_BLOB", FIELD_TYPE_LONG_BLOB, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLI_TYPE_BLOB", FIELD_TYPE_BLOB, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLI_TYPE_VAR_STRING", FIELD_TYPE_VAR_STRING, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLI_TYPE_STRING", FIELD_TYPE_STRING, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLI_TYPE_CHAR", FIELD_TYPE_CHAR, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLI_TYPE_INTERVAL", FIELD_TYPE_INTERVAL, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLI_TYPE_GEOMETRY", FIELD_TYPE_GEOMETRY, CONST_CS | CONST_PERSISTENT);

	/* replication */
	REGISTER_LONG_CONSTANT("MYSQLI_RPL_MASTER", MYSQL_RPL_MASTER, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLI_RPL_SLAVE", MYSQL_RPL_SLAVE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLI_RPL_ADMIN", MYSQL_RPL_ADMIN, CONST_CS | CONST_PERSISTENT);
	
	/* bind blob support */
	REGISTER_LONG_CONSTANT("MYSQLI_NO_DATA", MYSQL_NO_DATA, CONST_CS | CONST_PERSISTENT);
#ifdef MYSQL_DATA_TRUNCATED
	REGISTER_LONG_CONSTANT("MYSQLI_DATA_TRUNCATED", MYSQL_DATA_TRUNCATED, CONST_CS | CONST_PERSISTENT);
#endif

	/* reporting */
	REGISTER_LONG_CONSTANT("MYSQLI_REPORT_INDEX", MYSQLI_REPORT_INDEX, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLI_REPORT_ERROR", MYSQLI_REPORT_ERROR, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLI_REPORT_ALL", MYSQLI_REPORT_ALL, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLI_REPORT_OFF", 0, CONST_CS | CONST_PERSISTENT);

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(mysqli)
{
	zend_hash_destroy(&mysqli_link_properties);
	zend_hash_destroy(&mysqli_result_properties);
	zend_hash_destroy(&mysqli_stmt_properties);
	zend_hash_destroy(&classes);

	UNREGISTER_INI_ENTRIES();
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(mysqli)
{
	MyG(error_msg) = NULL;
	MyG(error_no) = 0;

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(mysqli)
{
	if (MyG(error_msg)) {
		efree(MyG(error_msg));
	}
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(mysqli)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "MysqlI Support", "enabled");
	php_info_print_table_row(2, "Client API version", mysql_get_client_info());
	php_info_print_table_row(2, "MYSQLI_SOCKET", MYSQL_UNIX_ADDR);
	
	
	php_info_print_table_end();

	DISPLAY_INI_ENTRIES();
}
/* }}} */

/* {{{ php_mysqli_fetch_into_hash
 */
void php_mysqli_fetch_into_hash(INTERNAL_FUNCTION_PARAMETERS, int override_flags, int into_object)
{
	MYSQL_RES		*result;
	zval			*mysql_result;
	long			fetchtype;
	unsigned int	i;
	MYSQL_FIELD		*fields;
	MYSQL_ROW		row;
	unsigned long	*field_len;
	zval            *ctor_params = NULL;
	zend_class_entry *ce = NULL;

	if (into_object) {
		char *class_name;
		int class_name_len;

		if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O|sz", &mysql_result, mysqli_result_class_entry, &class_name, &class_name_len, &ctor_params) == FAILURE) {
			return;
		}
		if (ZEND_NUM_ARGS() < (getThis() ? 1 : 2)) {
			ce = zend_standard_class_def;
		} else {
			ce = zend_fetch_class(class_name, class_name_len, ZEND_FETCH_CLASS_AUTO TSRMLS_CC);
		}
		if (!ce) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Could not find class '%s'", class_name);
			return;
		}
		fetchtype = MYSQLI_ASSOC;
	} else {
		if (override_flags) {
			if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O", &mysql_result, mysqli_result_class_entry) == FAILURE) {
				return;
			}
			fetchtype = override_flags;
		} else {
			fetchtype = MYSQLI_BOTH;
			if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O|l", &mysql_result, mysqli_result_class_entry, &fetchtype) == FAILURE) {
				return;
			}
		}
	}

	MYSQLI_FETCH_RESOURCE(result, MYSQL_RES *, &mysql_result, "mysqli_result"); 

	if ((fetchtype & MYSQLI_BOTH) == 0) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "The result type should be either MYSQLI_NUM, MYSQLI_ASSOC or MYSQLI_BOTH");
		RETURN_FALSE;
	}

	if (!(row = mysql_fetch_row(result))) {
		RETURN_NULL();
	}

	if (fetchtype & MYSQLI_ASSOC) {
		fields = mysql_fetch_fields(result);
	}

	array_init(return_value);
	field_len = mysql_fetch_lengths(result);

	for (i = 0; i < mysql_num_fields(result); i++) {
		if (row[i]) {
			zval *res;

			MAKE_STD_ZVAL(res);

			/* check if we need magic quotes */
			if (PG(magic_quotes_runtime)) {
				Z_TYPE_P(res) = IS_STRING;
				Z_STRVAL_P(res) = php_addslashes(row[i], field_len[i], &Z_STRLEN_P(res), 0 TSRMLS_CC);
			} else {
				ZVAL_STRINGL(res, row[i], field_len[i], 1);	
			}

			if (fetchtype & MYSQLI_NUM) {
				add_index_zval(return_value, i, res);
			}
			if (fetchtype & MYSQLI_ASSOC) {
				if (fetchtype & MYSQLI_NUM) {
					ZVAL_ADDREF(res);
				}
				add_assoc_zval(return_value, fields[i].name, res);
			}
		} else {
			if (fetchtype & MYSQLI_NUM) {
				add_index_null(return_value, i);
			}
			if (fetchtype & MYSQLI_ASSOC) {
				add_assoc_null(return_value, fields[i].name);
			}
		}
	}

	if (into_object) {
		zval dataset = *return_value;
		zend_fcall_info fci;
		zend_fcall_info_cache fcc;
		zval *retval_ptr; 
	
		object_and_properties_init(return_value, ce, NULL);
		zend_merge_properties(return_value, Z_ARRVAL(dataset), 1 TSRMLS_CC);
	
		if (ce->constructor) {
			fci.size = sizeof(fci);
			fci.function_table = &ce->function_table;
			fci.function_name = NULL;
			fci.symbol_table = NULL;
			fci.object_pp = &return_value;
			fci.retval_ptr_ptr = &retval_ptr;
			if (ctor_params && Z_TYPE_P(ctor_params) != IS_NULL) {
				if (Z_TYPE_P(ctor_params) == IS_ARRAY) {
					HashTable *ht = Z_ARRVAL_P(ctor_params);
					Bucket *p;
	
					fci.param_count = 0;
					fci.params = safe_emalloc(sizeof(zval*), ht->nNumOfElements, 0);
					p = ht->pListHead;
					while (p != NULL) {
						fci.params[fci.param_count++] = (zval**)p->pData;
						p = p->pListNext;
					}
				} else {
					/* Two problems why we throw exceptions here: PHP is typeless
					 * and hence passing one argument that's not an array could be
					 * by mistake and the other way round is possible, too. The 
					 * single value is an array. Also we'd have to make that one
					 * argument passed by reference.
					 */
					zend_throw_exception(zend_exception_get_default(), "Parameter ctor_params must be an array", 0 TSRMLS_CC);
					return;
				}
			} else {
				fci.param_count = 0;
				fci.params = NULL;
			}
			fci.no_separation = 1;

			fcc.initialized = 1;
			fcc.function_handler = ce->constructor;
			fcc.calling_scope = EG(scope);
			fcc.object_pp = &return_value;
		
			if (zend_call_function(&fci, &fcc TSRMLS_CC) == FAILURE) {
				zend_throw_exception_ex(zend_exception_get_default(), 0 TSRMLS_CC, "Could not execute %s::%s()", ce->name, ce->constructor->common.function_name);
			} else {
				if (retval_ptr) {
					zval_ptr_dtor(&retval_ptr);
				}
			}
			if (fci.params) {
				efree(fci.params);
			}
		} else if (ctor_params) {
			zend_throw_exception_ex(zend_exception_get_default(), 0 TSRMLS_CC, "Class %s does not have a constructor hence you cannot use ctor_params", ce->name);
		}
	}
}
/* }}} */

/* {{{ php_mysqli_set_error
 */
PHP_MYSQLI_API void php_mysqli_set_error(long mysql_errno, char *mysql_err TSRMLS_DC)
{
	MyG(error_no) = mysql_errno;
	if (MyG(error_msg)) {
		efree(MyG(error_msg));
	}
	MyG(error_msg) = estrdup(mysql_err);
}
/* }}} */

#define ALLOC_CALLBACK_ARGS(a, b, c)\
if (c) {\
	a = (zval ***)safe_emalloc(c, sizeof(zval **), 0);\
	for (i = b; i < c; i++) {\
		a[i] = emalloc(sizeof(zval *));\
		MAKE_STD_ZVAL(*a[i]);\
	}\
}

#define FREE_CALLBACK_ARGS(a, b, c)\
if (a) {\
	for (i=b; i < c; i++) {\
		zval_ptr_dtor(a[i]);\
		efree(a[i]);\
	}\
	efree(a);\
}

#define LOCAL_INFILE_ERROR_MSG(source,dest)\
memset(source, 0, LOCAL_INFILE_ERROR_LEN);\
memcpy(source, dest, LOCAL_INFILE_ERROR_LEN-1);

/* {{{ void php_set_local_infile_handler_default 
*/
void php_set_local_infile_handler_default(MY_MYSQL *mysql) {
	/* register internal callback functions */
	mysql_set_local_infile_handler(mysql->mysql, &php_local_infile_init, &php_local_infile_read,
				&php_local_infile_end, &php_local_infile_error, (void *)mysql);
	mysql->li_read = NULL;
}
/* }}} */

/* {{{ php_local_infile_init
 */
int php_local_infile_init(void **ptr, const char *filename, void *userdata)
{
	mysqli_local_infile			*data;
	MY_MYSQL 					*mysql;
	php_stream_context 			*context = NULL;

	TSRMLS_FETCH();

	/* save pointer to MY_MYSQL structure (userdata) */
	if (!(*ptr= data= ((mysqli_local_infile *)calloc(1, sizeof(mysqli_local_infile))))) {
		return 1;
	}

	if (!(mysql = (MY_MYSQL *)userdata)) {
		LOCAL_INFILE_ERROR_MSG(data->error_msg, ER(CR_UNKNOWN_ERROR));
		return 1;
	}

	/* check open_basedir */
	if (PG(open_basedir)) {
		if (php_check_open_basedir_ex(filename, 0 TSRMLS_CC) == -1) {
			LOCAL_INFILE_ERROR_MSG(data->error_msg, "open_basedir restriction in effect. Unable to open file");
			return 1;
		}
	}

	mysql->li_stream = php_stream_open_wrapper_ex((char *)filename, "r", 0, NULL, context);

	if (mysql->li_stream == NULL) {
		return 1;
	}

	data->userdata = mysql;

	return 0;
}
/* }}} */

int php_local_infile_read(void *ptr, char *buf, uint buf_len)
{
	mysqli_local_infile 		*data;
	MY_MYSQL 					*mysql;
	zval						***callback_args;
	zval						*retval;
	zval						*fp;
	int							argc = 4;
	int							i;
	long						rc;

	TSRMLS_FETCH();

	data= (mysqli_local_infile *)ptr;
	mysql = data->userdata;

	/* default processing */
	if (!mysql->li_read) {
		int			count;

		count = (int)php_stream_read(mysql->li_stream, buf, buf_len);

		if (count < 0) {
			LOCAL_INFILE_ERROR_MSG(data->error_msg, ER(2));
		}

		return count;
	}

	ALLOC_CALLBACK_ARGS(callback_args, 1, argc);
	
	/* set parameters: filepointer, buffer, buffer_len, errormsg */

	MAKE_STD_ZVAL(fp);
	php_stream_to_zval(mysql->li_stream, fp);
	callback_args[0] = &fp;
	ZVAL_STRING(*callback_args[1], "", 1);	
	ZVAL_LONG(*callback_args[2], buf_len);	
	ZVAL_STRING(*callback_args[3], "", 1);	
	
	if (call_user_function_ex(EG(function_table), 
						NULL,
						mysql->li_read,
						&retval,
						argc,	 	
						callback_args,
						0,
						NULL TSRMLS_CC) == SUCCESS) {

		rc = Z_LVAL_P(retval);
		zval_ptr_dtor(&retval);

		if (rc > 0) {
			if (rc > buf_len) {
				/* check buffer overflow */
				LOCAL_INFILE_ERROR_MSG(data->error_msg, "Read buffer too large");
				rc = -1;
			} else {
				memcpy(buf, Z_STRVAL_P(*callback_args[1]), rc);
			}
		}
		if (rc < 0) {
			LOCAL_INFILE_ERROR_MSG(data->error_msg, Z_STRVAL_P(*callback_args[3]));
		}
	} else {
		LOCAL_INFILE_ERROR_MSG(data->error_msg, "Can't execute load data local init callback function");
		rc = -1;
	}
	
	FREE_CALLBACK_ARGS(callback_args, 1, argc);
	efree(fp);
	return rc;
}

/* {{{ php_local_infile_error
 */
int php_local_infile_error(void *ptr, char *error_msg, uint error_msg_len)
{
	mysqli_local_infile *data = (mysqli_local_infile *) ptr;

	if (data) {
		strcpy(error_msg, data->error_msg);
		return 2000;
	} 
	strcpy(error_msg, ER(CR_OUT_OF_MEMORY));
	return CR_OUT_OF_MEMORY;
}
/* }}} */

/* {{{ php_local_infile_end
 */
void php_local_infile_end(void *ptr) 
{
	mysqli_local_infile			*data;
	MY_MYSQL 					*mysql;

	TSRMLS_FETCH();

	data= (mysqli_local_infile *)ptr;

	if (!(mysql = data->userdata)) {
		return;
	}

	php_stream_close(mysql->li_stream);
	return;	
}
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
