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
   | Author: Sterling Hughes <sterling@php.net>                           |
   +----------------------------------------------------------------------+
*/

/* $Id: curl.c,v 1.51.2.4 2001/06/12 20:25:15 ssb Exp $ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"

#if HAVE_CURL

#include <stdio.h>
#include <string.h>

#ifdef PHP_WIN32
#include <winsock.h>
#include <sys/types.h>
#endif

#include <curl/curl.h>
#include <curl/easy.h>

#include "ext/standard/info.h"
#include "ext/standard/file.h"
#include "php_curl.h"

static int  le_curl;
#define le_curl_name "cURL handle"

static void _php_curl_close(zend_rsrc_list_entry *rsrc);

#define SAVE_CURL_ERROR(__handle, __err) (__handle)->err.no = (int) __err;

function_entry curl_functions[] = {
	PHP_FE(curl_init,     NULL)
	PHP_FE(curl_version,  NULL)
	PHP_FE(curl_setopt,   NULL)
	PHP_FE(curl_exec,     NULL)
	PHP_FE(curl_getinfo,  NULL)
	PHP_FE(curl_error,    NULL)
	PHP_FE(curl_errno,    NULL)
	PHP_FE(curl_close,    NULL)
	{NULL, NULL, NULL}
};

zend_module_entry curl_module_entry = {
	"curl",
	curl_functions,
	PHP_MINIT(curl),
	PHP_MSHUTDOWN(curl),
	NULL,
	NULL,
	PHP_MINFO(curl),
	STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_CURL
ZEND_GET_MODULE (curl)
#endif

PHP_MINFO_FUNCTION(curl)
{
	php_info_print_table_start();
	php_info_print_table_row(2, "CURL support",    "enabled");
	php_info_print_table_row(2, "CURL Information", curl_version());
	php_info_print_table_end();
}

#define REGISTER_CURL_CONSTANT(name, value) REGISTER_LONG_CONSTANT(name, value, CONST_CS | CONST_PERSISTENT)

PHP_MINIT_FUNCTION(curl)
{
	le_curl = zend_register_list_destructors_ex(_php_curl_close, NULL, "curl", module_number);
	
	/* Constants for curl_setopt() */
	REGISTER_CURL_CONSTANT("CURLOPT_PORT",            CURLOPT_PORT);
	REGISTER_CURL_CONSTANT("CURLOPT_FILE",            CURLOPT_FILE);
	REGISTER_CURL_CONSTANT("CURLOPT_INFILE",          CURLOPT_INFILE);
	REGISTER_CURL_CONSTANT("CURLOPT_INFILESIZE",      CURLOPT_INFILESIZE);
	REGISTER_CURL_CONSTANT("CURLOPT_URL",             CURLOPT_URL);
	REGISTER_CURL_CONSTANT("CURLOPT_PROXY",           CURLOPT_PROXY);
	REGISTER_CURL_CONSTANT("CURLOPT_VERBOSE",         CURLOPT_VERBOSE);
	REGISTER_CURL_CONSTANT("CURLOPT_HEADER",          CURLOPT_HEADER);
	REGISTER_CURL_CONSTANT("CURLOPT_HTTPHEADER",      CURLOPT_HTTPHEADER);
	REGISTER_CURL_CONSTANT("CURLOPT_NOPROGRESS",      CURLOPT_NOPROGRESS);
	REGISTER_CURL_CONSTANT("CURLOPT_NOBODY",          CURLOPT_NOBODY);
	REGISTER_CURL_CONSTANT("CURLOPT_FAILONERROR",     CURLOPT_FAILONERROR);
	REGISTER_CURL_CONSTANT("CURLOPT_UPLOAD",          CURLOPT_UPLOAD);
	REGISTER_CURL_CONSTANT("CURLOPT_POST",            CURLOPT_POST);
	REGISTER_CURL_CONSTANT("CURLOPT_FTPLISTONLY",     CURLOPT_FTPLISTONLY);
	REGISTER_CURL_CONSTANT("CURLOPT_FTPAPPEND",       CURLOPT_FTPAPPEND);
	REGISTER_CURL_CONSTANT("CURLOPT_NETRC",           CURLOPT_NETRC);
	REGISTER_CURL_CONSTANT("CURLOPT_FOLLOWLOCATION",  CURLOPT_FOLLOWLOCATION);
	REGISTER_CURL_CONSTANT("CURLOPT_FTPASCII",        CURLOPT_FTPASCII);
	REGISTER_CURL_CONSTANT("CURLOPT_PUT",             CURLOPT_PUT);
	REGISTER_CURL_CONSTANT("CURLOPT_MUTE",            CURLOPT_MUTE);
	REGISTER_CURL_CONSTANT("CURLOPT_USERPWD",         CURLOPT_USERPWD);
	REGISTER_CURL_CONSTANT("CURLOPT_PROXYUSERPWD",    CURLOPT_PROXYUSERPWD);
	REGISTER_CURL_CONSTANT("CURLOPT_RANGE",           CURLOPT_RANGE);
	REGISTER_CURL_CONSTANT("CURLOPT_TIMEOUT",         CURLOPT_TIMEOUT);
	REGISTER_CURL_CONSTANT("CURLOPT_POSTFIELDS",      CURLOPT_POSTFIELDS);
	REGISTER_CURL_CONSTANT("CURLOPT_REFERER",         CURLOPT_REFERER);
	REGISTER_CURL_CONSTANT("CURLOPT_USERAGENT",       CURLOPT_USERAGENT);
	REGISTER_CURL_CONSTANT("CURLOPT_FTPPORT",         CURLOPT_FTPPORT);
	REGISTER_CURL_CONSTANT("CURLOPT_LOW_SPEED_LIMIT", CURLOPT_LOW_SPEED_LIMIT);
	REGISTER_CURL_CONSTANT("CURLOPT_LOW_SPEED_TIME",  CURLOPT_LOW_SPEED_TIME);
	REGISTER_CURL_CONSTANT("CURLOPT_RESUME_FROM",     CURLOPT_RESUME_FROM);
	REGISTER_CURL_CONSTANT("CURLOPT_COOKIE",          CURLOPT_COOKIE);
	REGISTER_CURL_CONSTANT("CURLOPT_SSLCERT",         CURLOPT_SSLCERT);
	REGISTER_CURL_CONSTANT("CURLOPT_SSLCERTPASSWD",   CURLOPT_SSLCERTPASSWD);
	REGISTER_CURL_CONSTANT("CURLOPT_WRITEHEADER",     CURLOPT_WRITEHEADER);
	REGISTER_CURL_CONSTANT("CURLOPT_COOKIEFILE",      CURLOPT_COOKIEFILE);
	REGISTER_CURL_CONSTANT("CURLOPT_SSLVERSION",      CURLOPT_SSLVERSION);
	REGISTER_CURL_CONSTANT("CURLOPT_TIMECONDITION",   CURLOPT_TIMECONDITION);
	REGISTER_CURL_CONSTANT("CURLOPT_TIMEVALUE",       CURLOPT_TIMEVALUE);
	REGISTER_CURL_CONSTANT("CURLOPT_CUSTOMREQUEST",   CURLOPT_CUSTOMREQUEST);
	REGISTER_CURL_CONSTANT("CURLOPT_STDERR",          CURLOPT_STDERR);
	REGISTER_CURL_CONSTANT("CURLOPT_TRANSFERTEXT",    CURLOPT_TRANSFERTEXT);
	REGISTER_CURL_CONSTANT("CURLOPT_RETURNTRANSFER",  CURLOPT_RETURNTRANSFER);
	REGISTER_CURL_CONSTANT("CURLOPT_QUOTE",           CURLOPT_QUOTE);
	REGISTER_CURL_CONSTANT("CURLOPT_POSTQUOTE",       CURLOPT_POSTQUOTE);
	REGISTER_CURL_CONSTANT("CURLOPT_INTERFACE",       CURLOPT_INTERFACE);
	REGISTER_CURL_CONSTANT("CURLOPT_KRB4LEVEL",       CURLOPT_KRB4LEVEL);
	REGISTER_CURL_CONSTANT("CURLOPT_HTTPPROXYTUNNEL", CURLOPT_HTTPPROXYTUNNEL);
	REGISTER_CURL_CONSTANT("CURLOPT_FILETIME",        CURLOPT_FILETIME);
	REGISTER_CURL_CONSTANT("CURLOPT_WRITEFUNCTION",   CURLOPT_WRITEFUNCTION);
	REGISTER_CURL_CONSTANT("CURLOPT_READFUNCTION",    CURLOPT_READFUNCTION);
	REGISTER_CURL_CONSTANT("CURLOPT_PASSWDFUNCTION",  CURLOPT_PASSWDFUNCTION);
	REGISTER_CURL_CONSTANT("CURLOPT_HEADERFUNCTION",  CURLOPT_HEADERFUNCTION);
	REGISTER_CURL_CONSTANT("CURLOPT_MAXREDIRS",       CURLOPT_MAXREDIRS);
	REGISTER_CURL_CONSTANT("CURLOPT_MAXCONNECTS",     CURLOPT_MAXCONNECTS);
	REGISTER_CURL_CONSTANT("CURLOPT_CLOSEPOLICY",     CURLOPT_CLOSEPOLICY);
	REGISTER_CURL_CONSTANT("CURLOPT_FRESH_CONNECT",   CURLOPT_FRESH_CONNECT);
	REGISTER_CURL_CONSTANT("CURLOPT_FORBID_REUSE",    CURLOPT_FORBID_REUSE);
	REGISTER_CURL_CONSTANT("CURLOPT_RANDOM_FILE",     CURLOPT_RANDOM_FILE);
	REGISTER_CURL_CONSTANT("CURLOPT_EGDSOCKET",       CURLOPT_EGDSOCKET);
	REGISTER_CURL_CONSTANT("CURLOPT_CONNECTTIMEOUT",  CURLOPT_CONNECTTIMEOUT);
	REGISTER_CURL_CONSTANT("CURLOPT_SSL_VERIFYPEER",  CURLOPT_SSL_VERIFYPEER);
	REGISTER_CURL_CONSTANT("CURLOPT_CAINFO",          CURLOPT_CAINFO);
	REGISTER_CURL_CONSTANT("CURLOPT_BINARYTRANSER",   CURLOPT_BINARYTRANSFER);
	
	/* Constants effecting the way CURLOPT_CLOSEPOLICY works */
	REGISTER_CURL_CONSTANT("CURLCLOSEPOLICY_LEAST_RECENTLY_USED", CURLCLOSEPOLICY_LEAST_RECENTLY_USED);
	REGISTER_CURL_CONSTANT("CURLCLOSEPOLICY_OLDEST",              CURLCLOSEPOLICY_OLDEST);

	/* Info constants */
	REGISTER_CURL_CONSTANT("CURLINFO_EFFECTIVE_URL",    CURLINFO_EFFECTIVE_URL);
	REGISTER_CURL_CONSTANT("CURLINFO_HTTP_CODE",        CURLINFO_HTTP_CODE);
	REGISTER_CURL_CONSTANT("CURLINFO_HEADER_SIZE",      CURLINFO_HEADER_SIZE);
	REGISTER_CURL_CONSTANT("CURLINFO_REQUEST_SIZE",     CURLINFO_REQUEST_SIZE);
	REGISTER_CURL_CONSTANT("CURLINFO_TOTAL_TIME",       CURLINFO_TOTAL_TIME);
	REGISTER_CURL_CONSTANT("CURLINFO_NAMELOOKUP_TIME",  CURLINFO_NAMELOOKUP_TIME);
	REGISTER_CURL_CONSTANT("CURLINFO_CONNECT_TIME",     CURLINFO_CONNECT_TIME);
	REGISTER_CURL_CONSTANT("CURLINFO_PRETRANSFER_TIME", CURLINFO_PRETRANSFER_TIME);
	REGISTER_CURL_CONSTANT("CURLINFO_SIZE_UPLOAD",      CURLINFO_SIZE_UPLOAD);
	REGISTER_CURL_CONSTANT("CURLINFO_SIZE_DOWNLOAD",    CURLINFO_SIZE_DOWNLOAD);
	REGISTER_CURL_CONSTANT("CURLINFO_SPEED_DOWNLOAD",   CURLINFO_SPEED_DOWNLOAD);
	REGISTER_CURL_CONSTANT("CURLINFO_SPEED_UPLOAD",     CURLINFO_SPEED_UPLOAD);
	REGISTER_CURL_CONSTANT("CURLINFO_FILETIME",         CURLINFO_FILETIME);

	/* Error Constants */
	REGISTER_CURL_CONSTANT("CURLE_OK",                          CURLE_OK);
	REGISTER_CURL_CONSTANT("CURLE_UNSUPPORTED_PROTOCOL",        CURLE_UNSUPPORTED_PROTOCOL);
	REGISTER_CURL_CONSTANT("CURLE_FAILED_INIT",                 CURLE_FAILED_INIT);
	REGISTER_CURL_CONSTANT("CURLE_URL_MALFORMAT",               CURLE_URL_MALFORMAT);
	REGISTER_CURL_CONSTANT("CURLE_URL_MALFORMAT_USER",          CURLE_URL_MALFORMAT_USER);
	REGISTER_CURL_CONSTANT("CURLE_COULDNT_RESOLVE_PROXY",       CURLE_COULDNT_RESOLVE_PROXY);
	REGISTER_CURL_CONSTANT("CURLE_COULDNT_RESOLVE_HOST",        CURLE_COULDNT_RESOLVE_HOST);
	REGISTER_CURL_CONSTANT("CURLE_COULDNT_CONNECT",             CURLE_COULDNT_CONNECT);
	REGISTER_CURL_CONSTANT("CURLE_FTP_WEIRD_SERVER_REPLY",      CURLE_FTP_WEIRD_SERVER_REPLY);
	REGISTER_CURL_CONSTANT("CURLE_FTP_ACCESS_DENIED",           CURLE_FTP_ACCESS_DENIED);
	REGISTER_CURL_CONSTANT("CURLE_FTP_USER_PASSWORD_INCORRECT", CURLE_FTP_USER_PASSWORD_INCORRECT);
	REGISTER_CURL_CONSTANT("CURLE_FTP_WEIRD_PASS_REPLY",        CURLE_FTP_WEIRD_PASS_REPLY);
	REGISTER_CURL_CONSTANT("CURLE_FTP_WEIRD_USER_REPLY",        CURLE_FTP_WEIRD_USER_REPLY);
	REGISTER_CURL_CONSTANT("CURLE_FTP_WEIRD_PASV_REPLY",        CURLE_FTP_WEIRD_PASV_REPLY);
	REGISTER_CURL_CONSTANT("CURLE_FTP_WEIRD_227_FORMAT",        CURLE_FTP_WEIRD_227_FORMAT);
	REGISTER_CURL_CONSTANT("CURLE_FTP_CANT_GET_HOST",           CURLE_FTP_CANT_GET_HOST);
	REGISTER_CURL_CONSTANT("CURLE_FTP_CANT_RECONNECT",          CURLE_FTP_CANT_RECONNECT);
	REGISTER_CURL_CONSTANT("CURLE_FTP_COULDNT_SET_BINARY",      CURLE_FTP_COULDNT_SET_BINARY);
	REGISTER_CURL_CONSTANT("CURLE_PARTIAL_FILE",                CURLE_PARTIAL_FILE);
	REGISTER_CURL_CONSTANT("CURLE_FTP_COULDNT_RETR_FILE",       CURLE_FTP_COULDNT_RETR_FILE);
	REGISTER_CURL_CONSTANT("CURLE_FTP_WRITE_ERROR",             CURLE_FTP_WRITE_ERROR);
	REGISTER_CURL_CONSTANT("CURLE_FTP_QUOTE_ERROR",             CURLE_FTP_QUOTE_ERROR);
	REGISTER_CURL_CONSTANT("CURLE_HTTP_NOT_FOUND",              CURLE_HTTP_NOT_FOUND);
	REGISTER_CURL_CONSTANT("CURLE_WRITE_ERROR",                 CURLE_WRITE_ERROR);
	REGISTER_CURL_CONSTANT("CURLE_MALFORMAT_USER",              CURLE_MALFORMAT_USER);
	REGISTER_CURL_CONSTANT("CURLE_FTP_COULDNT_STOR_FILE",       CURLE_FTP_COULDNT_STOR_FILE);
	REGISTER_CURL_CONSTANT("CURLE_READ_ERROR",                  CURLE_READ_ERROR);
	REGISTER_CURL_CONSTANT("CURLE_OUT_OF_MEMORY",               CURLE_OUT_OF_MEMORY);
	REGISTER_CURL_CONSTANT("CURLE_OPERATION_TIMEOUTED",         CURLE_OPERATION_TIMEOUTED);
	REGISTER_CURL_CONSTANT("CURLE_FTP_COULDNT_SET_ASCII",       CURLE_FTP_COULDNT_SET_ASCII);
	REGISTER_CURL_CONSTANT("CURLE_FTP_PORT_FAILED",             CURLE_FTP_PORT_FAILED);
	REGISTER_CURL_CONSTANT("CURLE_FTP_COULDNT_USE_REST",        CURLE_FTP_COULDNT_USE_REST);
	REGISTER_CURL_CONSTANT("CURLE_FTP_COULDNT_GET_SIZE",        CURLE_FTP_COULDNT_GET_SIZE);
	REGISTER_CURL_CONSTANT("CURLE_HTTP_RANGE_ERROR",            CURLE_HTTP_RANGE_ERROR);
	REGISTER_CURL_CONSTANT("CURLE_HTTP_POST_ERROR",             CURLE_HTTP_POST_ERROR);
	REGISTER_CURL_CONSTANT("CURLE_SSL_CONNECT_ERROR",           CURLE_SSL_CONNECT_ERROR);
	REGISTER_CURL_CONSTANT("CURLE_FTP_BAD_DOWNLOAD_RESUME",     CURLE_FTP_BAD_DOWNLOAD_RESUME);
	REGISTER_CURL_CONSTANT("CURLE_FILE_COULDNT_READ_FILE",      CURLE_FILE_COULDNT_READ_FILE);
	REGISTER_CURL_CONSTANT("CURLE_LDAP_CANNOT_BIND",            CURLE_LDAP_CANNOT_BIND);
	REGISTER_CURL_CONSTANT("CURLE_LDAP_SEARCH_FAILED",          CURLE_LDAP_SEARCH_FAILED);
	REGISTER_CURL_CONSTANT("CURLE_LIBRARY_NOT_FOUND",           CURLE_LIBRARY_NOT_FOUND);
	REGISTER_CURL_CONSTANT("CURLE_FUNCTION_NOT_FOUND",          CURLE_FUNCTION_NOT_FOUND);
	REGISTER_CURL_CONSTANT("CURLE_ABORTED_BY_CALLBACK",         CURLE_ABORTED_BY_CALLBACK);
	REGISTER_CURL_CONSTANT("CURLE_BAD_FUNCTION_ARGUMENT",       CURLE_BAD_FUNCTION_ARGUMENT);
	REGISTER_CURL_CONSTANT("CURLE_BAD_CALLING_ORDER",           CURLE_BAD_CALLING_ORDER);
	REGISTER_CURL_CONSTANT("CURLE_HTTP_PORT_FAILED",            CURLE_HTTP_PORT_FAILED);
	REGISTER_CURL_CONSTANT("CURLE_BAD_PASSWORD_ENTERED",        CURLE_BAD_PASSWORD_ENTERED);
	REGISTER_CURL_CONSTANT("CURLE_TOO_MANY_REDIRECTS",          CURLE_TOO_MANY_REDIRECTS);
	REGISTER_CURL_CONSTANT("CURLE_UNKOWN_TELNET_OPTION",        CURLE_UNKNOWN_TELNET_OPTION);
	REGISTER_CURL_CONSTANT("CURLE_TELNET_OPTION_SYNTAX",        CURLE_TELNET_OPTION_SYNTAX);
	REGISTER_CURL_CONSTANT("CURLE_ALREADY_COMPLETE",            CURLE_ALREADY_COMPLETE);

	if (curl_global_init(0) != CURLE_OK) {
		return FAILURE;
	}
	
	return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(curl)
{
	curl_global_cleanup();

	return SUCCESS;
}


#define PHP_CURL_STDOUT 0
#define PHP_CURL_FILE   1
#define PHP_CURL_USER   2
#define PHP_CURL_DIRECT 3
#define PHP_CURL_RETURN 4
#define PHP_CURL_ASCII  5
#define PHP_CURL_BINARY 6

static size_t curl_write(char *data, size_t size, size_t nmemb, void *ctx)
{
	php_curl       *ch     = (php_curl *) ctx;
	php_curl_write *t      = ch->handlers->write;
	size_t          length = size * nmemb;
	ELS_FETCH();

	switch (t->method) {
	case PHP_CURL_STDOUT:
		PUTS(data);
		break;
	case PHP_CURL_FILE:
		return fwrite(data, size, nmemb, t->fp);
	case PHP_CURL_RETURN:
		smart_str_appendl(&t->buf, data, (int) length);
		break;
	case PHP_CURL_USER: {
		zval *argv[2];
		zval *retval;
		int   error;
		ELS_FETCH();

		MAKE_STD_ZVAL(argv[0]);
		MAKE_STD_ZVAL(argv[1]);
		MAKE_STD_ZVAL(retval);

		ZVAL_RESOURCE(argv[0], ch->id);
		zend_list_addref(ch->id);
		ZVAL_STRINGL(argv[1], data, (int) length, 1);

		error = call_user_function(EG(function_table),
		                           NULL,
		                           t->func,
		                           retval, 2, argv);
		if (error == FAILURE) {
			php_error(E_WARNING, "Cannot call the CURLOPT_WRITEFUNCTION");
			return -1;
		}

		length = Z_LVAL_P(retval);

		zval_ptr_dtor(&argv[0]);
		zval_ptr_dtor(&argv[1]);
		zval_ptr_dtor(&retval);

		break;
	}
	}

	return length;
}

static size_t curl_read(char *data, size_t size, size_t nmemb, void *ctx)
{
	php_curl       *ch = (php_curl *) ctx;
	php_curl_read  *t  = ch->handlers->read;
	int             length = -1;
	ELS_FETCH();

	switch (t->method) {
	case PHP_CURL_DIRECT:
		length = fread(data, size, nmemb, t->fp);
	case PHP_CURL_USER: {
		zval *argv[3];
		zval *retval;
		int   length;
		int   error;

		MAKE_STD_ZVAL(argv[0]);
		MAKE_STD_ZVAL(argv[1]);
		MAKE_STD_ZVAL(argv[2]);
		MAKE_STD_ZVAL(retval);

		ZVAL_RESOURCE(argv[0], ch->id);
		zend_list_addref(ch->id);
		ZVAL_RESOURCE(argv[1], t->fd);
		zend_list_addref(t->fd);
		ZVAL_LONG(argv[2], size * nmemb);

		error = call_user_function(EG(function_table),
		                           NULL,
		                           t->func,
		                           retval, 3, argv);
		if (error == FAILURE) {
			php_error(E_WARNING, "Cannot call the CURLOPT_READFUNCTION");
			break;
		}
		
		memcpy(data, Z_STRVAL_P(retval), Z_STRLEN_P(retval));
		length = Z_STRLEN_P(retval);

		zval_ptr_dtor(&argv[0]);
		zval_ptr_dtor(&argv[1]);
		zval_ptr_dtor(&argv[2]);
		zval_ptr_dtor(&retval);

		break;
	}
	}

	return length;
}

static size_t _php_curl_write_header(char *data, size_t size, size_t nmemb, void *ctx)
{
	php_curl  *ch   = (php_curl *) ctx;
	zval      *func = ch->handlers->write_header;
	zval      *argv[2];
	zval      *retval;
	int        error;
	int        length;
	ELS_FETCH();

	MAKE_STD_ZVAL(argv[0]);
	MAKE_STD_ZVAL(argv[1]);
	MAKE_STD_ZVAL(retval);

	ZVAL_RESOURCE(argv[0], ch->id);
	zend_list_addref(ch->id);
	ZVAL_STRINGL(argv[0], data, size * nmemb, 1);

	error = call_user_function(EG(function_table), 
	                           NULL,
	                           func,
	                           retval, 2, argv);
	if (error == FAILURE) {
		php_error(E_WARNING, "Couldn't call the CURLOPT_HEADERFUNCTION");
		return -1;
	}

	length = Z_LVAL_P(retval);

	zval_ptr_dtor(&argv[0]);
	zval_ptr_dtor(&argv[1]);
	zval_ptr_dtor(&retval);

	return length;
}

static size_t _php_curl_passwd(void *ctx, char *prompt, char *buf, int buflen)
{
	php_curl    *ch   = (php_curl *) ctx;
	zval        *func = ch->handlers->passwd;
	zval        *argv[3];
	zval        *retval;
	int          error;
	ELS_FETCH();

	MAKE_STD_ZVAL(argv[0]);
	MAKE_STD_ZVAL(argv[1]);
	MAKE_STD_ZVAL(argv[2]);

	ZVAL_RESOURCE(argv[0], ch->id);
	zend_list_addref(ch->id);
	ZVAL_STRING(argv[1], prompt, 1);
	ZVAL_LONG(argv[2], buflen);

	error = call_user_function(EG(function_table),
	                           NULL,
	                           func,
	                           retval, 2, argv);
	if (error == FAILURE) {
		php_error(E_WARNING, "Couldn't call the CURLOPT_PASSWDFUNCTION");
		return -1;
	}

	if (Z_STRLEN_P(retval) > buflen) {
		php_error(E_WARNING, "Returned password is too long for libcurl to handle");
		return -1;
	}

	strlcpy(buf, Z_STRVAL_P(retval), buflen);

	zval_ptr_dtor(&argv[0]);
	zval_ptr_dtor(&argv[1]);
	zval_ptr_dtor(&argv[2]);
	zval_ptr_dtor(&retval);

	return 0;
}
	
	

static void curl_free_string(void **string)
{
	efree(*string);
}

static void curl_free_post(void **post)
{
	curl_formfree((struct HttpPost *) *post);
}

static void curl_free_slist(void **slist)
{
	curl_slist_free_all((struct curl_slist *) *slist);
}

/* {{{ proto string curl_version(void)
   Return the CURL version string. */
PHP_FUNCTION(curl_version)
{
	RETURN_STRING(curl_version(), 1);
}
/* }}} */

static void alloc_curl_handle(php_curl **ch)
{
	*ch                    = emalloc(sizeof(php_curl));
	(*ch)->handlers        = ecalloc(1, sizeof(php_curl_handlers));
	(*ch)->handlers->write = ecalloc(1, sizeof(php_curl_write));
	(*ch)->handlers->read  = ecalloc(1, sizeof(php_curl_read));

	zend_llist_init(&(*ch)->to_free.str, sizeof(char *), 
	                (void(*)(void *)) curl_free_string, 0);
	zend_llist_init(&(*ch)->to_free.slist, sizeof(struct curl_slist),
	                (void(*)(void *)) curl_free_slist, 0);
	zend_llist_init(&(*ch)->to_free.post, sizeof(struct HttpPost),
	                (void(*)(void *)) curl_free_post, 0);
}

/* {{{ proto int curl_init([string url])
   Initialize a CURL session */
PHP_FUNCTION(curl_init)
{
	zval       **url;
	php_curl    *ch;
	int          argc = ZEND_NUM_ARGS();

	if (argc < 0 || argc > 1 ||
	    zend_get_parameters_ex(argc, &url) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	alloc_curl_handle(&ch);

	ch->cp = curl_easy_init();
	if (! ch->cp) {
		php_error(E_WARNING, "Cannot initialize a new cURL handle");
		RETURN_FALSE;
	}

	ch->handlers->write->method = PHP_CURL_STDOUT;
	ch->handlers->write->type   = PHP_CURL_ASCII;
	ch->handlers->read->method  = PHP_CURL_DIRECT;

	curl_easy_setopt(ch->cp, CURLOPT_NOPROGRESS,        1);
	curl_easy_setopt(ch->cp, CURLOPT_VERBOSE,           0);
	curl_easy_setopt(ch->cp, CURLOPT_ERRORBUFFER,       ch->err.str);
	curl_easy_setopt(ch->cp, CURLOPT_WRITEFUNCTION,     curl_write);
	curl_easy_setopt(ch->cp, CURLOPT_FILE,              (void *) ch);
	curl_easy_setopt(ch->cp, CURLOPT_READFUNCTION,      curl_read);
	curl_easy_setopt(ch->cp, CURLOPT_INFILE,            (void *) ch);
	if (argc > 0) {
		char *urlcopy;
		convert_to_string_ex(url);

		urlcopy = estrndup(Z_STRVAL_PP(url), Z_STRLEN_PP(url));
		curl_easy_setopt(ch->cp, CURLOPT_URL, urlcopy);
		zend_llist_add_element(&ch->to_free.str, &urlcopy);
	}

	ZEND_REGISTER_RESOURCE(return_value, ch, le_curl);
	ch->id = Z_LVAL_P(return_value);
}
/* }}} */

/* {{{ proto bool curl_setopt(int ch, string option, mixed value)
   Set an option for a CURL transfer */
PHP_FUNCTION(curl_setopt)
{
	zval       **zid, 
	           **zoption, 
	           **zvalue;
	php_curl    *ch;
	CURLcode     error;
	int          option;
	
	if (ZEND_NUM_ARGS() != 3 ||
	    zend_get_parameters_ex(3, &zid, &zoption, &zvalue) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	ZEND_FETCH_RESOURCE(ch, php_curl *, zid, -1, le_curl_name, le_curl);
	convert_to_long_ex(zoption);

	option = Z_LVAL_PP(zoption);
	switch (option) {
	case CURLOPT_INFILESIZE:
	case CURLOPT_VERBOSE:
	case CURLOPT_HEADER:
	case CURLOPT_NOPROGRESS:
	case CURLOPT_NOBODY:
	case CURLOPT_FAILONERROR:
	case CURLOPT_UPLOAD:
	case CURLOPT_POST:
	case CURLOPT_FTPLISTONLY:
	case CURLOPT_FTPAPPEND:
	case CURLOPT_NETRC:
	case CURLOPT_FOLLOWLOCATION:
	case CURLOPT_PUT:
	case CURLOPT_MUTE:
	case CURLOPT_TIMEOUT:
	case CURLOPT_LOW_SPEED_LIMIT:
	case CURLOPT_SSLVERSION:
	case CURLOPT_LOW_SPEED_TIME:
	case CURLOPT_RESUME_FROM:
	case CURLOPT_TIMEVALUE:
	case CURLOPT_TIMECONDITION:
	case CURLOPT_TRANSFERTEXT:
	case CURLOPT_HTTPPROXYTUNNEL:
	case CURLOPT_FILETIME:
	case CURLOPT_MAXREDIRS:
	case CURLOPT_MAXCONNECTS:
	case CURLOPT_CLOSEPOLICY:
	case CURLOPT_FRESH_CONNECT:
	case CURLOPT_FORBID_REUSE:
	case CURLOPT_CONNECTTIMEOUT:
	case CURLOPT_SSL_VERIFYPEER:
		convert_to_long_ex(zvalue);
		error = curl_easy_setopt(ch->cp, option, Z_LVAL_PP(zvalue));
		break;
	case CURLOPT_URL:
	case CURLOPT_PROXY:
	case CURLOPT_USERPWD:
	case CURLOPT_PROXYUSERPWD:
	case CURLOPT_RANGE:
	case CURLOPT_CUSTOMREQUEST:
	case CURLOPT_USERAGENT:
	case CURLOPT_FTPPORT:
	case CURLOPT_COOKIE:
	case CURLOPT_SSLCERT:
	case CURLOPT_SSLCERTPASSWD:
	case CURLOPT_COOKIEFILE:
	case CURLOPT_REFERER:
	case CURLOPT_INTERFACE:
	case CURLOPT_KRB4LEVEL: 
	case CURLOPT_RANDOM_FILE:
	case CURLOPT_EGDSOCKET:
	case CURLOPT_CAINFO: {
		char *copystr = NULL;
	
		convert_to_string_ex(zvalue);
		copystr = estrndup(Z_STRVAL_PP(zvalue), Z_STRLEN_PP(zvalue));

		error = curl_easy_setopt(ch->cp, option, copystr);
		zend_llist_add_element(&ch->to_free.str, &copystr);

		break;
	}
	case CURLOPT_FILE:
	case CURLOPT_INFILE: 
	case CURLOPT_WRITEHEADER:
	case CURLOPT_STDERR: {
		FILE *fp;
		ZEND_FETCH_RESOURCE(fp, FILE *, zvalue, -1, "File-Handle", php_file_le_fopen());
		
		if (option == CURLOPT_FILE) {
			ch->handlers->write->fp = fp;
			ch->handlers->write->method = PHP_CURL_FILE;
		}
		else if (option == CURLOPT_INFILE) {
			zend_list_addref(Z_LVAL_PP(zvalue));
			ch->handlers->read->fp = fp;
			ch->handlers->read->fd = Z_LVAL_PP(zvalue);
		}
		else {
			error = curl_easy_setopt(ch->cp, option, fp);
		}

		break;
	}
	case CURLOPT_RETURNTRANSFER:
		convert_to_long_ex(zvalue);

		if (Z_LVAL_PP(zvalue)) {
			ch->handlers->write->method = PHP_CURL_RETURN;
		}
		break;
	case CURLOPT_BINARYTRANSFER:
		convert_to_long_ex(zvalue);
		
		ch->handlers->write->type = PHP_CURL_BINARY;
	case CURLOPT_WRITEFUNCTION:
		zval_add_ref(zvalue);
		ch->handlers->write->func = *zvalue;
		ch->handlers->write->method = PHP_CURL_USER;
		break;
	case CURLOPT_READFUNCTION:
		zval_add_ref(zvalue);
		ch->handlers->read->func   = *zvalue;
		ch->handlers->read->method = PHP_CURL_USER;
		break;
	case CURLOPT_HEADERFUNCTION:
		zval_add_ref(zvalue);
		ch->handlers->write_header = *zvalue;
		error = curl_easy_setopt(ch->cp, CURLOPT_HEADERFUNCTION, _php_curl_write_header);
		error = curl_easy_setopt(ch->cp, CURLOPT_WRITEHEADER, (void *) ch);
		break;
	case CURLOPT_PASSWDFUNCTION:
		zval_add_ref(zvalue);
		ch->handlers->passwd = *zvalue;
		error = curl_easy_setopt(ch->cp, CURLOPT_PASSWDFUNCTION, _php_curl_passwd);
		error = curl_easy_setopt(ch->cp, CURLOPT_PASSWDDATA,     (void *) ch);
		break;
	case CURLOPT_POSTFIELDS:
		if (Z_TYPE_PP(zvalue) == IS_ARRAY || Z_TYPE_PP(zvalue) == IS_OBJECT) {
			zval            **current;
			HashTable        *postfields;
			struct HttpPost  *first = NULL;
			struct HttpPost  *last  = NULL;
	
			postfields = HASH_OF(*zvalue);
			if (! postfields) {
				php_error(E_WARNING, "Couldn't get HashTable in CURLOPT_POSTFIELDS");
				RETURN_FALSE;
			}

			for (zend_hash_internal_pointer_reset(postfields);
			     zend_hash_get_current_data(postfields, (void **) &current) == SUCCESS;
			     zend_hash_move_forward(postfields)) {
				char  *string_key = NULL;
				char  *postval    = NULL;
				ulong  num_key;

				SEPARATE_ZVAL(current);
				convert_to_string_ex(current);

				zend_hash_get_current_key(postfields, &string_key, &num_key, 0);

				postval = emalloc(strlen(string_key) + Z_STRLEN_PP(current) + 1);
				sprintf(postval, "%s=%s", string_key, Z_STRVAL_PP(current));

				error = curl_formparse(postval, &first, &last);
			}

			if (error != CURLE_OK) {
				SAVE_CURL_ERROR(ch, error);
				RETURN_FALSE;
			}

			zend_llist_add_element(&ch->to_free.post, &first);
			error = curl_easy_setopt(ch->cp, CURLOPT_HTTPPOST, first);
		}
		else {
			char *post = NULL;

			convert_to_string_ex(zvalue);
			post = estrndup(Z_STRVAL_PP(zvalue), Z_STRLEN_PP(zvalue));
			zend_llist_add_element(&ch->to_free.str, &post);

			error = curl_easy_setopt(ch->cp, CURLOPT_POSTFIELDS, post);
			error = curl_easy_setopt(ch->cp, CURLOPT_POSTFIELDSIZE, Z_STRLEN_PP(zvalue));
		}

		break;
	case CURLOPT_HTTPHEADER: 
	case CURLOPT_QUOTE:
	case CURLOPT_POSTQUOTE: {
		zval              **current;
		HashTable          *hash;
		struct curl_slist  *slist = NULL;

		hash = HASH_OF(*zvalue);
		if (! hash) {
			php_error(E_WARNING, 
			          "You must pass either an object or an array with the CURLOPT_HTTPHEADER, CURLOPT_QUOTE and CURLOPT_POSTQUOTE arguments");
			RETURN_FALSE;
		}

		for (zend_hash_internal_pointer_reset(hash);
		     zend_hash_get_current_data(hash, (void **) &current) == SUCCESS;
		     zend_hash_move_forward(hash)) {
			char *indiv = NULL;

			SEPARATE_ZVAL(current);
			convert_to_string_ex(current);

			indiv = estrndup(Z_STRVAL_PP(current), Z_STRLEN_PP(current) + 1);
			indiv[Z_STRLEN_PP(current)] = '\0';
			slist = curl_slist_append(slist, indiv);
			if (! slist) {
				efree(indiv);
				php_error(E_WARNING, "Couldn't build curl_slist from curl_setopt()");
				RETURN_FALSE;
			}
			zend_llist_add_element(&ch->to_free.str, &indiv);
		}
		zend_llist_add_element(&ch->to_free.slist, &slist);

		error = curl_easy_setopt(ch->cp, option, slist);

		break;
	}
	}
	
	if (error != CURLE_OK) {
		SAVE_CURL_ERROR(ch, error);
		RETURN_FALSE;
	} else {
		RETURN_TRUE;
	}
}
/* }}} */

/* {{{ proto bool curl_exec(int ch)
   Perform a CURL session */
PHP_FUNCTION(curl_exec)
{
	zval      **zid;
	php_curl   *ch;
	CURLcode    error;

	if (ZEND_NUM_ARGS() != 1 ||
	    zend_get_parameters_ex(1, &zid) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	ZEND_FETCH_RESOURCE(ch, php_curl *, zid, -1, le_curl_name, le_curl);

	error = curl_easy_perform(ch->cp);
	if (error != CURLE_OK) {
		SAVE_CURL_ERROR(ch, error);
		RETURN_FALSE;
	}

	if (ch->handlers->write->method == PHP_CURL_RETURN) {
		if (ch->handlers->write->type != PHP_CURL_BINARY) 
			smart_str_0(&ch->handlers->write->buf);
		RETURN_STRINGL(ch->handlers->write->buf.c, ch->handlers->write->buf.len, 1);
		smart_str_free(&ch->handlers->write->buf);
	}

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto string curl_getinfo(int ch, int opt)
   Get information regarding a specific transfer */
PHP_FUNCTION(curl_getinfo)
{
	zval       **zid, 
	           **zoption;
	php_curl    *ch;
	int          option,
	             argc = ZEND_NUM_ARGS();

	if (argc < 1 || argc > 2 ||
	    zend_get_parameters_ex(argc, &zid, &zoption) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	ZEND_FETCH_RESOURCE(ch, php_curl *, zid, -1, le_curl_name, le_curl);

	if (argc < 2) {
		char   *url;
		long    l_code;
		double  d_code;

		array_init(return_value);

		curl_easy_getinfo(ch->cp, CURLINFO_EFFECTIVE_URL, &url);
		add_assoc_string(return_value, "url", url, 1);
		curl_easy_getinfo(ch->cp, CURLINFO_HTTP_CODE, &l_code);
		add_assoc_long(return_value, "http_code", l_code);
		curl_easy_getinfo(ch->cp, CURLINFO_HEADER_SIZE, &l_code);
		add_assoc_long(return_value, "header_size", l_code);
		curl_easy_getinfo(ch->cp, CURLINFO_REQUEST_SIZE, &l_code);
		add_assoc_long(return_value, "request_size", l_code);
		curl_easy_getinfo(ch->cp, CURLINFO_FILETIME, &l_code);
		add_assoc_long(return_value, "filetime", l_code);
		curl_easy_getinfo(ch->cp, CURLINFO_TOTAL_TIME, &d_code);
		add_assoc_double(return_value, "total_time", d_code);
		curl_easy_getinfo(ch->cp, CURLINFO_NAMELOOKUP_TIME, &d_code);
		add_assoc_double(return_value, "namelookup_time", d_code);
		curl_easy_getinfo(ch->cp, CURLINFO_CONNECT_TIME, &d_code);
		add_assoc_double(return_value, "connect_time", d_code);
		curl_easy_getinfo(ch->cp, CURLINFO_PRETRANSFER_TIME, &d_code);
		add_assoc_double(return_value, "pretransfer_time", d_code);
		curl_easy_getinfo(ch->cp, CURLINFO_SIZE_UPLOAD, &d_code);
		add_assoc_double(return_value, "size_upload", d_code);
		curl_easy_getinfo(ch->cp, CURLINFO_SIZE_DOWNLOAD, &d_code);
		add_assoc_double(return_value, "size_download", d_code);
		curl_easy_getinfo(ch->cp, CURLINFO_SPEED_DOWNLOAD, &d_code);
		add_assoc_double(return_value, "speed_download", d_code);
		curl_easy_getinfo(ch->cp, CURLINFO_SPEED_UPLOAD, &d_code);
		add_assoc_double(return_value, "speed_upload", d_code);
	} else {
		option = Z_LVAL_PP(zoption);
		switch (option) {
		case CURLINFO_EFFECTIVE_URL: {
			char *url;

			curl_easy_getinfo(ch->cp, option, &url);
			RETURN_STRING(url, 1);

			break;
		}
		case CURLINFO_HTTP_CODE: 
		case CURLINFO_HEADER_SIZE: 
		case CURLINFO_REQUEST_SIZE: 
		case CURLINFO_FILETIME: {
			long code;
					
			curl_easy_getinfo(ch->cp, option, &code);
			RETURN_LONG(code);
				
			break;
		}
		case CURLINFO_TOTAL_TIME: 
		case CURLINFO_NAMELOOKUP_TIME: 
		case CURLINFO_CONNECT_TIME:
		case CURLINFO_PRETRANSFER_TIME: 
		case CURLINFO_SIZE_UPLOAD: 
		case CURLINFO_SIZE_DOWNLOAD:
		case CURLINFO_SPEED_DOWNLOAD: 
		case CURLINFO_SPEED_UPLOAD: {
			double code;
	
			curl_easy_getinfo(ch->cp, option, &code);
			RETURN_DOUBLE(code);

			break;
		}
		}
	}			
}
/* }}} */

/* {{{ proto string curl_error(int ch)
   Return a string contain the last error for the current session */
PHP_FUNCTION(curl_error)
{
	zval      **zid;
	php_curl   *ch;
	
	if (ZEND_NUM_ARGS() != 1 ||
	    zend_get_parameters_ex(1, &zid) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	ZEND_FETCH_RESOURCE(ch, php_curl *, zid, -1, le_curl_name, le_curl);

	RETURN_STRING(ch->err.str, 1);
}
/* }}} */

/* {{{ proto int curl_errno(int ch)
   Return an integer containing the last error number */
PHP_FUNCTION(curl_errno)
{
	zval      **zid;
	php_curl   *ch;

	if (ZEND_NUM_ARGS() != 1 ||
	    zend_get_parameters_ex(1, &zid) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	ZEND_FETCH_RESOURCE(ch, php_curl *, zid, -1, le_curl_name, le_curl);

	RETURN_LONG(ch->err.no);
}
/* }}} */

/* {{{ proto void curl_close(int ch)
   Close a CURL session */
PHP_FUNCTION(curl_close)
{
	zval      **zid;
	php_curl   *ch;

	if (ZEND_NUM_ARGS() != 1 ||
	    zend_get_parameters_ex(1, &zid) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	ZEND_FETCH_RESOURCE(ch, php_curl *, zid, -1, le_curl_name, le_curl);
	
	zend_list_delete(Z_LVAL_PP(zid));
}
/* }}} */

/* {{{ _php_curl_close()
   List destructor for curl handles */
static void _php_curl_close(zend_rsrc_list_entry *rsrc)
{
	php_curl *ch = (php_curl *) rsrc->ptr;

	curl_easy_cleanup(ch->cp);
	zend_llist_clean(&ch->to_free.str);
	zend_llist_clean(&ch->to_free.slist);
	zend_llist_clean(&ch->to_free.post);

	if (ch->handlers->write->func) zval_ptr_dtor(&ch->handlers->write->func);
	if (ch->handlers->read->func)  zval_ptr_dtor(&ch->handlers->read->func);
	if (ch->handlers->write_header) zval_ptr_dtor(&ch->handlers->write_header);
	if (ch->handlers->passwd) zval_ptr_dtor(&ch->handlers->passwd);

	efree(ch->handlers->write);
	efree(ch->handlers->read);
	efree(ch->handlers);
	efree(ch);
}	
/* }}} */

#endif
