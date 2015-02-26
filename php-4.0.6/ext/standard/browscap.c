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
 */

/* $Id: browscap.c,v 1.45 2001/04/30 12:43:39 andi Exp $ */

#include "php.h"
#include "php_regex.h"
#include "php_browscap.h"
#include "php_ini.h"

#include "zend_globals.h"

static HashTable browser_hash;
static zval *current_section;

#define DEFAULT_SECTION_NAME "Default Browser Capability Settings"


static void browscap_entry_dtor(zval *pvalue)
{
	if (pvalue->type == IS_OBJECT) {
		zend_hash_destroy(pvalue->value.obj.properties);
		free(pvalue->value.obj.properties);
	}
}


static void convert_browscap_pattern(zval *pattern)
{
	register int i,j;
	char *t;

	for (i=0; i<pattern->value.str.len; i++) {
		if (pattern->value.str.val[i]=='*' || pattern->value.str.val[i]=='?' || pattern->value.str.val[i]=='.') {
			break;
		}
	}

	if (i==pattern->value.str.len) { /* no wildcards */
		pattern->value.str.val = zend_strndup(pattern->value.str.val, pattern->value.str.len);
		return;
	}

	t = (char *) malloc(pattern->value.str.len*2);
	
	for (i=0,j=0; i<pattern->value.str.len; i++,j++) {
		switch (pattern->value.str.val[i]) {
			case '?':
				t[j] = '.';
				break;
			case '*':
				t[j++] = '.';
				t[j] = '*';
				break;
			case '.':
				t[j++] = '\\';
				t[j] = '.';
				break;
			default:
				t[j] = pattern->value.str.val[i];
				break;
		}
	}
	t[j]=0;
	pattern->value.str.val = t;
	pattern->value.str.len = j;
}


static void php_browscap_parser_cb(zval *arg1, zval *arg2, int callback_type, void *arg)
{
	switch (callback_type) {
		case ZEND_INI_PARSER_ENTRY:
			if (current_section) {
				zval *new_property;
				char *new_key;

				new_property = (zval *) malloc(sizeof(zval));
				INIT_PZVAL(new_property);
				new_property->value.str.val = Z_STRLEN_P(arg2)?zend_strndup(Z_STRVAL_P(arg2), Z_STRLEN_P(arg2)):"";
				new_property->value.str.len = Z_STRLEN_P(arg2);
				new_property->type = IS_STRING;
				
				new_key = zend_strndup(Z_STRVAL_P(arg1), Z_STRLEN_P(arg1));
				zend_str_tolower(new_key, Z_STRLEN_P(arg1));
				zend_hash_update(current_section->value.obj.properties, new_key, Z_STRLEN_P(arg1)+1, &new_property, sizeof(zval *), NULL);
				free(new_key);
			}
			break;
		case ZEND_INI_PARSER_SECTION: {
				zval *processed;

				/*printf("'%s' (%d)\n",$1.value.str.val,$1.value.str.len+1);*/
				current_section = (zval *) malloc(sizeof(zval));
				INIT_PZVAL(current_section);
				processed = (zval *) malloc(sizeof(zval));
				INIT_PZVAL(processed);

				current_section->value.obj.ce = &zend_standard_class_def;
				current_section->value.obj.properties = (HashTable *) malloc(sizeof(HashTable));
				current_section->type = IS_OBJECT;
				zend_hash_init(current_section->value.obj.properties, 0, NULL, (dtor_func_t) browscap_entry_dtor, 1);
				zend_hash_update(&browser_hash, Z_STRVAL_P(arg1), Z_STRLEN_P(arg1)+1, (void *) &current_section, sizeof(zval *), NULL);  

				processed->value.str.val = Z_STRVAL_P(arg1);
				processed->value.str.len = Z_STRLEN_P(arg1);
				processed->type = IS_STRING;
				convert_browscap_pattern(processed);
				zend_hash_update(current_section->value.obj.properties, "browser_name_pattern", sizeof("browser_name_pattern"), (void *) &processed, sizeof(zval *), NULL);
			}
			break;
	}
}


PHP_MINIT_FUNCTION(browscap)
{
	char *browscap = INI_STR("browscap");

	if (browscap) {
		zend_file_handle fh;

		if (zend_hash_init(&browser_hash, 0, NULL, (dtor_func_t) browscap_entry_dtor, 1)==FAILURE) {
			return FAILURE;
		}

		fh.handle.fp = VCWD_FOPEN(browscap, "r");
		if (!fh.handle.fp) {
			php_error(E_CORE_WARNING,"Cannot open '%s' for reading", browscap);
			return FAILURE;
		}
		fh.filename = browscap;
		fh.type = ZEND_HANDLE_FP;
		zend_parse_ini_file(&fh, 1, (zend_ini_parser_cb_t) php_browscap_parser_cb, &browser_hash);
	}

	return SUCCESS;
}


PHP_MSHUTDOWN_FUNCTION(browscap)
{
	if (INI_STR("browscap")) {
		zend_hash_destroy(&browser_hash);
	}
	return SUCCESS;
}


static int browser_reg_compare(zval **browser,int num_args, va_list args, zend_hash_key *key)
{
	zval **browser_name;
	regex_t r;
	char *lookup_browser_name = va_arg(args,char *);
	zval **found_browser_entry = va_arg(args,zval **);

	if (*found_browser_entry) { /* already found */
		return 0;
	}
	if(zend_hash_find((*browser)->value.obj.properties, "browser_name_pattern",sizeof("browser_name_pattern"),(void **) &browser_name) == FAILURE) {
		return 0;
	}

	if (!strchr((*browser_name)->value.str.val,'*')) {
		return 0;
	}
	if (regcomp(&r,(*browser_name)->value.str.val,REG_NOSUB)!=0) {
		return 0;
	}
	if (regexec(&r,lookup_browser_name,0,NULL,0)==0) {
		*found_browser_entry = *browser;
	}
	regfree(&r);
	return 0;
}

/* {{{ proto object get_browser(string browser_name)
   Get information about the capabilities of a browser */
PHP_FUNCTION(get_browser)
{
	zval **agent_name,**agent;
	zval *found_browser_entry,*tmp_copy;
	char *lookup_browser_name;
	PLS_FETCH();

	if (!INI_STR("browscap")) {
		RETURN_FALSE;
	}
	
	switch(ZEND_NUM_ARGS()) {
		case 0:
			if (!PG(http_globals)[TRACK_VARS_SERVER]
				|| zend_hash_find(PG(http_globals)[TRACK_VARS_SERVER]->value.ht, "HTTP_USER_AGENT", sizeof("HTTP_USER_AGENT"), (void **) &agent_name)==FAILURE) {
				zend_error(E_WARNING,"HTTP_USER_AGENT variable is not set, cannot determine user agent name");
				RETURN_FALSE;
			}
			break;
		case 1:
			if (zend_get_parameters_ex(1,&agent_name)==FAILURE) {
				RETURN_FALSE;
			}
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}
	
	convert_to_string_ex(agent_name);

	if (zend_hash_find(&browser_hash, (*agent_name)->value.str.val,(*agent_name)->value.str.len+1, (void **) &agent)==FAILURE) {
		lookup_browser_name = (*agent_name)->value.str.val;
		found_browser_entry = NULL;
		zend_hash_apply_with_arguments(&browser_hash,(int (*)(void *, int, va_list, zend_hash_key *)) browser_reg_compare,2,lookup_browser_name,&found_browser_entry);
		
		if (found_browser_entry) {
			agent = &found_browser_entry;
		} else if (zend_hash_find(&browser_hash, DEFAULT_SECTION_NAME, sizeof(DEFAULT_SECTION_NAME), (void **) &agent)==FAILURE) {
			RETURN_FALSE;
		}
	}
	
	object_init(return_value);
	zend_hash_copy(return_value->value.obj.properties,(*agent)->value.obj.properties, (copy_ctor_func_t) zval_add_ref, (void *) &tmp_copy, sizeof(zval *));
	
	while (zend_hash_find((*agent)->value.obj.properties, "parent",sizeof("parent"), (void **) &agent_name)==SUCCESS) {

		if (zend_hash_find(&browser_hash,(*agent_name)->value.str.val, (*agent_name)->value.str.len+1, (void **)&agent)==FAILURE) {
			break;
		}
		
		zend_hash_merge(return_value->value.obj.properties,(*agent)->value.obj.properties, (copy_ctor_func_t) zval_add_ref, (void *) &tmp_copy, sizeof(zval *), 0);
	}
}
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */
