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

/* $Id: php_spl.c,v 1.28.2.1 2005/08/24 10:18:15 johannes Exp $ */

#ifdef HAVE_CONFIG_H
	#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_spl.h"
#include "spl_functions.h"
#include "spl_engine.h"
#include "spl_array.h"
#include "spl_directory.h"
#include "spl_iterators.h"
#include "spl_sxe.h"

#ifdef COMPILE_DL_SPL
ZEND_GET_MODULE(spl)
#endif

ZEND_DECLARE_MODULE_GLOBALS(spl)

/* {{{ spl_functions
 */
function_entry spl_functions[] = {
	PHP_FE(spl_classes,             NULL)
	PHP_FE(class_parents,           NULL)
	PHP_FE(class_implements,        NULL)
	{NULL, NULL, NULL}
};
/* }}} */

/* {{{ spl_module_entry
 */
zend_module_entry spl_module_entry = {
	STANDARD_MODULE_HEADER,
	"SPL",
	spl_functions,
	PHP_MINIT(spl),
	PHP_MSHUTDOWN(spl),
	PHP_RINIT(spl),
	PHP_RSHUTDOWN(spl),
	PHP_MINFO(spl),
	"0.2",
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

/* {{{ spl_functions_none
 */
function_entry spl_functions_none[] = {
	{NULL, NULL, NULL}
};
/* }}} */

/* {{{ spl_init_globals
 */
static void spl_init_globals(zend_spl_globals *spl_globals)
{
}
/* }}} */

/* {{{ PHP_MINIT_FUNCTION(spl)
 */
PHP_MINIT_FUNCTION(spl)
{
	ZEND_INIT_MODULE_GLOBALS(spl, spl_init_globals, NULL);

	PHP_MINIT(spl_iterators)(INIT_FUNC_ARGS_PASSTHRU);
	PHP_MINIT(spl_array)(INIT_FUNC_ARGS_PASSTHRU);
	PHP_MINIT(spl_directory)(INIT_FUNC_ARGS_PASSTHRU);
	PHP_MINIT(spl_sxe)(INIT_FUNC_ARGS_PASSTHRU);

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_RINIT_FUNCTION(spl)
 */
PHP_RINIT_FUNCTION(spl)
{
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_RSHUTDOWN_FUNCTION(spl)
 */
PHP_RSHUTDOWN_FUNCTION(spl)
{                      
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION(spl)
 */
PHP_MSHUTDOWN_FUNCTION(spl)
{                      
	SPL_DEBUG(fprintf(stderr, "%s\n", "Shutting down SPL");)

	return SUCCESS;
}
/* }}} */

/* {{{ class_parents
 */
PHP_FUNCTION(class_parents)
{
	zval *obj;
	zend_class_entry *parent_class;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "o", &obj) == FAILURE) {
		RETURN_FALSE;
	}
	array_init(return_value);
	parent_class = Z_OBJCE_P(obj)->parent;
	while (parent_class) {
		spl_add_class_name(return_value, parent_class, 0, 0 TSRMLS_CC);
		parent_class = parent_class->parent;
	}
}
/* }}} */

/* {{{ class_implements
 */
PHP_FUNCTION(class_implements)
{
	zval *obj;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "o", &obj) == FAILURE) {
		RETURN_FALSE;
	}
	array_init(return_value);
	spl_add_interfaces(return_value, Z_OBJCE_P(obj), 1, ZEND_ACC_INTERFACE TSRMLS_CC);
}
/* }}} */

#define SPL_ADD_CLASS(class_name, z_list, sub, allow, ce_flags) \
	spl_add_classes(&spl_ce_ ## class_name, z_list, sub, allow, ce_flags TSRMLS_CC)

#define SPL_LIST_CLASSES(z_list, sub, allow, ce_flags) \
	SPL_ADD_CLASS(ArrayIterator, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(ArrayObject, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(CachingIterator, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(CachingRecursiveIterator, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(DirectoryIterator, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(FilterIterator, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(LimitIterator, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(ParentIterator, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(RecursiveDirectoryIterator, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(RecursiveIterator, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(RecursiveIteratorIterator, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(SeekableIterator, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(SimpleXMLIterator, z_list, sub, allow, ce_flags); \

/* {{{ spl_classes */
PHP_FUNCTION(spl_classes)
{
	array_init(return_value);
	
	SPL_LIST_CLASSES(return_value, 0, 0, 0)
}
/* }}} */

int spl_build_class_list_string(zval **entry, char **list TSRMLS_DC)
{
	char *res;
	
	spprintf(&res, 0, "%s, %s", *list, Z_STRVAL_PP(entry));
	efree(*list);
	*list = res;
	return ZEND_HASH_APPLY_KEEP;
}

/* {{{ PHP_MINFO(spl)
 */
PHP_MINFO_FUNCTION(spl)
{
	zval list;
	char *strg;

	php_info_print_table_start();
	php_info_print_table_header(2, "SPL support",        "enabled");

	INIT_PZVAL(&list);
	array_init(&list);
	SPL_LIST_CLASSES(&list, 0, 1, ZEND_ACC_INTERFACE)
	strg = estrdup("");
	zend_hash_apply_with_argument(Z_ARRVAL_P(&list), (apply_func_arg_t)spl_build_class_list_string, &strg TSRMLS_CC);
	zval_dtor(&list);
	php_info_print_table_row(2, "Interfaces", strg + 2);
	efree(strg);

	INIT_PZVAL(&list);
	array_init(&list);
	SPL_LIST_CLASSES(&list, 0, -1, ZEND_ACC_INTERFACE)
	strg = estrdup("");
	zend_hash_apply_with_argument(Z_ARRVAL_P(&list), (apply_func_arg_t)spl_build_class_list_string, &strg TSRMLS_CC);
	zval_dtor(&list);
	php_info_print_table_row(2, "Classes", strg + 2);
	efree(strg);

	php_info_print_table_end();
}
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: fdm=marker
 * vim: noet sw=4 ts=4
 */
