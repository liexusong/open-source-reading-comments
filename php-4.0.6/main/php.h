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
   |          Zeev Suraski <zeev@zend.com>                                |
   +----------------------------------------------------------------------+
 */

/* $Id: php.h,v 1.137 2001/04/30 14:23:41 dbeu Exp $ */

#ifndef PHP_H
#define PHP_H

#ifdef HAVE_DMALLOC
#include <dmalloc.h>
#endif

#define PHP_API_VERSION 20010119

#define YYDEBUG 0

#include "php_version.h"
#include "zend.h"
#include "php_compat.h"

#include "zend_API.h"

#if PHP_BROKEN_SPRINTF
#undef sprintf
#define sprintf php_sprintf
#endif

#ifdef PHP_WIN32
#include "tsrm_win32.h"
#include "win95nt.h"
#	ifdef PHP_EXPORTS
#	define PHPAPI __declspec(dllexport)
#	else
#	define PHPAPI __declspec(dllimport)
#	endif
#define PHP_DIR_SEPARATOR '\\'
#else
#define PHPAPI
#define THREAD_LS
#define PHP_DIR_SEPARATOR '/'
#endif

#include "php_regex.h"

/* PHP's DEBUG value must match Zend's ZEND_DEBUG value */
#undef PHP_DEBUG
#define PHP_DEBUG ZEND_DEBUG


#define APACHE 0
#define CGI_BINARY 0

#if HAVE_UNIX_H
#include <unix.h>
#endif

#if HAVE_ALLOCA_H
#include <alloca.h>
#endif

/*
 * This is a fast version of strlcpy which should be used, if you
 * know the size of the destination buffer and if you know
 * the length of the source string.
 *
 * size is the allocated number of bytes of dst
 * src_size is the number of bytes excluding the NUL of src
 */

#define PHP_STRLCPY(dst, src, size, src_size)	\
	{											\
		size_t php_str_len;						\
												\
		if (src_size >= size)					\
			php_str_len = size - 1;				\
		else									\
			php_str_len = src_size;				\
		memcpy(dst, src, php_str_len);			\
		dst[php_str_len] = '\0';				\
	}

#ifndef HAVE_STRLCPY
PHPAPI size_t strlcpy(char *dst, const char *src, size_t siz);
#endif

#ifndef HAVE_STRLCAT
PHPAPI size_t strlcat(char *dst, const char *src, size_t siz);
#endif

#ifndef HAVE_STRTOK_R
char *strtok_r(char *s, const char *delim, char **last);
#endif

#ifndef HAVE_SOCKLEN_T
typedef unsigned int socklen_t;
#endif

#define CREATE_MUTEX(a,b)
#define SET_MUTEX(a)
#define FREE_MUTEX(a)

/*
 * Then the ODBC support can use both iodbc and Solid,
 * uncomment this.
 * #define HAVE_ODBC (HAVE_IODBC|HAVE_SOLID)
 */

#include <stdlib.h>
#include <ctype.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_STDARG_H
#include <stdarg.h>
#else
# if HAVE_SYS_VARARGS_H
# include <sys/varargs.h>
# endif
#endif


#include "zend_hash.h"
#include "php3_compat.h"
#include "zend_alloc.h"
#include "zend_stack.h"

#if STDC_HEADERS
# include <string.h>
#else
# ifndef HAVE_MEMCPY
#  define memcpy(d, s, n)	bcopy((s), (d), (n))
# endif
# ifndef HAVE_MEMMOVE
#  define memmove(d, s, n)	bcopy ((s), (d), (n))
# endif
#endif

#include "safe_mode.h"

#ifndef HAVE_STRERROR
char *strerror(int);
#endif

#include "php_streams.h"
#include "fopen_wrappers.h"

#if (REGEX == 1 || REGEX == 0) && !defined(NO_REGEX_EXTRA_H)
#include "regex/regex_extra.h"
#endif

#if HAVE_PWD_H
# ifdef PHP_WIN32
#include "win32/pwd.h"
#include "win32/param.h"
# else
#include <pwd.h>
#include <sys/param.h>
# endif
#endif

#if HAVE_LIMITS_H
#include <limits.h>
#endif

#ifndef LONG_MAX
#define LONG_MAX 2147483647L
#endif

#ifndef LONG_MIN
#define LONG_MIN (- LONG_MAX - 1)
#endif

#if !defined(HAVE_SNPRINTF) || !defined(HAVE_VSNPRINTF) || defined(BROKEN_SPRINTF)
#include "snprintf.h"
#endif

#define EXEC_INPUT_BUF 4096

#define PHP_MIME_TYPE "application/x-httpd-php"

/* macros */
#define STR_PRINT(str)	((str)?(str):"")

#ifndef MAXPATHLEN
# ifdef PATH_MAX
#  define MAXPATHLEN PATH_MAX
# else
#  define MAXPATHLEN 256    /* Should be safe for any weird systems that do not define it */
# endif
#endif

#define PHP_FN(name) php_if_##name
#define PHP_NAMED_FUNCTION(name) void name(INTERNAL_FUNCTION_PARAMETERS)
#define PHP_FUNCTION(name) PHP_NAMED_FUNCTION(PHP_FN(name))

#define PHP_NAMED_FE(php_name, name, arg_types) { #php_name, name, arg_types },
#define PHP_FE(name, arg_types) PHP_NAMED_FE(name, PHP_FN(name), arg_types)
#define PHP_FALIAS(name, alias, arg_types) PHP_NAMED_FE(name, PHP_FN(alias), arg_types)
#define PHP_STATIC_FE(php_name, func_name, arg_types) { php_name, func_name, arg_types },

#define PHP_MINIT(module)	php_minit_##module
#define PHP_MSHUTDOWN(module)	php_mshutdown_##module
#define PHP_RINIT(module)	php_rinit_##module
#define PHP_RSHUTDOWN(module)	php_rshutdown_##module
#define PHP_MINFO(module)	php_info_##module
#define PHP_GINIT(module)	php_ginit_##module
#define PHP_GSHUTDOWN(module)	php_gshutdown_##module

#define PHP_MINIT_FUNCTION(module)	int PHP_MINIT(module)(INIT_FUNC_ARGS)
#define PHP_MSHUTDOWN_FUNCTION(module)	int PHP_MSHUTDOWN(module)(SHUTDOWN_FUNC_ARGS)
#define PHP_RINIT_FUNCTION(module)	int PHP_RINIT(module)(INIT_FUNC_ARGS)
#define PHP_RSHUTDOWN_FUNCTION(module)	int PHP_RSHUTDOWN(module)(SHUTDOWN_FUNC_ARGS)
#define PHP_MINFO_FUNCTION(module)	void PHP_MINFO(module)(ZEND_MODULE_INFO_FUNC_ARGS)
#define PHP_GINIT_FUNCTION(module)	int PHP_GINIT(module)(GINIT_FUNC_ARGS)
#define PHP_GSHUTDOWN_FUNCTION(module)	int PHP_GSHUTDOWN(module)(void)


/* global variables */
extern pval *data;
#if !defined(PHP_WIN32)
extern char **environ;
#define php_sleep sleep
#endif

void phperror(char *error);
PHPAPI int php_write(void *buf, uint size);
PHPAPI int php_printf(const char *format, ...);
PHPAPI void php_log_err(char *log_message);
int Debug(char *format, ...);
int cfgparse(void);

#define php_error zend_error

#define zenderror phperror
#define zendlex phplex

#define phpparse zendparse
#define phprestart zendrestart
#define phpin zendin

/* functions */
int php_startup_internal_extensions(void);
int php_global_startup_internal_extensions(void);
int php_global_shutdown_internal_extensions(void);

int php_mergesort(void *base, size_t nmemb, register size_t size, int (*cmp) (const void *, const void *));

PHPAPI void php_register_pre_request_shutdown(void (*func)(void *), void *userdata);

PHPAPI int cfg_get_long(char *varname, long *result);
PHPAPI int cfg_get_double(char *varname, double *result);
PHPAPI int cfg_get_string(char *varname, char **result);


/* Output support */
#include "ext/standard/php_output.h"
#define PHPWRITE(str, str_len)		php_body_write((str), (str_len))
#define PUTS(str)					php_body_write((str), strlen((str)))
#define PUTC(c)						(php_body_write(&(c), 1), (c))
#define PHPWRITE_H(str, str_len)	php_header_write((str), (str_len))
#define PUTS_H(str)					php_header_write((str), strlen((str)))
#define PUTC_H(c)					(php_header_write(&(c), 1), (c))

#ifdef ZTS
#define VIRTUAL_DIR
#endif

/* Virtual current working directory support */
#include "tsrm_virtual_cwd.h"

#include "zend_constants.h"

/* connection status states */
#define PHP_CONNECTION_NORMAL  0
#define PHP_CONNECTION_ABORTED 1
#define PHP_CONNECTION_TIMEOUT 2

#include "php_reentrancy.h"

/* Finding offsets of elements within structures.
 * Taken from the Apache code, which in turn, was taken from X code...
 */

#if defined(CRAY) || (defined(__arm) && !defined(LINUX))
#ifdef __STDC__
#define XtOffset(p_type,field) _Offsetof(p_type,field)
#else
#ifdef CRAY2
#define XtOffset(p_type,field) \
    (sizeof(int)*((unsigned int)&(((p_type)NULL)->field)))

#else /* !CRAY2 */

#define XtOffset(p_type,field) ((unsigned int)&(((p_type)NULL)->field))

#endif /* !CRAY2 */
#endif /* __STDC__ */
#else /* ! (CRAY || __arm) */

#define XtOffset(p_type,field) \
    ((long) (((char *) (&(((p_type)NULL)->field))) - ((char *) NULL)))

#endif /* !CRAY */

#ifdef offsetof
#define XtOffsetOf(s_type,field) offsetof(s_type,field)
#else
#define XtOffsetOf(s_type,field) XtOffset(s_type*,field)
#endif

PHPAPI PHP_FUNCTION(warn_not_available);

#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */
