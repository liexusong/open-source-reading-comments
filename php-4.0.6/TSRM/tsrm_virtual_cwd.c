/*
   +----------------------------------------------------------------------+
   | PHP version 4.0                                                      |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997, 1998, 1999, 2000 The PHP Group                   |
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
   |          Sascha Schumann <sascha@schumann.cx>                        |
   +----------------------------------------------------------------------+
*/

/* $Id: tsrm_virtual_cwd.c,v 1.19 2001/05/05 16:05:19 zeev Exp $ */

#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>

#include "tsrm_virtual_cwd.h"
#include "tsrm_strtok_r.h"

#ifdef TSRM_WIN32
#include <io.h>
#include "tsrm_win32.h"
#endif

#define VIRTUAL_CWD_DEBUG 0

#ifdef ZTS
#include "TSRM.h"
#endif

/* Only need mutex for popen() in Windows because it doesn't chdir() on UNIX */
#if defined(TSRM_WIN32) && defined(ZTS)
MUTEX_T cwd_mutex;
#endif

#ifdef ZTS
static ts_rsrc_id cwd_globals_id;
#else
static virtual_cwd_globals cwd_globals;
#endif

cwd_state main_cwd_state; /* True global */

#ifndef TSRM_WIN32
#include <unistd.h>
#else
#include <direct.h>
#endif

#ifndef S_ISDIR
#define S_ISDIR(mode) ((mode) & _S_IFDIR)
#endif

#ifndef S_ISREG
#define S_ISREG(mode) ((mode) & _S_IFREG)
#endif

#ifdef TSRM_WIN32
#define tsrm_strtok_r(a,b,c) strtok((a),(b))
#define TOKENIZER_STRING "/\\"
	
static int php_check_dots(const char *element, int n) 
{
	while (n-- > 0) if (element[n] != '.') break;

	return (n != -1);
}
	
#define IS_DIRECTORY_UP(element, len) \
	(len >= 2 && !php_check_dots(element, len))

#define IS_DIRECTORY_CURRENT(element, len) \
	(len == 1 && ptr[0] == '.')


#else
#define TOKENIZER_STRING "/"
#endif


/* default macros */

#ifndef IS_DIRECTORY_UP
#define IS_DIRECTORY_UP(element, len) \
	(len == 2 && memcmp(element, "..", 2) == 0)
#endif

#ifndef IS_DIRECTORY_CURRENT
#define IS_DIRECTORY_CURRENT(element, len) \
	(len == 1 && ptr[0] == '.')
#endif

/* define this to check semantics */
#define IS_DIR_OK(s) (1)
	
#ifndef IS_DIR_OK
#define IS_DIR_OK(state) (php_is_dir_ok(state) == 0)
#endif


#define CWD_STATE_COPY(d, s)				\
	(d)->cwd_length = (s)->cwd_length;		\
	(d)->cwd = (char *) malloc((s)->cwd_length+1);	\
	memcpy((d)->cwd, (s)->cwd, (s)->cwd_length+1);

#define CWD_STATE_FREE(s)			\
	free((s)->cwd);
	
static int php_is_dir_ok(const cwd_state *state) 
{
	struct stat buf;

	if (stat(state->cwd, &buf) == 0 && S_ISDIR(buf.st_mode))
		return (0);

	return (1);
}

static int php_is_file_ok(const cwd_state *state) 
{
	struct stat buf;

	if (stat(state->cwd, &buf) == 0 && S_ISREG(buf.st_mode))
		return (0);

	return (1);
}

static void cwd_globals_ctor(virtual_cwd_globals *cwd_globals)
{
	CWD_STATE_COPY(&cwd_globals->cwd, &main_cwd_state);
}

static void cwd_globals_dtor(virtual_cwd_globals *cwd_globals)
{
	CWD_STATE_FREE(&cwd_globals->cwd);
}

static char *tsrm_strndup(const char *s, size_t length)
{
    char *p;

    p = (char *) malloc(length+1);
    if (!p) {
        return (char *)NULL;
    }
    if (length) {
        memcpy(p,s,length);
    }
    p[length]=0;
    return p;
}

CWD_API void virtual_cwd_startup(void)
{
	char cwd[MAXPATHLEN];
	char *result;

	result = getcwd(cwd, sizeof(cwd));	
	if (!result) {
		cwd[0] = '\0';
	}
	main_cwd_state.cwd = strdup(cwd);
	main_cwd_state.cwd_length = strlen(cwd);

#ifdef ZTS
	cwd_globals_id = ts_allocate_id(sizeof(virtual_cwd_globals), (ts_allocate_ctor) cwd_globals_ctor, (ts_allocate_dtor) cwd_globals_dtor);
#else
	cwd_globals_ctor(&cwd_globals);
#endif

#if defined(TSRM_WIN32) && defined(ZTS)
	cwd_mutex = tsrm_mutex_alloc();
#endif
}

CWD_API void virtual_cwd_shutdown(void)
{
#ifndef ZTS
	cwd_globals_dtor(&cwd_globals);
#endif
#if defined(TSRM_WIN32) && defined(ZTS)
	tsrm_mutex_free(cwd_mutex);
#endif

	free(main_cwd_state.cwd); /* Don't use CWD_STATE_FREE because the non global states will probably use emalloc()/efree() */
}

CWD_API char *virtual_getcwd_ex(size_t *length)
{
	cwd_state *state;
	CWDLS_FETCH();

	state = &CWDG(cwd);

	if (state->cwd_length == 0) {
		char *retval;

		*length = 1;
		retval = (char *) malloc(2);
		retval[0] = DEFAULT_SLASH;
		retval[1] = '\0';	
		return retval;
	}

#ifdef TSRM_WIN32
	/* If we have something like C: */
	if (state->cwd_length == 2 && state->cwd[state->cwd_length-1] == ':') {
		char *retval;

		*length = state->cwd_length+1;
		retval = (char *) malloc(*length+1);
		memcpy(retval, state->cwd, *length);
		retval[*length-1] = DEFAULT_SLASH;
		retval[*length] = '\0';
		return retval;
	}
#endif
	*length = state->cwd_length;
	return strdup(state->cwd);
}


/* Same semantics as UNIX getcwd() */
CWD_API char *virtual_getcwd(char *buf, size_t size)
{
	size_t length;
	char *cwd;

	cwd = virtual_getcwd_ex(&length);

	if (buf == NULL) {
		return cwd;
	}
	if (length > size-1) {
		free(cwd);
		errno = ERANGE; /* Is this OK? */
		return NULL;
	}
	memcpy(buf, cwd, length+1);
	free(cwd);
	return buf;
}

/* Resolve path relatively to state and put the real path into state */
/* returns 0 for ok, 1 for error */
CWD_API int virtual_file_ex(cwd_state *state, const char *path, verify_path_func verify_path)
{
	int path_length = strlen(path);
	char *ptr, *path_copy;
	char *tok = NULL;
	int ptr_length;
	cwd_state *old_state;
	int ret = 0;
	int copy_amount = -1;
	char *free_path;
	unsigned char is_absolute = 0;
#ifndef TSRM_WIN32
	char resolved_path[MAXPATHLEN];
#endif

	if (path_length == 0) 
		return (0);

#if !defined(TSRM_WIN32) && !defined(__BEOS__)
	if (IS_ABSOLUTE_PATH(path, path_length)) {
		if (realpath(path, resolved_path)) {
			path = resolved_path;
			path_length = strlen(path);
		}
	} else { /* Concat current directory with relative path and then run realpath() on it */
		char *tmp;
		char *ptr;

		ptr = tmp = (char *) malloc(state->cwd_length+path_length+sizeof("/"));
		if (!tmp) {
			return 1;
		}
		memcpy(ptr, state->cwd, state->cwd_length);
		ptr += state->cwd_length;
		*ptr++ = DEFAULT_SLASH;
		memcpy(ptr, path, path_length);
		ptr += path_length;
		*ptr = '\0';
		if (realpath(tmp, resolved_path)) {
			path = resolved_path;
			path_length = strlen(path);
		}
		free(tmp);
	}
#endif
	free_path = path_copy = tsrm_strndup(path, path_length);

	old_state = (cwd_state *) malloc(sizeof(cwd_state));
	CWD_STATE_COPY(old_state, state);
#if VIRTUAL_CWD_DEBUG
	fprintf(stderr,"cwd = %s path = %s\n", state->cwd, path);
#endif
	if (IS_ABSOLUTE_PATH(path_copy, path_length)) {
		copy_amount = COPY_WHEN_ABSOLUTE;
		is_absolute = 1;
#ifdef TSRM_WIN32
	} else if (IS_UNC_PATH(path_copy, path_length)) {
		copy_amount = 1;
		is_absolute = 1;
	} else if (IS_SLASH(path_copy[0])) {
		copy_amount = 2;
#endif
	}

	if (copy_amount != -1) {
		state->cwd = (char *) realloc(state->cwd, copy_amount + 1);
		if (copy_amount) {
			if (is_absolute) {
				memcpy(state->cwd, path_copy, copy_amount);
				path_copy += copy_amount;
			} else {
				memcpy(state->cwd, old_state->cwd, copy_amount);
			}
		}
		state->cwd[copy_amount] = '\0';
		state->cwd_length = copy_amount;
	}


	ptr = tsrm_strtok_r(path_copy, TOKENIZER_STRING, &tok);
	while (ptr) {
		ptr_length = strlen(ptr);

		if (IS_DIRECTORY_UP(ptr, ptr_length)) {
			char save;

			save = DEFAULT_SLASH;

#define PREVIOUS state->cwd[state->cwd_length - 1]

			while (IS_ABSOLUTE_PATH(state->cwd, state->cwd_length) &&
					!IS_SLASH(PREVIOUS)) {
				save = PREVIOUS;
				PREVIOUS = '\0';
				state->cwd_length--;
			}

			if (!IS_ABSOLUTE_PATH(state->cwd, state->cwd_length)) {
				state->cwd[state->cwd_length++] = save;
				state->cwd[state->cwd_length] = '\0';
			} else {
				PREVIOUS = '\0';
				state->cwd_length--;
			}
		} else if (!IS_DIRECTORY_CURRENT(ptr, ptr_length)) {
			state->cwd = (char *) realloc(state->cwd, state->cwd_length+ptr_length+1+1);
#ifdef TSRM_WIN32
			/* Windows 9x will consider C:\\Foo as a network path. Avoid it. */
			if (state->cwd[state->cwd_length-1]!=DEFAULT_SLASH) {
				state->cwd[state->cwd_length++] = DEFAULT_SLASH;
			}
#else
			state->cwd[state->cwd_length++] = DEFAULT_SLASH;
#endif
			memcpy(&state->cwd[state->cwd_length], ptr, ptr_length+1);
			state->cwd_length += ptr_length;
		}
		ptr = tsrm_strtok_r(NULL, TOKENIZER_STRING, &tok);
	}

	if (state->cwd_length == COPY_WHEN_ABSOLUTE) {
		state->cwd = (char *) realloc(state->cwd, state->cwd_length+1+1);
		state->cwd[state->cwd_length] = DEFAULT_SLASH;
		state->cwd[state->cwd_length+1] = '\0';
		state->cwd_length++;
	}

	if (verify_path && verify_path(state)) {
		CWD_STATE_FREE(state);

		*state = *old_state;

		ret = 1;
	} else {
		CWD_STATE_FREE(old_state);
		ret = 0;
	}
	
	free(old_state);
	
	free(free_path);
#if VIRTUAL_CWD_DEBUG
	fprintf (stderr, "virtual_file_ex() = %s\n",state->cwd);
#endif
	return (ret);
}

CWD_API int virtual_chdir(const char *path)
{
	CWDLS_FETCH();

	return virtual_file_ex(&CWDG(cwd), path, php_is_dir_ok)?-1:0;
}

CWD_API int virtual_chdir_file(const char *path, int (*p_chdir)(const char *path))
{
	int length = strlen(path);
	char *temp;
	int retval;

	if (length == 0) {
		return 1; /* Can't cd to empty string */
	}	
	while(--length >= 0 && !IS_SLASH(path[length])) {
	}

	if (length == -1) {
		/* No directory only file name */
		errno = ENOENT;
		return -1;
	}

	if (length == COPY_WHEN_ABSOLUTE && IS_ABSOLUTE_PATH(path, length+1)) { /* Also use trailing slash if this is absolute */
		length++;
	}
	temp = (char *) tsrm_do_alloca(length+1);
	memcpy(temp, path, length);
	temp[length] = 0;
#if VIRTUAL_CWD_DEBUG
	fprintf (stderr, "Changing directory to %s\n", temp);
#endif
	retval = p_chdir(temp);
	tsrm_free_alloca(temp);
	return retval;
}

CWD_API char *virtual_realpath(const char *path, char *real_path)
{
	cwd_state new_state;
	int retval;
    CWDLS_FETCH();

	CWD_STATE_COPY(&new_state, &CWDG(cwd));
	retval = virtual_file_ex(&new_state, path, NULL);
	
	if (!retval) {
		int len = new_state.cwd_length>MAXPATHLEN-1?MAXPATHLEN-1:new_state.cwd_length;
		memcpy(real_path, new_state.cwd, len);
		real_path[len] = '\0';
		return real_path;
	}

	return NULL;
}

CWD_API int virtual_filepath_ex(const char *path, char **filepath, verify_path_func verify_path)
{
	cwd_state new_state;
	int retval;
	CWDLS_FETCH();

	CWD_STATE_COPY(&new_state, &CWDG(cwd));
	retval = virtual_file_ex(&new_state, path, verify_path);

	*filepath = new_state.cwd;

	return retval;

}

CWD_API int virtual_filepath(const char *path, char **filepath)
{
	return virtual_filepath_ex(path, filepath, php_is_file_ok);
}

CWD_API FILE *virtual_fopen(const char *path, const char *mode)
{
	cwd_state new_state;
	FILE *f;
	CWDLS_FETCH();

	if (path[0] == '\0') { /* Fail to open empty path */
		return NULL;
	}

	CWD_STATE_COPY(&new_state, &CWDG(cwd));
	virtual_file_ex(&new_state, path, NULL);

	f = fopen(new_state.cwd, mode);

	CWD_STATE_FREE(&new_state);
	return f;
}

#if HAVE_UTIME
CWD_API int virtual_utime(const char *filename, struct utimbuf *buf)
{
	cwd_state new_state;
	int ret;
	CWDLS_FETCH();

	CWD_STATE_COPY(&new_state, &CWDG(cwd));
	virtual_file_ex(&new_state, filename, NULL);

	ret = utime(new_state.cwd, buf);

	CWD_STATE_FREE(&new_state);
	return ret;
}
#endif

CWD_API int virtual_chmod(const char *filename, mode_t mode)
{
	cwd_state new_state;
	int ret;
	CWDLS_FETCH();

	CWD_STATE_COPY(&new_state, &CWDG(cwd));
	virtual_file_ex(&new_state, filename, NULL);

	ret = chmod(new_state.cwd, mode);

	CWD_STATE_FREE(&new_state);
	return ret;
}

#ifndef TSRM_WIN32
CWD_API int virtual_chown(const char *filename, uid_t owner, gid_t group)
{
	cwd_state new_state;
	int ret;
	CWDLS_FETCH();

	CWD_STATE_COPY(&new_state, &CWDG(cwd));
	virtual_file_ex(&new_state, filename, NULL);

	ret = chown(new_state.cwd, owner, group);

	CWD_STATE_FREE(&new_state);
	return ret;
}
#endif

CWD_API int virtual_open(const char *path, int flags, ...)
{
	cwd_state new_state;
	int f;
	CWDLS_FETCH();

	CWD_STATE_COPY(&new_state, &CWDG(cwd));
	virtual_file_ex(&new_state, path, NULL);

	if (flags & O_CREAT) {
		mode_t mode;
		va_list arg;

		va_start(arg, flags);
		mode = va_arg(arg, mode_t);
		va_end(arg);

		f = open(new_state.cwd, flags, mode);
	} else {
		f = open(new_state.cwd, flags);
	}	
	CWD_STATE_FREE(&new_state);
	return f;
}

CWD_API int virtual_creat(const char *path, mode_t mode)
{
	cwd_state new_state;
	int f;
	CWDLS_FETCH();

	CWD_STATE_COPY(&new_state, &CWDG(cwd));
	virtual_file_ex(&new_state, path, NULL);

	f = creat(new_state.cwd,  mode);

	CWD_STATE_FREE(&new_state);
	return f;
}

CWD_API int virtual_rename(char *oldname, char *newname)
{
	cwd_state old_state;
	cwd_state new_state;
	int retval;
	CWDLS_FETCH();

	CWD_STATE_COPY(&old_state, &CWDG(cwd));
	virtual_file_ex(&old_state, oldname, NULL);
	oldname = old_state.cwd;

	CWD_STATE_COPY(&new_state, &CWDG(cwd));
	virtual_file_ex(&new_state, newname, NULL);
	newname = new_state.cwd;
 
	retval = rename(oldname, newname);

	CWD_STATE_FREE(&old_state);
	CWD_STATE_FREE(&new_state);

	return retval;
}

CWD_API int virtual_stat(const char *path, struct stat *buf)
{
	cwd_state new_state;
	int retval;
	CWDLS_FETCH();

	CWD_STATE_COPY(&new_state, &CWDG(cwd));
	virtual_file_ex(&new_state, path, NULL);

	retval = stat(new_state.cwd, buf);

	CWD_STATE_FREE(&new_state);
	return retval;
}

#ifndef TSRM_WIN32

CWD_API int virtual_lstat(const char *path, struct stat *buf)
{
	cwd_state new_state;
	int retval;
	CWDLS_FETCH();

	CWD_STATE_COPY(&new_state, &CWDG(cwd));
	virtual_file_ex(&new_state, path, NULL);

	retval = lstat(new_state.cwd, buf);

	CWD_STATE_FREE(&new_state);
	return retval;
}

#endif

CWD_API int virtual_unlink(const char *path)
{
	cwd_state new_state;
	int retval;
	CWDLS_FETCH();

	CWD_STATE_COPY(&new_state, &CWDG(cwd));
	virtual_file_ex(&new_state, path, NULL);

	retval = unlink(new_state.cwd);

	CWD_STATE_FREE(&new_state);
	return retval;
}

CWD_API int virtual_mkdir(const char *pathname, mode_t mode)
{
	cwd_state new_state;
	int retval;
	CWDLS_FETCH();

	CWD_STATE_COPY(&new_state, &CWDG(cwd));
	virtual_file_ex(&new_state, pathname, NULL);

#ifdef TSRM_WIN32
	retval = mkdir(new_state.cwd);
#else
	retval = mkdir(new_state.cwd, mode);
#endif
	CWD_STATE_FREE(&new_state);
	return retval;
}

CWD_API int virtual_rmdir(const char *pathname)
{
	cwd_state new_state;
	int retval;
	CWDLS_FETCH();

	CWD_STATE_COPY(&new_state, &CWDG(cwd));
	virtual_file_ex(&new_state, pathname, NULL);

	retval = rmdir(new_state.cwd);

	CWD_STATE_FREE(&new_state);
	return retval;
}

#ifdef TSRM_WIN32
DIR *opendir(const char *name);
#endif

CWD_API DIR *virtual_opendir(const char *pathname)
{
	cwd_state new_state;
	DIR *retval;
	CWDLS_FETCH();

	CWD_STATE_COPY(&new_state, &CWDG(cwd));
	virtual_file_ex(&new_state, pathname, NULL);

	retval = opendir(new_state.cwd);

	CWD_STATE_FREE(&new_state);
	return retval;
}

#ifndef TSRM_WIN32

CWD_API FILE *virtual_popen(const char *command, const char *type)
{
	int command_length;
	char *command_line;
	char *ptr;
	FILE *retval;
	CWDLS_FETCH();

	command_length = strlen(command);

	ptr = command_line = (char *) malloc(command_length + sizeof("cd  ; ") + CWDG(cwd).cwd_length+1);
	if (!command_line) {
		return NULL;
	}
	memcpy(ptr, "cd ", sizeof("cd ")-1);
	ptr += sizeof("cd ")-1;

	if (CWDG(cwd).cwd_length == 0) {
		*ptr++ = DEFAULT_SLASH;
	} else {
		memcpy(ptr, CWDG(cwd).cwd, CWDG(cwd).cwd_length);
		ptr += CWDG(cwd).cwd_length;
	}
	
	*ptr++ = ' ';
	*ptr++ = ';';
	*ptr++ = ' ';

	memcpy(ptr, command, command_length+1);
	retval = popen(command_line, type);

	free(command_line);
	return retval;
}

#else

/* On Windows the trick of prepending "cd cwd; " doesn't work so we need to perform
   a real chdir() and mutex it
 */
CWD_API FILE *virtual_popen(const char *command, const char *type)
{
	char prev_cwd[MAXPATHLEN];
	char *getcwd_result;
	FILE *retval;
	CWDLS_FETCH();

	getcwd_result = getcwd(prev_cwd, MAXPATHLEN);
	if (!getcwd_result) {
		return NULL;
	}

#ifdef ZTS
	tsrm_mutex_lock(cwd_mutex);
#endif

	chdir(CWDG(cwd).cwd);
	retval = popen(command, type);
	chdir(prev_cwd);

#ifdef ZTS
	tsrm_mutex_unlock(cwd_mutex);
#endif

	return retval;
}

#endif


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */
