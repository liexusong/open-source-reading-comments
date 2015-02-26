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
   | Authors: Jim Winstead (jimw@php.net)                                 |
   +----------------------------------------------------------------------+
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "safe_mode.h"
#include "fopen_wrappers.h"
#include "php_globals.h"

#include <stdlib.h>

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if DBASE
#include "php_dbase.h"
#include "dbf.h"
#if defined(THREAD_SAFE)
DWORD DbaseTls;
static int numthreads=0;
void *dbase_mutex;

typedef struct dbase_global_struct{
	int le_dbhead;
}dbase_global_struct;

#define DBase_GLOBAL(a) dbase_globals->a

#define DBase_TLS_VARS \
	dbase_global_struct *dbase_globals; \
	dbase_globals=TlsGetValue(DbaseTls); 

#else
static int le_dbhead;
#define DBase_GLOBAL(a) a
#define DBase_TLS_VARS
#endif

#include <fcntl.h>
#include <errno.h>


static void _close_dbase(zend_rsrc_list_entry *rsrc)
{
	dbhead_t *dbhead = (dbhead_t *)rsrc->ptr;
	close(dbhead->db_fd);
	free_dbf_head(dbhead);
}


PHP_MINIT_FUNCTION(dbase)
{
#if defined(THREAD_SAFE)
	dbase_global_struct *dbase_globals;
#ifdef COMPILE_DL_DBASE
	CREATE_MUTEX(dbase_mutex,"DBase_TLS");
	SET_MUTEX(dbase_mutex);
	numthreads++;
	if (numthreads==1){
	if ((DbaseTls=TlsAlloc())==0xFFFFFFFF){
		FREE_MUTEX(dbase_mutex);
		return 0;
	}}
	FREE_MUTEX(dbase_mutex);
#endif
	dbase_globals = (dbase_global_struct *) LocalAlloc(LPTR, sizeof(dbase_global_struct)); 
	TlsSetValue(DbaseTls, (void *) dbase_globals);
#endif
	DBase_GLOBAL(le_dbhead) =
		zend_register_list_destructors_ex(_close_dbase, NULL, "dbase", module_number);
	return SUCCESS;
}

static PHP_MSHUTDOWN_FUNCTION(dbase)
{
#if defined(THREAD_SAFE)
	dbase_global_struct *dbase_globals;
	dbase_globals = TlsGetValue(DbaseTls); 
	if (dbase_globals != 0) 
		LocalFree((HLOCAL) dbase_globals); 
#ifdef COMPILE_DL_DBASE
	SET_MUTEX(dbase_mutex);
	numthreads--;
	if (!numthreads){
	if (!TlsFree(DbaseTls)){
		FREE_MUTEX(dbase_mutex);
		return 0;
	}}
	FREE_MUTEX(dbase_mutex);
#endif
#endif
	return SUCCESS;
}

/* {{{ proto int dbase_open(string name, int mode)
   Opens a dBase-format database file */
PHP_FUNCTION(dbase_open) {
	pval *dbf_name, *options;
	dbhead_t *dbh;
	int handle;
	PLS_FETCH();
	DBase_TLS_VARS;

	if (ZEND_NUM_ARGS() != 2 || getParameters(ht,2,&dbf_name,&options)==FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string(dbf_name);
	convert_to_long(options);

	if (PG(safe_mode) && (!php_checkuid(dbf_name->value.str.val, NULL, CHECKUID_CHECK_FILE_AND_DIR))) {
		RETURN_FALSE;
	}
	
	if (php_check_open_basedir(dbf_name->value.str.val)) {
		RETURN_FALSE;
	}

	dbh = dbf_open(dbf_name->value.str.val, options->value.lval);
	if (dbh == NULL) {
		php_error(E_WARNING, "unable to open database %s", dbf_name->value.str.val);
		RETURN_FALSE;
	}

	handle = zend_list_insert(dbh, DBase_GLOBAL(le_dbhead));
	RETURN_LONG(handle);
}
/* }}} */

/* {{{ proto bool dbase_close(int identifier)
   Closes an open dBase-format database file */
PHP_FUNCTION(dbase_close) {
	pval *dbh_id;
	dbhead_t *dbh;
	int dbh_type;
	DBase_TLS_VARS;

	if (ZEND_NUM_ARGS() != 1 || getParameters(ht,1,&dbh_id)==FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_long(dbh_id);
	dbh = zend_list_find(dbh_id->value.lval, &dbh_type);
	if (!dbh || dbh_type != DBase_GLOBAL(le_dbhead)) {
		php_error(E_WARNING, "Unable to find database for identifier %d", dbh_id->value.lval);
		RETURN_FALSE;
	}

	zend_list_delete(dbh_id->value.lval);
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto int dbase_numrecords(int identifier)
   Returns the number of records in the database */
PHP_FUNCTION(dbase_numrecords) {
	pval *dbh_id;
	dbhead_t *dbh;
	int dbh_type;
	DBase_TLS_VARS;

	if (ZEND_NUM_ARGS() != 1 || getParameters(ht,1,&dbh_id)==FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_long(dbh_id);
	dbh = zend_list_find(dbh_id->value.lval, &dbh_type);
	if (!dbh || dbh_type != DBase_GLOBAL(le_dbhead)) {
		php_error(E_WARNING, "Unable to find database for identifier %d", dbh_id->value.lval);
		RETURN_FALSE;
	}

	RETURN_LONG(dbh->db_records);
}
/* }}} */

/* {{{ proto int dbase_numfields(int identifier)
   Returns the number of fields (columns) in the database */
PHP_FUNCTION(dbase_numfields) {
	pval *dbh_id;
	dbhead_t *dbh;
	int dbh_type;
	DBase_TLS_VARS;

	if (ZEND_NUM_ARGS() != 1 || getParameters(ht,1,&dbh_id)==FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_long(dbh_id);
	dbh = zend_list_find(dbh_id->value.lval, &dbh_type);
	if (!dbh || dbh_type != DBase_GLOBAL(le_dbhead)) {
		php_error(E_WARNING, "Unable to find database for identifier %d", dbh_id->value.lval);
		RETURN_FALSE;
	}

	RETURN_LONG(dbh->db_nfields);
}
/* }}} */

/* {{{ proto bool dbase_pack(int identifier)
   Packs the database (deletes records marked for deletion) */
PHP_FUNCTION(dbase_pack) {
	pval *dbh_id;
	dbhead_t *dbh;
	int dbh_type;
	DBase_TLS_VARS;

	if (ZEND_NUM_ARGS() != 1 || getParameters(ht,1,&dbh_id)==FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_long(dbh_id);
	dbh = zend_list_find(dbh_id->value.lval, &dbh_type);
	if (!dbh || dbh_type != DBase_GLOBAL(le_dbhead)) {
		php_error(E_WARNING, "Unable to find database for identifier %d", dbh_id->value.lval);
		RETURN_FALSE;
	}

        pack_dbf(dbh);
        put_dbf_info(dbh);
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool dbase_add_record(int identifier, array data)
   Adds a record to the database */
PHP_FUNCTION(dbase_add_record) {
	pval *dbh_id, *fields, **field;
	dbhead_t *dbh;
	int dbh_type;

	int num_fields;
	dbfield_t *dbf, *cur_f;
	char *cp, *t_cp;
	int i;
	DBase_TLS_VARS;

	if (ZEND_NUM_ARGS() != 2 || getParameters(ht,2,&dbh_id,&fields)==FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_long(dbh_id);
	if (fields->type != IS_ARRAY) {
		php_error(E_WARNING, "Expected array as second parameter");
		RETURN_FALSE;
	}

	dbh = zend_list_find(dbh_id->value.lval, &dbh_type);
	if (!dbh || dbh_type != DBase_GLOBAL(le_dbhead)) {
		php_error(E_WARNING, "Unable to find database for identifier %d", dbh_id->value.lval);
		RETURN_FALSE;
	}

	num_fields = zend_hash_num_elements(fields->value.ht);

	if (num_fields != dbh->db_nfields) {
		php_error(E_WARNING, "Wrong number of fields specified");
		RETURN_FALSE;
	}

	cp = t_cp = (char *)emalloc(dbh->db_rlen + 1);
	if (!cp) {
		php_error(E_WARNING, "unable to allocate memory");
		RETURN_FALSE;
	}
	*t_cp++ = VALID_RECORD;

	dbf = dbh->db_fields;
	for (i = 0, cur_f = dbf; cur_f < &dbf[num_fields]; i++, cur_f++) {
		zval tmp;
		if (zend_hash_index_find(fields->value.ht, i, (void **)&field) == FAILURE) {
			php_error(E_WARNING, "unexpected error");
			efree(cp);
			RETURN_FALSE;
		}
		
		tmp = **field;
		zval_copy_ctor(&tmp);
		convert_to_string(&tmp);
		sprintf(t_cp, cur_f->db_format, tmp.value.str.val);
		zval_dtor(&tmp); 
		t_cp += cur_f->db_flen;
	}

	dbh->db_records++;
	if (put_dbf_record(dbh, dbh->db_records, cp) < 0) {
		php_error(E_WARNING, "unable to put record at %ld", dbh->db_records);
		efree(cp);
		RETURN_FALSE;
	}

        put_dbf_info(dbh);
	efree(cp);

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool dbase_replace_record(int identifier, array data, int recnum)
   Replaces a record to the database */
PHP_FUNCTION(dbase_replace_record)
{
	pval *dbh_id, *fields, *field, *recnum;
	dbhead_t *dbh;
	int dbh_type;

	int num_fields;
	dbfield_t *dbf, *cur_f;
	char *cp, *t_cp;
	int i;
	DBase_TLS_VARS;

	if (ZEND_NUM_ARGS() != 3 || getParameters(ht,3,&dbh_id,&fields,&recnum)==FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_long(dbh_id);
	convert_to_long(recnum);
	if (fields->type != IS_ARRAY) {
		php_error(E_WARNING, "Expected array as second parameter");
		RETURN_FALSE;
	}

	dbh = zend_list_find(dbh_id->value.lval, &dbh_type);
	if (!dbh || dbh_type != DBase_GLOBAL(le_dbhead)) {
		php_error(E_WARNING, "Unable to find database for identifier %d", dbh_id->value.lval);
		RETURN_FALSE;
	}

	num_fields = zend_hash_num_elements(fields->value.ht);

	if (num_fields != dbh->db_nfields) {
		php_error(E_WARNING, "Wrong number of fields specified");
		RETURN_FALSE;
	}

	cp = t_cp = (char *)emalloc(dbh->db_rlen + 1);
	if (!cp) {
		php_error(E_WARNING, "unable to allocate memory");
		RETURN_FALSE;
	}
	*t_cp++ = VALID_RECORD;

	dbf = dbh->db_fields;
	for (i = 0, cur_f = dbf; cur_f < &dbf[num_fields]; i++, cur_f++) {
		if (zend_hash_index_find(fields->value.ht, i, (void **)&field) == FAILURE) {
			php_error(E_WARNING, "unexpected error");
			efree(cp);
			RETURN_FALSE;
		}
		convert_to_string(field);
		sprintf(t_cp, cur_f->db_format, field->value.str.val); 
		t_cp += cur_f->db_flen;
	}

	if (put_dbf_record(dbh, recnum->value.lval, cp) < 0) {
		php_error(E_WARNING, "unable to put record at %ld", dbh->db_records);
		efree(cp);
		RETURN_FALSE;
	}

        put_dbf_info(dbh);
	efree(cp);

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool dbase_delete_record(int identifier, int record)
   Marks a record to be deleted */
PHP_FUNCTION(dbase_delete_record) {
	pval *dbh_id, *record;
	dbhead_t *dbh;
	int dbh_type;
	DBase_TLS_VARS;

	if (ZEND_NUM_ARGS() != 2 || getParameters(ht,2,&dbh_id,&record)==FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_long(dbh_id);
	convert_to_long(record);

	dbh = zend_list_find(dbh_id->value.lval, &dbh_type);
	if (!dbh || dbh_type != DBase_GLOBAL(le_dbhead)) {
		php_error(E_WARNING, "Unable to find database for identifier %d", dbh_id->value.lval);
		RETURN_FALSE;
	}

	if (del_dbf_record(dbh, record->value.lval) < 0) {
		if (record->value.lval > dbh->db_records) {
			php_error(E_WARNING, "record %d out of bounds", record->value.lval);
		} else {
			php_error(E_WARNING, "unable to delete record %d", record->value.lval);
		}
		RETURN_FALSE;
	}

        put_dbf_info(dbh);
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto array dbase_get_record(int identifier, int record)
   Returns an array representing a record from the database */
PHP_FUNCTION(dbase_get_record) {
	pval *dbh_id, *record;
	dbhead_t *dbh;
	int dbh_type;
	dbfield_t *dbf, *cur_f;
	char *data, *fnp, *str_value;
	size_t cursize = 0;
	DBase_TLS_VARS;

	if (ZEND_NUM_ARGS() != 2 || getParameters(ht,2,&dbh_id,&record)==FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_long(dbh_id);
	convert_to_long(record);

	dbh = zend_list_find(dbh_id->value.lval, &dbh_type);
	if (!dbh || dbh_type != DBase_GLOBAL(le_dbhead)) {
		php_error(E_WARNING, "Unable to find database for identifier %d", dbh_id->value.lval);
		RETURN_FALSE;
	}

	if ((data = get_dbf_record(dbh, record->value.lval)) == NULL) {
		php_error(E_WARNING, "Tried to read bad record %d", record->value.lval);
		RETURN_FALSE;
	}

	dbf = dbh->db_fields;

	if (array_init(return_value) == FAILURE) {
		RETURN_FALSE;
	}

        fnp = NULL;
        for (cur_f = dbf; cur_f < &dbf[dbh->db_nfields]; cur_f++) {
		/* get the value */
		str_value = (char *)emalloc(cur_f->db_flen + 1);

		if(cursize <= cur_f->db_flen) {
			cursize = cur_f->db_flen + 1;
			fnp = erealloc(fnp, cursize);
		}
                snprintf(str_value, cursize, cur_f->db_format, get_field_val(data, cur_f, fnp));

		/* now convert it to the right php internal type */
	        switch (cur_f->db_type) {
		case 'C':
		case 'D':
			add_next_index_string(return_value,str_value,1);
			break;
		case 'N':	/* FALLS THROUGH */
		case 'L':	/* FALLS THROUGH */
			if (cur_f->db_fdc == 0) {
				add_next_index_long(return_value, strtol(str_value, NULL, 10));
			} else {
				add_next_index_double(return_value, atof(str_value));
			}
			break;
		case 'M':
			/* this is a memo field. don't know how to deal with
			   this yet */
			break;
		default:
			/* should deal with this in some way */
			break;
		}
		efree(str_value);
        }
        efree(fnp);

	/* mark whether this record was deleted */
	if (data[0] == '*') {
		add_assoc_long(return_value,"deleted",1);
	}
	else {
		add_assoc_long(return_value,"deleted",0);
	}

	free(data);
}
/* }}} */

/* From Martin Kuba <makub@aida.inet.cz> */
/* {{{ proto array dbase_get_record_with_names(int identifier, int record)
   Returns an associative array representing a record from the database */
PHP_FUNCTION(dbase_get_record_with_names) {
	pval *dbh_id, *record;
	dbhead_t *dbh;
	int dbh_type;
	dbfield_t *dbf, *cur_f;
	char *data, *fnp, *str_value;
	DBase_TLS_VARS;

	if (ZEND_NUM_ARGS() != 2 || getParameters(ht,2,&dbh_id,&record)==FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_long(dbh_id);
	convert_to_long(record);

	dbh = zend_list_find(dbh_id->value.lval, &dbh_type);
	if (!dbh || dbh_type != DBase_GLOBAL(le_dbhead)) {
		php_error(E_WARNING, "Unable to find database for identifier %d", dbh_id->value.lval);
		RETURN_FALSE;
	}

	if ((data = get_dbf_record(dbh, record->value.lval)) == NULL) {
		php_error(E_WARNING, "Tried to read bad record %d", record->value.lval);
		RETURN_FALSE;
	}

	dbf = dbh->db_fields;

	if (array_init(return_value) == FAILURE) {
		RETURN_FALSE;
	}

	fnp = (char *)emalloc(dbh->db_rlen);
	for (cur_f = dbf; cur_f < &dbf[dbh->db_nfields]; cur_f++) {
		/* get the value */
		str_value = (char *)emalloc(cur_f->db_flen + 1);
		sprintf(str_value, cur_f->db_format, get_field_val(data, cur_f, fnp));

		/* now convert it to the right php internal type */
		switch (cur_f->db_type) {
			case 'C':
			case 'D':
				add_assoc_string(return_value,cur_f->db_fname,str_value,1);
				break;
			case 'N':       /* FALLS THROUGH */
			case 'L':       /* FALLS THROUGH */
				if (cur_f->db_fdc == 0) {
					add_assoc_long(return_value,cur_f->db_fname,strtol(str_value, NULL, 10));
				} else {
					add_assoc_double(return_value,cur_f->db_fname,atof(str_value));
				}
				break;
			case 'M':
				/* this is a memo field. don't know how to deal with this yet */
				break;
			default:
				/* should deal with this in some way */
				break;
		}
		efree(str_value);
	}
	efree(fnp);

	/* mark whether this record was deleted */
	if (data[0] == '*') {
		add_assoc_long(return_value,"deleted",1);
	} else {
		add_assoc_long(return_value,"deleted",0);
	}

	free(data);
}
/* }}} */

/* {{{ proto bool dbase_create(string filename, array fields)
   Creates a new dBase-format database file */
PHP_FUNCTION(dbase_create) {
	pval *filename, *fields, **field, **value;
	int fd;
	dbhead_t *dbh;

	int num_fields;
	dbfield_t *dbf, *cur_f;
	int i, rlen, handle;
	PLS_FETCH();
	DBase_TLS_VARS;

	if (ZEND_NUM_ARGS() != 2 || getParameters(ht,2,&filename,&fields)==FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string(filename);

	if (fields->type != IS_ARRAY) {
		php_error(E_WARNING, "Expected array as second parameter");
		RETURN_FALSE;
	}

	if (PG(safe_mode) && (!php_checkuid(Z_STRVAL_P(filename), NULL, CHECKUID_CHECK_FILE_AND_DIR))) {
		RETURN_FALSE;
	}
	
	if (php_check_open_basedir(Z_STRVAL_P(filename))) {
		RETURN_FALSE;
	}

	if ((fd = VCWD_OPEN((Z_STRVAL_P(filename), O_BINARY|O_RDWR|O_CREAT, 0644))) < 0) {
		php_error(E_WARNING, "Unable to create database (%d): %s", errno, strerror(errno));
		RETURN_FALSE;
	}

	num_fields = zend_hash_num_elements(fields->value.ht);

	/* have to use regular malloc() because this gets free()d by
	   code in the dbase library */
	dbh = (dbhead_t *)malloc(sizeof(dbhead_t));
	dbf = (dbfield_t *)malloc(sizeof(dbfield_t) * num_fields);
	if (!dbh || !dbf) {
		php_error(E_WARNING, "Unable to allocate memory for header info");
		RETURN_FALSE;
	}
	
	/* initialize the header structure */
	dbh->db_fields = dbf;
	dbh->db_fd = fd;
	dbh->db_dbt = DBH_TYPE_NORMAL;
	strcpy(dbh->db_date, "19930818");
	dbh->db_records = 0;
	dbh->db_nfields = num_fields;
	dbh->db_hlen = sizeof(struct dbf_dhead) + 2 + num_fields * sizeof(struct dbf_dfield);

	rlen = 1;
	/**
	 * Patch by greg@darkphoton.com
	 **/
	/* make sure that the db_format entries for all fields are set to NULL to ensure we
       don't seg fault if there's an error and we need to call free_dbf_head() before all
       fields have been defined. */
	for (i = 0, cur_f = dbf; i < num_fields; i++, cur_f++) {
		cur_f->db_format = NULL;
	}
	/**
	 * end patch
	 */


	for (i = 0, cur_f = dbf; i < num_fields; i++, cur_f++) {
		/* look up the first field */
		if (zend_hash_index_find(fields->value.ht, i, (void **)&field) == FAILURE) {
			php_error(E_WARNING, "unable to find field %d", i);
			free_dbf_head(dbh);
			RETURN_FALSE;
		}

		if (Z_TYPE_PP (field) != IS_ARRAY) {
			php_error(E_WARNING, "second parameter must be array of arrays");
			free_dbf_head(dbh);
			RETURN_FALSE;
		}

		/* field name */
		if (zend_hash_index_find(Z_ARRVAL_PP(field), 0, (void **)&value) == FAILURE) {
			php_error(E_WARNING, "expected field name as first element of list in field %d", i);
			free_dbf_head(dbh);
			RETURN_FALSE;
		}
		convert_to_string_ex(value);
		if ((*value)->value.str.len > 10 || (*value)->value.str.len == 0) {
			php_error(E_WARNING, "invalid field name '%s' (must be non-empty and less than or equal to 10 characters)", (*value)->value.str.val);
			free_dbf_head(dbh);
			RETURN_FALSE;
		}
		copy_crimp(cur_f->db_fname, (*value)->value.str.val, (*value)->value.str.len);

		/* field type */
		if (zend_hash_index_find(Z_ARRVAL_PP (field), 1,(void **)&value) == FAILURE) {
			php_error(E_WARNING, "expected field type as sececond element of list in field %d", i);
			RETURN_FALSE;
		}
		convert_to_string_ex(value);
		cur_f->db_type = toupper(*(*value)->value.str.val);

		cur_f->db_fdc = 0;

		/* verify the field length */
		switch (cur_f->db_type) {
		case 'L':
			cur_f->db_flen = 1;
			break;
		case 'M':
			cur_f->db_flen = 9;
			dbh->db_dbt = DBH_TYPE_MEMO;
			/* should create the memo file here, probably */
			break;
		case 'D':
			cur_f->db_flen = 8;
			break;
		case 'N':
		case 'C':
			/* field length */
			if (zend_hash_index_find(Z_ARRVAL_PP (field), 2,(void **)&value) == FAILURE) {
				php_error(E_WARNING, "expected field length as third element of list in field %d", i);
				free_dbf_head(dbh);
				RETURN_FALSE;
			}
			convert_to_long_ex(value);
			cur_f->db_flen = (*value)->value.lval;

			if (cur_f->db_type == 'N') {
				if (zend_hash_index_find(Z_ARRVAL_PP (field), 3, (void **)&value) == FAILURE) {
					php_error(E_WARNING, "expected field precision as fourth element of list in field %d", i);
					free_dbf_head(dbh);
					RETURN_FALSE;
				}
				convert_to_long_ex(value);
				cur_f->db_fdc = (*value)->value.lval;
			}
			break;
		default:
			php_error(E_WARNING, "unknown field type '%c'", cur_f->db_type);
		}
		cur_f->db_foffset = rlen;
		rlen += cur_f->db_flen;
	
		cur_f->db_format = get_dbf_f_fmt(cur_f);
	}

	dbh->db_rlen = rlen;
        put_dbf_info(dbh);

	handle = zend_list_insert(dbh, DBase_GLOBAL(le_dbhead));
	RETURN_LONG(handle);
}
/* }}} */

function_entry dbase_functions[] = {
	PHP_FE(dbase_open,								NULL)
	PHP_FE(dbase_create,							NULL)
	PHP_FE(dbase_close,								NULL)
	PHP_FE(dbase_numrecords,						NULL)
	PHP_FE(dbase_numfields,							NULL)
	PHP_FE(dbase_add_record,						NULL)
	PHP_FE(dbase_replace_record,					NULL)
	PHP_FE(dbase_get_record,						NULL)
	PHP_FE(dbase_get_record_with_names,				NULL)
	PHP_FE(dbase_delete_record,						NULL)
	PHP_FE(dbase_pack,								NULL)
	{NULL, NULL, NULL}
};

zend_module_entry dbase_module_entry = {
	"dbase", dbase_functions, PHP_MINIT(dbase), PHP_MSHUTDOWN(dbase), NULL, NULL, NULL, STANDARD_MODULE_PROPERTIES
};


#ifdef COMPILE_DL_DBASE
ZEND_GET_MODULE(dbase)

#if (WIN32|WINNT) && defined(THREAD_SAFE)

/*NOTE: You should have an odbc.def file where you
export DllMain*/
BOOL WINAPI DllMain(HANDLE hModule, 
                      DWORD  ul_reason_for_call, 
                      LPVOID lpReserved)
{
    return 1;
}
#endif
#endif

#endif
/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */
