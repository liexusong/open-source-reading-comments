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
   |          Jim Winstead <jimw@php.net>                                 |
   +----------------------------------------------------------------------+
 */

/* $Id: db.c,v 1.50.2.1 2001/05/24 12:41:44 ssb Exp $ */
#define IS_EXT_MODULE

#if 1

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_globals.h"
#include "safe_mode.h"
#include "fopen_wrappers.h"
#include "ext/standard/flock_compat.h" 
#include "ext/standard/info.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef PHP_31
#include "os/nt/flock.h"
#else
#ifdef PHP_WIN32
#include "win32/flock.h"
#else
#include <sys/file.h>
#endif
#endif

#if HAVE_FCNTL_H
#include <fcntl.h>
#endif

#if GDBM
#include <gdbm.h>

#define DBM_TYPE GDBM_FILE
#define DBM_MODE_TYPE int
#define DBM_WRITE_MODE GDBM_WRITER
#define DBM_CREATE_MODE GDBM_WRCREAT
#define DBM_NEW_MODE GDBM_NEWDB
#define DBM_DEFAULT_MODE GDBM_READER
#define DBM_OPEN(filename, mode) gdbm_open(filename, 512, mode, 0666, 0)
#define DBM_CLOSE(dbf) gdbm_close(dbf)
#define DBM_STORE(dbf, key, value, mode) gdbm_store(dbf, key, value, mode)
#define DBM_FETCH(dbf, key) gdbm_fetch(dbf, key)
#define DBM_EXISTS(dbf, key) gdbm_exists(dbf, key)
#define DBM_DELETE(dbf, key) gdbm_delete(dbf, key)
#define DBM_FIRSTKEY(dbf) gdbm_firstkey(dbf)
#define DBM_NEXTKEY(dbf, key) gdbm_nextkey(dbf, key)

#define DBM_INSERT GDBM_INSERT
#define DBM_REPLACE GDBM_REPLACE
#endif

#if NDBM && !GDBM
#if BSD2
#define DB_DBM_HSEARCH 1
#include <db.h>
#else
#ifdef HAVE_DB1_NDBM_H
#include <db1/ndbm.h>
#else
#include <ndbm.h>
#endif
#endif

#define DBM_TYPE DBM *
#define DBM_MODE_TYPE int
#define DBM_WRITE_MODE O_RDWR
#define DBM_CREATE_MODE O_RDWR | O_CREAT
#define DBM_NEW_MODE O_RDWR | O_CREAT | O_TRUNC
#define DBM_DEFAULT_MODE O_RDONLY
#define DBM_OPEN(filename, mode) dbm_open(filename, mode, 0666)
#define DBM_CLOSE(dbf) dbm_close(dbf)
#define DBM_STORE(dbf, key, value, mode) dbm_store(dbf, key, value, mode)
#define DBM_FETCH(dbf, key) dbm_fetch(dbf, key)
#define DBM_EXISTS(dbf, key) php_dbm_key_exists(dbf, key)
#define DBM_DELETE(dbf, key) dbm_delete(dbf, key)
#define DBM_FIRSTKEY(dbf) dbm_firstkey(dbf)
#define DBM_NEXTKEY(dbf, key) dbm_nextkey(dbf)

static int php_dbm_key_exists(DBM *dbf, datum key_datum) {
	datum value_datum;
	int ret;

	value_datum = dbm_fetch(dbf, key_datum);
	if (value_datum.dptr)
		ret = 1;
	else
		ret = 0;
	return ret;
}
#endif

#if !NDBM && !GDBM
#define DBM_TYPE FILE *

#define DBM_MODE_TYPE char *
#define DBM_WRITE_MODE "r+b"
#define DBM_CREATE_MODE "a+b"
#define DBM_NEW_MODE "w+b"
#define DBM_DEFAULT_MODE "r"
#define DBM_OPEN(filename, mode) VCWD_FOPEN(filename, mode)
#define DBM_CLOSE(dbf) fclose(dbf)
#define DBM_STORE(dbf, key, value, mode) flatfile_store(dbf, key, value, mode)
#define DBM_FETCH(dbf, key) flatfile_fetch(dbf, key)
#define DBM_EXISTS(dbf, key) flatfile_findkey(dbf, key)
#define DBM_DELETE(dbf, key) flatfile_delete(dbf, key)
#define DBM_FIRSTKEY(dbf) flatfile_firstkey(dbf)
#define DBM_NEXTKEY(dbf, key) flatfile_nextkey(dbf)

#define DBM_INSERT 0
#define DBM_REPLACE 1

typedef struct {
        char *dptr;
        int dsize;
} datum;

int flatfile_store(FILE *dbf, datum key, datum value, int mode);
datum flatfile_fetch(FILE *dbf, datum key);
int flatfile_findkey(FILE *dbf, datum key);
int flatfile_delete(FILE *dbf, datum key);
datum flatfile_firstkey(FILE *dbf);
datum flatfile_nextkey(FILE *dbf);

#endif

#include "php_db.h"
#include "ext/standard/php_string.h"

static int le_db;

#if THREAD_SAFE
DWORD DbmTls;
static int numthreads=0;
#endif

/*needed for blocking calls in windows*/
void *dbm_mutex;

dbm_info *php_find_dbm(pval *id)
{
	list_entry *le;
	dbm_info *info;
	int numitems, i;
	int info_type;
	ELS_FETCH();

	if (Z_TYPE_P(id) == IS_STRING) {
		numitems = zend_hash_num_elements(&EG(regular_list));
		for (i=1; i<=numitems; i++) {
			if (zend_hash_index_find(&EG(regular_list), i, (void **) &le)==FAILURE) {
				continue;
			}
			if (Z_TYPE_P(le) == le_db) {
				info = (dbm_info *)(le->ptr);
				if (!strcmp(info->filename, Z_STRVAL_P(id))) {
					return (dbm_info *)(le->ptr);
				}
			}
		}
	}

	/* didn't find it as a database filename, try as a number */
	convert_to_long(id);
	info = zend_list_find(Z_LVAL_P(id), &info_type);
	if (info_type != le_db)
		return NULL;
	return info;
}

static char *php_get_info_db(void)
{
	static char temp1[128];
	static char temp[256];

	temp1[0]='\0';
	temp[0]='\0';

#ifdef DB_VERSION_STRING /* using sleepycat dbm */
	strcat(temp,DB_VERSION_STRING);
#endif

#if GDBM
	sprintf(temp1,"%s",gdbm_version);
	strcat(temp,temp1);
#endif

#if NDBM && !GDBM
	strcat(temp,"ndbm support enabled");
#endif	

#if !GDBM && !NDBM
	strcat(temp,"flat file support enabled");
#endif	

#if NFS_HACK
	strcat(temp,"NFS hack in effect");
#endif

	if (!*temp)
		strcat(temp,"No database support");

	return temp;
}


PHP_MINFO_FUNCTION(db)
{
	/* this isn't pretty ... should break out the info a bit more (cmv) */
	php_info_print_box_start(0);
	php_printf(php_get_info_db());
	php_info_print_box_end();
}

/* {{{ proto string dblist(void)
   Describes the dbm-compatible library being used */ 
PHP_FUNCTION(dblist)
{
	char *str = php_get_info_db();
	RETURN_STRING(str,1);
}
/* }}} */

/* {{{ proto int dbmopen(string filename, string mode)
   Opens a dbm database */
PHP_FUNCTION(dbmopen) {
	pval *filename, *mode;
	dbm_info *info=NULL;
	int ret;

	if (ZEND_NUM_ARGS()!=2 || zend_get_parameters(ht,2,&filename,&mode)==FAILURE) {
		WRONG_PARAM_COUNT;
	}

	convert_to_string(filename);
	convert_to_string(mode);
	
	info = php_dbm_open(Z_STRVAL_P(filename), Z_STRVAL_P(mode));
	if (info) {
		ret = zend_list_insert(info, le_db);
		RETURN_LONG(ret);
	} else {
		RETURN_FALSE;
	}
}
/* }}} */


dbm_info *php_dbm_open(char *filename, char *mode) {
	dbm_info *info;
	int ret, lock=0;
	char *lockfn = NULL;
	int lockfd = 0;

#if NFS_HACK
	int last_try = 0;
	struct stat sb;
	int retries = 0;
#endif

	DBM_TYPE dbf=NULL;
	DBM_MODE_TYPE imode;
	PLS_FETCH();

	if (filename == NULL) {
		php_error(E_WARNING, "NULL filename passed to php_dbm_open()");
		return NULL;
	}

	if (PG(safe_mode) && (!php_checkuid(filename, NULL, CHECKUID_CHECK_FILE_AND_DIR))) {
		return NULL;
	}

	if (php_check_open_basedir(filename)) {
		return NULL;
	}

	switch (*mode) {
		case 'w': 
			imode = DBM_WRITE_MODE;
			lock = 1;
			break;
		case 'c': 
			imode = DBM_CREATE_MODE;
			lock = 1;
			break;
		case 'n': 
			imode = DBM_NEW_MODE;
			lock = 1;
			break;
		default: 
			imode = DBM_DEFAULT_MODE;
			lock = 0;
			break;
	}

	if (lock) {
		lockfn = emalloc(strlen(filename) + 5);
		strcpy(lockfn, filename);
		strcat(lockfn, ".lck");

#if NFS_HACK 
		while((last_try = VCWD_STAT(lockfn,&sb))==0) {
			retries++;
			php_sleep(1);
			if (retries>30) break;
		}	
		if (last_try!=0) {
			lockfd = open(lockfn,O_RDWR|O_CREAT,0644);
			close(lockfd);
		} else {
			php_error(E_WARNING, "File appears to be locked [%s]\n",lockfn);
			return -1;
		}
#else /* NFS_HACK */

		lockfd = VCWD_OPEN((lockfn,O_RDWR|O_CREAT,0644));

		if (lockfd) {
			flock(lockfd,LOCK_EX);
			close(lockfd);
		} else {
			php_error(E_WARNING, "Unable to establish lock: %s",filename);
		}
#endif /* else NFS_HACK */

	}

	dbf = DBM_OPEN(filename, imode);

#if !NDBM && !GDBM
	if (dbf) {
		setvbuf(dbf, NULL, _IONBF, 0);
	}	
#endif

	if (dbf) {
		info = (dbm_info *)emalloc(sizeof(dbm_info));
		if (!info) {
			php_error(E_ERROR, "problem allocating memory!");
			return NULL;
		}

		info->filename = estrdup(filename);
		info->lockfn = lockfn;
		info->lockfd = lockfd;
		info->dbf = dbf;

		return info;
	} else {
#if GDBM 
		php_error(E_WARNING, "dbmopen_gdbm(%s): %d [%s], %d [%s]",filename,gdbm_errno,gdbm_strerror(gdbm_errno),errno,strerror(errno));
		if (gdbm_errno)
			ret = gdbm_errno;
		else if (errno)
			ret = errno;
		else
			ret = -1;
#else 
#if NDBM 
#if PHP_DEBUG
		php_error(E_WARNING, "dbmopen_ndbm(%s): errno = %d [%s]\n",filename,errno,strerror(errno));
#endif
		if (errno) ret=errno;
		else ret = -1;
#else
#if PHP_DEBUG
		php_error(E_WARNING, "dbmopen_flatfile(%s): errno = %d [%s]\n",filename,errno,strerror(errno));
#endif
		if (errno) ret=errno;
		else ret = -1;
#endif 
#endif 	

#if NFS_HACK
		if (lockfn) {
			VCWD_UNLINK(lockfn);
		}
#endif
		if (lockfn) efree(lockfn);
	}

	return NULL;
}

/* {{{ proto bool dbmclose(int dbm_identifier)
   Closes a dbm database */
PHP_FUNCTION(dbmclose) {
	pval *id;

	if (ZEND_NUM_ARGS() != 1 || zend_get_parameters(ht,1,&id)==FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_long(id);

	if (zend_list_delete(Z_LVAL_P(id)) == SUCCESS) {
		RETURN_TRUE;
	} else {
		RETURN_FALSE;
	}
}
/* }}} */

int php_dbm_close(zend_rsrc_list_entry *rsrc) {
	int ret = 0;
	dbm_info *info = (dbm_info *)rsrc->ptr;
	DBM_TYPE dbf;
	int lockfd;

	dbf = info->dbf;

#if NFS_HACK
	VCWD_UNLINK(info->lockfn);
#else
	if (info->lockfn) {
		lockfd = VCWD_OPEN((info->lockfn,O_RDWR,0644));
		flock(lockfd,LOCK_UN);
		close(lockfd);
	}
#endif

	if (dbf)
		DBM_CLOSE(dbf);

	/* free the memory used by the dbm_info struct */
	if (info->filename) efree(info->filename);
	if (info->lockfn) efree(info->lockfn);
	efree(info);

	return(ret);
}	

/*
 * ret = -1 means that database was opened for read-only
 * ret = 0  success
 * ret = 1  key already exists - nothing done
 */
/* {{{ proto int dbminsert(int dbm_identifier, string key, string value)
   Inserts a value for a key in a dbm database */
PHP_FUNCTION(dbminsert)
{
	pval *id, *key, *value;
	dbm_info *info;
	int ret;

	if (ZEND_NUM_ARGS()!=3||zend_get_parameters(ht,3,&id,&key,&value) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string(key);
	convert_to_string(value);

	info = php_find_dbm(id);
	if (!info) {
		php_error(E_WARNING, "not a valid database identifier %d", Z_LVAL_P(id));
		RETURN_FALSE;
	}
	
	ret = php_dbm_insert(info, Z_STRVAL_P(key), Z_STRVAL_P(value));
	RETURN_LONG(ret);
}
/* }}} */



int php_dbm_insert(dbm_info *info, char *key, char *value) {
	datum key_datum, value_datum;
	int ret;
	DBM_TYPE dbf;

	php_stripslashes(key,NULL);
	php_stripslashes(value,NULL);

	value_datum.dptr = estrdup(value);
	value_datum.dsize = strlen(value);

	key_datum.dptr = estrdup(key);
	key_datum.dsize = strlen(key);
#if GDBM_FIX
	key_datum.dsize++;
#endif

	dbf = info->dbf;
	if (!dbf) {
		php_error(E_WARNING, "Unable to locate dbm file");
		return 1;
	}

	ret = DBM_STORE(dbf, key_datum, value_datum, DBM_INSERT);

	/* free the memory */
	efree(key_datum.dptr); efree(value_datum.dptr);

	return(ret);	
}	

/* {{{ proto int dbmreplace(int dbm_identifier, string key, string value)
   Replaces the value for a key in a dbm database */
PHP_FUNCTION(dbmreplace)
{
	pval *id, *key, *value;
	dbm_info *info;
	int ret;

	if (ZEND_NUM_ARGS()!=3||zend_get_parameters(ht,3,&id,&key,&value) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string(key);
	convert_to_string(value);

	info = php_find_dbm(id);
	if (!info) {
		php_error(E_WARNING, "not a valid database identifier %d", Z_LVAL_P(id));
		RETURN_FALSE;
	}
	
	ret = php_dbm_replace(info, Z_STRVAL_P(key), Z_STRVAL_P(value));
	RETURN_LONG(ret);
}
/* }}} */

int php_dbm_replace(dbm_info *info, char *key, char *value) {
	DBM_TYPE dbf;
	int ret;
	datum key_datum, value_datum;
	PLS_FETCH();

	if (PG(magic_quotes_runtime)) {
		php_stripslashes(key,NULL);
		php_stripslashes(value,NULL);
	}

	value_datum.dptr = estrdup(value);
	value_datum.dsize = strlen(value);

	key_datum.dptr = estrdup(key);
	key_datum.dsize = strlen(key);
#if GDBM_FIX
	key_datum.dsize++;
#endif

	dbf = info->dbf;
	if (!dbf) {
		php_error(E_WARNING, "Unable to locate dbm file");
		return 1;
	}

	ret = DBM_STORE(dbf, key_datum, value_datum, DBM_REPLACE);

	/* free the memory */
	efree(key_datum.dptr); efree(value_datum.dptr);

	return(ret);	
}	

/* {{{ proto string dbmfetch(int dbm_identifier, string key)
   Fetches a value for a key from a dbm database */
PHP_FUNCTION(dbmfetch)
{
	pval *id, *key;
	dbm_info *info;
	char *ret;

	if (ZEND_NUM_ARGS()!=2||zend_get_parameters(ht,2,&id,&key)==FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string(key);

	info = php_find_dbm(id);
	if (!info) {
		php_error(E_WARNING, "not a valid database identifier %d", Z_LVAL_P(id));
		RETURN_FALSE;
	}

	ret = php_dbm_fetch(info, Z_STRVAL_P(key));
	if (ret) {
		RETVAL_STRING(ret, 0);
	} else {
		RETURN_FALSE;
	}
}
/* }}} */

char *php_dbm_fetch(dbm_info *info, char *key) {
	datum key_datum, value_datum;
	char *ret;
	DBM_TYPE dbf;
	PLS_FETCH();

	key_datum.dptr = key;
	key_datum.dsize = strlen(key);
#if GDBM_FIX
	key_datum.dsize++;
#endif
	value_datum.dptr = NULL;
	value_datum.dsize = 0;

	dbf = info->dbf;
	if (!dbf) {
		php_error(E_WARNING, "Unable to locate dbm file");
		return(NULL);
	}

	value_datum = DBM_FETCH(dbf, key_datum);

	if (value_datum.dptr) {
		ret = (char *)emalloc(sizeof(char) * value_datum.dsize + 1);
		strncpy(ret, value_datum.dptr, value_datum.dsize);
		ret[value_datum.dsize] = '\0';

#if GDBM
/* all but NDBM use malloc to allocate the content blocks, so we need to free it */
		free(value_datum.dptr);
#else
# if !NDBM
		efree(value_datum.dptr);
# endif
#endif
	}
	else
		ret = NULL;

	if (ret && PG(magic_quotes_runtime)) {
		ret = php_addslashes(ret, value_datum.dsize, NULL, 1);
	}
	return(ret);
}

/* {{{ proto int dbmexists(int dbm_identifier, string key)
   Tells if a value exists for a key in a dbm database */
PHP_FUNCTION(dbmexists)
{
	pval *id, *key;
	dbm_info *info;
	int ret;

	if (ZEND_NUM_ARGS()!=2||zend_get_parameters(ht,2,&id,&key)==FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string(key);

	info = php_find_dbm(id);
	if (!info) {
		php_error(E_WARNING, "not a valid database identifier %d", Z_LVAL_P(id));
		RETURN_FALSE;
	}

	ret = php_dbm_exists(info, Z_STRVAL_P(key));
	RETURN_LONG(ret);
}
/* }}} */

int php_dbm_exists(dbm_info *info, char *key) {
	datum key_datum;
	int ret;
	DBM_TYPE dbf;

	key_datum.dptr = key;
	key_datum.dsize = strlen(key);
#if GDBM_FIX
	key_datum.dsize++;
#endif

	dbf = info->dbf;
	if (!dbf) {
		php_error(E_WARNING, "Unable to locate dbm file");
		return(0);
	}

	ret = DBM_EXISTS(dbf, key_datum);

	return(ret);
}

/* {{{ proto int dbmdelete(int dbm_identifier, string key)
   Deletes the value for a key from a dbm database */ 		
PHP_FUNCTION(dbmdelete)
{
	pval *id, *key;
	dbm_info *info;
	int ret;

	if (ZEND_NUM_ARGS()!=2||zend_get_parameters(ht,2,&id,&key)==FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string(key);

	info = php_find_dbm(id);
	if (!info) {
		php_error(E_WARNING, "not a valid database identifier %d", Z_LVAL_P(id));
		RETURN_FALSE;
	}

	ret = php_dbm_delete(info, Z_STRVAL_P(key));
	RETURN_LONG(ret);
}
/* }}} */

int php_dbm_delete(dbm_info *info, char *key) {
	datum key_datum;
	int ret;
	DBM_TYPE dbf;

	key_datum.dptr = key;
	key_datum.dsize = strlen(key);
#if GDBM_FIX
	key_datum.dsize++;
#endif

	dbf = info->dbf;
	if (!dbf) {
		php_error(E_WARNING, "Unable to locate dbm file");
		return(0);
	}

	ret = DBM_DELETE(dbf, key_datum);
	return(ret);
}

/* {{{ proto string dbmfirstkey(int dbm_identifier)
   Retrieves the first key from a dbm database */
PHP_FUNCTION(dbmfirstkey)
{
	pval *id;
	dbm_info *info;
	char *ret;

	if (ZEND_NUM_ARGS()!=1||zend_get_parameters(ht,1,&id)==FAILURE) {
		WRONG_PARAM_COUNT;
	}

	info = php_find_dbm(id);
	if (!info) {
		php_error(E_WARNING, "not a valid database identifier %d", Z_LVAL_P(id));
		RETURN_FALSE;
	}

	ret = php_dbm_first_key(info);
	if (!ret) {
		RETURN_FALSE;
	} else {
		RETVAL_STRING(ret, 0);
	}
}
/* }}} */

char *php_dbm_first_key(dbm_info *info) {
	datum ret_datum;
	char *ret;
	DBM_TYPE dbf;

	dbf = info->dbf;
	if (!dbf) {
		php_error(E_WARNING, "Unable to locate dbm file");
		return(NULL);
	}

	/* explicitly zero-out ret_datum */
	ret_datum.dptr = NULL;
	ret_datum.dsize = 0;

	ret_datum = DBM_FIRSTKEY(dbf);
	
	if (!ret_datum.dptr)
		return NULL;

	ret = (char *)emalloc((ret_datum.dsize + 1) * sizeof(char));
	strncpy(ret, ret_datum.dptr, ret_datum.dsize);	
	ret[ret_datum.dsize] = '\0';

#if !NDBM & !GDBM
	efree(ret_datum.dptr);
#endif

	return (ret);
}

/* {{{ proto string dbmnextkey(int dbm_identifier, string key)
   Retrieves the next key from a dbm database */
PHP_FUNCTION(dbmnextkey)
{
	pval *id, *key;
	dbm_info *info;
	char *ret;

	if (ZEND_NUM_ARGS()!=2||zend_get_parameters(ht,2,&id,&key)==FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string(key);

	info = php_find_dbm(id);
	if (!info) {
		php_error(E_WARNING, "not a valid database identifier %d", Z_LVAL_P(id));
		RETURN_FALSE;
	}

	ret = php_dbm_nextkey(info, Z_STRVAL_P(key));
	if (!ret) {
		RETURN_FALSE;
	} else {
		RETVAL_STRING(ret, 0);
	}
}
/* }}} */


char *php_dbm_nextkey(dbm_info *info, char *key) {
	datum key_datum, ret_datum;
	char *ret;
	DBM_TYPE dbf;
	PLS_FETCH();

	key_datum.dptr = key;
	key_datum.dsize = strlen(key);
#if GDBM_FIX
	key_datum.dsize++;
#endif

	dbf = info->dbf;
	if (!dbf) {
		php_error(E_WARNING, "Unable to locate dbm file");
		return(NULL);
	}

	/* explicitly zero-out ret_datum */
	ret_datum.dptr = NULL;
	ret_datum.dsize = 0;

	ret_datum = DBM_NEXTKEY(dbf, key_datum);

	if (ret_datum.dptr) {
		ret = (char *)emalloc(sizeof(char) * ret_datum.dsize + 1);
		strncpy(ret, ret_datum.dptr, ret_datum.dsize);
		ret[ret_datum.dsize] = '\0';
#if GDBM
/* GDBM uses malloc to allocate the value_datum block, so we need to free it */
		free(ret_datum.dptr);
#else
# if !NDBM
		efree(ret_datum.dptr);
# endif
#endif
	}
	else ret=NULL;

	if (ret && PG(magic_quotes_runtime)) {
		ret = php_addslashes(ret, ret_datum.dsize, NULL, 1);
	}
	return(ret);
}


#if !GDBM && !NDBM
static long CurrentFlatFilePos = 0L;

int flatfile_store(FILE *dbf, datum key_datum, datum value_datum, int mode) {
	int ret;

	if (mode == DBM_INSERT) {
		if (flatfile_findkey(dbf, key_datum)) {
			return 1;
		}
		fseek(dbf,0L,SEEK_END);
		fprintf(dbf,"%d\n",key_datum.dsize);
		fflush(dbf);
		ret = write(fileno(dbf),key_datum.dptr,key_datum.dsize);
		fprintf(dbf,"%d\n",value_datum.dsize);
		fflush(dbf);
		ret = write(fileno(dbf),value_datum.dptr,value_datum.dsize);
	} else { /* DBM_REPLACE */
		flatfile_delete(dbf,key_datum);
		fprintf(dbf,"%d\n",key_datum.dsize);
		fflush(dbf);
		ret = write(fileno(dbf),key_datum.dptr,key_datum.dsize);
		fprintf(dbf,"%d\n",value_datum.dsize);
		ret = write(fileno(dbf),value_datum.dptr,value_datum.dsize);
	}

	if (ret>0)
		ret=0;
	return ret;
}

datum flatfile_fetch(FILE *dbf, datum key_datum) {
	datum value_datum = {NULL, 0};
	int num=0, buf_size=1024;
	char *buf;	

	if (flatfile_findkey(dbf,key_datum)) {
		buf = emalloc((buf_size+1) * sizeof(char));
		if (fgets(buf, 15, dbf)) {
			num = atoi(buf);
			if (num > buf_size) {
				buf_size+=num;
				buf = emalloc((buf_size+1)*sizeof(char));
			}
			read(fileno(dbf),buf,num);
			value_datum.dptr = buf;
			value_datum.dsize = num;
		}
	}
	return value_datum;
}

int flatfile_delete(FILE *dbf, datum key_datum) {
	char *key = key_datum.dptr;
	int size = key_datum.dsize;

	char *buf;
	int num, buf_size = 1024;
	long pos;

	rewind(dbf);

	buf = emalloc((buf_size + 1)*sizeof(char));
	while(!feof(dbf)) {
		/* read in the length of the key name */
		if (!fgets(buf, 15, dbf))
			break;
		num = atoi(buf);
		if (num > buf_size) {
			buf_size += num;
			if (buf) efree(buf);
			buf = emalloc((buf_size+1)*sizeof(char));
		}
		pos = ftell(dbf);

		/* read in the key name */
		num = fread(buf, sizeof(char), num, dbf);
		if (num<0) break;
		*(buf+num) = '\0';

		if (size == num && !memcmp(buf, key, size)) {
			fseek(dbf, pos, SEEK_SET);
			fputc(0, dbf);
			fflush(dbf);
			fseek(dbf, 0L, SEEK_END);
			if (buf) efree(buf);
			return SUCCESS;
		}	

		/* read in the length of the value */
		if (!fgets(buf,15,dbf))
			break;
		num = atoi(buf);
		if (num > buf_size) {
			buf_size+=num;
			if (buf) efree(buf);
			buf = emalloc((buf_size+1)*sizeof(char));
		}
		/* read in the value */
		num = fread(buf, sizeof(char), num, dbf);
		if (num<0)
			break;
	}
	if (buf) efree(buf);
	return FAILURE;
}	

int flatfile_findkey(FILE *dbf, datum key_datum) {
	char *buf = NULL;
	int num;
	int buf_size=1024;
	int ret=0;
	void *key = key_datum.dptr;
	int size = key_datum.dsize;

	rewind(dbf);
	buf = emalloc((buf_size+1)*sizeof(char));
	while (!feof(dbf)) {
		if (!fgets(buf,15,dbf)) break;
		num = atoi(buf);
		if (num > buf_size) {
			if (buf) efree(buf);
			buf_size+=num;
			buf = emalloc((buf_size+1)*sizeof(char));
		}
		num = fread(buf, sizeof(char), num, dbf);
		if (num<0) break;
		*(buf+num) = '\0';
		if (size == num) {
			if (!memcmp(buf,key,size)) {
				ret = 1;
				break;
			}
		}	
		if (!fgets(buf,15,dbf))
			break;
		num = atoi(buf);
		if (num > buf_size) {
			if (buf) efree(buf);
			buf_size+=num;
			buf = emalloc((buf_size+1)*sizeof(char));
		}
		num = fread(buf, sizeof(char), num, dbf);
		if (num<0) break;
		*(buf+num) = '\0';
	}
	if (buf) efree(buf);
	return(ret);
}	

datum flatfile_firstkey(FILE *dbf) {
	datum buf;
	int num;
	int buf_size=1024;

	rewind(dbf);
	buf.dptr = emalloc((buf_size+1)*sizeof(char));
	while(!feof(dbf)) {
		if (!fgets(buf.dptr,15,dbf)) break;
		num = atoi(buf.dptr);
		if (num > buf_size) {
			buf_size+=num;
			if (buf.dptr) efree(buf.dptr);
			buf.dptr = emalloc((buf_size+1)*sizeof(char));
		}
		num=read(fileno(dbf),buf.dptr,num);
		if (num<0) break;
		buf.dsize = num;
		if (*(buf.dptr)!=0) {
			CurrentFlatFilePos = ftell(dbf);
			return(buf);
		}
		if (!fgets(buf.dptr,15,dbf)) break;
		num = atoi(buf.dptr);
		if (num > buf_size) {
			buf_size+=num;
			if (buf.dptr) efree(buf.dptr);
			buf.dptr = emalloc((buf_size+1)*sizeof(char));
		}
		num=read(fileno(dbf),buf.dptr,num);
		if (num<0) break;
	}
	if (buf.dptr) efree(buf.dptr);
	buf.dptr = NULL;
	return(buf);
}	

datum flatfile_nextkey(FILE *dbf) {
	datum buf;
	int num;
	int buf_size=1024;

	fseek(dbf,CurrentFlatFilePos,SEEK_SET);
	buf.dptr = emalloc((buf_size+1)*sizeof(char));
	while(!feof(dbf)) {
		if (!fgets(buf.dptr,15,dbf)) break;
		num = atoi(buf.dptr);
		if (num > buf_size) {
			buf_size+=num;
			if (buf.dptr) efree(buf.dptr);
			buf.dptr = emalloc((buf_size+1)*sizeof(char));
		}
		num=read(fileno(dbf),buf.dptr,num);
		if (num<0) break;
		if (!fgets(buf.dptr,15,dbf)) break;
		num = atoi(buf.dptr);
		if (num > buf_size) {
			buf_size+=num;
			if (buf.dptr) efree(buf.dptr);
			buf.dptr = emalloc((buf_size+1)*sizeof(char));
		}
		num=read(fileno(dbf),buf.dptr,num);
		if (num<0) break;
		buf.dsize = num;
		if (*(buf.dptr)!=0) {
			CurrentFlatFilePos = ftell(dbf);
			return(buf);
		}
	}
	if (buf.dptr) efree(buf.dptr);
	buf.dptr = NULL;
	return(buf);
}	
#endif


PHP_MINIT_FUNCTION(db)
{
#if defined(THREAD_SAFE)
	dbm_global_struct *dbm_globals;
	PHP_MUTEX_ALLOC(dbm_mutex);
	PHP_MUTEX_LOCK(dbm_mutex);
	numthreads++;
	if (numthreads==1){
		if (!PHP3_TLS_PROC_STARTUP(DbmTls)){
			PHP_MUTEX_UNLOCK(dbm_mutex);
			PHP_MUTEX_FREE(dbm_mutex);
			return FAILURE;
		}
	}
	PHP_MUTEX_UNLOCK(dbm_mutex);
	if(!PHP3_TLS_THREAD_INIT(DbmTls,dbm_globals,dbm_global_struct)){
		PHP_MUTEX_FREE(dbm_mutex);
		return FAILURE;
	}
#endif

	le_db = zend_register_list_destructors_ex(php_dbm_close, NULL, "dbm", module_number);
	return SUCCESS;
}

static PHP_MSHUTDOWN_FUNCTION(db)
{
#ifdef THREAD_SAFE
	PHP3_TLS_THREAD_FREE(dbm_globals);
	PHP_MUTEX_LOCK(dbm_mutex);
	numthreads--;
	if (numthreads<1) {
		PHP3_TLS_PROC_SHUTDOWN(DbmTls);
		PHP_MUTEX_UNLOCK(dbm_mutex);
		PHP_MUTEX_FREE(dbm_mutex);
		return SUCCESS;
	}
	PHP_MUTEX_UNLOCK(dbm_mutex);
#endif
	return SUCCESS;
}

PHP_RINIT_FUNCTION(db)
{
#if !GDBM && !NDBM
	CurrentFlatFilePos = 0L;
#endif
	return SUCCESS;
}


function_entry dbm_functions[] = {
	PHP_FE(dblist,									NULL)
	PHP_FE(dbmopen,									NULL)
	PHP_FE(dbmclose,								NULL)
	PHP_FE(dbminsert,								NULL)
	PHP_FE(dbmfetch,								NULL)
	PHP_FE(dbmreplace,								NULL)
	PHP_FE(dbmexists,								NULL)
	PHP_FE(dbmdelete,								NULL)
	PHP_FE(dbmfirstkey,								NULL)
	PHP_FE(dbmnextkey,								NULL)
	{NULL,NULL,NULL}
};

zend_module_entry dbm_module_entry = {
	"db", dbm_functions, PHP_MINIT(db), PHP_MSHUTDOWN(db), PHP_RINIT(db), NULL, PHP_MINFO(db), STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_DB
ZEND_GET_MODULE(dbm)
#endif

#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */
