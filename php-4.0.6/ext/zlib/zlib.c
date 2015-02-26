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
   | Authors: Rasmus Lerdorf <rasmus@lerdorf.on.ca>                       |
   |          Stefan R�hrich <sr@linux.de>                                |
   +----------------------------------------------------------------------+
 */
/* $Id: zlib.c,v 1.77.2.1 2001/05/24 12:42:13 ssb Exp $ */
#define IS_EXT_MODULE

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "SAPI.h"
#include "php_ini.h"

#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifdef PHP_WIN32
#include <windows.h>
#include <winsock.h>
#define O_RDONLY _O_RDONLY
#include "win32/param.h"
#else
#include <sys/param.h>
/* #include <sys/uio.h> */
#endif
#include "ext/standard/head.h"
#include "safe_mode.h"
#include "ext/standard/php_standard.h"
#include "ext/standard/info.h"
#include "php_zlib.h"
#include "fopen_wrappers.h"
#if HAVE_PWD_H
#ifdef PHP_WIN32
#include "win32/pwd.h"
#else
#include <pwd.h>
#endif
#endif
#if defined(HAVE_UNISTD_H) && defined(PHP_WIN32)
#undef HAVE_UNISTD_H
#endif


#ifdef COMPILE_DL_ZLIB
#ifndef PUTS
#define PUTS(a) php_printf("%s",a)
#endif
#ifndef PUTC
#define PUTC(a) PUTS(a)
#endif
#ifndef PHPWRITE
#define PHPWRITE(a,n) php_write((a),(n))
#endif
#endif

#ifdef ZTS
int zlib_globals_id;
#else
static php_zlib_globals zlib_globals;
#endif

#define OS_CODE			0x03 /* FIXME */
#define CODING_GZIP		1
#define CODING_DEFLATE	2

/* True globals, no need for thread safety */
static int le_zp;
static int gz_magic[2] = {0x1f, 0x8b};	/* gzip magic header */


function_entry php_zlib_functions[] = {
	PHP_FE(readgzfile,					NULL)
	PHP_FE(gzrewind,					NULL)
	PHP_FE(gzclose,						NULL)
	PHP_FE(gzeof,						NULL)
	PHP_FE(gzgetc,						NULL)
	PHP_FE(gzgets,						NULL)
	PHP_FE(gzgetss,						NULL)
	PHP_FE(gzread,						NULL)
	PHP_FE(gzopen,						NULL)
	PHP_FE(gzpassthru,					NULL)
	PHP_FE(gzseek,						NULL)
	PHP_FE(gztell,						NULL)
	PHP_FE(gzwrite,						NULL)
	PHP_FALIAS(gzputs,		gzwrite,	NULL)
	PHP_FE(gzfile,						NULL)
	PHP_FE(gzcompress,                  NULL)
	PHP_FE(gzuncompress,                NULL)
	PHP_FE(gzdeflate,                   NULL)
	PHP_FE(gzinflate,                   NULL)
	PHP_FE(gzencode,					NULL)
	PHP_FE(ob_gzhandler,				NULL)
	{NULL, NULL, NULL}
};


PHP_INI_BEGIN()
    STD_PHP_INI_BOOLEAN("zlib.output_compression",   "0",    PHP_INI_ALL,     OnUpdateInt,        output_compression,   php_zlib_globals,     zlib_globals)
PHP_INI_END()


zend_module_entry php_zlib_module_entry = {
	"zlib",
	php_zlib_functions,
	PHP_MINIT(zlib),
	PHP_MSHUTDOWN(zlib),
	PHP_RINIT(zlib),
	NULL,
	PHP_MINFO(zlib),
	STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_ZLIB
ZEND_GET_MODULE(php_zlib)
#endif

static void phpi_destructor_gzclose(zend_rsrc_list_entry *rsrc)
{
	gzFile *zp = (gzFile *)rsrc->ptr;
	(void)gzclose(zp);
}

#ifdef ZTS
static void php_zlib_init_globals(ZLIBLS_D)
{
        ZLIBG(gzgetss_state) = 0;
}
#endif

PHP_MINIT_FUNCTION(zlib)
{
	PLS_FETCH();

#ifdef ZTS
        zlib_globals_id = ts_allocate_id(sizeof(php_zlib_globals), (ts_allocate_ctor) php_zlib_init_globals, NULL);
#else
        ZLIBG(gzgetss_state)=0;
#endif
	le_zp = zend_register_list_destructors_ex(phpi_destructor_gzclose, NULL, "zlib", module_number);

#if HAVE_FOPENCOOKIE

	if(PG(allow_url_fopen)) {
		php_register_url_wrapper("zlib",zlib_fopen_wrapper);
	}
#endif

	REGISTER_LONG_CONSTANT("FORCE_GZIP", CODING_GZIP, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FORCE_DEFLATE", CODING_DEFLATE, CONST_CS | CONST_PERSISTENT);

	REGISTER_INI_ENTRIES();

	return SUCCESS;
}

PHP_RINIT_FUNCTION(zlib)
{
	ZLIBLS_FETCH();

	ZLIBG(ob_gzhandler_status) = 0;
	switch (ZLIBG(output_compression)) {
		case 0:
			break;
		case 1:
			php_enable_output_compression(4096);
			break;
		default:
			php_enable_output_compression(ZLIBG(output_compression));
	}
	return SUCCESS;
}


PHP_MSHUTDOWN_FUNCTION(zlib)
{
#if HAVE_FOPENCOOKIE
	PLS_FETCH();

	if(PG(allow_url_fopen)) {
	    php_unregister_url_wrapper("zlib"); 
    }
#endif
	
	UNREGISTER_INI_ENTRIES();

	return SUCCESS;
}

PHP_MINFO_FUNCTION(zlib)
{
	php_info_print_table_start();
	php_info_print_table_row(2, "ZLib Support", "enabled");
#if HAVE_FOPENCOOKIE
	php_info_print_table_row(2, "'zlib:' fopen wrapper", "enabled");
#endif
	php_info_print_table_row(2, "Compiled Version", ZLIB_VERSION );
	php_info_print_table_row(2, "Linked Version", (char *)zlibVersion() );
	php_info_print_table_end();
}

static gzFile php_gzopen_wrapper(char *path, char *mode, int options)
{
	FILE *f;
	int issock=0, socketd=0;

	f = php_fopen_wrapper(path, mode, options, &issock, &socketd, NULL);

	if (!f) {
		return NULL;
	}
	return gzdopen(fileno(f), mode);
}

/* {{{ proto array gzfile(string filename [, int use_include_path])
   Read und uncompress entire .gz-file into an array */
PHP_FUNCTION(gzfile)
{
	pval **filename, **arg2;
	gzFile zp;
	char *slashed, buf[8192];
	register int i=0;
	int use_include_path = 0;
	PLS_FETCH();

	/* check args */
	switch (ZEND_NUM_ARGS()) {
	case 1:
		if (zend_get_parameters_ex(1,&filename) == FAILURE) {
			WRONG_PARAM_COUNT;
		}
		break;
	case 2:
		if (zend_get_parameters_ex(2,&filename,&arg2) == FAILURE) {
			WRONG_PARAM_COUNT;
		}
		convert_to_long_ex(arg2);
		use_include_path = (*arg2)->value.lval?USE_PATH:0;
		break;
	default:
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(filename);

	zp = php_gzopen_wrapper((*filename)->value.str.val,"r", use_include_path|ENFORCE_SAFE_MODE);
	if (!zp) {
		php_error(E_WARNING,"gzFile(\"%s\") - %s",(*filename)->value.str.val,strerror(errno));
		RETURN_FALSE;
	}

	/* Initialize return array */
	if (array_init(return_value) == FAILURE) {
		RETURN_FALSE;
	}

	/* Now loop through the file and do the magic quotes thing if needed */
	memset(buf,0,8191);
	while(gzgets(zp, buf, 8191) != NULL) {
		if (PG(magic_quotes_runtime)) {
			int len;
			
			slashed = php_addslashes(buf,0,&len,0); /* 0 = don't free source string */
            add_index_stringl(return_value, i++, slashed, len, 0);
		} else {
			add_index_string(return_value, i++, buf, 1);
		}
	}
	gzclose(zp);
}
/* }}} */

/* {{{ proto int gzopen(string filename, string mode [, int use_include_path])
   Open a .gz-file and return a .gz-file pointer */
PHP_FUNCTION(gzopen)
{
	pval **arg1, **arg2, **arg3;
	gzFile *zp;
	char *p;
	int use_include_path = 0;
	ZLIBLS_FETCH();
	
	switch(ZEND_NUM_ARGS()) {
	case 2:
		if (zend_get_parameters_ex(2,&arg1,&arg2) == FAILURE) {
			WRONG_PARAM_COUNT;
		}
		break;
	case 3:
		if (zend_get_parameters_ex(3,&arg1,&arg2,&arg3) == FAILURE) {
			WRONG_PARAM_COUNT;
		}
		convert_to_long_ex(arg3);
		use_include_path = (*arg3)->value.lval?USE_PATH:0;
		break;
	default:
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(arg1);
	convert_to_string_ex(arg2);
	p = estrndup((*arg2)->value.str.val,(*arg2)->value.str.len);

	/*
	 * We need a better way of returning error messages from
	 * php_gzopen_wrapper().
	 */
	zp = php_gzopen_wrapper((*arg1)->value.str.val, p, use_include_path|ENFORCE_SAFE_MODE);
	if (!zp) {
		php_error(E_WARNING,"gzopen(\"%s\",\"%s\") - %s",
					(*arg1)->value.str.val, p, strerror(errno));
		efree(p);
		RETURN_FALSE;
	}
	ZLIBG(gzgetss_state)=0;
	efree(p);
	ZEND_REGISTER_RESOURCE(return_value, zp, le_zp);
}	
/* }}} */

/* {{{ proto int gzclose(int zp)
   Close an open .gz-file pointer */
PHP_FUNCTION(gzclose)
{
	pval **arg1;
	gzFile *zp;

	if (ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &arg1) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	ZEND_FETCH_RESOURCE(zp, gzFile *, arg1, -1, "Zlib file", le_zp);
	zend_list_delete((*arg1)->value.lval);
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto int gzeof(int zp)
   Test for end-of-file on a .gz-file pointer */
PHP_FUNCTION(gzeof)
{
	pval **arg1;
	gzFile *zp;
	
	if (ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &arg1) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	ZEND_FETCH_RESOURCE(zp, gzFile *, arg1, -1, "Zlib file", le_zp);

	if ((gzeof(zp))) {
		RETURN_TRUE;
	} else {
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ proto string gzgets(int zp, int length)
   Get a line from .gz-file pointer */
PHP_FUNCTION(gzgets)
{
	pval **arg1, **arg2;
	gzFile *zp;
	int len;
	char *buf;
	PLS_FETCH();
	
	if (ZEND_NUM_ARGS() != 2 || zend_get_parameters_ex(2, &arg1, &arg2) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_long_ex(arg2);
	len = (*arg2)->value.lval;

	ZEND_FETCH_RESOURCE(zp, gzFile *, arg1, -1, "Zlib file", le_zp);

	buf = emalloc(sizeof(char) * (len + 1));
	/* needed because recv doesnt put a null at the end*/
	memset(buf,0,len+1);
	if (!(gzgets(zp, buf, len) != NULL)) {
		efree(buf);
		RETVAL_FALSE;
	} else {
		if (PG(magic_quotes_runtime)) {
			return_value->value.str.val = php_addslashes(buf,0,&return_value->value.str.len,1);
		} else {
			return_value->value.str.val = buf;
			return_value->value.str.len = strlen(return_value->value.str.val);
		}
		return_value->type = IS_STRING;
	}
	return;
}
/* }}} */

/* {{{ proto string gzgetc(int zp)
   Get a character from .gz-file pointer */
PHP_FUNCTION(gzgetc)
{
	pval **arg1;
	gzFile *zp;
	int c;
	char *buf;
	
	if (ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &arg1) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(zp, gzFile *, arg1, -1, "Zlib file", le_zp);

	buf = emalloc(sizeof(char) * 2);
	if ((c=gzgetc(zp)) == (-1)) {
		efree(buf);
		RETVAL_FALSE;
	} else {
		buf[0]=(char)c;
		buf[1]='\0';
		return_value->value.str.val = buf; 
		return_value->value.str.len = 1; 
		return_value->type = IS_STRING;
	}
	return;
}
/* }}} */

/* Strip any HTML tags while reading */
/* {{{ proto string gzgetss(int zp, int length [, string allowable_tags])
   Get a line from file pointer and strip HTML tags */
PHP_FUNCTION(gzgetss)
{
	pval **fd, **bytes, **allow=NULL;
	gzFile *zp;
	int len;
	char *buf;
	char *allowed_tags=NULL;
	int allowed_tags_len=0;
	ZLIBLS_FETCH();
	
	switch(ZEND_NUM_ARGS()) {
		case 2:
			if(zend_get_parameters_ex(2, &fd, &bytes) == FAILURE) {
				RETURN_FALSE;
			}
			break;
		case 3:
			if(zend_get_parameters_ex(3, &fd, &bytes, &allow) == FAILURE) {
				RETURN_FALSE;
			}
			convert_to_string_ex(allow);
			allowed_tags = (*allow)->value.str.val;
			allowed_tags_len = (*allow)->value.str.len;
			break;
		default:
			WRONG_PARAM_COUNT;
			/* NOTREACHED */
			break;
	}

	convert_to_long_ex(bytes);

	len = (*bytes)->value.lval;

	ZEND_FETCH_RESOURCE(zp, gzFile *, fd, -1, "Zlib file", le_zp);

	buf = emalloc(sizeof(char) * (len + 1));
	/*needed because recv doesnt set null char at end*/
	memset(buf,0,len+1);
	if (!(gzgets(zp, buf, len) != NULL)) {
		efree(buf);
		RETURN_FALSE;
	}

	/* strlen() can be used here since we are doing it on the return of an fgets() anyway */
	php_strip_tags(buf, strlen(buf), ZLIBG(gzgetss_state), allowed_tags, allowed_tags_len);
	RETURN_STRING(buf, 0);
	
}
/* }}} */

/* {{{ proto int gzwrite(int zp, string str [, int length])
   Binary-safe .gz-file write */
PHP_FUNCTION(gzwrite)
{
	pval **arg1, **arg2, **arg3=NULL;
	gzFile *zp;
	int ret;
	int num_bytes;
	PLS_FETCH();

	switch (ZEND_NUM_ARGS()) {
		case 2:
			if (zend_get_parameters_ex(2, &arg1, &arg2)==FAILURE) {
				RETURN_FALSE;
			}
			convert_to_string_ex(arg2);
			num_bytes = (*arg2)->value.str.len;
			break;
		case 3:
			if (zend_get_parameters_ex(3, &arg1, &arg2, &arg3)==FAILURE) {
				RETURN_FALSE;
			}
			convert_to_string_ex(arg2);
			convert_to_long_ex(arg3);
			num_bytes = MIN((*arg3)->value.lval, (*arg2)->value.str.len);
			break;
		default:
			WRONG_PARAM_COUNT;
			/* NOTREACHED */
			break;
	}				
	ZEND_FETCH_RESOURCE(zp, gzFile *, arg1, -1, "Zlib file", le_zp);

	/* strip slashes only if the length wasn't specified explicitly */
	if (!arg3 && PG(magic_quotes_runtime)) {
		php_stripslashes((*arg2)->value.str.val,&num_bytes);
	}

	ret = gzwrite(zp, (*arg2)->value.str.val,num_bytes);
	RETURN_LONG(ret);
}	
/* }}} */

/* {{{ proto int gzputs(int zp, string str [, int length])
   An alias for gzwrite */
/* }}} */

/* {{{ proto int gzrewind(int zp)
   Rewind the position of a .gz-file pointer */
PHP_FUNCTION(gzrewind)
{
	pval **arg1;
	gzFile *zp;
	
	if (ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &arg1) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(zp, gzFile *, arg1, -1, "Zlib file", le_zp);

	gzrewind(zp);
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto int gztell(int zp)
   Get .gz-file pointer's read/write position */
PHP_FUNCTION(gztell)
{
	pval **arg1;
	long pos;
	gzFile *zp;
	
	if (ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &arg1) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(zp, gzFile *, arg1, -1, "Zlib file", le_zp);

	pos = gztell(zp);
	RETURN_LONG(pos);
}
/* }}} */

/* {{{ proto int gzseek(int zp, int offset)
   Seek on a file pointer */
PHP_FUNCTION(gzseek)
{
	pval **arg1, **arg2;
	int ret;
	gzFile *zp;
	
	if (ZEND_NUM_ARGS() != 2 || zend_get_parameters_ex(2, &arg1, &arg2) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_long_ex(arg2);

	ZEND_FETCH_RESOURCE(zp, gzFile *, arg1, -1, "Zlib file", le_zp);

 	ret = gzseek(zp, (*arg2)->value.lval, SEEK_SET);
	RETURN_LONG(ret);
}
/* }}} */

/*
 * Read a file and write the ouput to stdout
 */
/* {{{ proto int readgzfile(string filename [, int use_include_path])
   Output a .gz-file */
PHP_FUNCTION(readgzfile)
{
	pval **arg1, **arg2;
	char buf[8192];
	gzFile *zp;
	int b, size;
	int use_include_path = 0;

	
	/* check args */
	switch (ZEND_NUM_ARGS()) {
	case 1:
		if (zend_get_parameters_ex(1,&arg1) == FAILURE) {
			WRONG_PARAM_COUNT;
		}
		break;
	case 2:
		if (zend_get_parameters_ex(2,&arg1,&arg2) == FAILURE) {
			WRONG_PARAM_COUNT;
		}
		convert_to_long_ex(arg2);
		use_include_path = (*arg2)->value.lval?USE_PATH:0;
		break;
	default:
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(arg1);

	/*
	 * We need a better way of returning error messages from
	 * php_gzopen_wrapper().
	 */
	zp = php_gzopen_wrapper((*arg1)->value.str.val,"r", use_include_path|ENFORCE_SAFE_MODE);
	if (!zp){
		php_error(E_WARNING,"ReadGzFile(\"%s\") - %s",(*arg1)->value.str.val,strerror(errno));
		RETURN_FALSE;
	}
	size= 0;
	while((b = gzread(zp, buf, sizeof(buf))) > 0) {
		PHPWRITE(buf,b);
		size += b ;
	}
   	gzclose(zp);
	RETURN_LONG(size);
}
/* }}} */

/*
 * Read to EOF on a file descriptor and write the output to stdout.
 */
/* {{{ proto int gzpassthru(int zp)
   Output all remaining data from a .gz-file pointer */
PHP_FUNCTION(gzpassthru)
{
	pval **arg1;
	gzFile *zp;
	char buf[8192];
	int size, b;
	
	if (ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &arg1) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(zp, gzFile *, arg1, -1, "Zlib file", le_zp);

	size = 0;
	while((b = gzread(zp, buf, sizeof(buf))) > 0) {
		PHPWRITE(buf,b);
		size += b ;
	}
/*  gzclose(zp); */
	zend_list_delete((*arg1)->value.lval);
	RETURN_LONG(size);
}
/* }}} */

/* {{{ proto string gzread(int zp, int length)
   Binary-safe file read */
PHP_FUNCTION(gzread)
{
	pval **arg1, **arg2;
	gzFile *zp;
	int len;
	PLS_FETCH();
	
	if (ZEND_NUM_ARGS() != 2 || zend_get_parameters_ex(2, &arg1, &arg2) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_long_ex(arg2);
	len = (*arg2)->value.lval;

	ZEND_FETCH_RESOURCE(zp, gzFile *, arg1, -1, "Zlib file", le_zp);

	return_value->value.str.val = emalloc(sizeof(char) * (len + 1));
	/* needed because recv doesnt put a null at the end*/
	
	return_value->value.str.len = gzread(zp, return_value->value.str.val, len);
	return_value->value.str.val[return_value->value.str.len] = 0;

	if (PG(magic_quotes_runtime)) {
		return_value->value.str.val = php_addslashes(return_value->value.str.val,return_value->value.str.len,&return_value->value.str.len,1);
	}
	return_value->type = IS_STRING;
}

/* }}} */
	

/* {{{ proto string gzcompress(string data [, int level]) 
   Gzip-compress a string */
PHP_FUNCTION(gzcompress)
{
	zval **data, **zlimit = NULL;
	int limit,status;
	unsigned long l2;
	char *s2;

	switch (ZEND_NUM_ARGS()) {
	case 1:
		if (zend_get_parameters_ex(1, &data) == FAILURE)
			WRONG_PARAM_COUNT;
		limit=-1;
		break;
	case 2:
		if (zend_get_parameters_ex(2, &data, &zlimit) == FAILURE)
			WRONG_PARAM_COUNT;
		convert_to_long_ex(zlimit);
		limit = (*zlimit)->value.lval;
		if((limit<0)||(limit>9)) {
			php_error(E_WARNING,"gzcompress: compression level must be whithin 0..9");
			RETURN_FALSE;
		}
		break;
	default:
		WRONG_PARAM_COUNT;                                         
	}
	convert_to_string_ex(data);
	
	l2 = (*data)->value.str.len + ((*data)->value.str.len/1000) + 15;
	s2 = (char *) emalloc(l2);
	if(! s2) RETURN_FALSE;
	
	if(limit>=0) {
		status = compress2(s2,&l2,(*data)->value.str.val, (*data)->value.str.len,limit);
	} else {
		status = compress(s2,&l2,(*data)->value.str.val, (*data)->value.str.len);
	}
	
	if(status==Z_OK) {
		RETURN_STRINGL(s2, l2, 0);
	} else {
		efree(s2);
		php_error(E_WARNING,"gzcompress: %s",zError(status));
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ proto string gzuncompress(string data, int length) 
   Unzip a gzip-compressed string */
PHP_FUNCTION(gzuncompress)
{
	zval **data, **zlimit = NULL;
	int status,factor=1,maxfactor=8;
	unsigned long plength=0,length;
	char *s1=NULL,*s2=NULL;

	switch (ZEND_NUM_ARGS()) {
	case 1:
		if (zend_get_parameters_ex(1, &data) == FAILURE)
			WRONG_PARAM_COUNT;
		length=0;
		break;
	case 2:
		if (zend_get_parameters_ex(2, &data, &zlimit) == FAILURE)
			WRONG_PARAM_COUNT;
		convert_to_long_ex(zlimit);
		if((*zlimit)->value.lval<=0) {
			php_error(E_WARNING,"gzuncompress: length must be greater zero");
			RETURN_FALSE;
		}
		plength = (*zlimit)->value.lval;
		break;
	default:
		WRONG_PARAM_COUNT;                                         
	}
	convert_to_string_ex(data);

	/*
	 zlib::uncompress() wants to know the output data length
	 if none was given as a parameter
	 we try from input length * 2 up to input length * 2^8
	 doubling it whenever it wasn't big enough
	 that should be eneugh for all real life cases	
	*/
	do {
		length=plength?plength:(*data)->value.str.len*(1<<factor++);
		s2 = (char *) erealloc(s1,length);
		if(! s2) { if(s1) efree(s1); RETURN_FALSE; }
		status = uncompress(s2, &length ,(*data)->value.str.val, (*data)->value.str.len);
		s1=s2;
	} while((status==Z_BUF_ERROR)&&(!plength)&&(factor<maxfactor));

	if(status==Z_OK) {
		s2 = erealloc(s2, length);
		RETURN_STRINGL(s2, length, 0);
	} else {
		efree(s2);
		php_error(E_WARNING,"gzuncompress: %s",zError(status));
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ proto string gzdeflate(string data [, int level]) 
   Gzip-compress a string */
PHP_FUNCTION(gzdeflate)
{
	zval **data, **zlimit = NULL;
	int level,status;
	z_stream stream;
	char *s2;

	switch (ZEND_NUM_ARGS()) {
	case 1:
		if (zend_get_parameters_ex(1, &data) == FAILURE)
			WRONG_PARAM_COUNT;
		level=Z_DEFAULT_COMPRESSION;
		break;
	case 2:
		if (zend_get_parameters_ex(2, &data, &zlimit) == FAILURE)
			WRONG_PARAM_COUNT;
		convert_to_long_ex(zlimit);
		level = (*zlimit)->value.lval;
		if((level<0)||(level>9)) {
			php_error(E_WARNING,"gzdeflate: compression level must be whithin 0..9");
			RETURN_FALSE;
		}
		break;
	default:
		WRONG_PARAM_COUNT;                                         
	}
	convert_to_string_ex(data);

	stream.data_type = Z_ASCII;
	stream.zalloc = (alloc_func) Z_NULL;
	stream.zfree  = (free_func) Z_NULL;
	stream.opaque = (voidpf) Z_NULL;

	stream.next_in = (Bytef*) (*data)->value.str.val;
	stream.avail_in = (*data)->value.str.len;

	stream.avail_out = stream.avail_in + (stream.avail_in/1000) + 15;
	s2 = (char *) emalloc(stream.avail_out);
	if(!s2) RETURN_FALSE;
	stream.next_out = s2;

    /* init with -MAX_WBITS disables the zlib internal headers */
	status = deflateInit2(&stream, level, Z_DEFLATED, -MAX_WBITS, MAX_MEM_LEVEL, 0);
	if (status == Z_OK) {

		status = deflate(&stream, Z_FINISH);
		if (status != Z_STREAM_END) {
			deflateEnd(&stream);
			if (status == Z_OK) {
				status = Z_BUF_ERROR;
			}
		} else {
			status = deflateEnd(&stream);
		}
	}

	if(status==Z_OK) {
		RETURN_STRINGL(s2, stream.total_out, 0);
	} else {
		efree(s2);
		php_error(E_WARNING,"gzdeflate: %s",zError(status));
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ proto string gzinflate(string data, int length) 
   Unzip a gzip-compressed string */
PHP_FUNCTION(gzinflate)
{
	zval **data, **zlimit = NULL;
	int status,factor=1,maxfactor=8;
	unsigned long plength=0,length;
	char *s1=NULL,*s2=NULL;
	z_stream stream;

	switch (ZEND_NUM_ARGS()) {
	case 1:
		if (zend_get_parameters_ex(1, &data) == FAILURE)
			WRONG_PARAM_COUNT;
		length=0;
		break;
	case 2:
		if (zend_get_parameters_ex(2, &data, &zlimit) == FAILURE)
			WRONG_PARAM_COUNT;
		convert_to_long_ex(zlimit);
		if((*zlimit)->value.lval<=0) {
			php_error(E_WARNING,"gzinflate: length must be greater zero");
			RETURN_FALSE;
		}
		plength = (*zlimit)->value.lval;
		break;
	default:
		WRONG_PARAM_COUNT;                                         
	}
	convert_to_string_ex(data);

	/*
	  stream.avail_out wants to know the output data length
	  if none was given as a parameter
	  we try from input length * 2 up to input length * 2^8
	  doubling it whenever it wasn't big enough
	  that should be enaugh for all real life cases	
	*/
	stream.zalloc = (alloc_func) Z_NULL;
	stream.zfree = (free_func) Z_NULL;

	do {
		length=plength?plength:(*data)->value.str.len*(1<<factor++);
		s2 = (char *) erealloc(s1,length);
		if(! s2) { if(s1) efree(s1); RETURN_FALSE; }

		stream.next_in = (Bytef*) (*data)->value.str.val;
		stream.avail_in = (uInt) (*data)->value.str.len;

		stream.next_out = s2;
		stream.avail_out = (uInt) length;

		/* init with -MAX_WBITS disables the zlib internal headers */
		status = inflateInit2(&stream, -MAX_WBITS);
		if (status == Z_OK) {
			status = inflate(&stream, Z_FINISH);
			if (status != Z_STREAM_END) {
				inflateEnd(&stream);
				if (status == Z_OK) {
					status = Z_BUF_ERROR;
				}
			} else {
				status = inflateEnd(&stream);
			}
		}
		s1=s2;
		
	} while((status==Z_BUF_ERROR)&&(!plength)&&(factor<maxfactor));

	if(status==Z_OK) {
		s2 = erealloc(s2, stream.total_out);
		RETURN_STRINGL(s2, stream.total_out, 0);
	} else {
		efree(s2);
		php_error(E_WARNING,"gzinflate: %s",zError(status));
		RETURN_FALSE;
	}
}
/* }}} */



static int php_do_deflate(uint str_length, Bytef **p_buffer, uint *p_buffer_len, zend_bool do_start, zend_bool do_end ZLIBLS_DC)
{
	Bytef *buffer;
	uInt prev_outlen, outlen;
	int err;
	int start_offset = (do_start?10:0);
	int end_offset = (do_end?8:0);

	outlen = sizeof(char) * (str_length * 1.001 + 12);
	if ((outlen+start_offset+end_offset) > *p_buffer_len) {
		buffer = (Bytef *) emalloc(outlen+start_offset+end_offset);
	} else {
		buffer = *p_buffer;
	}
	
	ZLIBG(stream).next_out = buffer+start_offset;
	ZLIBG(stream).avail_out = outlen;


	err = deflate(&ZLIBG(stream), Z_SYNC_FLUSH);
	while (err == Z_OK && !ZLIBG(stream).avail_out) {
		prev_outlen = outlen;
		outlen *= 3;
		if ((outlen+start_offset+end_offset) > *p_buffer_len) {
			buffer = realloc(buffer, outlen+start_offset+end_offset);
		}
		
		ZLIBG(stream).next_out = buffer+start_offset + prev_outlen;
		ZLIBG(stream).avail_out = prev_outlen * 2;

		err = deflate(&ZLIBG(stream), Z_SYNC_FLUSH);
	}

	if (do_end) {
		err = deflate(&ZLIBG(stream), Z_FINISH);
	}


	*p_buffer = buffer;
	*p_buffer_len = outlen - ZLIBG(stream).avail_out;

	return err;
}


int php_deflate_string(const char *str, uint str_length, char **newstr, uint *new_length, int coding, zend_bool do_start, zend_bool do_end)
{
	int err;
	ZLIBLS_FETCH();

	ZLIBG(compression_coding) = coding;

	if (do_start) {
		ZLIBG(stream).zalloc = Z_NULL;
		ZLIBG(stream).zfree = Z_NULL;
		ZLIBG(stream).opaque = Z_NULL;
		switch (coding) {
			case CODING_GZIP:
				/* windowBits is passed < 0 to suppress zlib header & trailer */
				if (deflateInit2(&ZLIBG(stream), Z_DEFAULT_COMPRESSION, Z_DEFLATED,
						-MAX_WBITS, MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY)
							!= Z_OK) {
					/* TODO: print out error */
					return FAILURE;
				}
		
				ZLIBG(crc) = crc32(0L, Z_NULL, 0);
				break;
			case CODING_DEFLATE:
				if (deflateInit(&ZLIBG(stream), Z_DEFAULT_COMPRESSION) != Z_OK) {
					/* TODO: print out error */
					return FAILURE;
				}
				break;		
		}
	}


	ZLIBG(stream).next_in = (Bytef*) str;
	ZLIBG(stream).avail_in = (uInt) str_length;

	if (ZLIBG(compression_coding) == 1) {
		ZLIBG(crc) = crc32(ZLIBG(crc), (const Bytef *) str, str_length);
	}

	err = php_do_deflate(str_length, (Bytef **) newstr, new_length, do_start, do_end ZLIBLS_CC);
	/* TODO: error handling (err may be Z_STREAM_ERROR, Z_BUF_ERROR, ?) */

	if (do_start) {
		/* Write a very simple .gz header: */
		(*newstr)[0] = gz_magic[0];
		(*newstr)[1] = gz_magic[1];
		(*newstr)[2] = Z_DEFLATED;
		(*newstr)[3] = (*newstr)[4] = (*newstr)[5] = (*newstr)[6] = (*newstr)[7] = (*newstr)[8] = 0;
		(*newstr)[9] = OS_CODE;
		*new_length += 10;
	}
	if (do_end) {
		if (ZLIBG(compression_coding) == 1) {
			char *trailer = (*newstr)+(*new_length);

			/* write crc & stream.total_in in LSB order */
			trailer[0] = (char) ZLIBG(crc) & 0xFF;
			trailer[1] = (char) (ZLIBG(crc) >> 8) & 0xFF;
			trailer[2] = (char) (ZLIBG(crc) >> 16) & 0xFF;
			trailer[3] = (char) (ZLIBG(crc) >> 24) & 0xFF;
			trailer[4] = (char) ZLIBG(stream).total_in & 0xFF;
			trailer[5] = (char) (ZLIBG(stream).total_in >> 8) & 0xFF;
			trailer[6] = (char) (ZLIBG(stream).total_in >> 16) & 0xFF;
			trailer[7] = (char) (ZLIBG(stream).total_in >> 24) & 0xFF;
			*new_length += 8;
		}
		deflateEnd(&ZLIBG(stream));
	}

	return SUCCESS;
}


PHP_FUNCTION(gzencode)
{
	zval **zv_coding, **zv_string;
	int coding;

	switch(ZEND_NUM_ARGS()) {
		case 1:
			if (zend_get_parameters_ex(1, &zv_string)==FAILURE) {
				RETURN_FALSE;
			}
			convert_to_string_ex(zv_string);
			coding = 1;
			break;
		case 2:
			if (zend_get_parameters_ex(2, &zv_string, &zv_coding)==FAILURE) {
				RETURN_FALSE;
			}
			convert_to_string_ex(zv_string);
			convert_to_long_ex(zv_coding);
			coding = Z_LVAL_PP(zv_coding);
			break;
		default:
			ZEND_WRONG_PARAM_COUNT();
			break;
	}
	if (php_deflate_string(Z_STRVAL_PP(zv_string), Z_STRLEN_PP(zv_string), &Z_STRVAL_P(return_value), &Z_STRLEN_P(return_value), coding, 1, 1)==SUCCESS) {
		Z_TYPE_P(return_value) = IS_STRING;
	} else {
		RETURN_FALSE;
	}
}


PHP_FUNCTION(ob_gzhandler)
{
	int coding;
	zval **zv_string, **zv_mode;
	zval **data, **a_encoding;
	zend_bool return_original=0;
	zend_bool do_start, do_end;
	ZLIBLS_FETCH();

	if (ZEND_NUM_ARGS()!=2 || zend_get_parameters_ex(2, &zv_string, &zv_mode)==FAILURE) {
		ZEND_WRONG_PARAM_COUNT();
	}

	if (ZLIBG(ob_gzhandler_status)==-1
		|| zend_hash_find(&EG(symbol_table), "HTTP_SERVER_VARS", sizeof("HTTP_SERVER_VARS"), (void **) &data)==FAILURE
		|| Z_TYPE_PP(data)!=IS_ARRAY
		|| zend_hash_find(Z_ARRVAL_PP(data), "HTTP_ACCEPT_ENCODING", sizeof("HTTP_ACCEPT_ENCODING"), (void **) &a_encoding)==FAILURE) {
		/* return the original string */
		*return_value = **zv_string;
		zval_copy_ctor(return_value);
		ZLIBG(ob_gzhandler_status)=-1;
		return;
	}
	convert_to_string_ex(a_encoding);
	if (php_memnstr(Z_STRVAL_PP(a_encoding), "gzip", 4, Z_STRVAL_PP(a_encoding) + Z_STRLEN_PP(a_encoding))) {
		coding = CODING_GZIP;
	} else if(php_memnstr(Z_STRVAL_PP(a_encoding), "deflate", 7, Z_STRVAL_PP(a_encoding) + Z_STRLEN_PP(a_encoding))) {
		coding = CODING_DEFLATE;
	} else {
		RETURN_FALSE;
	}
	
	convert_to_long_ex(zv_mode);
	do_start = ((Z_LVAL_PP(zv_mode) & PHP_OUTPUT_HANDLER_START) ? 1 : 0);
	do_end = ((Z_LVAL_PP(zv_mode) & PHP_OUTPUT_HANDLER_END) ? 1 : 0);
	Z_STRVAL_P(return_value) = NULL;
	Z_STRLEN_P(return_value) = 0;
	if (php_deflate_string(Z_STRVAL_PP(zv_string), Z_STRLEN_PP(zv_string), &Z_STRVAL_P(return_value), &Z_STRLEN_P(return_value), coding, do_start, do_end)==SUCCESS) {
		Z_TYPE_P(return_value) = IS_STRING;
		if (do_start) {
			switch (coding) {
				case CODING_GZIP:
					if (sapi_add_header("Content-Encoding: gzip", sizeof("Content-Encoding: gzip") - 1, 1)==FAILURE) {
						return_original = 1;
					}
					if (sapi_add_header("Vary: Accept-Encoding", sizeof("Vary: Accept-Encoding") - 1, 1)==FAILURE) {
						return_original = 1;
					}
					break;
				case CODING_DEFLATE:
					if (sapi_add_header("Content-Encoding: deflate", sizeof("Content-Encoding: deflate") - 1, 1)==FAILURE) {
						return_original = 1;
					}
					if (sapi_add_header("Vary: Accept-Encoding", sizeof("Vary: Accept-Encoding") - 1, 1)==FAILURE) {
						return_original = 1;
					}
					break;
				default:
					return_original = 1;
					break;
			}
		}

		if (return_original) {
			zval_dtor(return_value);
#if 0
		} else {
			char lenbuf[64];
			
			sprintf(lenbuf,"Content-Length: %d",Z_STRLEN_P(return_value));
			sapi_add_header(lenbuf,strlen(lenbuf), 1);
#endif
		}
	} else {
		return_original = 1;
	}

	if (return_original) {
		/* return the original string */
		*return_value = **zv_string;
		zval_copy_ctor(return_value);
	}
}



static void php_gzip_output_handler(char *output, uint output_len, char **handled_output, uint *handled_output_len, int mode)
{
	zend_bool do_start, do_end;
	ZLIBLS_FETCH();

	do_start = (mode & PHP_OUTPUT_HANDLER_START ? 1 : 0);
	do_end = (mode & PHP_OUTPUT_HANDLER_END ? 1 : 0);
	if (php_deflate_string(output, output_len, handled_output, handled_output_len, ZLIBG(ob_gzip_coding), do_start, do_end)!=SUCCESS) {
		zend_error(E_ERROR, "Compression failed");
	}
}


int php_enable_output_compression(int buffer_size)
{
	zval **a_encoding, **data;
	ELS_FETCH();
	ZLIBLS_FETCH();

	if (zend_hash_find(&EG(symbol_table), "HTTP_SERVER_VARS", sizeof("HTTP_SERVER_VARS"), (void **) &data)==FAILURE
		|| Z_TYPE_PP(data)!=IS_ARRAY
		|| zend_hash_find(Z_ARRVAL_PP(data), "HTTP_ACCEPT_ENCODING", sizeof("HTTP_ACCEPT_ENCODING"), (void **) &a_encoding)==FAILURE) {
		return FAILURE;
	}
	convert_to_string_ex(a_encoding);
	if (php_memnstr(Z_STRVAL_PP(a_encoding), "gzip", 4, Z_STRVAL_PP(a_encoding) + Z_STRLEN_PP(a_encoding))) {
		if (sapi_add_header("Content-Encoding: gzip", sizeof("Content-Encoding: gzip") - 1, 1)==FAILURE) {
			return FAILURE;
		}
		ZLIBG(ob_gzip_coding) = CODING_GZIP;
	} else if(php_memnstr(Z_STRVAL_PP(a_encoding), "deflate", 7, Z_STRVAL_PP(a_encoding) + Z_STRLEN_PP(a_encoding))) {
		if (sapi_add_header("Content-Encoding: deflate", sizeof("Content-Encoding: deflate") - 1, 1)==FAILURE) {
			return FAILURE;
		}
		ZLIBG(ob_gzip_coding) = CODING_DEFLATE;
	} else {
		return FAILURE;
	}
	
	php_start_ob_buffer(NULL, buffer_size);
	php_ob_set_internal_handler(php_gzip_output_handler, buffer_size*1.5);
	return SUCCESS;
}
