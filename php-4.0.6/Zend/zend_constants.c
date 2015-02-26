/*
   +----------------------------------------------------------------------+
   | Zend Engine                                                          |
   +----------------------------------------------------------------------+
   | Copyright (c) 1998-2001 Zend Technologies Ltd. (http://www.zend.com) |
   +----------------------------------------------------------------------+
   | This source file is subject to version 0.92 of the Zend license,     |
   | that is bundled with this package in the file LICENSE, and is        | 
   | available at through the world-wide-web at                           |
   | http://www.zend.com/license/0_92.txt.                                |
   | If you did not receive a copy of the Zend license and are unable to  |
   | obtain it through the world-wide-web, please send a note to          |
   | license@zend.com so we can mail you a copy immediately.              |
   +----------------------------------------------------------------------+
   | Authors: Andi Gutmans <andi@zend.com>                                |
   |          Zeev Suraski <zeev@zend.com>                                |
   +----------------------------------------------------------------------+
*/


#include "zend.h"
#include "zend_constants.h"
#include "zend_variables.h"
#include "zend_operators.h"
#include "zend_globals.h"


void free_zend_constant(zend_constant *c)
{
	if (!(c->flags & CONST_PERSISTENT)) {
		zval_dtor(&c->value);
	}
	free(c->name);
}


void copy_zend_constant(zend_constant *c)
{
	c->name = zend_strndup(c->name, c->name_len);
	if (!(c->flags & CONST_PERSISTENT)) {
		zval_copy_ctor(&c->value);
	}
}


void zend_copy_constants(HashTable *target, HashTable *source)
{
	zend_constant tmp_constant;

	zend_hash_copy(target, source, (copy_ctor_func_t) copy_zend_constant, &tmp_constant, sizeof(zend_constant));
}


static int clean_non_persistent_constant(zend_constant *c)
{
	if (c->flags & CONST_PERSISTENT) {
		return 0;
	} else {
		return 1;
	}
}


static int clean_module_constant(zend_constant *c, int *module_number)
{
	if (c->module_number == *module_number) {
		return 1;
	} else {
		return 0;
	}
}


void clean_module_constants(int module_number)
{
	ELS_FETCH();

	zend_hash_apply_with_argument(EG(zend_constants), (int (*)(void *,void *)) clean_module_constant, (void *) &module_number);
}


int zend_startup_constants(ELS_D)
{
#ifdef ZEND_WIN32
	DWORD dwBuild=0;
	DWORD dwVersion = GetVersion();
	DWORD dwWindowsMajorVersion =  (DWORD)(LOBYTE(LOWORD(dwVersion)));
	DWORD dwWindowsMinorVersion =  (DWORD)(HIBYTE(LOWORD(dwVersion)));
#endif

	EG(zend_constants) = (HashTable *) malloc(sizeof(HashTable));

	if (zend_hash_init(EG(zend_constants), 20, NULL, ZEND_CONSTANT_DTOR, 1)==FAILURE) {
		return FAILURE;
	}
	return SUCCESS;
}



void zend_register_standard_constants(ELS_D)
{
	REGISTER_MAIN_LONG_CONSTANT("E_ERROR", E_ERROR, CONST_PERSISTENT | CONST_CS);
	REGISTER_MAIN_LONG_CONSTANT("E_WARNING", E_WARNING, CONST_PERSISTENT | CONST_CS);
	REGISTER_MAIN_LONG_CONSTANT("E_PARSE", E_PARSE, CONST_PERSISTENT | CONST_CS);
	REGISTER_MAIN_LONG_CONSTANT("E_NOTICE", E_NOTICE, CONST_PERSISTENT | CONST_CS);
	REGISTER_MAIN_LONG_CONSTANT("E_CORE_ERROR", E_CORE_ERROR, CONST_PERSISTENT | CONST_CS);
	REGISTER_MAIN_LONG_CONSTANT("E_CORE_WARNING", E_CORE_WARNING, CONST_PERSISTENT | CONST_CS);
	REGISTER_MAIN_LONG_CONSTANT("E_COMPILE_ERROR", E_COMPILE_ERROR, CONST_PERSISTENT | CONST_CS);
	REGISTER_MAIN_LONG_CONSTANT("E_COMPILE_WARNING", E_COMPILE_WARNING, CONST_PERSISTENT | CONST_CS);
	REGISTER_MAIN_LONG_CONSTANT("E_USER_ERROR", E_USER_ERROR, CONST_PERSISTENT | CONST_CS);
	REGISTER_MAIN_LONG_CONSTANT("E_USER_WARNING", E_USER_WARNING, CONST_PERSISTENT | CONST_CS);
	REGISTER_MAIN_LONG_CONSTANT("E_USER_NOTICE", E_USER_NOTICE, CONST_PERSISTENT | CONST_CS);

	REGISTER_MAIN_LONG_CONSTANT("E_ALL", E_ALL, CONST_PERSISTENT | CONST_CS);

	/* true/false constants */
	{
		zend_constant c;
	
		c.value.type = IS_BOOL;
		c.flags = CONST_PERSISTENT;
		c.module_number = 0;

		c.name = zend_strndup(ZEND_STRL("TRUE"));
		c.name_len = sizeof("TRUE");
		c.value.value.lval = 1;
		c.value.type = IS_BOOL;
		zend_register_constant(&c ELS_CC);
		
		c.name = zend_strndup(ZEND_STRL("FALSE"));
		c.name_len = sizeof("FALSE");
		c.value.value.lval = 0;
		c.value.type = IS_BOOL;
		zend_register_constant(&c ELS_CC);

		c.name = zend_strndup(ZEND_STRL("ZEND_THREAD_SAFE"));
		c.name_len = sizeof("ZEND_THREAD_SAFE");
		c.value.value.lval = ZTS_V;
		c.value.type = IS_BOOL;
		zend_register_constant(&c ELS_CC);

		c.name = zend_strndup(ZEND_STRL("NULL"));
		c.name_len = sizeof("NULL");
		c.value.type = IS_NULL;
		zend_register_constant(&c ELS_CC);
	}
}


int zend_shutdown_constants(ELS_D)
{
	zend_hash_destroy(EG(zend_constants));
	free(EG(zend_constants));
	return SUCCESS;
}


void clean_non_persistent_constants(void)
{
	ELS_FETCH();

	zend_hash_apply(EG(zend_constants), (int (*)(void *)) clean_non_persistent_constant);
}


ZEND_API void zend_register_long_constant(char *name, uint name_len, long lval, int flags, int module_number ELS_DC)
{
	zend_constant c;
	
	c.value.type = IS_LONG;
	c.value.value.lval = lval;
	c.flags = flags;
	c.name = zend_strndup(name,name_len);
	c.name_len = name_len;
	c.module_number = module_number;
	zend_register_constant(&c ELS_CC);
}


ZEND_API void zend_register_double_constant(char *name, uint name_len, double dval, int flags, int module_number ELS_DC)
{
	zend_constant c;
	
	c.value.type = IS_DOUBLE;
	c.value.value.dval = dval;
	c.flags = flags;
	c.name = zend_strndup(name,name_len);
	c.name_len = name_len;
	c.module_number = module_number;
	zend_register_constant(&c ELS_CC);
}


ZEND_API void zend_register_stringl_constant(char *name, uint name_len, char *strval, uint strlen, int flags, int module_number ELS_DC)
{
	zend_constant c;
	
	c.value.type = IS_STRING;
	c.value.value.str.val = strval;
	c.value.value.str.len = strlen;
	c.flags = flags;
	c.name = zend_strndup(name,name_len);
	c.name_len = name_len;
	c.module_number = module_number;
	zend_register_constant(&c ELS_CC);
}


ZEND_API void zend_register_string_constant(char *name, uint name_len, char *strval, int flags, int module_number ELS_DC)
{
	zend_register_stringl_constant(name, name_len, strval, strlen(strval), flags, module_number ELS_CC);
}


ZEND_API int zend_get_constant(char *name, uint name_len, zval *result)
{
	zend_constant *c;
	char *lookup_name = estrndup(name,name_len);
	int retval;
	ELS_FETCH();

	zend_str_tolower(lookup_name, name_len);

	if (zend_hash_find(EG(zend_constants), lookup_name, name_len+1, (void **) &c)==SUCCESS) {
		if ((c->flags & CONST_CS) && memcmp(c->name, name, name_len)!=0) {
			retval=0;
		} else {
			retval=1;
			*result = c->value;
			zval_copy_ctor(result);
		}
	} else {
		retval=0;
	}
	
	efree(lookup_name);
	return retval;
}


ZEND_API int zend_register_constant(zend_constant *c ELS_DC)
{
	char *lowercase_name = zend_strndup(c->name, c->name_len);
	int ret = SUCCESS;

#if 0
	printf("Registering constant for module %d\n",c->module_number);
#endif

	zend_str_tolower(lowercase_name, c->name_len);
	if (zend_hash_add(EG(zend_constants), lowercase_name, c->name_len, (void *) c, sizeof(zend_constant), NULL)==FAILURE) {
		zval_dtor(&c->value);
		zend_error(E_NOTICE,"Constant %s already defined",lowercase_name);
		ret = FAILURE;
	}
	free(lowercase_name);
	return ret;
}


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */
