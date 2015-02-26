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
   | Pi3Web version 2.0                                                   |
   +----------------------------------------------------------------------+
   | This file is committed by the Pi3 development group.                 |
   | (pi3web.sourceforge.net)                                             |
   |                                                                      |
   | Author: Holger Zimmermann (zimpel@users.sourceforge.net)             |
   +----------------------------------------------------------------------+
 */

/* $Id: pi3web_sapi.c,v 1.17 2001/04/08 10:49:07 holger Exp $ */

#include "pi3web_sapi.h"
#include "php.h"
#include "php_main.h"
#include "SAPI.h"
#include "php_globals.h"
#include "ext/standard/info.h"
#include "zend_highlight.h"
#include "zend_indent.h"
#include "ext/standard/basic_functions.h"
#include "TSRM/TSRM.h"
#include "PiAPI.h"
#include "Pi3API.h"

#define MAX_STATUS_LENGTH sizeof("xxxx LONGEST STATUS DESCRIPTION")
#define PI3WEB_SERVER_VAR_BUF_SIZE 1024
#define PI3WEB_POST_DATA_BUF 1024

int IWasLoaded=0;

static char *pi3web_server_variables[] = {
	"ALL_HTTP",
	"AUTH_TYPE",
	"CONTENT_LENGTH",
	"CONTENT_TYPE",
	"GATEWAY_INTERFACE",
	"PATH_INFO",
	"PATH_TRANSLATED",
	"QUERY_STRING",
	"REQUEST_METHOD",
	"REMOTE_ADDR",
	"REMOTE_HOST",
	"REMOTE_USER",
	"SCRIPT_NAME",
	"SERVER_NAME",
	"SERVER_PORT",
	"SERVER_PROTOCOL",
	"SERVER_SOFTWARE",
	NULL
};


static void php_info_pi3web(ZEND_MODULE_INFO_FUNC_ARGS)
{
	char **p = pi3web_server_variables;
	char variable_buf[PI3WEB_SERVER_VAR_BUF_SIZE];
	DWORD variable_len;
	LPCONTROL_BLOCK lpCB;
	SLS_FETCH();

	lpCB = (LPCONTROL_BLOCK) SG(server_context);

	PUTS("<table border=0 cellpadding=3 cellspacing=1 width=600 align=center>\n");
	PUTS("<tr><th colspan=2 bgcolor=\"" PHP_HEADER_COLOR "\">Pi3Web Server Information</th></tr>\n");
	php_info_print_table_header(2, "Information Field", "Value");
	php_info_print_table_row(2, "Pi3Web SAPI module version", "$Id: pi3web_sapi.c,v 1.17 2001/04/08 10:49:07 holger Exp $");
	php_info_print_table_row(2, "Server Name Stamp", HTTPCore_getServerStamp());
	snprintf(variable_buf, 511, "%d", HTTPCore_debugEnabled());
	php_info_print_table_row(2, "Debug Enabled", variable_buf);
	PIPlatform_getCurrentDirectory( variable_buf, PI3WEB_SERVER_VAR_BUF_SIZE);
	php_info_print_table_row(2, "Current Path", variable_buf);
	if (lpCB->GetServerVariable(lpCB->ConnID, "SERVER_NAME", variable_buf, &variable_len)
		&& variable_buf[0]) {
		php_info_print_table_row(2, "Main Virtual Hostname", variable_buf);
	};
	snprintf(variable_buf, 511, "%d", PIPlatform_getProcessId());
	php_info_print_table_row(2, "Server PID", variable_buf);
	php_info_print_table_row(2, "Server Platform", PIPlatform_getDescription());

	PUTS("</table><BR>");	

	PUTS("<table border=0 cellpadding=3 cellspacing=1 width=600 align=center>\n");
	PUTS("<tr><th colspan=2 bgcolor=\"" PHP_HEADER_COLOR "\">HTTP Request Information</th></tr>\n");
	php_info_print_table_row(2, "HTTP Request Line", lpCB->lpszReq);
	PUTS("<tr><th colspan=2 bgcolor=\"" PHP_HEADER_COLOR "\">HTTP Headers</th></tr>\n");
	php_info_print_table_header(2, "Server Variable", "Value");
	while (*p) {
		variable_len = PI3WEB_SERVER_VAR_BUF_SIZE;
		if (lpCB->GetServerVariable(lpCB->ConnID, *p, variable_buf, &variable_len)
			&& variable_buf[0]) {
			php_info_print_table_row(2, *p, variable_buf);
		} else if (PIPlatform_getLastError() == PIAPI_EINVAL) {
			char *tmp_variable_buf;

			tmp_variable_buf = (char *) emalloc(variable_len);
			if (lpCB->GetServerVariable(lpCB->ConnID, *p, tmp_variable_buf, &variable_len)
				&& variable_buf[0]) {
				php_info_print_table_row(2, *p, tmp_variable_buf);
			}
			efree(tmp_variable_buf);
		}
		p++;
	}

	PUTS("</table>");
}


static zend_module_entry php_pi3web_module = {
	"PI3WEB",
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	php_info_pi3web,
	STANDARD_MODULE_PROPERTIES
};


static int zend_pi3web_ub_write(const char *str, uint str_length)
{
	DWORD num_bytes = str_length;
	LPCONTROL_BLOCK cb;
	SLS_FETCH();
	
	cb = (LPCONTROL_BLOCK) SG(server_context);

	if ( !IWasLoaded ) return 0;
	cb->WriteClient(cb->ConnID, (char *) str, &num_bytes, 0 );

	if (num_bytes != str_length)
		php_handle_aborted_connection();
	return num_bytes;
}


static int sapi_pi3web_header_handler(sapi_header_struct *sapi_header, sapi_headers_struct *sapi_headers SLS_DC)
{
	return SAPI_HEADER_ADD;
}



static void accumulate_header_length(sapi_header_struct *sapi_header, uint *total_length)
{
	*total_length += sapi_header->header_len+2;
}


static void concat_header(sapi_header_struct *sapi_header, char **combined_headers_ptr)
{
	memcpy(*combined_headers_ptr, sapi_header->header, sapi_header->header_len);
	*combined_headers_ptr += sapi_header->header_len;
	**combined_headers_ptr = '\r';
	(*combined_headers_ptr)++;
	**combined_headers_ptr = '\n';
	(*combined_headers_ptr)++;
}


static int sapi_pi3web_send_headers(sapi_headers_struct *sapi_headers SLS_DC)
{
	uint total_length = 2;		/* account for the trailing \r\n */
	char *combined_headers, *combined_headers_ptr;
	LPCONTROL_BLOCK lpCB = (LPCONTROL_BLOCK) SG(server_context);
	sapi_header_struct default_content_type;
	PLS_FETCH();
	
	if ( !IWasLoaded ) return SAPI_HEADER_SENT_SUCCESSFULLY;


 	if (SG(sapi_headers).send_default_content_type) {
		sapi_get_default_content_type_header(&default_content_type SLS_CC);
		accumulate_header_length(&default_content_type, (void *) &total_length);
	}
	zend_llist_apply_with_argument(&SG(sapi_headers).headers, (void (*)(void *, void *)) accumulate_header_length, (void *) &total_length);

	/* Generate headers */
	combined_headers = (char *) emalloc(total_length+1);
	combined_headers_ptr = combined_headers;
	if (SG(sapi_headers).send_default_content_type) {
		concat_header(&default_content_type, (void *) &combined_headers_ptr);
		sapi_free_header(&default_content_type); /* we no longer need it */
	}
	zend_llist_apply_with_argument(&SG(sapi_headers).headers, (void (*)(void *, void *)) concat_header, (void *) &combined_headers_ptr);
	*combined_headers_ptr++ = '\r';
	*combined_headers_ptr++ = '\n';
	*combined_headers_ptr = 0;

	lpCB->dwHttpStatusCode = SG(sapi_headers).http_response_code;
	lpCB->SendHeaderFunction(lpCB->ConnID, &total_length, (LPDWORD) combined_headers);

	efree(combined_headers);
	if (SG(sapi_headers).http_status_line) {
		efree(SG(sapi_headers).http_status_line);
	}
	return SAPI_HEADER_SENT_SUCCESSFULLY;
}


static int php_pi3web_startup(sapi_module_struct *sapi_module)
{
	if (php_module_startup(sapi_module)==FAILURE
		|| zend_register_module(&php_pi3web_module)==FAILURE) {
		return FAILURE;
	} else {
		return SUCCESS;
	}
}


static int sapi_pi3web_read_post(char *buffer, uint count_bytes SLS_DC)
{
	LPCONTROL_BLOCK lpCB = (LPCONTROL_BLOCK) SG(server_context);
	DWORD read_from_buf=0;
	DWORD read_from_input=0;
	DWORD total_read=0;

	if (SG(read_post_bytes) < lpCB->cbAvailable) {
		read_from_buf = MIN(lpCB->cbAvailable-SG(read_post_bytes), count_bytes);
		memcpy(buffer, lpCB->lpbData+SG(read_post_bytes), read_from_buf);
		total_read += read_from_buf;
	}
	if (read_from_buf<count_bytes
		&& (SG(read_post_bytes)+read_from_buf) < lpCB->cbTotalBytes) {
		DWORD cbRead=0, cbSize;

		read_from_input = MIN(count_bytes-read_from_buf, lpCB->cbTotalBytes-SG(read_post_bytes)-read_from_buf);
		while (cbRead < read_from_input) {
			cbSize = read_from_input - cbRead;
			if (!lpCB->ReadClient(lpCB->ConnID, buffer+read_from_buf+cbRead, &cbSize) || cbSize==0) {
				break;
			}
			cbRead += cbSize;
		}
		total_read += cbRead;
	}
	SG(read_post_bytes) += total_read;
	return total_read;
}


static char *sapi_pi3web_read_cookies(SLS_D)
{
	LPCONTROL_BLOCK lpCB = (LPCONTROL_BLOCK) SG(server_context);
	char variable_buf[PI3WEB_SERVER_VAR_BUF_SIZE];
	DWORD variable_len = PI3WEB_SERVER_VAR_BUF_SIZE;

	if (lpCB->GetServerVariable(lpCB->ConnID, "HTTP_COOKIE", variable_buf, &variable_len)) {
		return estrndup(variable_buf, variable_len);
	} else if (PIPlatform_getLastError()==PIAPI_EINVAL) {
		char *tmp_variable_buf = (char *) emalloc(variable_len+1);

		if (lpCB->GetServerVariable(lpCB->ConnID, "HTTP_COOKIE", tmp_variable_buf, &variable_len)) {
			tmp_variable_buf[variable_len] = 0;
			return tmp_variable_buf;
		} else {
			efree(tmp_variable_buf);
		}
	}
	return NULL;
}


static sapi_module_struct pi3web_sapi_module = {
	"pi3web",				/* name */
	"PI3WEB",				/* pretty name */

	php_pi3web_startup,			/* startup */
	php_module_shutdown_wrapper,		/* shutdown */
	NULL,					/* activate */
	NULL,					/* deactivate */
	zend_pi3web_ub_write,			/* unbuffered write */
	NULL,					/* flush */
	NULL,					/* get uid */
	NULL,					/* getenv */
	php_error,				/* error handler */
	sapi_pi3web_header_handler,		/* header handler */
	sapi_pi3web_send_headers,		/* send headers handler */
	NULL,					/* send header handler */
	sapi_pi3web_read_post,			/* read POST data */
	sapi_pi3web_read_cookies,		/* read Cookies */
	NULL,					/* register server variables */
	NULL,					/* Log message */
	NULL,					/* Block interruptions */
	NULL,					/* Unblock interruptions */	

	STANDARD_SAPI_MODULE_PROPERTIES
};


static void init_request_info(sapi_globals_struct *sapi_globals, LPCONTROL_BLOCK lpCB)
{
	char *path_end = strrchr(lpCB->lpszFileName, PHP_DIR_SEPARATOR);
	if ( path_end ) *path_end = PHP_DIR_SEPARATOR;

	SG(server_context) = lpCB;
	SG(request_info).request_method  = lpCB->lpszMethod;
	SG(request_info).query_string    = lpCB->lpszQueryString;
	SG(request_info).path_translated = lpCB->lpszPathTranslated;
	SG(request_info).request_uri     = lpCB->lpszUri;
	SG(request_info).content_type    = lpCB->lpszContentType;
	SG(request_info).content_length  = lpCB->cbTotalBytes;
	SG(request_info).auth_user       = lpCB->lpszUser;
	SG(request_info).auth_password   = lpCB->lpszPassword;
	SG(sapi_headers).http_response_code = 200;
}

static void hash_pi3web_variables(ELS_D SLS_DC)
{
	char static_variable_buf[PI3WEB_SERVER_VAR_BUF_SIZE];
	char *variable_buf;
	DWORD variable_len = PI3WEB_SERVER_VAR_BUF_SIZE;
	char *variable;
	char *strtok_buf = NULL;
	LPCONTROL_BLOCK lpCB;

	lpCB = (LPCONTROL_BLOCK) SG(server_context);

	if (lpCB->GetServerVariable(lpCB->ConnID, "ALL_HTTP", static_variable_buf, &variable_len)) {
		variable_buf = static_variable_buf;
	} else {
		if (PIPlatform_getLastError()==PIAPI_EINVAL) {
			variable_buf = (char *) emalloc(variable_len);
			if (!lpCB->GetServerVariable(lpCB->ConnID, "ALL_HTTP", variable_buf, &variable_len)) {
				efree(variable_buf);
				return;
			}
		} else {
			return;
		}
	}
	variable = php_strtok_r(variable_buf, "\r\n", &strtok_buf);
	while (variable) {
		char *colon = strchr(variable, ':');

		if (colon) {
			char *value = colon+1;
			zval *entry;
			ALLOC_ZVAL(entry);

			while (*value==' ') {
				value++;
			}
			*colon = 0;
			INIT_PZVAL(entry);
			entry->value.str.len = strlen(value);
			entry->value.str.val = estrndup(value, entry->value.str.len);
			entry->type = IS_STRING;
			zend_hash_add(&EG(symbol_table), variable, strlen(variable)+1, &entry, sizeof(zval *), NULL);
			*colon = ':';
		}
		variable = php_strtok_r(NULL, "\r\n", &strtok_buf);
	}
	if (variable_buf!=static_variable_buf) {
		efree(variable_buf);
	}
}


DWORD PHP4_wrapper(LPCONTROL_BLOCK lpCB)
{
	zend_file_handle file_handle;
	SLS_FETCH();
	CLS_FETCH();
	ELS_FETCH();
	PLS_FETCH();

	if (setjmp( EG(bailout)) != 0 ) return PIAPI_ERROR;

	file_handle.filename = lpCB->lpszFileName;
	file_handle.free_filename = 0;
	file_handle.type = ZEND_HANDLE_FILENAME;
	file_handle.opened_path = NULL;

	CG(extended_info) = 0;
	init_request_info(sapi_globals, lpCB);
	php_request_startup(CLS_C ELS_CC PLS_CC SLS_CC);

	hash_pi3web_variables(ELS_C SLS_CC);

	switch ( lpCB->dwBehavior ) {
		case PHP_MODE_STANDARD:
			php_execute_script( &file_handle CLS_CC ELS_CC PLS_CC );
			break;
		case PHP_MODE_HIGHLIGHT: {
				zend_syntax_highlighter_ini syntax_highlighter_ini;
				if ( open_file_for_scanning( &file_handle CLS_CC ) == SUCCESS ) {
					php_get_highlight_struct( &syntax_highlighter_ini );
					zend_highlight( &syntax_highlighter_ini );
					/* fclose( file_handle.handle.fp ); */
				};
			};
			break;
		case PHP_MODE_INDENT:
			if ( open_file_for_scanning( &file_handle CLS_CC ) == SUCCESS ) {
				zend_indent();
			};
			/* fclose( file_handle.handle.fp ); */
			break;
	}

	if (SG(request_info).cookie_data) {
		efree(SG(request_info).cookie_data);
	};

	php_request_shutdown(NULL);
	return PIAPI_COMPLETED;
}

BOOL PHP4_startup() {
	tsrm_startup(1, 1, 0, NULL);
	sapi_startup(&pi3web_sapi_module);
	if (pi3web_sapi_module.startup) {
		pi3web_sapi_module.startup(&pi3web_sapi_module);
	};
	IWasLoaded = 1;
	return IWasLoaded;
};

BOOL PHP4_shutdown() {
	if (pi3web_sapi_module.shutdown) {
		pi3web_sapi_module.shutdown(&pi3web_sapi_module);
	};
	sapi_shutdown();
	tsrm_shutdown();
	IWasLoaded = 0;
	return !IWasLoaded;
};

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */
