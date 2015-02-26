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
   | Authors: Andi Gutmans <andi@zend.com>                                |
   |          Rasmus Lerdorf <rasmus@lerdorf.on.ca>                       |
   |          Zeev Suraski <zeev@zend.com>                                |
   +----------------------------------------------------------------------+
*/


/* $Id: main.c,v 1.365 2001/05/08 20:11:46 sniper Exp $ */


#include <stdio.h>
#include "php.h"
#ifdef PHP_WIN32
#include "win32/time.h"
#include "win32/signal.h"
#include <process.h>
#else
#include "build-defs.h"
#endif
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_SIGNAL_H
#include <signal.h>
#endif
#if HAVE_SETLOCALE
#include <locale.h>
#endif
#include "zend.h"
#include "zend_extensions.h"
#include "php_ini.h"
#include "php_globals.h"
#include "php_main.h"
#include "fopen_wrappers.h"
#include "ext/standard/php_standard.h"
#include "php_variables.h"
#include "ext/standard/credits.h"
#ifdef PHP_WIN32
#include <io.h>
#include <fcntl.h>
#include "win32/php_registry.h"
#endif
#include "php_syslog.h"

#if PHP_SIGCHILD
#include <sys/types.h>
#include <sys/wait.h>
#endif

#include "zend_compile.h"
#include "zend_execute.h"
#include "zend_highlight.h"
#include "zend_indent.h"
#include "zend_extensions.h"

#include "php_content_types.h"
#include "php_ticks.h"
#include "php_logos.h"

#include "SAPI.h"

#ifndef ZTS
php_core_globals core_globals;
#else
PHPAPI int core_globals_id;
#endif
 

static void php_build_argv(char *s, zval *track_vars_array ELS_DC PLS_DC);

#define SAFE_FILENAME(f) ((f)?(f):"-")

static PHP_INI_MH(OnSetPrecision)
{
	ELS_FETCH();

	EG(precision) = atoi(new_value);
	return SUCCESS;
}


#if MEMORY_LIMIT
static PHP_INI_MH(OnChangeMemoryLimit)
{
	int new_limit;

	if (new_value) {
		new_limit = zend_atoi(new_value, new_value_length);
	} else {
		new_limit = 1<<30;		/* effectively, no limit */
	}
	return zend_set_memory_limit(new_limit);
}
#endif


static PHP_INI_MH(OnUpdateErrorReporting)
{
	ELS_FETCH();

	if (!new_value) {
		EG(error_reporting) = E_ALL & ~E_NOTICE;
	} else {
		EG(error_reporting) = atoi(new_value);
	}
	return SUCCESS;
}


static void php_disable_functions()
{
	char *func;
	char *new_value_dup = strdup(INI_STR("disable_functions"));	/* This is an intentional leak,
																 * it's not a big deal as it's process-wide
																 */

	func = strtok(new_value_dup, ", ");
	while (func) {
		zend_disable_function(func, strlen(func));
		func = strtok(NULL, ", ");
	}
}


static PHP_INI_MH(OnUpdateTimeout)
{
	ELS_FETCH();

	EG(timeout_seconds) = atoi(new_value);
	if (stage==PHP_INI_STAGE_STARTUP) {
		/* Don't set a timeout on startup, only per-request */
		return SUCCESS;
	}
	zend_unset_timeout();
	zend_set_timeout(EG(timeout_seconds));
	return SUCCESS;
}


/* Need to convert to strings and make use of:
 * PHP_SAFE_MODE
 *
 * Need to be read from the environment (?):
 * PHP_AUTO_PREPEND_FILE
 * PHP_AUTO_APPEND_FILE
 * PHP_DOCUMENT_ROOT
 * PHP_USER_DIR
 * PHP_INCLUDE_PATH
 */

#ifndef SAFE_MODE_EXEC_DIR
#	define SAFE_MODE_EXEC_DIR "/"
#endif

#ifdef PHP_PROG_SENDMAIL
#	define DEFAULT_SENDMAIL_PATH PHP_PROG_SENDMAIL " -t -i "
#else
#	define DEFAULT_SENDMAIL_PATH NULL
#endif
PHP_INI_BEGIN()
	PHP_INI_ENTRY_EX("define_syslog_variables",	"0",				PHP_INI_ALL,	NULL,			php_ini_boolean_displayer_cb)			
	PHP_INI_ENTRY_EX("highlight.bg",			HL_BG_COLOR,		PHP_INI_ALL,	NULL,			php_ini_color_displayer_cb)
	PHP_INI_ENTRY_EX("highlight.comment",		HL_COMMENT_COLOR,	PHP_INI_ALL,	NULL,			php_ini_color_displayer_cb)
	PHP_INI_ENTRY_EX("highlight.default",		HL_DEFAULT_COLOR,	PHP_INI_ALL,	NULL,			php_ini_color_displayer_cb)
	PHP_INI_ENTRY_EX("highlight.html",			HL_HTML_COLOR,		PHP_INI_ALL,	NULL,			php_ini_color_displayer_cb)
	PHP_INI_ENTRY_EX("highlight.keyword",		HL_KEYWORD_COLOR,	PHP_INI_ALL,	NULL,			php_ini_color_displayer_cb)
	PHP_INI_ENTRY_EX("highlight.string",		HL_STRING_COLOR,	PHP_INI_ALL,	NULL,			php_ini_color_displayer_cb)

	STD_PHP_INI_BOOLEAN("allow_call_time_pass_reference","1",PHP_INI_SYSTEM|PHP_INI_PERDIR,		OnUpdateBool,	allow_call_time_pass_reference,	zend_compiler_globals,	compiler_globals)
	STD_PHP_INI_BOOLEAN("asp_tags",				"0",		PHP_INI_SYSTEM|PHP_INI_PERDIR,		OnUpdateBool,			asp_tags,				zend_compiler_globals,	compiler_globals)
	STD_PHP_INI_BOOLEAN("display_errors",		"1",		PHP_INI_ALL,		OnUpdateBool,			display_errors,			php_core_globals,	core_globals)
	STD_PHP_INI_BOOLEAN("display_startup_errors",	"0",	PHP_INI_ALL,		OnUpdateBool,			display_startup_errors,	php_core_globals,	core_globals)
	STD_PHP_INI_BOOLEAN("enable_dl",			"1",		PHP_INI_SYSTEM,		OnUpdateBool,			enable_dl,				php_core_globals,	core_globals)
	STD_PHP_INI_BOOLEAN("expose_php",			"1",		PHP_INI_SYSTEM,		OnUpdateBool,			expose_php,				php_core_globals,	core_globals)
	STD_PHP_INI_BOOLEAN("html_errors",			"1",		PHP_INI_SYSTEM,		OnUpdateBool,			html_errors,			php_core_globals,	core_globals)
	STD_PHP_INI_BOOLEAN("ignore_user_abort",	"0",		PHP_INI_ALL,		OnUpdateBool,			ignore_user_abort,		php_core_globals,	core_globals)
	STD_PHP_INI_BOOLEAN("implicit_flush",		"0",		PHP_INI_PERDIR|PHP_INI_SYSTEM,OnUpdateBool,	implicit_flush,			php_core_globals,	core_globals)
	STD_PHP_INI_BOOLEAN("log_errors",			"0",		PHP_INI_ALL,		OnUpdateBool,			log_errors,				php_core_globals,	core_globals)
	STD_PHP_INI_BOOLEAN("magic_quotes_gpc",		"1",		PHP_INI_ALL,		OnUpdateBool,			magic_quotes_gpc,		php_core_globals,	core_globals)
	STD_PHP_INI_BOOLEAN("magic_quotes_runtime",	"0",		PHP_INI_ALL,		OnUpdateBool,			magic_quotes_runtime,	php_core_globals,	core_globals)
	STD_PHP_INI_BOOLEAN("magic_quotes_sybase",	"0",		PHP_INI_ALL,		OnUpdateBool,			magic_quotes_sybase,	php_core_globals,	core_globals)
	STD_PHP_INI_BOOLEAN("output_buffering",		"0",		PHP_INI_PERDIR|PHP_INI_SYSTEM,OnUpdateBool,	output_buffering,		php_core_globals,	core_globals)
	STD_PHP_INI_ENTRY("output_handler",			NULL,		PHP_INI_PERDIR|PHP_INI_SYSTEM,OnUpdateString,	output_handler,		php_core_globals,	core_globals)
	STD_PHP_INI_BOOLEAN("register_argc_argv",	"1",		PHP_INI_ALL,		OnUpdateBool,			register_argc_argv,		php_core_globals,	core_globals)
	STD_PHP_INI_BOOLEAN("register_globals",		"1",		PHP_INI_ALL,		OnUpdateBool,			register_globals,		php_core_globals,	core_globals)
	STD_PHP_INI_BOOLEAN("safe_mode",			"0",		PHP_INI_SYSTEM,		OnUpdateBool,			safe_mode,				php_core_globals,	core_globals)
	STD_PHP_INI_BOOLEAN("short_open_tag",DEFAULT_SHORT_OPEN_TAG,	PHP_INI_SYSTEM|PHP_INI_PERDIR,		OnUpdateBool,			short_tags,				zend_compiler_globals,	compiler_globals)
	STD_PHP_INI_BOOLEAN("sql.safe_mode",		"0",		PHP_INI_SYSTEM,		OnUpdateBool,			sql_safe_mode,			php_core_globals,	core_globals)
	STD_PHP_INI_BOOLEAN("track_errors",			"0",		PHP_INI_ALL,		OnUpdateBool,			track_errors,			php_core_globals,	core_globals)
	STD_PHP_INI_BOOLEAN("y2k_compliance",		"0",		PHP_INI_ALL,		OnUpdateBool,			y2k_compliance,			php_core_globals,	core_globals)

	STD_PHP_INI_ENTRY("arg_separator.output",	"&",		PHP_INI_ALL,		OnUpdateStringUnempty,	arg_separator.output,	php_core_globals,	core_globals)
	STD_PHP_INI_ENTRY("arg_separator.input",	"&",		PHP_INI_SYSTEM|PHP_INI_PERDIR,	OnUpdateStringUnempty,	arg_separator.input,	php_core_globals,	core_globals)

	STD_PHP_INI_ENTRY("auto_append_file",		NULL,		PHP_INI_ALL,		OnUpdateString,			auto_append_file,		php_core_globals,	core_globals)
	STD_PHP_INI_ENTRY("auto_prepend_file",		NULL,		PHP_INI_ALL,		OnUpdateString,			auto_prepend_file,		php_core_globals,	core_globals)
	STD_PHP_INI_ENTRY("doc_root",				NULL,		PHP_INI_SYSTEM,		OnUpdateStringUnempty,	doc_root,				php_core_globals,	core_globals)
	STD_PHP_INI_ENTRY("default_charset", SAPI_DEFAULT_CHARSET,	PHP_INI_ALL,	OnUpdateString,			default_charset,		sapi_globals_struct,sapi_globals)
	STD_PHP_INI_ENTRY("default_mimetype",SAPI_DEFAULT_MIMETYPE,	PHP_INI_ALL,	OnUpdateString,			default_mimetype,		sapi_globals_struct,sapi_globals)
	STD_PHP_INI_ENTRY("error_log",				NULL,		PHP_INI_ALL,		OnUpdateString,			error_log,				php_core_globals,	core_globals)
	STD_PHP_INI_ENTRY("extension_dir",			PHP_EXTENSION_DIR,		PHP_INI_SYSTEM,		OnUpdateStringUnempty,	extension_dir,			php_core_globals,	core_globals)
	STD_PHP_INI_ENTRY("gpc_order",				"GPC",		PHP_INI_ALL,		OnUpdateStringUnempty,	gpc_order,				php_core_globals,	core_globals)
	STD_PHP_INI_ENTRY("include_path",			PHP_INCLUDE_PATH,		PHP_INI_ALL,		OnUpdateStringUnempty,	include_path,			php_core_globals,	core_globals)
	PHP_INI_ENTRY("max_execution_time",			"30",		PHP_INI_ALL,			OnUpdateTimeout)
	STD_PHP_INI_ENTRY("open_basedir",			NULL,		PHP_INI_SYSTEM,		OnUpdateStringUnempty,	open_basedir,			php_core_globals,	core_globals)
	STD_PHP_INI_ENTRY("safe_mode_exec_dir",		"1",		PHP_INI_SYSTEM,		OnUpdateString,			safe_mode_exec_dir,		php_core_globals,	core_globals)
	STD_PHP_INI_ENTRY("upload_max_filesize",	"2M",		PHP_INI_ALL,		OnUpdateInt,			upload_max_filesize,	php_core_globals,	core_globals)
	STD_PHP_INI_ENTRY("file_uploads",			"1",		PHP_INI_ALL,		OnUpdateBool,			file_uploads,			php_core_globals,	core_globals)
	STD_PHP_INI_ENTRY("post_max_size",			"8M",		PHP_INI_SYSTEM,		OnUpdateInt,			post_max_size,			sapi_globals_struct,sapi_globals)
	STD_PHP_INI_ENTRY("upload_tmp_dir",			NULL,		PHP_INI_SYSTEM,		OnUpdateStringUnempty,	upload_tmp_dir,			php_core_globals,	core_globals)
	STD_PHP_INI_ENTRY("user_dir",				NULL,		PHP_INI_SYSTEM,		OnUpdateStringUnempty,	user_dir,				php_core_globals,	core_globals)
	STD_PHP_INI_ENTRY("variables_order",		NULL,		PHP_INI_ALL,		OnUpdateStringUnempty,	variables_order,		php_core_globals,	core_globals)

	STD_PHP_INI_ENTRY("error_append_string",	NULL,		PHP_INI_ALL,		OnUpdateStringUnempty,	error_append_string,	php_core_globals,	core_globals)
	STD_PHP_INI_ENTRY("error_prepend_string",	NULL,		PHP_INI_ALL,		OnUpdateStringUnempty,	error_prepend_string,	php_core_globals,	core_globals)

	PHP_INI_ENTRY("SMTP",						"localhost",PHP_INI_ALL,		NULL)
	PHP_INI_ENTRY("browscap",					NULL,		PHP_INI_SYSTEM,		NULL)
	PHP_INI_ENTRY("error_reporting",			NULL,		PHP_INI_ALL,		OnUpdateErrorReporting)
#if MEMORY_LIMIT
	PHP_INI_ENTRY("memory_limit",				"8M",		PHP_INI_ALL,		OnChangeMemoryLimit)
#endif
	PHP_INI_ENTRY("precision",					"14",		PHP_INI_ALL,		OnSetPrecision)
	PHP_INI_ENTRY("sendmail_from",				NULL,		PHP_INI_ALL,		NULL)
	PHP_INI_ENTRY("sendmail_path",	DEFAULT_SENDMAIL_PATH,	PHP_INI_SYSTEM,		NULL)
	PHP_INI_ENTRY("disable_functions",			"",			PHP_INI_SYSTEM,		NULL)

	STD_PHP_INI_ENTRY("allow_url_fopen",		"1",		PHP_INI_ALL,		OnUpdateBool,			allow_url_fopen,			php_core_globals,	core_globals)

PHP_INI_END()



/* True global (no need for thread safety */
static int module_initialized = 0;

PHPAPI void php_log_err(char *log_message)
{
	FILE *log_file;
	char error_time_str[128];
	struct tm tmbuf;
	time_t error_time;
	PLS_FETCH();

	/* Try to use the specified logging location. */
	if (PG(error_log) != NULL) {
#ifdef HAVE_SYSLOG_H
		if (!strcmp(PG(error_log), "syslog")) {
			php_syslog(LOG_NOTICE, "%.500s", log_message);
			return;
		}
#endif
		log_file = VCWD_FOPEN(PG(error_log), "a");
		if (log_file != NULL) {
			time(&error_time);
			strftime(error_time_str, 128, "%d-%b-%Y %H:%M:%S", php_localtime_r(&error_time, &tmbuf)); 
			fprintf(log_file, "[%s] ", error_time_str);
			fprintf(log_file, "%s", log_message);
			fprintf(log_file, "\n");
			fclose(log_file);
			return;
		}
	}

	/* Otherwise fall back to the default logging location, if we have one */

	if (sapi_module.log_message) {
		sapi_module.log_message(log_message);
	}
}


/* is 4K big enough? */
#define PRINTF_BUFFER_SIZE 1024*4

/* wrapper for modules to use PHPWRITE */
PHPAPI int php_write(void *buf, uint size)
{
	return PHPWRITE(buf, size);
}

PHPAPI int php_printf(const char *format, ...)
{
	va_list args;
	int ret;
	char buffer[PRINTF_BUFFER_SIZE];
	int size;

	va_start(args, format);
	size = vsnprintf(buffer, sizeof(buffer), format, args);
	if(size > sizeof(buffer) - 1) {
		size = sizeof(buffer) - 1;
	}
	ret = PHPWRITE(buffer, size);
	va_end(args);
	
	return ret;
}


/* extended error handling function */
static void php_error_cb(int type, const char *error_filename, const uint error_lineno, const char *format, va_list args)
{
	char buffer[1024];
	int buffer_len;
	ELS_FETCH();
	PLS_FETCH();

	buffer_len = vsnprintf(buffer, sizeof(buffer)-1, format, args);
	buffer[sizeof(buffer)-1]=0;
	if(buffer_len > sizeof(buffer) - 1 || buffer_len < 0) {
		buffer_len = sizeof(buffer) - 1;
	}

	/* display/log the error if necessary */
	if ((EG(error_reporting) & type || (type & E_CORE))
		&& (PG(log_errors) || PG(display_errors) || (!module_initialized))) {
		char *error_type_str;

		switch (type) {
			case E_ERROR:
			case E_CORE_ERROR:
			case E_COMPILE_ERROR:
			case E_USER_ERROR:
				error_type_str = "Fatal error";
				break;
			case E_WARNING:
			case E_CORE_WARNING:
			case E_COMPILE_WARNING:
			case E_USER_WARNING:
				error_type_str = "Warning";
				break;
			case E_PARSE:
				error_type_str = "Parse error";
				break;
			case E_NOTICE:
				error_type_str = "Warning";
				break;
			case E_USER_NOTICE:
				error_type_str = "Notice";
				break;
			default:
				error_type_str = "Unknown error";
				break;
		}

		if (!module_initialized || PG(log_errors)) {
			char log_buffer[1024];

#ifdef PHP_WIN32
			if (type==E_CORE_ERROR || type==E_CORE_WARNING) {
				MessageBox(NULL, buffer, error_type_str, MB_OK|ZEND_SERVICE_MB_STYLE);
			}
#endif
			snprintf(log_buffer, 1024, "PHP %s:  %s in %s on line %d", error_type_str, buffer, error_filename, error_lineno);
			php_log_err(log_buffer);
		}
		if (module_initialized && PG(display_errors)
			&& (!PG(during_request_startup) || PG(display_startup_errors))) {
			char *prepend_string = INI_STR("error_prepend_string");
			char *append_string = INI_STR("error_append_string");
			char *error_format;

			error_format = PG(html_errors) ?
				"<br>\n<b>%s</b>:  %s in <b>%s</b> on line <b>%d</b><br>\n"
				: "\n%s: %s in %s on line %d\n";

			if (prepend_string) {
				PUTS(prepend_string);
			}
			php_printf(error_format, error_type_str, buffer,
					   error_filename, error_lineno);
			if (append_string) {
				PUTS(append_string);
			}
		}
#if ZEND_DEBUG
		{
			zend_bool trigger_break;

			switch (type) {
				case E_ERROR:
				case E_CORE_ERROR:
				case E_COMPILE_ERROR:
				case E_USER_ERROR:
					trigger_break=1;
					break;
				default:
					trigger_break=0;
					break;
			}
			zend_output_debug_string(trigger_break, "%s(%d) : %s - %s", error_filename, error_lineno, error_type_str, buffer);
		}
#endif
	}

	/* Bail out if we can't recover */
	switch (type) {
		case E_CORE_ERROR:
			if(!module_initialized) {
				/* bad error in module startup - no way we can live with this */
				exit(-2);
			}
		/* no break - intentionally */
		case E_ERROR:
		/*case E_PARSE: the parser would return 1 (failure), we can bail out nicely */
		case E_COMPILE_ERROR:
		case E_USER_ERROR:
			if (module_initialized) {
				zend_bailout();
				return;
			}
			break;
	}

	/* Log if necessary */
	if (PG(track_errors) && EG(active_symbol_table)) {
		pval *tmp;

		ALLOC_ZVAL(tmp);
		INIT_PZVAL(tmp);
		tmp->value.str.val = (char *) estrndup(buffer, buffer_len);
		tmp->value.str.len = buffer_len;
		tmp->type = IS_STRING;
		zend_hash_update(EG(active_symbol_table), "php_errormsg", sizeof("php_errormsg"), (void **) & tmp, sizeof(pval *), NULL);
	}
}



/* {{{ proto void set_time_limit(int seconds)
   Sets the maximum time a script can run */
PHP_FUNCTION(set_time_limit)
{
	zval **new_timeout;
	PLS_FETCH();

	if (PG(safe_mode)) {
		php_error(E_WARNING, "Cannot set time limit in safe mode");
		RETURN_FALSE;
	}

	if (ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &new_timeout) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	convert_to_long_ex(new_timeout);
	zend_unset_timeout();
	zend_set_timeout(Z_LVAL_PP(new_timeout));
}
/* }}} */

static FILE *php_fopen_wrapper_for_zend(const char *filename, char **opened_path)
{
	int issock=0, socketd=0;
	int old_chunk_size;
	FILE *retval;
	
	old_chunk_size = php_sock_set_def_chunk_size(1);
	retval=php_fopen_wrapper((char *) filename, "rb", USE_PATH|IGNORE_URL_WIN, &issock, &socketd, opened_path);
	php_sock_set_def_chunk_size(old_chunk_size);
	
	if (issock) {
		retval = fdopen(socketd, "rb");
	}
	return retval;
}


static int php_get_configuration_directive_for_zend(char *name, uint name_length, zval *contents)
{
	zval *retval = cfg_get_entry(name, name_length);

	if (retval) {
		*contents = *retval;
		return SUCCESS;
	} else {
		return FAILURE;
	}
}


static void php_message_handler_for_zend(long message, void *data)
{
	switch (message) {
		case ZMSG_FAILED_INCLUDE_FOPEN: {
				PLS_FETCH();

				php_error(E_WARNING, "Failed opening '%s' for inclusion (include_path='%s')", php_strip_url_passwd((char *) data), STR_PRINT(PG(include_path)));
			}
			break;
		case ZMSG_FAILED_REQUIRE_FOPEN: {
				PLS_FETCH();

				php_error(E_COMPILE_ERROR, "Failed opening required '%s' (include_path='%s')", php_strip_url_passwd((char *) data), STR_PRINT(PG(include_path)));
			}
			break;
		case ZMSG_FAILED_HIGHLIGHT_FOPEN:
			php_error(E_WARNING, "Failed opening '%s' for highlighting", php_strip_url_passwd((char *) data));
			break;
		case ZMSG_MEMORY_LEAK_DETECTED:
		case ZMSG_MEMORY_LEAK_REPEATED: {
				ELS_FETCH();

				if (EG(error_reporting)&E_WARNING) {
#if ZEND_DEBUG
					char memory_leak_buf[512];
					SLS_FETCH();

					if (message==ZMSG_MEMORY_LEAK_DETECTED) {
						zend_mem_header *t = (zend_mem_header *) data;
						void *ptr = (void *)((char *)t+sizeof(zend_mem_header)+MEM_HEADER_PADDING);

						snprintf(memory_leak_buf, 512, "%s(%d) :  Freeing 0x%.8lX (%d bytes), script=%s\n", t->filename, t->lineno, (unsigned long)ptr, t->size, SAFE_FILENAME(SG(request_info).path_translated));
						if (t->orig_filename) {
							char relay_buf[512];

							snprintf(relay_buf, 512, "%s(%d) : Actual location (location was relayed)\n", t->orig_filename, t->orig_lineno);
							strcat(memory_leak_buf, relay_buf);
						}
					} else {
						unsigned long leak_count = (unsigned long) data;

						snprintf(memory_leak_buf, 512, "Last leak repeated %ld time%s\n", leak_count, (leak_count>1?"s":""));
					}
#	if defined(PHP_WIN32)
					OutputDebugString(memory_leak_buf);
#	else
					fprintf(stderr, memory_leak_buf);
#	endif
#endif
				}
			}
			break;
		case ZMSG_LOG_SCRIPT_NAME: {
				struct tm *ta, tmbuf;
				time_t curtime;
				char *datetime_str, asctimebuf[52];
				SLS_FETCH();

				time(&curtime);
				ta = php_localtime_r(&curtime, &tmbuf);
				datetime_str = php_asctime_r(ta, asctimebuf);
				datetime_str[strlen(datetime_str)-1]=0;	/* get rid of the trailing newline */
				fprintf(stderr, "[%s]  Script:  '%s'\n", datetime_str, SAFE_FILENAME(SG(request_info).path_translated));
			}
			break;
	}
}


#if PHP_SIGCHILD
static void sigchld_handler(int apar)
{
    while (waitpid(-1, NULL, WNOHANG) > 0)
		;
    signal(SIGCHLD,sigchld_handler);   
}
#endif

static int php_hash_environment(ELS_D SLS_DC PLS_DC);

int php_request_startup(CLS_D ELS_DC PLS_DC SLS_DC)
{
#if PHP_SIGCHILD
	signal(SIGCHLD,sigchld_handler);
#endif

	PG(during_request_startup) = 1;
	
	php_output_startup();

	/* initialize global variables */
	PG(modules_activated) = 0;
	PG(header_is_being_sent) = 0;
	PG(connection_status) = PHP_CONNECTION_NORMAL;
	
	zend_activate(CLS_C ELS_CC);
	sapi_activate(SLS_C);

	zend_set_timeout(EG(timeout_seconds));

	if (PG(expose_php)) {
		sapi_add_header(SAPI_PHP_VERSION_HEADER, sizeof(SAPI_PHP_VERSION_HEADER)-1, 1);
	}

	if (PG(output_handler) && PG(output_handler)[0]) {
		zval *output_handler;

		ALLOC_INIT_ZVAL(output_handler);
		Z_STRLEN_P(output_handler) = strlen(PG(output_handler));	/* this can be optimized */
		Z_STRVAL_P(output_handler) = estrndup(PG(output_handler), Z_STRLEN_P(output_handler));
		Z_TYPE_P(output_handler) = IS_STRING;
		php_start_ob_buffer(output_handler, 0);
	} else if (PG(output_buffering)) {
		php_start_ob_buffer(NULL, 0);
	} else if (PG(implicit_flush)) {
		php_start_implicit_flush();
	}

	/* We turn this off in php_execute_script() */
	/* PG(during_request_startup) = 0; */

	php_hash_environment(ELS_C SLS_CC PLS_CC);
	zend_activate_modules();
	PG(modules_activated)=1;

	return SUCCESS;
}


void php_request_shutdown_for_exec(void *dummy)
{
	/* used to close fd's in the 3..255 range here, but it's problematic
	 */
	shutdown_memory_manager(1, 1);
}


void php_request_shutdown(void *dummy)
{
	CLS_FETCH();
	ELS_FETCH();
	SLS_FETCH();
	PLS_FETCH();

	if (setjmp(EG(bailout))==0) {
		php_end_ob_buffers((zend_bool)(SG(request_info).headers_only?0:1));
	}

	if (setjmp(EG(bailout))==0) {
		sapi_send_headers();
	}

	if (PG(modules_activated) && setjmp(EG(bailout))==0) {
		php_call_shutdown_functions();
	}
	
	if (PG(modules_activated)) {
		zend_deactivate_modules();
	}
		
	zend_deactivate(CLS_C ELS_CC);

	if (setjmp(EG(bailout))==0) {
		sapi_deactivate(SLS_C);
	}

	if (setjmp(EG(bailout))==0) { 
		shutdown_memory_manager(CG(unclean_shutdown), 0);
	}

	if (setjmp(EG(bailout))==0) { 
		zend_unset_timeout();
	}
}

 
static void php_config_ini_shutdown(void)
{
	php_shutdown_config();
}


static int php_body_write_wrapper(const char *str, uint str_length)
{
	return php_body_write(str, str_length);
}


#ifdef ZTS
static void core_globals_ctor(php_core_globals *core_globals)
{
	memset(core_globals,0,sizeof(*core_globals));
}

#endif


int php_startup_extensions(zend_module_entry **ptr, int count)
{
	zend_module_entry **end = ptr+count;

	while (ptr < end) {
		if (*ptr) {
			if (zend_startup_module(*ptr)==FAILURE) {
				return FAILURE;
			}
		}
		ptr++;
	}
	return SUCCESS;
}

int php_global_startup_extensions(zend_module_entry **ptr, int count)
{
	zend_module_entry **end = ptr+count;

	while (ptr < end) {
		if (*ptr) {
			if ((*ptr)->global_startup_func && 
					(*ptr)->global_startup_func()==FAILURE) {
				return FAILURE;
			}
		}
		ptr++;
	}
	return SUCCESS;
}

int php_global_shutdown_extensions(zend_module_entry **ptr, int count)
{
	zend_module_entry **end = ptr+count;

	while (ptr < end) {
		if (*ptr) {
			if ((*ptr)->global_shutdown_func && 
					(*ptr)->global_shutdown_func()==FAILURE) {
				return FAILURE;
			}
		}
		ptr++;
	}
	return SUCCESS;
}

int php_module_startup(sapi_module_struct *sf)
{
	zend_utility_functions zuf;
	zend_utility_values zuv;
	int module_number=0;	/* for REGISTER_INI_ENTRIES() */
	char *php_os;
#ifdef ZTS
	zend_executor_globals *executor_globals;
	php_core_globals *core_globals;
	sapi_globals_struct *sapi_globals = ts_resource(sapi_globals_id);
#endif
#ifdef PHP_WIN32
	WORD wVersionRequested = MAKEWORD(2, 0);
	WSADATA wsaData;
#endif
#ifdef PHP_WIN32
	{
		DWORD dwVersion = GetVersion();

		/* Get build numbers for Windows NT or Win95 */
		if (dwVersion < 0x80000000){
			php_os="WINNT";
		} else {
			php_os="WIN32";
		}
	}
#else
    php_os=PHP_OS;
#endif

	sapi_initialize_empty_request(SLS_C);
	sapi_activate(SLS_C);

	if (module_initialized) {
		return SUCCESS;
	}

	sapi_module = *sf;

	php_output_startup();

	zuf.error_function = php_error_cb;
	zuf.printf_function = php_printf;
	zuf.write_function = php_body_write_wrapper;
	zuf.fopen_function = php_fopen_wrapper_for_zend;
	zuf.message_handler = php_message_handler_for_zend;
	zuf.block_interruptions = sapi_module.block_interruptions;
	zuf.unblock_interruptions = sapi_module.unblock_interruptions;
	zuf.get_configuration_directive = php_get_configuration_directive_for_zend;
	zuf.ticks_function = php_run_ticks;
	zend_startup(&zuf, NULL, 1);

#ifdef ZTS
	executor_globals = ts_resource(executor_globals_id);
	core_globals_id = ts_allocate_id(sizeof(php_core_globals), (ts_allocate_ctor) core_globals_ctor, NULL);
	core_globals = ts_resource(core_globals_id);
#endif
	EG(error_reporting) = E_ALL & ~E_NOTICE;
	
	PG(header_is_being_sent) = 0;
	SG(request_info).headers_only = 0;
	SG(request_info).argv0 = NULL;
	PG(connection_status) = PHP_CONNECTION_NORMAL;
	PG(during_request_startup) = 0;

#if HAVE_SETLOCALE
	setlocale(LC_CTYPE, "");
#endif

#ifdef PHP_WIN32
	/* start up winsock services */
	if (WSAStartup(wVersionRequested, &wsaData) != 0) {
		php_printf("\nwinsock.dll unusable. %d\n", WSAGetLastError());
		return FAILURE;
	}
#endif

	le_index_ptr = zend_register_list_destructors_ex(NULL, NULL, "index pointer", 0);


	/* this will read in php.ini, set up the configuration parameters,
	   load zend extensions and register php function extensions 
	   to be loaded later */
	if (php_init_config(sf->php_ini_path_override) == FAILURE) {
		return FAILURE;
	}

	REGISTER_INI_ENTRIES();

	/* initialize fopen wrappers registry 
	   (this uses configuration parameters from php.ini)
	 */
	if (php_init_fopen_wrappers() == FAILURE) {
		php_printf("PHP:  Unable to initialize fopen url wrappers.\n");
		return FAILURE;
	}

	/* initialize registry for images to be used in phpinfo() 
	   (this uses configuration parameters from php.ini)
	 */
	if (php_init_info_logos() == FAILURE) {
		php_printf("PHP:  Unable to initialize info phpinfo logos.\n");
		return FAILURE;
	}

	zuv.import_use_extension = ".php";
	zend_set_utility_values(&zuv);
	php_startup_sapi_content_types();

	REGISTER_MAIN_STRINGL_CONSTANT("PHP_VERSION", PHP_VERSION, sizeof(PHP_VERSION)-1, CONST_PERSISTENT | CONST_CS);
	REGISTER_MAIN_STRINGL_CONSTANT("PHP_OS", php_os, strlen(php_os), CONST_PERSISTENT | CONST_CS);
	REGISTER_MAIN_STRINGL_CONSTANT("DEFAULT_INCLUDE_PATH", PHP_INCLUDE_PATH, sizeof(PHP_INCLUDE_PATH)-1, CONST_PERSISTENT | CONST_CS);
	REGISTER_MAIN_STRINGL_CONSTANT("PEAR_INSTALL_DIR", PEAR_INSTALLDIR, sizeof(PEAR_INSTALLDIR)-1, CONST_PERSISTENT | CONST_CS);
	REGISTER_MAIN_STRINGL_CONSTANT("PEAR_EXTENSION_DIR", PHP_EXTENSION_DIR, sizeof(PHP_EXTENSION_DIR)-1, CONST_PERSISTENT | CONST_CS);
	REGISTER_MAIN_STRINGL_CONSTANT("PHP_EXTENSION_DIR", PHP_EXTENSION_DIR, sizeof(PHP_EXTENSION_DIR)-1, CONST_PERSISTENT | CONST_CS);
	REGISTER_MAIN_STRINGL_CONSTANT("PHP_BINDIR", PHP_BINDIR, sizeof(PHP_BINDIR)-1, CONST_PERSISTENT | CONST_CS);
	REGISTER_MAIN_STRINGL_CONSTANT("PHP_LIBDIR", PHP_LIBDIR, sizeof(PHP_LIBDIR)-1, CONST_PERSISTENT | CONST_CS);
	REGISTER_MAIN_STRINGL_CONSTANT("PHP_DATADIR", PHP_DATADIR, sizeof(PHP_DATADIR)-1, CONST_PERSISTENT | CONST_CS);
	REGISTER_MAIN_STRINGL_CONSTANT("PHP_SYSCONFDIR", PHP_SYSCONFDIR, sizeof(PHP_SYSCONFDIR)-1, CONST_PERSISTENT | CONST_CS);
	REGISTER_MAIN_STRINGL_CONSTANT("PHP_LOCALSTATEDIR", PHP_LOCALSTATEDIR, sizeof(PHP_LOCALSTATEDIR)-1, CONST_PERSISTENT | CONST_CS);
	REGISTER_MAIN_STRINGL_CONSTANT("PHP_CONFIG_FILE_PATH", PHP_CONFIG_FILE_PATH, sizeof(PHP_CONFIG_FILE_PATH)-1, CONST_PERSISTENT | CONST_CS);
	php_output_register_constants();

	if (php_startup_ticks(PLS_C) == FAILURE) {
		php_printf("Unable to start PHP ticks\n");
		return FAILURE;
	}

	/* startup extensions staticly compiled in */
	if (php_startup_internal_extensions() == FAILURE) {
		php_printf("Unable to start builtin modules\n");
		return FAILURE;
	}

	/* load and startup extensions compiled as shared objects (aka DLLs)
	   as requested by php.ini entries
	   theese are loaded after initialization of internal extensions
	   as extensions *might* rely on things from ext/standard
	   which is always an internal extension and to be initialized
       ahead of all other internals
	 */
	php_ini_delayed_modules_startup();

	/* disable certain functions as requested by php.ini */
	php_disable_functions();

	zend_startup_extensions();

	/* */
	module_initialized = 1;
	sapi_deactivate(SLS_C);

	/* we're done */
	return SUCCESS;
}



void php_module_shutdown_for_exec()
{
	/* used to close fd's in the range 3.255 here, but it's problematic */
}


int php_module_shutdown_wrapper(sapi_module_struct *sapi_globals)
{
	php_module_shutdown();
	return SUCCESS;
}


void php_module_shutdown()
{
	int module_number=0;	/* for UNREGISTER_INI_ENTRIES() */
	PLS_FETCH();

	if (!module_initialized) {
		return;
	}

	/* close down the ini config */
	php_config_ini_shutdown();

#ifdef PHP_WIN32
	/*close winsock */
	WSACleanup();
#endif

	php_shutdown_ticks(PLS_C);
	sapi_flush();

	zend_shutdown();
	php_shutdown_fopen_wrappers();
	php_shutdown_info_logos();
	UNREGISTER_INI_ENTRIES();
#ifndef ZTS
	zend_ini_shutdown(ELS_C);
#endif
	shutdown_memory_manager(0, 1);
	module_initialized = 0;
}


static inline void php_register_server_variables(ELS_D SLS_DC PLS_DC)
{
	zval *array_ptr=NULL;

	ALLOC_ZVAL(array_ptr);
	array_init(array_ptr);
	INIT_PZVAL(array_ptr);
	PG(http_globals)[TRACK_VARS_SERVER] = array_ptr;

	/* Server variables */
	if (sapi_module.register_server_variables) {
		sapi_module.register_server_variables(array_ptr ELS_CC SLS_CC PLS_CC);
	}

	/* argv/argc support */
	if (PG(register_argc_argv)) {
		php_build_argv(SG(request_info).query_string, array_ptr ELS_CC PLS_CC);
	}

	/* PHP Authentication support */
	if (SG(request_info).auth_user) {
		php_register_variable("PHP_AUTH_USER", SG(request_info).auth_user, array_ptr ELS_CC PLS_CC);
	}
	if (SG(request_info).auth_password) {
		php_register_variable("PHP_AUTH_PW", SG(request_info).auth_password, array_ptr ELS_CC PLS_CC);
	}
}


static int php_hash_environment(ELS_D SLS_DC PLS_DC)
{
	char *p;
	unsigned char _gpc_flags[3] = {0,0,0};
	zend_bool have_variables_order;
	zval *dummy_track_vars_array;
	zend_bool initialized_dummy_track_vars_array=0;
	int i;
	char *track_vars_names[] = {
		"HTTP_POST_VARS",
		"HTTP_GET_VARS",
		"HTTP_COOKIE_VARS",
		"HTTP_SERVER_VARS",
		"HTTP_ENV_VARS",
		"HTTP_POST_FILES",
		NULL
	};
	int track_vars_names_length[] = {
		sizeof("HTTP_POST_VARS"),
		sizeof("HTTP_GET_VARS"),
		sizeof("HTTP_COOKIE_VARS"),
		sizeof("HTTP_SERVER_VARS"),
		sizeof("HTTP_ENV_VARS"),
		sizeof("HTTP_POST_FILES")
	};


	for (i=0; i<6; i++) {
		PG(http_globals)[i] = NULL;
	}

	if (PG(variables_order)) {
		p = PG(variables_order);
		have_variables_order=1;
	} else {
		p = PG(gpc_order);
		have_variables_order=0;
		ALLOC_ZVAL(PG(http_globals)[TRACK_VARS_ENV]);
		array_init(PG(http_globals)[TRACK_VARS_ENV]);
		INIT_PZVAL(PG(http_globals)[TRACK_VARS_ENV]);
		php_import_environment_variables(PG(http_globals)[TRACK_VARS_ENV] ELS_CC PLS_CC);
	}

	while(p && *p) {
		switch(*p++) {
			case 'p':
			case 'P':
				if (!_gpc_flags[0] && !SG(headers_sent) && SG(request_info).request_method && !strcasecmp(SG(request_info).request_method, "POST")) {
					php_treat_data(PARSE_POST, NULL, NULL ELS_CC PLS_CC SLS_CC);	/* POST Data */
					_gpc_flags[0]=1;
				}
				break;
			case 'c':
			case 'C':
				if (!_gpc_flags[1]) {
					php_treat_data(PARSE_COOKIE, NULL, NULL  ELS_CC PLS_CC SLS_CC);	/* Cookie Data */
					_gpc_flags[1]=1;
				}
				break;
			case 'g':
			case 'G':
				if (!_gpc_flags[2]) {
					php_treat_data(PARSE_GET, NULL, NULL  ELS_CC PLS_CC SLS_CC);	/* GET Data */
					_gpc_flags[2]=1;
				}
				break;
			case 'e':
			case 'E':
				if (have_variables_order) {
					ALLOC_ZVAL(PG(http_globals)[TRACK_VARS_ENV]);
					array_init(PG(http_globals)[TRACK_VARS_ENV]);
					INIT_PZVAL(PG(http_globals)[TRACK_VARS_ENV]);
					php_import_environment_variables(PG(http_globals)[TRACK_VARS_ENV] ELS_CC PLS_CC);
				} else {
					php_error(E_WARNING, "Unsupported 'e' element (environment) used in gpc_order - use variables_order instead");
				}
				break;
			case 's':
			case 'S':
				php_register_server_variables(ELS_C SLS_CC PLS_CC);
				break;
		}
	}

	if (!have_variables_order) {
		php_register_server_variables(ELS_C SLS_CC PLS_CC);
	}

	for (i=0; i<6; i++) {
		if (!PG(http_globals)[i]) {
			if (!initialized_dummy_track_vars_array) {
				ALLOC_ZVAL(dummy_track_vars_array);
				array_init(dummy_track_vars_array);
				INIT_PZVAL(dummy_track_vars_array);
				initialized_dummy_track_vars_array = 1;
			} else {
				dummy_track_vars_array->refcount++;
			}
			PG(http_globals)[i] = dummy_track_vars_array;
		}
		zend_hash_update(&EG(symbol_table), track_vars_names[i], track_vars_names_length[i], &PG(http_globals)[i], sizeof(zval *), NULL);
	}
	return SUCCESS;
}


static void php_build_argv(char *s, zval *track_vars_array ELS_DC PLS_DC)
{
	pval *arr, *argc, *tmp;
	int count = 0;
	char *ss, *space;
	
	ALLOC_ZVAL(arr);
	array_init(arr);
	INIT_PZVAL(arr);

	/* Prepare argv */
	if (s && *s) {
		ss = s;
		while (ss) {
			space = strchr(ss, '+');
			if (space) {
				*space = '\0';
			}
			/* auto-type */
			ALLOC_ZVAL(tmp);
			tmp->type = IS_STRING;
			tmp->value.str.len = strlen(ss);
			tmp->value.str.val = estrndup(ss, tmp->value.str.len);
			INIT_PZVAL(tmp);
			count++;
			if (zend_hash_next_index_insert(arr->value.ht, &tmp, sizeof(pval *), NULL)==FAILURE) {
				if (tmp->type == IS_STRING) {
					efree(tmp->value.str.val);
				}
			}
			if (space) {
				*space = '+';
				ss = space + 1;
			} else {
				ss = space;
			}
		}
	}

	/* prepare argc */
	ALLOC_ZVAL(argc);
	argc->value.lval = count;
	argc->type = IS_LONG;
	INIT_PZVAL(argc);

	if (PG(register_globals)) {
		zend_hash_update(&EG(symbol_table), "argv", sizeof("argv"), &arr, sizeof(zval *), NULL);
		zend_hash_add(&EG(symbol_table), "argc", sizeof("argc"), &argc, sizeof(zval *), NULL);
	}

	if (PG(register_globals)) {
		arr->refcount++;
		argc->refcount++;
	}
	zend_hash_update(track_vars_array->value.ht, "argv", sizeof("argv"), &arr, sizeof(pval *), NULL);
	zend_hash_update(track_vars_array->value.ht, "argc", sizeof("argc"), &argc, sizeof(pval *), NULL);
}


PHPAPI int php_handle_special_queries(SLS_D PLS_DC)
{
	if (SG(request_info).query_string && SG(request_info).query_string[0]=='=' 
			&& PG(expose_php)) {
		if (php_info_logos(SG(request_info).query_string+1)) {	
			return 1;
		} else if (!strcmp(SG(request_info).query_string+1, PHP_CREDITS_GUID)) {
			php_print_credits(PHP_CREDITS_ALL);
			return 1;
		}
	}
	return 0;
}

PHPAPI int php_execute_script(zend_file_handle *primary_file CLS_DC ELS_DC PLS_DC)
{
	zend_file_handle *prepend_file_p, *append_file_p;
	zend_file_handle prepend_file, append_file;
	char *old_cwd;
	SLS_FETCH();

	EG(exit_status) = 0;
	if (php_handle_special_queries(SLS_C PLS_CC))
		return 0;
#define OLD_CWD_SIZE 4096
	old_cwd = do_alloca(OLD_CWD_SIZE);
	old_cwd[0] = '\0';

	if (setjmp(EG(bailout))!=0) {
		if (old_cwd[0] != '\0')
			VCWD_CHDIR(old_cwd);
		free_alloca(old_cwd);
		return EG(exit_status);
	}

#ifdef PHP_WIN32
	UpdateIniFromRegistry(primary_file->filename);
#endif

	PG(during_request_startup) = 0;

	if (primary_file->type == ZEND_HANDLE_FILENAME 
			&& primary_file->filename) {
		VCWD_GETCWD(old_cwd, OLD_CWD_SIZE-1);
		VCWD_CHDIR_FILE(primary_file->filename);
	}

	if (PG(auto_prepend_file) && PG(auto_prepend_file)[0]) {
		prepend_file.filename = PG(auto_prepend_file);
		prepend_file.opened_path = NULL;
		prepend_file.free_filename = 0;
		prepend_file.type = ZEND_HANDLE_FILENAME;
		prepend_file_p = &prepend_file;
	} else {
		prepend_file_p = NULL;
	}
	if (PG(auto_append_file) && PG(auto_append_file)[0]) {
		append_file.filename = PG(auto_append_file);
		append_file.opened_path = NULL;
		append_file.free_filename = 0;
		append_file.type = ZEND_HANDLE_FILENAME;
		append_file_p = &append_file;
	} else {
		append_file_p = NULL;
	}
	zend_execute_scripts(ZEND_REQUIRE CLS_CC ELS_CC, 3, prepend_file_p, primary_file, append_file_p);

	if (old_cwd[0] != '\0')
		VCWD_CHDIR(old_cwd);
	free_alloca(old_cwd);

	return EG(exit_status);
}

PHPAPI void php_handle_aborted_connection(void)
{
	PLS_FETCH();

	PG(connection_status) = PHP_CONNECTION_ABORTED;

	if (!PG(ignore_user_abort)) {
		zend_bailout();
	}
}

PHPAPI int php_handle_auth_data(const char *auth SLS_DC)
{
	int ret = -1;

	if (auth && auth[0] != '\0'
			&& strncmp(auth, "Basic ", 6) == 0) {
		char *pass;
		char *user;

		user = php_base64_decode(auth + 6, strlen(auth) - 6, NULL);
		if (user) {
			pass = strchr(user, ':');
			if (pass) {
				*pass++ = '\0';
				SG(request_info).auth_user = user;
				SG(request_info).auth_password = estrdup(pass);
				ret = 0;
			} else {
				efree(user);
			}
		}
	}

	if (ret == -1)
		SG(request_info).auth_user = SG(request_info).auth_password = NULL;

	return ret;
}

PHPAPI int php_lint_script(zend_file_handle *file CLS_DC ELS_DC PLS_DC)
{
	zend_op_array *op_array;
	int retval;
	SLS_FETCH();

	php_hash_environment(ELS_C SLS_CC PLS_CC);

	zend_activate_modules();
	PG(modules_activated)=1;

	if (setjmp(EG(bailout))!=0) {
		return FAILURE;
	}

#ifdef PHP_WIN32
	UpdateIniFromRegistry(file->filename);
#endif

	op_array = zend_compile_file(file, ZEND_INCLUDE CLS_CC);
	retval = (op_array?SUCCESS:FAILURE);

	if (op_array != NULL) {
		destroy_op_array(op_array);
	}

	return retval;
}

#ifdef PHP_WIN32
/* just so that this symbol gets exported... */
PHPAPI void dummy_indent()
{
	zend_indent();
}
#endif


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */
