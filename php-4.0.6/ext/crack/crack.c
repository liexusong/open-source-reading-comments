#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_crack.h"

#if HAVE_CRACK

#include <packer.h>

ZEND_DECLARE_MODULE_GLOBALS(crack)

/* True global resources - no need for thread safety here */
static int le_crack;

function_entry crack_functions[] = {
	ZEND_FE(crack_opendict, NULL)
	ZEND_FE(crack_closedict, NULL)
	ZEND_FE(crack_check,	NULL)
	ZEND_FE(crack_getlastmessage, NULL)
	{NULL, NULL, NULL}
};

zend_module_entry crack_module_entry = {
	"crack",
	crack_functions,
	ZEND_MINIT(crack),
	ZEND_MSHUTDOWN(crack),
	ZEND_RINIT(crack),
	ZEND_RSHUTDOWN(crack),
	PHP_MINFO(crack),
	STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_CRACK
ZEND_GET_MODULE(crack)
#endif

PHP_INI_BEGIN()
	STD_PHP_INI_ENTRY("crack.default_dictionary", NULL, PHP_INI_SYSTEM, OnUpdateString, default_dictionary, zend_crack_globals, crack_globals)
PHP_INI_END()

long _crack_open_dict(char *dictpath)
{
	PWDICT *pwdict;
	long resource;

	CRACKLS_FETCH();

	if (CRACKG(current_id) != -1) {
		zend_error(E_WARNING, "Can not use more than one open dictionary with this implementation of libcrack");
		return -1;
	}
	if (NULL == (pwdict = PWOpen(dictpath, "r"))) {
		zend_error(E_WARNING, "Unable to open a crack dictionary");
		return -1;
	}

	resource = zend_list_insert(pwdict, le_crack);

/*	if (CRACKG(current_id) != -1) {
		zend_list_delete(CRACKG(current_id));
	}
*/
	CRACKG(current_id) = resource;
	
	return resource;
}

void _close_crack_dict(PWDICT *pwdict)
{
	PWClose(pwdict);
}

ZEND_MINIT_FUNCTION(crack)
{
	REGISTER_INI_ENTRIES();

	le_crack = register_list_destructors(_close_crack_dict, NULL);

	return SUCCESS;
}

ZEND_MSHUTDOWN_FUNCTION(crack)
{
	UNREGISTER_INI_ENTRIES();

	return SUCCESS;
}

ZEND_RINIT_FUNCTION(crack)
{
	CRACKLS_FETCH();

	CRACKG(last_message) = NULL;
	CRACKG(current_id) = -1;

	return SUCCESS;
}

ZEND_RSHUTDOWN_FUNCTION(crack)
{
	CRACKLS_FETCH();

	if (NULL != CRACKG(last_message)) {
		efree(CRACKG(last_message));
	}

	return SUCCESS;
}

PHP_MINFO_FUNCTION(crack)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "crack support", "enabled");
	php_info_print_table_end();

	DISPLAY_INI_ENTRIES();
}

/* {{{ proto string crack_opendict(string dictionary)
   Opens a new cracklib dictionary */
ZEND_FUNCTION(crack_opendict)
{
	zval **dictpath;
	long resource;

	if (ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &dictpath) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	convert_to_string_ex(dictpath);

	if (-1 == (resource = _crack_open_dict(Z_STRVAL_PP(dictpath)))) {
		RETURN_FALSE;
	}
	
	RETURN_RESOURCE(resource);
}
/* }}} */

/* {{{ proto string crack_closedict([int link_identifier])
   Closes an open cracklib dictionary. */
ZEND_FUNCTION(crack_closedict)
{
	PWDICT *pwdict;
	zval **dictionary;
	long id;

	CRACKLS_FETCH();

	switch (ZEND_NUM_ARGS()) {
		case 0:
			id = CRACKG(current_id);
			break;
		case 1:
			if (zend_get_parameters_ex(1, &dictionary) == FAILURE) {
				WRONG_PARAM_COUNT;
			}
			id = (*dictionary)->value.lval;
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}

	ZEND_FETCH_RESOURCE(pwdict, PWDICT *, dictionary, id, "cracklib dictionary", le_crack);
	if (CRACKG(current_id) == id) {
		CRACKG(current_id) = -1;
	}
	zend_list_delete(id);

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto string crack_check([int dictionary,] string password)
   Performs an obscure check with the given password */
ZEND_FUNCTION(crack_check)
{
	zval **dictionary = NULL, **password;
	char pwtrunced[STRINGSIZE];
	char *message;
	PWDICT *pwdict;
	long id;

	CRACKLS_FETCH();

	switch (ZEND_NUM_ARGS()) {
		case 1:
			if (zend_get_parameters_ex(1, &password) == FAILURE) {
				RETURN_FALSE;
			}
			if (NULL != CRACKG(default_dictionary) && CRACKG(current_id) == -1) {
				_crack_open_dict(CRACKG(default_dictionary));
			}
			id = CRACKG(current_id);
			break;
		case 2:
			if (zend_get_parameters_ex(2, &dictionary, &password) == FAILURE) {
				RETURN_FALSE;
			}
			id = -1;
			break;
		default:
			WRONG_PARAM_COUNT;
		break;
	}

	ZEND_FETCH_RESOURCE(pwdict, PWDICT *, dictionary, id, "cracklib dictionary", le_crack);
	convert_to_string_ex(password);

	/* Prevent buffer overflow attacks. */
	strlcpy(pwtrunced, Z_STRVAL_PP(password), sizeof(pwtrunced));

	message = (char *)FascistLook(pwdict, pwtrunced);

	if (NULL != CRACKG(last_message)) {
		efree(CRACKG(last_message));
	}

	if (NULL == message) {
		CRACKG(last_message) = estrdup("strong password");
		RETURN_TRUE;
	}
	
	CRACKG(last_message) = estrdup(message);
	RETURN_FALSE;
}
/* }}} */

/* {{{ proto string crack_getlastmessage()
   Returns the message from the last obscure check. */
ZEND_FUNCTION(crack_getlastmessage)
{
	CRACKLS_FETCH();

	if (ZEND_NUM_ARGS() != 0) {
		WRONG_PARAM_COUNT;
	}
	
	if (NULL == CRACKG(last_message)) {
		zend_error(E_WARNING, "No obscure checks in this session");
		RETURN_FALSE;
	}

	RETURN_STRING(CRACKG(last_message), 1);
}
/* }}} */

#endif	/* HAVE_CRACK */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */
