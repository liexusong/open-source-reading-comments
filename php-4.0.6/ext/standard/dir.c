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
   | Authors:                                                             |
   | PHP 4.0 patches by Thies C. Arntzen (thies@thieso.net)               |
   +----------------------------------------------------------------------+
 */

/* $Id: dir.c,v 1.65.2.2 2001/06/20 15:54:46 rasmus Exp $ */

/* {{{ includes/startup/misc */

#include "php.h"
#include "fopen_wrappers.h"

#include "php_dir.h"

#ifdef HAVE_DIRENT_H
# include <dirent.h>
#endif

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <errno.h>

#ifdef PHP_WIN32
#include "win32/readdir.h"
#endif

typedef struct {
	int default_dir;
} php_dir_globals;

#ifdef ZTS
#define DIRG(v) (dir_globals->v)
#define DIRLS_FETCH() php_dir_globals *dir_globals = ts_resource(dir_globals_id)
#define DIRLS_D		php_dir_globals *dir_globals
#define DIRLS_DC	, DIRLS_D
#define DIRLS_C		dir_globals
#define DIRLS_CC	, DIRLS_C
int dir_globals_id;
#else
#define DIRG(v) (dir_globals.v)
#define DIRLS_FETCH()
#define DIRLS_D
#define DIRLS_DC
#define DIRLS_C
#define DIRLS_CC
php_dir_globals dir_globals;
#endif

typedef struct {
	int id;
	DIR *dir;
} php_dir;

static int le_dirp;

static zend_class_entry *dir_class_entry_ptr;

#define FETCH_DIRP() \
	if (ZEND_NUM_ARGS() == 0) { \
		myself = getThis(); \
		if (myself) { \
			if (zend_hash_find(myself->value.obj.properties, "handle", sizeof("handle"), (void **)&tmp) == FAILURE) { \
				php_error(E_WARNING, "unable to find my handle property"); \
				RETURN_FALSE; \
			} \
			ZEND_FETCH_RESOURCE(dirp,php_dir *,tmp,-1, "Directory", le_dirp); \
		} else { \
			ZEND_FETCH_RESOURCE(dirp,php_dir *,0,DIRG(default_dir), "Directory", le_dirp); \
		} \
	} else if ((ZEND_NUM_ARGS() != 1) || zend_get_parameters_ex(1, &id) == FAILURE) { \
		WRONG_PARAM_COUNT; \
	} else { \
		ZEND_FETCH_RESOURCE(dirp,php_dir *,id,-1, "Directory", le_dirp); \
	} 

static zend_function_entry php_dir_class_functions[] = {
	PHP_FALIAS(close,	closedir,	NULL)
	PHP_FALIAS(rewind,	rewinddir,	NULL)
	PHP_STATIC_FE("read", php_if_readdir, NULL)
	{NULL, NULL, NULL}
};


static void php_set_default_dir(int id DIRLS_DC)
{
    if (DIRG(default_dir)!=-1) {
        zend_list_delete(DIRG(default_dir));
    }

	if (id != -1) {
		zend_list_addref(id);
	}
	
	DIRG(default_dir) = id;
}


static void _dir_dtor(zend_rsrc_list_entry *rsrc)
{
	php_dir *dirp = (php_dir *)rsrc->ptr;
	closedir(dirp->dir);
	efree(dirp);
}

PHP_RINIT_FUNCTION(dir)
{
	DIRLS_FETCH();

	DIRG(default_dir) = -1;
	return SUCCESS;
}

PHP_MINIT_FUNCTION(dir)
{
	static char tmpstr[2];
	zend_class_entry dir_class_entry;

	le_dirp = zend_register_list_destructors_ex(_dir_dtor, NULL, "dir", module_number);

	INIT_CLASS_ENTRY(dir_class_entry, "Directory", php_dir_class_functions);
	dir_class_entry_ptr = zend_register_internal_class(&dir_class_entry);

#ifdef ZTS
	dir_globals_id = ts_allocate_id(sizeof(php_dir_globals), NULL, NULL);
#endif
	tmpstr[0] = DEFAULT_SLASH;
	tmpstr[1] = '\0';
	REGISTER_STRING_CONSTANT("DIRECTORY_SEPARATOR", tmpstr, CONST_CS|CONST_PERSISTENT);

	return SUCCESS;
}

/* }}} */
/* {{{ internal functions */

static void _php_do_opendir(INTERNAL_FUNCTION_PARAMETERS, int createobject)
{
	pval **arg;
	php_dir *dirp;
	DIRLS_FETCH();
	
	if (ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &arg) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(arg);

	if (php_check_open_basedir((*arg)->value.str.val)) {
		RETURN_FALSE;
	}
	
	dirp = emalloc(sizeof(php_dir));

	dirp->dir = VCWD_OPENDIR((*arg)->value.str.val);

#ifdef PHP_WIN32
	if (!dirp->dir || dirp->dir->finished) {
		if (dirp->dir) {
			closedir(dirp->dir);
		}
#else
	if (!dirp->dir) {
#endif
		efree(dirp);
		php_error(E_WARNING, "OpenDir: %s (errno %d)", strerror(errno), errno);
		RETURN_FALSE;
	}

	dirp->id = zend_list_insert(dirp,le_dirp);

	php_set_default_dir(dirp->id DIRLS_CC);

	if (createobject) {
		object_init_ex(return_value, dir_class_entry_ptr);
		add_property_stringl(return_value, "path", (*arg)->value.str.val, (*arg)->value.str.len, 1);
		add_property_resource(return_value, "handle", dirp->id);
		zend_list_addref(dirp->id);
	} else {
		RETURN_RESOURCE(dirp->id);
	}
}

/* }}} */
/* {{{ proto int opendir(string path)
   Open a directory and return a dir_handle */

PHP_FUNCTION(opendir)
{
	_php_do_opendir(INTERNAL_FUNCTION_PARAM_PASSTHRU,0);
}

/* }}} */
/* {{{ proto class dir(string directory)
   Directory class with properties, handle and class and methods read, rewind and close */

PHP_FUNCTION(getdir)
{
	_php_do_opendir(INTERNAL_FUNCTION_PARAM_PASSTHRU,1);
}

/* }}} */
/* {{{ proto void closedir([int dir_handle])
   Close directory connection identified by the dir_handle */

PHP_FUNCTION(closedir)
{
	pval **id, **tmp, *myself;
	php_dir *dirp;
	DIRLS_FETCH();

	FETCH_DIRP();

	zend_list_delete(dirp->id);

	if (dirp->id == DIRG(default_dir)) {
		php_set_default_dir(-1 DIRLS_CC);
	}
}

/* }}} */

#if defined(HAVE_CHROOT) && !defined(ZTS)
/* {{{ proto int chroot(string directory)
   Change root directory */

PHP_FUNCTION(chroot)
{
	pval **arg;
	int ret;
	
	if (ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &arg) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(arg);

	ret = chroot((*arg)->value.str.val);
	
	if (ret != 0) {
		php_error(E_WARNING, "chroot: %s (errno %d)", strerror(errno), errno);
		RETURN_FALSE;
	}

	ret = chdir("/");
	
	if (ret != 0) {
		php_error(E_WARNING, "chdir: %s (errno %d)", strerror(errno), errno);
		RETURN_FALSE;
	}

	RETURN_TRUE;
}

/* }}} */
#endif

/* {{{ proto int chdir(string directory)
   Change the current directory */

PHP_FUNCTION(chdir)
{
	pval **arg;
	int ret;
	PLS_FETCH();
	
	if (ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &arg) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(arg);

	if (PG(safe_mode) && !php_checkuid((*arg)->value.str.val, NULL, CHECKUID_ALLOW_ONLY_DIR)) {
		RETURN_FALSE;
	}
	ret = VCWD_CHDIR((*arg)->value.str.val);
	
	if (ret != 0) {
		php_error(E_WARNING, "ChDir: %s (errno %d)", strerror(errno), errno);
		RETURN_FALSE;
	}

	RETURN_TRUE;
}

/* }}} */
/* {{{ proto string getcwd(void)
   Gets the current directory */

PHP_FUNCTION(getcwd)
{
	char path[MAXPATHLEN];
	char *ret=NULL;
	
	if (ZEND_NUM_ARGS() != 0) {
		WRONG_PARAM_COUNT;
	}

#if HAVE_GETCWD
	ret = VCWD_GETCWD(path, MAXPATHLEN);
#elif HAVE_GETWD
	ret = VCWD_GETWD(path);
/*
 * #warning is not ANSI C
 * #else
 * #warning no proper getcwd support for your site
 */
#endif

	if (ret) {
		RETURN_STRING(path,1);
	} else {
		RETURN_FALSE;
	}
}

/* }}} */
/* {{{ proto void rewinddir([int dir_handle])
   Rewind dir_handle back to the start */

PHP_FUNCTION(rewinddir)
{
	pval **id, **tmp, *myself;
	php_dir *dirp;
	DIRLS_FETCH();
	
	FETCH_DIRP();

	rewinddir(dirp->dir);
}
/* }}} */
/* {{{ proto string readdir([int dir_handle])
   Read directory entry from dir_handle */

PHP_NAMED_FUNCTION(php_if_readdir)
{
	pval **id, **tmp, *myself;
	php_dir *dirp;
	char entry[sizeof(struct dirent)+MAXPATHLEN];
	struct dirent *result = (struct dirent *)&entry; /* patch for libc5 readdir problems */
	DIRLS_FETCH();

	FETCH_DIRP();

	if (php_readdir_r(dirp->dir, (struct dirent *) entry, &result) == 0 && result) {
		RETURN_STRINGL(result->d_name, strlen(result->d_name), 1);
	}
	RETURN_FALSE;
}

/* }}} */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 tw=78 fdm=marker
 * vim<600: sw=4 ts=4 tw=78
 */
