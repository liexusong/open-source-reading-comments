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
  | Author: Sascha Schumann <sascha@schumann.cx>                         |
  +----------------------------------------------------------------------+

  $Id: fam.c,v 1.8.2.1 2005/02/08 05:38:04 rasmus Exp $ 
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_fam.h"

#include <fam.h>

/* If you declare any globals in php_fam.h uncomment this:
ZEND_DECLARE_MODULE_GLOBALS(fam)
*/

/* True global resources - no need for thread safety here */
static int le_fam, le_fam_r;

/* {{{ fam_functions[]
 *
 * Every user visible function must have an entry in fam_functions[].
 */
function_entry fam_functions[] = {
	PHP_FE(fam_open,	NULL)
	PHP_FE(fam_close,	NULL)
	PHP_FE(fam_monitor_directory,	NULL)
	PHP_FE(fam_monitor_file,	NULL)
	PHP_FE(fam_monitor_collection,	NULL)
	PHP_FE(fam_suspend_monitor,	NULL)
	PHP_FE(fam_resume_monitor,	NULL)
	PHP_FE(fam_cancel_monitor,	NULL)
	PHP_FE(fam_pending,	NULL)
	PHP_FE(fam_next_event,	NULL)
	{NULL, NULL, NULL}	/* Must be the last line in fam_functions[] */
};
/* }}} */

/* {{{ fam_module_entry
 */
zend_module_entry fam_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
	STANDARD_MODULE_HEADER,
#endif
	"fam",
	fam_functions,
	PHP_MINIT(fam),
	NULL,
	NULL,
	NULL,
	PHP_MINFO(fam),
#if ZEND_MODULE_API_NO >= 20010901
	"0.1", /* Replace with version number for your extension */
#endif
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_FAM
ZEND_GET_MODULE(fam)
#endif

/* {{{ php_fam_init_globals
 */
/* Uncomment this function if you have INI entries
static void php_fam_init_globals(zend_fam_globals *fam_globals)
{
	fam_globals->global_value = 0;
	fam_globals->global_string = NULL;
}
*/
/* }}} */
	
static void rsrc_close_fam(zend_rsrc_list_entry *rsrc TSRMLS_DC)
{
	FAMConnection *fc = (FAMConnection *) rsrc->ptr;
	FAMClose(fc);
	efree(fc);
}

static void rsrc_close_fam_r(zend_rsrc_list_entry *rsrc TSRMLS_DC)
{
	/* No need to close */
	efree(rsrc->ptr);
}

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(fam)
{
	le_fam = zend_register_list_destructors_ex(rsrc_close_fam, NULL, "fam connections", module_number);
	le_fam_r = zend_register_list_destructors_ex(rsrc_close_fam_r, NULL, "fam requests", module_number);
	/* If you have INI entries, uncomment these lines 
	ZEND_INIT_MODULE_GLOBALS(fam, php_fam_init_globals, NULL);
	REGISTER_INI_ENTRIES();
	*/


	REGISTER_LONG_CONSTANT("FAMChanged", FAMChanged, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FAMDeleted", FAMDeleted, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FAMStartExecuting", FAMStartExecuting, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FAMStopExecuting", FAMStopExecuting, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FAMCreated", FAMCreated, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FAMMoved", FAMMoved, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FAMAcknowledge", FAMAcknowledge, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FAMExists", FAMExists, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FAMEndExist", FAMEndExist, CONST_PERSISTENT);
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(fam)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "fam support", "enabled");
	php_info_print_table_end();

	/* Remove comments if you have entries in php.ini
	DISPLAY_INI_ENTRIES();
	*/
}
/* }}} */


/* {{{ proto resource fam_open([string appname])
   Open FAM connection */
PHP_FUNCTION(fam_open)
{
	char *appname = NULL;
	int argc = ZEND_NUM_ARGS();
	int appname_len;
	FAMConnection *fc;
	int r;
	
	if (zend_parse_parameters(argc TSRMLS_CC, "|s", &appname, &appname_len) == FAILURE) 
		return;

	fc = emalloc(sizeof *fc);
	r = FAMOpen2(fc, appname);

	if (r < 0) {
		efree(fc);
		RETURN_FALSE;
	}

	ZEND_REGISTER_RESOURCE(return_value, fc, le_fam);
}
/* }}} */

/* {{{ proto void fam_close(resource id)
   Close FAM connection */
PHP_FUNCTION(fam_close)
{
	int argc = ZEND_NUM_ARGS();
	zval *id;

	if (zend_parse_parameters(argc TSRMLS_CC, "r", &id) == FAILURE) 
		return;

	zend_list_delete(Z_RESVAL_P(id));
}
/* }}} */

/* {{{ proto resource fam_monitor_directory(resource id, string dirname)
   Monitor a directory for changes */
PHP_FUNCTION(fam_monitor_directory)
{
	char *dirname = NULL;
	int argc = ZEND_NUM_ARGS();
	int dirname_len;
	zval *id;
	FAMRequest *fr;
	FAMConnection *fc;
	
	if (zend_parse_parameters(argc TSRMLS_CC, "rs", &id, &dirname, &dirname_len) == FAILURE) 
		return;

	ZEND_FETCH_RESOURCE(fc, FAMConnection *, &id, -1, "fam", le_fam);
	ZEND_VERIFY_RESOURCE(fc);
	
	fr = emalloc(sizeof *fr);

	if (FAMMonitorDirectory(fc, dirname, fr, NULL) < 0) {
		efree(fr);
		RETURN_FALSE;
	}

	ZEND_REGISTER_RESOURCE(return_value, fr, le_fam_r);
}
/* }}} */

/* {{{ proto resource fam_monitor_file(resource id, string filename)
    Monitor a regular file for changes */
PHP_FUNCTION(fam_monitor_file)
{
	char *filename = NULL;
	int argc = ZEND_NUM_ARGS();
	int filename_len;
	zval *id;
	FAMRequest *fr;
	FAMConnection *fc;

	if (zend_parse_parameters(argc TSRMLS_CC, "rs", &id, &filename, &filename_len) == FAILURE) 
		return;
	
	ZEND_FETCH_RESOURCE(fc, FAMConnection *, &id, -1, "fam", le_fam);
	ZEND_VERIFY_RESOURCE(fc);
	
	fr = emalloc(sizeof *fr);

	if (FAMMonitorFile(fc, filename, fr, NULL) < 0) {
		efree(fr);
		RETURN_FALSE;
	}

	ZEND_REGISTER_RESOURCE(return_value, fr, le_fam_r);
}
/* }}} */

/* {{{ proto resource fam_monitor_collection(resource id, string dirname, int depth, string mask)
    Monitor a collection of files in a directory for changes */
PHP_FUNCTION(fam_monitor_collection)
{
	char *dirname = NULL;
	char *mask = NULL;
	int argc = ZEND_NUM_ARGS();
	int dirname_len;
	int mask_len;
	zval *id;
	long depth;
	FAMRequest *fr;
	FAMConnection *fc;

	if (zend_parse_parameters(argc TSRMLS_CC, "rsls", &id, &dirname, &dirname_len, &depth, &mask, &mask_len) == FAILURE) 
		return;
	
	ZEND_FETCH_RESOURCE(fc, FAMConnection *, &id, -1, "fam", le_fam);
	ZEND_VERIFY_RESOURCE(fc);
	
	fr = emalloc(sizeof *fr);

	if (FAMMonitorCollection(fc, dirname, fr, NULL, depth, mask) < 0) {
		efree(fr);
		RETURN_FALSE;
	}

	ZEND_REGISTER_RESOURCE(return_value, fr, le_fam_r);

}
/* }}} */

/* {{{ proto bool fam_suspend_monitor(resource id, resource monitor_id)
   Temporary suspend monitoring */
PHP_FUNCTION(fam_suspend_monitor)
{
	int argc = ZEND_NUM_ARGS();
	zval *id;
	zval *monitor_id;
	FAMRequest *fr;
	FAMConnection *fc;

	if (zend_parse_parameters(argc TSRMLS_CC, "rr", &id, &monitor_id) == FAILURE) 
		return;
	
	ZEND_FETCH_RESOURCE(fc, FAMConnection *, &id, -1, "fam", le_fam);
	ZEND_VERIFY_RESOURCE(fc);
	ZEND_FETCH_RESOURCE(fr, FAMRequest *, &monitor_id, -1, "fam", le_fam_r);
	ZEND_VERIFY_RESOURCE(fr);

	if (FAMSuspendMonitor(fc, fr) < 0)
		RETURN_FALSE;
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool fam_resume_monitor(resource id, resource monitor_id)
   Resume suspended monitoring */
PHP_FUNCTION(fam_resume_monitor)
{
	int argc = ZEND_NUM_ARGS();
	zval *id;
	zval *monitor_id;
	FAMRequest *fr;
	FAMConnection *fc;

	if (zend_parse_parameters(argc TSRMLS_CC, "rr", &id, &monitor_id) == FAILURE) 
		return;
	
	ZEND_FETCH_RESOURCE(fc, FAMConnection *, &id, -1, "fam", le_fam);
	ZEND_VERIFY_RESOURCE(fc);
	ZEND_FETCH_RESOURCE(fr, FAMRequest *, &monitor_id, -1, "fam", le_fam_r);
	ZEND_VERIFY_RESOURCE(fr);

	if (FAMResumeMonitor(fc, fr) < 0)
		RETURN_FALSE;
	RETURN_TRUE;

}
/* }}} */

/* {{{ proto bool fam_cancel_monitor(resource id, resource monitor_id)
    Terminate monitoring */
PHP_FUNCTION(fam_cancel_monitor)
{
	int argc = ZEND_NUM_ARGS();
	zval *id;
	zval *monitor_id;
	FAMRequest *fr;
	FAMConnection *fc;
	int r;

	if (zend_parse_parameters(argc TSRMLS_CC, "rr", &id, &monitor_id) == FAILURE) 
		return;
	
	ZEND_FETCH_RESOURCE(fc, FAMConnection *, &id, -1, "fam", le_fam);
	ZEND_VERIFY_RESOURCE(fc);
	ZEND_FETCH_RESOURCE(fr, FAMRequest *, &monitor_id, -1, "fam", le_fam_r);
	ZEND_VERIFY_RESOURCE(fr);

	r = FAMCancelMonitor(fc, fr);

	zend_list_delete(Z_RESVAL_P(monitor_id));

	if (r < 0)
		RETURN_FALSE;
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto int fam_pending(resource id)
    Check for pending FAM events */
PHP_FUNCTION(fam_pending)
{
	int argc = ZEND_NUM_ARGS();
	zval *id;
	FAMConnection *fc;
	
	if (zend_parse_parameters(argc TSRMLS_CC, "r", &id) == FAILURE) 
		return;
	
	ZEND_FETCH_RESOURCE(fc, FAMConnection *, &id, -1, "fam", le_fam);
	ZEND_VERIFY_RESOURCE(fc);

	RETVAL_LONG(FAMPending(fc));
}
/* }}} */

/* {{{ proto array fam_next_event(resource id)
    Get next pending FAM event */
PHP_FUNCTION(fam_next_event)
{
	int argc = ZEND_NUM_ARGS();
	zval *id;
	FAMConnection *fc;
	FAMEvent fe;
	
	if (zend_parse_parameters(argc TSRMLS_CC, "r", &id) == FAILURE) 
		return;
	
	ZEND_FETCH_RESOURCE(fc, FAMConnection *, &id, -1, "fam", le_fam);
	ZEND_VERIFY_RESOURCE(fc);
	
	if (FAMNextEvent(fc, &fe) < 0)
		RETURN_FALSE;
	
	array_init(return_value);
	if (fe.hostname)
		add_assoc_string(return_value, "hostname", fe.hostname, 1);
	add_assoc_string(return_value, "filename", fe.filename, 1);
	add_assoc_long(return_value, "code", fe.code);
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
