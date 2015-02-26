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
   | Authors: Zeev Suraski <zeev@zend.com>                                |
   +----------------------------------------------------------------------+
   | php_sybase_get_column_content_with_type() based on code by:          |
   |                     Muhammad A Muquit <MA_Muquit@fccc.edu>           |
   |                     Rasmus Lerdorf <rasmus@lerdorf.on.ca>            |
   +----------------------------------------------------------------------+
 */
 
/* $Id: php_sybase_db.c,v 1.14.2.1 2001/05/24 12:42:08 ssb Exp $ */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_sybase_db.h"
#include "ext/standard/php_standard.h"
#include "ext/standard/info.h"
#include "php_globals.h"

#if HAVE_SYBASE

#include <sybfront.h>
#include <sybdb.h>
#include <syberror.h>

#if BROKEN_SYBASE_PCONNECTS
#include "http_log.h"
#endif

function_entry sybase_functions[] = {
	PHP_FE(sybase_connect,			NULL)
	PHP_FE(sybase_pconnect,			NULL)
	PHP_FE(sybase_close,			NULL)
	PHP_FE(sybase_select_db,		NULL)
	PHP_FE(sybase_query,			NULL)
	PHP_FE(sybase_free_result,		NULL)
	PHP_FE(sybase_get_last_message,	NULL)
	PHP_FE(sybase_num_rows,			NULL)
	PHP_FE(sybase_num_fields,		NULL)
	PHP_FE(sybase_fetch_row,		NULL)
	PHP_FE(sybase_fetch_array,		NULL)
	PHP_FE(sybase_fetch_object,		NULL)
	PHP_FE(sybase_data_seek,		NULL)
	PHP_FE(sybase_fetch_field,		NULL)
	PHP_FE(sybase_field_seek,		NULL)
	PHP_FE(sybase_result,			NULL)
	PHP_FE(sybase_affected_rows,		NULL)
	PHP_FE(sybase_min_error_severity,	NULL)
	PHP_FE(sybase_min_message_severity,	NULL)
	PHP_FALIAS(mssql_connect,		sybase_connect,			NULL)
	PHP_FALIAS(mssql_pconnect,		sybase_pconnect,		NULL)
	PHP_FALIAS(mssql_close,			sybase_close,			NULL)
	PHP_FALIAS(mssql_select_db,		sybase_select_db,		NULL)
	PHP_FALIAS(mssql_query,			sybase_query,			NULL)
	PHP_FALIAS(mssql_free_result,	sybase_free_result,		NULL)
	PHP_FALIAS(mssql_get_last_message,	sybase_get_last_message,	NULL)
	PHP_FALIAS(mssql_num_rows,		sybase_num_rows,		NULL)
	PHP_FALIAS(mssql_num_fields,	sybase_num_fields,		NULL)
	PHP_FALIAS(mssql_fetch_row,		sybase_fetch_row,		NULL)
	PHP_FALIAS(mssql_fetch_array,	sybase_fetch_array,		NULL)
	PHP_FALIAS(mssql_fetch_object,	sybase_fetch_object,	NULL)
	PHP_FALIAS(mssql_data_seek,		sybase_data_seek,		NULL)
	PHP_FALIAS(mssql_fetch_field,	sybase_fetch_field,		NULL)
	PHP_FALIAS(mssql_field_seek,	sybase_field_seek,		NULL)
	PHP_FALIAS(mssql_result,		sybase_result,			NULL)
	PHP_FALIAS(mssql_affected_rows,		sybase_affected_rows,			NULL)
	PHP_FALIAS(mssql_min_error_severity,	sybase_min_error_severity,		NULL)
	PHP_FALIAS(mssql_min_message_severity,	sybase_min_message_severity,	NULL)
	{NULL, NULL, NULL}
};

zend_module_entry sybase_module_entry = {
	"sybase", sybase_functions, PHP_MINIT(sybase), PHP_MSHUTDOWN(sybase), PHP_RINIT(sybase), PHP_RSHUTDOWN(sybase), PHP_MINFO(sybase), STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_SYBASE
ZEND_GET_MODULE(sybase)
#endif

THREAD_LS sybase_module php_sybase_module;


#define CHECK_LINK(link) { if (link==-1) { php_error(E_WARNING,"Sybase:  A link to the server could not be established"); RETURN_FALSE; } }


static void php_sybase_get_column_content(sybase_link *sybase_ptr,int offset,pval **result_ptr, int column_type);

/* error handler */
static int php_sybase_error_handler(DBPROCESS *dbproc,int severity,int dberr,
										int oserr,char *dberrstr,char *oserrstr)
{
	if (severity >= php_sybase_module.min_error_severity) {
		php_error(E_WARNING,"Sybase error:  %s (severity %d)",dberrstr,severity);
	}
	return INT_CANCEL;  
}

/* message handler */
static int php_sybase_message_handler(DBPROCESS *dbproc,DBINT msgno,int msgstate,
										int severity,char *msgtext,char *srvname,
										char *procname,DBUSMALLINT line)
{
	if (severity >= php_sybase_module.min_message_severity) {
		php_error(E_WARNING,"Sybase message:  %s (severity %d)",msgtext,severity);
	}
	STR_FREE(php_sybase_module.server_message);
	php_sybase_module.server_message = estrdup(msgtext);
	return 0;
}


static int _clean_invalid_results(list_entry *le)
{
	if (le->type == php_sybase_module.le_result) {
		sybase_link *sybase_ptr = ((sybase_result *) le->ptr)->sybase_ptr;
		
		if (!sybase_ptr->valid) {
			return 1;
		}
	}
	return 0;
}


static void _free_sybase_result(zend_rsrc_list_entry *rsrc)
{
	sybase_result *result = (sybase_result *)rsrc->ptr;
	int i,j;
	
	if (result->data) {
		for (i=0; i<result->num_rows; i++) {
			for (j=0; j<result->num_fields; j++) {
				zval_ptr_dtor(&result->data[i][j]);
			}
			efree(result->data[i]);
		}
		efree(result->data);
	}
	
	if (result->fields) {
		for (i=0; i<result->num_fields; i++) {
			STR_FREE(result->fields[i].name);
			STR_FREE(result->fields[i].column_source);
		}
		efree(result->fields);
	}
	efree(result);
}


static void _close_sybase_link(zend_rsrc_list_entry *rsrc)
{
	sybase_link *sybase_ptr = (sybase_link *)rsrc->ptr;
	ELS_FETCH();

	sybase_ptr->valid = 0;

    /* 
	  this can cause crashes in the current model.
      if the resource gets destroyed via destroy_resource_list() resource_list
      will *not* be in a consistent state. thies@thieso.net
    */

	zend_hash_apply(&EG(regular_list),(int (*)(void *))_clean_invalid_results);
	dbclose(sybase_ptr->link);
	dbloginfree(sybase_ptr->login);
	efree(sybase_ptr);
	php_sybase_module.num_links--;
}


static void _close_sybase_plink(zend_rsrc_list_entry *rsrc)
{
	sybase_link *sybase_ptr = (sybase_link *)rsrc->ptr;
	dbclose(sybase_ptr->link);
	dbloginfree(sybase_ptr->login);
	free(sybase_ptr);
	php_sybase_module.num_persistent--;
	php_sybase_module.num_links--;
}


PHP_MINIT_FUNCTION(sybase)
{
	char *interface_file;

	if (dbinit()==FAIL) {
		return FAILURE;
	}
	dberrhandle((EHANDLEFUNC) php_sybase_error_handler);
	dbmsghandle((MHANDLEFUNC) php_sybase_message_handler);

	if (cfg_get_string("sybase.interface_file",&interface_file)==SUCCESS) {
		dbsetifile(interface_file);
	}
	if (cfg_get_long("sybase.allow_persistent",&php_sybase_module.allow_persistent)==FAILURE) {
		php_sybase_module.allow_persistent=1;
	}
	if (cfg_get_long("sybase.max_persistent",&php_sybase_module.max_persistent)==FAILURE) {
		php_sybase_module.max_persistent=-1;
	}
	if (cfg_get_long("sybase.max_links",&php_sybase_module.max_links)==FAILURE) {
		php_sybase_module.max_links=-1;
	}
	if (cfg_get_long("sybase.min_error_severity",&php_sybase_module.cfg_min_error_severity)==FAILURE) {
		php_sybase_module.cfg_min_error_severity=10;
	}
	if (cfg_get_long("sybase.min_message_severity",&php_sybase_module.cfg_min_message_severity)==FAILURE) {
		php_sybase_module.cfg_min_message_severity=10;
	}
	if (cfg_get_long("sybase.compatability_mode",&php_sybase_module.compatability_mode)==FAILURE) {
		php_sybase_module.compatability_mode = 0;
	}
	
	php_sybase_module.num_persistent=0;
	php_sybase_module.le_link = zend_register_list_destructors_ex(_close_sybase_link, NULL, "sybase-db link", module_number);
	php_sybase_module.le_plink = zend_register_list_destructors_ex(NULL, _close_sybase_plink, "sybase-db link persistent", module_number);
	php_sybase_module.le_result = zend_register_list_destructors_ex(_free_sybase_result, NULL, "sybase-db result", module_number);
	
	return SUCCESS;
}


PHP_RINIT_FUNCTION(sybase)
{
	php_sybase_module.default_link=-1;
	php_sybase_module.num_links = php_sybase_module.num_persistent;
	php_sybase_module.appname = estrndup("PHP 4.0",7);
	php_sybase_module.server_message = empty_string;
	php_sybase_module.min_error_severity = php_sybase_module.cfg_min_error_severity;
	php_sybase_module.min_message_severity = php_sybase_module.cfg_min_message_severity;
	return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(sybase)
{
	dbexit();
	return SUCCESS;
}

PHP_RSHUTDOWN_FUNCTION(sybase)
{
	efree(php_sybase_module.appname);
	STR_FREE(php_sybase_module.server_message);
	return SUCCESS;
}

static void php_sybase_do_connect(INTERNAL_FUNCTION_PARAMETERS,int persistent)
{
	char *user=NULL,*passwd=NULL,*host=NULL,*charset=NULL;
	char *hashed_details;
	int hashed_details_length;
	sybase_link sybase,*sybase_ptr;

	switch(ZEND_NUM_ARGS()) {
		case 0: /* defaults */
			hashed_details_length=6+3;
			hashed_details = (char *) emalloc(hashed_details_length+1);
			strcpy(hashed_details,"sybase___");
			break;
		case 1: {
				pval *yyhost;
				
				if (getParameters(ht, 1, &yyhost)==FAILURE) {
					RETURN_FALSE;
				}
				convert_to_string(yyhost);
				host = yyhost->value.str.val;
				hashed_details_length = yyhost->value.str.len+6+3;
				hashed_details = (char *) emalloc(hashed_details_length+1);
				sprintf(hashed_details,"sybase_%s__",yyhost->value.str.val);
			}
			break;
		case 2: {
				pval *yyhost,*yyuser;
				
				if (getParameters(ht, 2, &yyhost, &yyuser)==FAILURE) {
					RETURN_FALSE;
				}
				convert_to_string(yyhost);
				convert_to_string(yyuser);
				host = yyhost->value.str.val;
				user = yyuser->value.str.val;
				hashed_details_length = yyhost->value.str.len+yyuser->value.str.len+6+3;
				hashed_details = (char *) emalloc(hashed_details_length+1);
				sprintf(hashed_details,"sybase_%s_%s_",yyhost->value.str.val,yyuser->value.str.val);
			}
			break;
		case 3: {
				pval *yyhost,*yyuser,*yypasswd;
			
				if (getParameters(ht, 3, &yyhost, &yyuser, &yypasswd) == FAILURE) {
					RETURN_FALSE;
				}
				convert_to_string(yyhost);
				convert_to_string(yyuser);
				convert_to_string(yypasswd);
				host = yyhost->value.str.val;
				user = yyuser->value.str.val;
				passwd = yypasswd->value.str.val;
				hashed_details_length = yyhost->value.str.len+yyuser->value.str.len+yypasswd->value.str.len+6+3;
				hashed_details = (char *) emalloc(hashed_details_length+1);
				sprintf(hashed_details,"sybase_%s_%s_%s",yyhost->value.str.val,yyuser->value.str.val,yypasswd->value.str.val); /* SAFE */
			}
			break;
		case 4: {
				pval *yyhost,*yyuser,*yypasswd,*yycharset;

				if (getParameters(ht, 4, &yyhost, &yyuser, &yypasswd, &yycharset) == FAILURE) {
					RETURN_FALSE;
				}
				convert_to_string(yyhost);
				convert_to_string(yyuser);
				convert_to_string(yypasswd);
				convert_to_string(yycharset);
				host = yyhost->value.str.val;
				user = yyuser->value.str.val;
				passwd = yypasswd->value.str.val;
				charset = yycharset->value.str.val;
				hashed_details_length = yyhost->value.str.len+yyuser->value.str.len+yypasswd->value.str.len+yycharset->value.str.len+6+3;
				hashed_details = (char *) emalloc(hashed_details_length+1);
				sprintf(hashed_details,"sybase_%s_%s_%s_%s",yyhost->value.str.val,yyuser->value.str.val,yypasswd->value.str.val,yycharset->value.str.val); /* SAFE */
			}
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}


	/* set a DBLOGIN record */	
	if ((sybase.login=dblogin())==NULL) {
		php_error(E_WARNING,"Sybase:  Unable to allocate login record");
		RETURN_FALSE;
	}
	
	if (user) {
		DBSETLUSER(sybase.login,user);
	}
	if (passwd) {
		DBSETLPWD(sybase.login,passwd);
	}
	if (charset) {
		DBSETLCHARSET(sybase.login,charset);
	}
	DBSETLAPP(sybase.login,php_sybase_module.appname);
	sybase.valid = 1;

	if (!php_sybase_module.allow_persistent) {
		persistent=0;
	}
	if (persistent) {
		list_entry *le;

		/* try to find if we already have this link in our persistent list */
		if (zend_hash_find(&EG(persistent_list), hashed_details, hashed_details_length+1, (void **) &le)==FAILURE) {  /* we don't */
			list_entry new_le;

			if (php_sybase_module.max_links!=-1 && php_sybase_module.num_links>=php_sybase_module.max_links) {
				php_error(E_WARNING,"Sybase:  Too many open links (%d)",php_sybase_module.num_links);
				efree(hashed_details);
				dbloginfree(sybase.login);
				RETURN_FALSE;
			}
			if (php_sybase_module.max_persistent!=-1 && php_sybase_module.num_persistent>=php_sybase_module.max_persistent) {
				php_error(E_WARNING,"Sybase:  Too many open persistent links (%d)",php_sybase_module.num_persistent);
				efree(hashed_details);
				dbloginfree(sybase.login);
				RETURN_FALSE;
			}
			/* create the link */
			if ((sybase.link=PHP_SYBASE_DBOPEN(sybase.login,host))==FAIL) {
				/*php_error(E_WARNING,"Sybase:  Unable to connect to server:  %s",sybase_error(sybase));*/
				efree(hashed_details);
				dbloginfree(sybase.login);
				RETURN_FALSE;
			}

			if (dbsetopt(sybase.link,DBBUFFER,"2",-1)==FAIL) {
				efree(hashed_details);
				dbloginfree(sybase.login);
				dbclose(sybase.link);
				RETURN_FALSE;
			}

			/* hash it up */
			sybase_ptr = (sybase_link *) malloc(sizeof(sybase_link));
			memcpy(sybase_ptr,&sybase,sizeof(sybase_link));
			new_le.type = php_sybase_module.le_plink;
			new_le.ptr = sybase_ptr;
			if (zend_hash_update(&EG(persistent_list), hashed_details, hashed_details_length+1, (void *) &new_le, sizeof(list_entry),NULL)==FAILURE) {
				free(sybase_ptr);
				efree(hashed_details);
				dbloginfree(sybase.login);
				RETURN_FALSE;
			}
			php_sybase_module.num_persistent++;
			php_sybase_module.num_links++;
		} else {  /* we do */
			if (le->type != php_sybase_module.le_plink) {
				php_error(E_WARNING,"Sybase:  Hashed persistent link is not a Sybase link!");
				RETURN_FALSE;
			}
			
			sybase_ptr = (sybase_link *) le->ptr;
			/* test that the link hasn't died */
			if (DBDEAD(sybase_ptr->link)==TRUE) {
				if ((sybase_ptr->link=PHP_SYBASE_DBOPEN(sybase_ptr->login,host))==FAIL) {
					/*php_error(E_WARNING,"Sybase:  Link to server lost, unable to reconnect");*/
					zend_hash_del(&EG(persistent_list), hashed_details, hashed_details_length+1);
					efree(hashed_details);
					RETURN_FALSE;
				}
				if (dbsetopt(sybase_ptr->link,DBBUFFER,"2",-1)==FAIL) {
					zend_hash_del(&EG(persistent_list), hashed_details, hashed_details_length+1);
					efree(hashed_details);
					RETURN_FALSE;
				}
			}
		}
		return_value->value.lval = zend_list_insert(sybase_ptr,php_sybase_module.le_plink);
		return_value->type = IS_LONG;
	} else { /* non persistent */
		list_entry *index_ptr,new_index_ptr;
		
		/* first we check the hash for the hashed_details key.  if it exists,
		 * it should point us to the right offset where the actual sybase link sits.
		 * if it doesn't, open a new sybase link, add it to the resource list,
		 * and add a pointer to it with hashed_details as the key.
		 */
		if (zend_hash_find(&EG(regular_list),hashed_details,hashed_details_length+1,(void **) &index_ptr)==SUCCESS) {
			int type,link;
			void *ptr;

			if (index_ptr->type != le_index_ptr) {
				RETURN_FALSE;
			}
			link = (int) index_ptr->ptr;
			ptr = zend_list_find(link,&type);   /* check if the link is still there */
			if (ptr && (type==php_sybase_module.le_link || type==php_sybase_module.le_plink)) {
				return_value->value.lval = php_sybase_module.default_link = link;
				return_value->type = IS_LONG;
				efree(hashed_details);
				return;
			} else {
				zend_hash_del(&EG(regular_list),hashed_details,hashed_details_length+1);
			}
		}
		if (php_sybase_module.max_links!=-1 && php_sybase_module.num_links>=php_sybase_module.max_links) {
			php_error(E_WARNING,"Sybase:  Too many open links (%d)",php_sybase_module.num_links);
			efree(hashed_details);
			RETURN_FALSE;
		}
		
		if ((sybase.link=PHP_SYBASE_DBOPEN(sybase.login,host))==NULL) {
			/*php_error(E_WARNING,"Sybase:  Unable to connect to server:  %s",sybase_error(sybase));*/
			efree(hashed_details);
			RETURN_FALSE;
		}

		if (dbsetopt(sybase.link,DBBUFFER,"2",-1)==FAIL) {
			efree(hashed_details);
			dbloginfree(sybase.login);
			dbclose(sybase.link);
			RETURN_FALSE;
		}
		
		/* add it to the list */
		sybase_ptr = (sybase_link *) emalloc(sizeof(sybase_link));
		memcpy(sybase_ptr,&sybase,sizeof(sybase_link));
		return_value->value.lval = zend_list_insert(sybase_ptr,php_sybase_module.le_link);
		return_value->type = IS_LONG;
		
		/* add it to the hash */
		new_index_ptr.ptr = (void *) return_value->value.lval;
		new_index_ptr.type = le_index_ptr;
		if (zend_hash_update(&EG(regular_list),hashed_details,hashed_details_length+1,(void *) &new_index_ptr, sizeof(list_entry),NULL)==FAILURE) {
			efree(hashed_details);
			RETURN_FALSE;
		}
		php_sybase_module.num_links++;
	}
	efree(hashed_details);
	php_sybase_module.default_link=return_value->value.lval;
}


static int php_sybase_get_default_link(INTERNAL_FUNCTION_PARAMETERS)
{
	if (php_sybase_module.default_link==-1) { /* no link opened yet, implicitly open one */
		ht = 0;
		php_sybase_do_connect(INTERNAL_FUNCTION_PARAM_PASSTHRU,0);
	}
	return php_sybase_module.default_link;
}


/* {{{ proto int sybase_connect([string host [, string user [, string password [, string charset]]]])
   Open Sybase server connection */
PHP_FUNCTION(sybase_connect)
{
	php_sybase_do_connect(INTERNAL_FUNCTION_PARAM_PASSTHRU,0);
}
/* }}} */

/* {{{ proto int sybase_pconnect([string host [, string user [, string password [, string charset]]]])
   Open persistent Sybase connection */
PHP_FUNCTION(sybase_pconnect)
{
	php_sybase_do_connect(INTERNAL_FUNCTION_PARAM_PASSTHRU,1);
}
/* }}} */

/* {{{ proto bool sybase_close([int link_id])
   Close Sybase connection */
PHP_FUNCTION(sybase_close)
{
	pval *sybase_link_index;
	int id,type;
	
	switch (ZEND_NUM_ARGS()) {
		case 0:
			id = php_sybase_module.default_link;
			break;
		case 1:
			if (getParameters(ht, 1, &sybase_link_index)==FAILURE) {
				RETURN_FALSE;
			}
			convert_to_long(sybase_link_index);
			id = sybase_link_index->value.lval;
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}
	
	zend_list_find(id,&type);
	if (type!=php_sybase_module.le_link && type!=php_sybase_module.le_plink) {
		php_error(E_WARNING,"%d is not a Sybase link index",id);
		RETURN_FALSE;
	}
	
	zend_list_delete(id);
	RETURN_TRUE;
}
/* }}} */
	

/* {{{ proto bool sybase_select_db(string database [, int link_id])
   Select Sybase database */
PHP_FUNCTION(sybase_select_db)
{
	pval *db,*sybase_link_index;
	int id,type;
	sybase_link  *sybase_ptr;
	
	switch(ZEND_NUM_ARGS()) {
		case 1:
			if (getParameters(ht, 1, &db)==FAILURE) {
				RETURN_FALSE;
			}
			id = php_sybase_get_default_link(INTERNAL_FUNCTION_PARAM_PASSTHRU);
			break;
		case 2:
			if (getParameters(ht, 2, &db, &sybase_link_index)==FAILURE) {
				RETURN_FALSE;
			}
			convert_to_long(sybase_link_index);
			id = sybase_link_index->value.lval;
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}
	
	CHECK_LINK(id);
	
	sybase_ptr = (sybase_link *) zend_list_find(id,&type);
	if (type!=php_sybase_module.le_link && type!=php_sybase_module.le_plink) {
		php_error(E_WARNING,"%d is not a Sybase link index",id);
		RETURN_FALSE;
	}
	
	convert_to_string(db);
	
	if (dbuse(sybase_ptr->link,db->value.str.val)==FAIL) {
		/*php_error(E_WARNING,"Sybase:  Unable to select database:  %s",sybase_error(sybase));*/
		RETURN_FALSE;
	} else {
		RETURN_TRUE;
	}
}
/* }}} */
	

static void php_sybase_get_column_content(sybase_link *sybase_ptr,int offset,pval **result_ptr, int column_type)
{
	zval *result;

	ALLOC_INIT_ZVAL(result);
	*result_ptr = result;

	if (dbdatlen(sybase_ptr->link,offset) == 0) {
		var_reset(result);
		return;
	}

	switch (column_type)
	{
		case SYBINT2:
		case SYBINT4: {	
			result->value.lval = (long) anyintcol(offset);
			result->type = IS_LONG;
			break;
		}
		case SYBCHAR:
		case SYBTEXT: {
			int length;
			char *data = charcol(offset);

			length=dbdatlen(sybase_ptr->link,offset);
			while (length>0 && charcol(offset)[length-1] == ' ') { /* nuke trailing whitespace */
				length--;
			}
			result->value.str.val = estrndup(data,length);
			result->value.str.len = length;
			result->type = IS_STRING;
			break;
		}
		/*case SYBFLT8:*/
		case SYBREAL: {
			result->value.dval = (double) floatcol(offset);
			result->type = IS_DOUBLE;
			break;
		}
		default: {
			if (dbwillconvert(coltype(offset),SYBCHAR)) {
				char *res_buf;
				int res_length = dbdatlen(sybase_ptr->link,offset);
				register char *p;
			
				switch (coltype(offset)) {
					case SYBBINARY:
					case SYBVARBINARY:
					case SYBCHAR:
					case SYBVARCHAR:
					case SYBTEXT:
					case SYBIMAGE:
						break;
					default:
						/* take no chances, no telling how big the result would really be */
						res_length += 20;
						break;
				}

				res_buf = (char *) emalloc(res_length+1);
				memset(res_buf,' ',res_length+1);  /* XXX i'm sure there's a better way
													  but i don't have sybase here to test
													  991105 thies@thieso.net  */
				dbconvert(NULL,coltype(offset),dbdata(sybase_ptr->link,offset), res_length,SYBCHAR,res_buf,-1);
		
				/* get rid of trailing spaces */
				p = res_buf + res_length;
				while (*p == ' ') {
					p--;
					res_length--;
				}
				*(++p) = 0; /* put a trailing NULL */
		
				result->value.str.len = res_length;
				result->value.str.val = res_buf;
				result->type = IS_STRING;
			} else {
				php_error(E_WARNING,"Sybase:  column %d has unknown data type (%d)", offset, coltype(offset));
				var_reset(result);
			}
		}
	}
}


/* {{{ proto int sybase_query(string query [, int link_id])
   Send Sybase query */
PHP_FUNCTION(sybase_query)
{
	pval *query,*sybase_link_index;
	int id,type,retvalue;
	sybase_link *sybase_ptr;
	sybase_result *result;
	int num_fields;
	int blocks_initialized=1;
	int i,j;
	int *column_types;

	switch(ZEND_NUM_ARGS()) {
		case 1:
			if (getParameters(ht, 1, &query)==FAILURE) {
				RETURN_FALSE;
			}
			id = php_sybase_module.default_link;
			break;
		case 2:
			if (getParameters(ht, 2, &query, &sybase_link_index)==FAILURE) {
				RETURN_FALSE;
			}
			convert_to_long(sybase_link_index);
			id = sybase_link_index->value.lval;
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}
	
	sybase_ptr = (sybase_link *) zend_list_find(id,&type);
	if (type!=php_sybase_module.le_link && type!=php_sybase_module.le_plink) {
		php_error(E_WARNING,"%d is not a Sybase link index",id);
		RETURN_FALSE;
	}
	
	convert_to_string(query);
	if (dbcmd(sybase_ptr->link,query->value.str.val)==FAIL) {
		/*php_error(E_WARNING,"Sybase:  Unable to set query");*/
		RETURN_FALSE;
	}
	if (dbsqlexec(sybase_ptr->link)==FAIL || dbresults(sybase_ptr->link)==FAIL) {
		/*php_error(E_WARNING,"Sybase:  Query failed");*/
		RETURN_FALSE;
	}
	
	/* The following is more or less the equivalent of mysql_store_result().
	 * fetch all rows from the server into the row buffer, thus:
	 * 1)  Being able to fire up another query without explicitly reading all rows
	 * 2)  Having numrows accessible
	 */
	
	retvalue=dbnextrow(sybase_ptr->link);
	
	if (retvalue==FAIL) {
		RETURN_FALSE;
	}

	num_fields = dbnumcols(sybase_ptr->link);
	if (num_fields<=0) {
		RETURN_TRUE;
	}
	
	column_types = (int *) emalloc(sizeof(int) * num_fields);
	for (i=0; i<num_fields; i++) {
		column_types[i] = coltype(i+1);
	}
	
	result = (sybase_result *) emalloc(sizeof(sybase_result));
	result->data = (pval ***) emalloc(sizeof(pval **)*SYBASE_ROWS_BLOCK);
	result->sybase_ptr = sybase_ptr;
	result->cur_field=result->cur_row=result->num_rows=0;
	result->num_fields = num_fields;

	i=0;
	while (retvalue!=FAIL && retvalue!=NO_MORE_ROWS) {
		result->num_rows++;
		if (result->num_rows > blocks_initialized*SYBASE_ROWS_BLOCK) {
			result->data = (pval ***) erealloc(result->data,sizeof(pval **)*SYBASE_ROWS_BLOCK*(++blocks_initialized));
		}
		result->data[i] = (pval **) emalloc(sizeof(pval *)*num_fields);
		for (j=1; j<=num_fields; j++) {
			php_sybase_get_column_content(sybase_ptr, j, &result->data[i][j-1], column_types[j-1]);
			if (!php_sybase_module.compatability_mode) {
				zval *cur_value = result->data[i][j-1];

				convert_to_string(cur_value);
				if (PG(magic_quotes_runtime)) {
					cur_value->value.str.val = php_addslashes(cur_value->value.str.val, cur_value->value.str.len, &cur_value->value.str.len,0);
				}
			}
		}
		retvalue=dbnextrow(sybase_ptr->link);
		dbclrbuf(sybase_ptr->link,DBLASTROW(sybase_ptr->link)-1); 
		i++;
	}
	result->num_rows = DBCOUNT(sybase_ptr->link);
	
	result->fields = (sybase_field *) emalloc(sizeof(sybase_field)*num_fields);
	j=0;
	for (i=0; i<num_fields; i++) {
		char *fname = dbcolname(sybase_ptr->link,i+1);
		char computed_buf[16];
		
		if (*fname) {
			result->fields[i].name = estrdup(fname);
		} else {
			if (j>0) {
				snprintf(computed_buf,16,"computed%d",j);
			} else {
				strcpy(computed_buf,"computed");
			}
			result->fields[i].name = estrdup(computed_buf);
			j++;
		}
		result->fields[i].max_length = dbcollen(sybase_ptr->link,i+1);
		result->fields[i].column_source = estrdup(dbcolsource(sybase_ptr->link,i+1));
		if (!result->fields[i].column_source) {
			result->fields[i].column_source = empty_string;
		}
		result->fields[i].type = column_types[i];
		/* set numeric flag */
		switch (column_types[i]) {
			case SYBINT2:
			case SYBINT4:
			case SYBFLT8:
			case SYBREAL:
				result->fields[i].numeric = 1;
				break;
			case SYBCHAR:
			case SYBTEXT:
			default:
				result->fields[i].numeric = 0;
				break;
		}
	}
	efree(column_types);
	return_value->value.lval = zend_list_insert(result,php_sybase_module.le_result);
	return_value->type = IS_LONG;
}
/* }}} */

                        
/* {{{ proto bool sybase_free_result(int result)
   Free result memory */
PHP_FUNCTION(sybase_free_result)
{
	pval *sybase_result_index;
	sybase_result *result;
	int type;
	
	if (ZEND_NUM_ARGS()!=1 || getParameters(ht, 1, &sybase_result_index)==FAILURE) {
		WRONG_PARAM_COUNT;
	}
	
	convert_to_long(sybase_result_index);
	if (sybase_result_index->value.lval==0) {
		RETURN_FALSE;
	}
	result = (sybase_result *) zend_list_find(sybase_result_index->value.lval,&type);
	
	if (type!=php_sybase_module.le_result) {
		php_error(E_WARNING,"%d is not a Sybase result index",sybase_result_index->value.lval);
		RETURN_FALSE;
	}
	zend_list_delete(sybase_result_index->value.lval);
	RETURN_TRUE;
}
/* }}} */



/* {{{ proto string sybase_get_last_message(void)
   Returns the last message from server (over min_message_severity) */
PHP_FUNCTION(sybase_get_last_message)
{
	RETURN_STRING(php_sybase_module.server_message,1);
}
/* }}} */

/* {{{ proto int sybase_num_rows(int result)
   Get number of rows in result */
PHP_FUNCTION(sybase_num_rows)
{
	pval *result_index;
	int type,id;
	sybase_result *result;
	
	if (ZEND_NUM_ARGS()!=1 || getParameters(ht, 1, &result_index)==FAILURE) {
		WRONG_PARAM_COUNT;
	}
	
	convert_to_long(result_index);
	id = result_index->value.lval;
	
	result = (sybase_result *) zend_list_find(id,&type);
	if (type!=php_sybase_module.le_result) {
		php_error(E_WARNING,"%d is not a Sybase result index",id);
		RETURN_FALSE;
	}	
	
	return_value->value.lval = result->num_rows;
	return_value->type = IS_LONG;
}
/* }}} */

/* {{{ proto int sybase_num_fields(int result)
   Get number of fields in result */
PHP_FUNCTION(sybase_num_fields)
{
	pval *result_index;
	int type,id;
	sybase_result *result;
	
	if (ZEND_NUM_ARGS()!=1 || getParameters(ht, 1, &result_index)==FAILURE) {
		WRONG_PARAM_COUNT;
	}
	
	convert_to_long(result_index);
	id = result_index->value.lval;
	
	result = (sybase_result *) zend_list_find(id,&type);
	if (type!=php_sybase_module.le_result) {
		php_error(E_WARNING,"%d is not a Sybase result index",id);
		RETURN_FALSE;
	}	
	
	return_value->value.lval = result->num_fields;
	return_value->type = IS_LONG;
}
/* }}} */

/* {{{ proto array sybase_fetch_row(int result)
   Get row as enumerated array */
PHP_FUNCTION(sybase_fetch_row)
{
	pval *sybase_result_index;
	int type,i,id;
	sybase_result *result;
	pval *field_content;

	if (ZEND_NUM_ARGS()!=1 || getParameters(ht, 1, &sybase_result_index)==FAILURE) {
		WRONG_PARAM_COUNT;
	}
	
	convert_to_long(sybase_result_index);
	id = sybase_result_index->value.lval;
	
	result = (sybase_result *) zend_list_find(id,&type);
	if (type!=php_sybase_module.le_result) {
		php_error(E_WARNING,"%d is not a Sybase result index",id);
		RETURN_FALSE;
	}
	
	if (result->cur_row >= result->num_rows) {
		RETURN_FALSE;
	}
	
	array_init(return_value);
	for (i=0; i<result->num_fields; i++) {
		ZVAL_ADDREF(result->data[result->cur_row][i]);
		zend_hash_index_update(return_value->value.ht, i, (void *) &result->data[result->cur_row][i], sizeof(pval *), NULL);
	}
	result->cur_row++;
}
/* }}} */


static void php_sybase_fetch_hash(INTERNAL_FUNCTION_PARAMETERS)
{
	pval *sybase_result_index;
	sybase_result *result;
	int type;
	int i;
	
	if (ZEND_NUM_ARGS()!=1 || getParameters(ht, 1, &sybase_result_index)==FAILURE) {
		WRONG_PARAM_COUNT;
	}
	
	convert_to_long(sybase_result_index);
	result = (sybase_result *) zend_list_find(sybase_result_index->value.lval,&type);
	
	if (type!=php_sybase_module.le_result) {
		php_error(E_WARNING,"%d is not a Sybase result index",sybase_result_index->value.lval);
		RETURN_FALSE;
	}
	
	if (result->cur_row >= result->num_rows) {
		RETURN_FALSE;
	}
	
	if (array_init(return_value)==FAILURE) {
		RETURN_FALSE;
	}
	
	for (i=0; i<result->num_fields; i++) {
		ZVAL_ADDREF(result->data[result->cur_row][i]);
		zend_hash_index_update(return_value->value.ht, i, (void *) &result->data[result->cur_row][i], sizeof(pval *), NULL);
		ZVAL_ADDREF(result->data[result->cur_row][i]);
		zend_hash_update(return_value->value.ht, result->fields[i].name, strlen(result->fields[i].name)+1, (void *) &result->data[result->cur_row][i], sizeof(pval  *), NULL);
	}
	result->cur_row++;
}


/* {{{ proto object sybase_fetch_object(int result)
   Fetch row as object */
PHP_FUNCTION(sybase_fetch_object)
{
	php_sybase_fetch_hash(INTERNAL_FUNCTION_PARAM_PASSTHRU);
	if (return_value->type==IS_ARRAY) {
		return_value->type=IS_OBJECT;
		return_value->value.obj.properties = return_value->value.ht;
		return_value->value.obj.ce = &zend_standard_class_def;
	}
}
/* }}} */

/* {{{ proto array sybase_fetch_array(int result)
   Fetch row as array */
PHP_FUNCTION(sybase_fetch_array)
{
	php_sybase_fetch_hash(INTERNAL_FUNCTION_PARAM_PASSTHRU);
}
/* }}} */

/* {{{ proto bool sybase_data_seek(int result, int offset)
   Move internal row pointer */
PHP_FUNCTION(sybase_data_seek)
{
	pval *sybase_result_index,*offset;
	int type,id;
	sybase_result *result;

	if (ZEND_NUM_ARGS()!=2 || getParameters(ht, 2, &sybase_result_index, &offset)==FAILURE) {
		WRONG_PARAM_COUNT;
	}
	
	convert_to_long(sybase_result_index);
	id = sybase_result_index->value.lval;
	
	result = (sybase_result *) zend_list_find(id,&type);
	if (type!=php_sybase_module.le_result) {
		php_error(E_WARNING,"%d is not a Sybase result index",id);
		RETURN_FALSE;
	}

	convert_to_long(offset);
	if (offset->value.lval<0 || offset->value.lval>=result->num_rows) {
		php_error(E_WARNING,"Sybase:  Bad row offset");
		RETURN_FALSE;
	}
	
	result->cur_row = offset->value.lval;
	RETURN_TRUE;
}
/* }}} */

static char *php_sybase_get_field_name(int type)
{
	switch (type) {
		case SYBBINARY:
		case SYBVARBINARY:
			return "blob";
			break;
		case SYBCHAR:
		case SYBVARCHAR:
		case SYBTEXT:
			return "string";
		case SYBDATETIME:
		case SYBDATETIME4:
		case SYBDATETIMN:
			return "datetime";
			break;
		case SYBDECIMAL:
		case SYBFLT8:
		case SYBFLTN:
		case SYBREAL:
		case SYBNUMERIC:
			return "real";
			break;
		case SYBINT1:
		case SYBINT2:
		case SYBINT4:
		case SYBINTN:
			return "int";
			break;
		case SYBMONEY:
		case SYBMONEY4:
		case SYBMONEYN:
			return "money";
			break;
		case SYBBIT:
			return "bit";
			break;
		case SYBIMAGE:
			return "image";
			break;
		default:
			return "unknown";
			break;
	}
}


/* {{{ proto object sybase_fetch_field(int result [, int offset])
   Get field information */
PHP_FUNCTION(sybase_fetch_field)
{
	pval *sybase_result_index,*offset;
	int type,id,field_offset;
	sybase_result *result;

	switch (ZEND_NUM_ARGS()) {
		case 1:
			if (getParameters(ht, 1, &sybase_result_index)==FAILURE) {
				RETURN_FALSE;
			}
			field_offset=-1;
			break;
		case 2:
			if (getParameters(ht, 2, &sybase_result_index, &offset)==FAILURE) {
				RETURN_FALSE;
			}
			convert_to_long(offset);
			field_offset = offset->value.lval;
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}
	
	convert_to_long(sybase_result_index);
	id = sybase_result_index->value.lval;
	
	result = (sybase_result *) zend_list_find(id,&type);
	if (type!=php_sybase_module.le_result) {
		php_error(E_WARNING,"%d is not a Sybase result index",id);
		RETURN_FALSE;
	}
	
	if (field_offset==-1) {
		field_offset = result->cur_field;
		result->cur_field++;
	}
	
	if (field_offset<0 || field_offset >= result->num_fields) {
		if (ZEND_NUM_ARGS()==2) { /* field specified explicitly */
			php_error(E_WARNING,"Sybase:  Bad column offset");
		}
		RETURN_FALSE;
	}

	if (object_init(return_value)==FAILURE) {
		RETURN_FALSE;
	}
	add_property_string(return_value, "name",result->fields[field_offset].name, 1);
	add_property_long(return_value, "max_length",result->fields[field_offset].max_length);
	add_property_string(return_value, "column_source",result->fields[field_offset].column_source, 1);
	add_property_long(return_value, "numeric", result->fields[field_offset].numeric);
	add_property_string(return_value, "type", php_sybase_get_field_name(result->fields[field_offset].type), 1);
}
/* }}} */

/* {{{ proto bool sybase_field_seek(int result, int offset)
   Set field offset */
PHP_FUNCTION(sybase_field_seek)
{
	pval *sybase_result_index,*offset;
	int type,id,field_offset;
	sybase_result *result;

	if (ZEND_NUM_ARGS()!=2 || getParameters(ht, 2, &sybase_result_index, &offset)==FAILURE) {
		WRONG_PARAM_COUNT;
	}
	
	convert_to_long(sybase_result_index);
	id = sybase_result_index->value.lval;
	
	result = (sybase_result *) zend_list_find(id,&type);
	if (type!=php_sybase_module.le_result) {
		php_error(E_WARNING,"%d is not a Sybase result index",id);
		RETURN_FALSE;
	}
	
	convert_to_long(offset);
	field_offset = offset->value.lval;
	
	if (field_offset<0 || field_offset >= result->num_fields) {
		php_error(E_WARNING,"Sybase:  Bad column offset");
		RETURN_FALSE;
	}

	result->cur_field = field_offset;
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto string sybase_result(int result, int row, mixed field)
   Get result data */
PHP_FUNCTION(sybase_result)
{
	pval *row, *field, *sybase_result_index;
	int id,type,field_offset=0;
	sybase_result *result;
	

	if (ZEND_NUM_ARGS()!=3 || getParameters(ht, 3, &sybase_result_index, &row, &field)==FAILURE) {
		WRONG_PARAM_COUNT;
	}

	convert_to_long(sybase_result_index);
	id = sybase_result_index->value.lval;
	
	result = (sybase_result *) zend_list_find(id,&type);
	if (type!=php_sybase_module.le_result) {
		php_error(E_WARNING,"%d is not a Sybase result index",id);
		RETURN_FALSE;
	}
	
	convert_to_long(row);
	if (row->value.lval<0 || row->value.lval>=result->num_rows) {
		php_error(E_WARNING,"Sybase:  Bad row offset (%d)",row->value.lval);
		RETURN_FALSE;
	}

	switch(field->type) {
		case IS_STRING: {
			int i;

			for (i=0; i<result->num_fields; i++) {
				if (!strcasecmp(result->fields[i].name,field->value.str.val)) {
					field_offset = i;
					break;
				}
			}
			if (i>=result->num_fields) { /* no match found */
				php_error(E_WARNING,"Sybase:  %s field not found in result",field->value.str.val);
				RETURN_FALSE;
			}
			break;
		}
		default:
			convert_to_long(field);
			field_offset = field->value.lval;
			if (field_offset<0 || field_offset>=result->num_fields) {
				php_error(E_WARNING,"Sybase:  Bad column offset specified");
				RETURN_FALSE;
			}
			break;
	}

	*return_value = *result->data[row->value.lval][field_offset];
	pval_copy_constructor(return_value);
}
/* }}} */


/* {{{ proto int sybase_affected_rows([int link_id])
    Get number of affected rows in last query */
PHP_FUNCTION(sybase_affected_rows)
{
   pval *sybase_link_index = NULL;
   sybase_link *sybase_ptr = NULL;
   int id                  = 0;
   int type                = 0;

   switch(ZEND_NUM_ARGS())
   {
      case 0:
      {
         id = php_sybase_module.default_link;
      }
      break;

      case 1:
      {
         if (getParameters(ht, 1, &sybase_link_index)==FAILURE)
         {
            RETURN_FALSE;
         }

         convert_to_long(sybase_link_index);
         id = sybase_link_index->value.lval;
      }
      break;

      default:
      {
         WRONG_PARAM_COUNT;
      }
      break;
   }
	
   sybase_ptr = (sybase_link *)zend_list_find(id, &type);

   if(type!=php_sybase_module.le_link && type!=php_sybase_module.le_plink)
   {
      php_error(E_WARNING,"%d is not a Sybase link index",id);
      RETURN_FALSE;
   }

   return_value->value.lval = DBCOUNT(sybase_ptr->link);
   return_value->type       = IS_LONG;
}
 

PHP_MINFO_FUNCTION(sybase)
{
	char maxp[32],maxl[32];
	
	if (php_sybase_module.max_persistent==-1) {
		snprintf(maxp, 31, "%ld/unlimited", php_sybase_module.num_persistent );
	} else {
		snprintf(maxp, 31, "%ld/%ld", php_sybase_module.num_persistent, php_sybase_module.max_persistent);
	}
	maxp[31]=0;

	if (php_sybase_module.max_links==-1) {
		snprintf(maxl, 31, "%ld/unlimited", php_sybase_module.num_links );
	} else {
		snprintf(maxl, 31, "%ld/%ld", php_sybase_module.num_links, php_sybase_module.max_links);
	}
	maxl[31]=0;

	php_info_print_table_start();
	php_info_print_table_row(2, "Sybase Support", "enabled");
	php_info_print_table_row(2, "Allow Persistent Links", (php_sybase_module.allow_persistent?"Yes":"No") );
	php_info_print_table_row(2, "Persistent Links", maxp);
	php_info_print_table_row(2, "Total Links", maxl);
	php_info_print_table_row(2, "Application Name", php_sybase_module.appname );
	php_info_print_table_row(2, "Client API Version", dbversion() );
	php_info_print_table_end();

}


/* {{{ proto void sybase_min_error_severity(int severity)
   Sets the minimum error severity */
PHP_FUNCTION(sybase_min_error_severity)
{
	pval *severity;
	
	if (ZEND_NUM_ARGS()!=1 || getParameters(ht, 1, &severity)==FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_long(severity);
	php_sybase_module.min_error_severity = severity->value.lval;
}
/* }}} */

/* {{{ proto void sybase_min_message_severity(int severity)
   Sets the minimum message severity */
PHP_FUNCTION(sybase_min_message_severity)
{
	pval *severity;
	
	if (ZEND_NUM_ARGS()!=1 || getParameters(ht, 1, &severity)==FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_long(severity);
	php_sybase_module.min_message_severity = severity->value.lval;
}
/* }}} */

#endif
