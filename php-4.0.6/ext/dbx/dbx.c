/*
   +----------------------------------------------------------------------+
   | PHP version 4.0                                                      |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2001 The PHP Group                                |
   +----------------------------------------------------------------------+
   | dbx module version 1.0                                               |
   +----------------------------------------------------------------------+
   | Copyright (c) 2001 Guidance Rotterdam BV                             |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.02 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available at through the world-wide-web at                           |
   | http://www.php.net/license/2_02.txt.                                 |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Author : Marc Boeren         <marc@guidance.nl>                      |
   +----------------------------------------------------------------------+
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
 
#include "php.h"
#include "php_ini.h"
#include "php_dbx.h"
#include "ext/standard/info.h"

/*/ defines for supported databases /*/
#define DBX_UNKNOWN 0
#define DBX_MYSQL 1
#define DBX_ODBC 2
#define DBX_PGSQL 3
#define DBX_MSSQL 4
/*/ includes for supported databases /*/
#include "dbx.h"
#include "dbx_mysql.h"
#include "dbx_odbc.h"
#include "dbx_pgsql.h"
#include "dbx_mssql.h"

/*/ support routines /*/
int module_exists(char * module_name) {
    zend_module_entry * zme;
    int r;
    r = zend_hash_find(&module_registry, module_name, strlen(module_name)+1, (void **) &zme);
    return r==0?1:0;
    }

int module_identifier_exists(long module_identifier) {
    switch (module_identifier) {
        case DBX_MYSQL: return module_exists("mysql");
        case DBX_ODBC: return module_exists("odbc");
        case DBX_PGSQL: return module_exists("pgsql");
        case DBX_MSSQL: return module_exists("mssql");
        }
    return 0;
    }

int get_module_identifier(char * module_name) {
    if (!strcmp("mysql", module_name)) return DBX_MYSQL;
    if (!strcmp("odbc", module_name)) return DBX_ODBC;
    if (!strcmp("pgsql", module_name)) return DBX_PGSQL;
    if (!strcmp("mssql", module_name)) return DBX_MSSQL;
    return DBX_UNKNOWN;
    }

int split_dbx_handle_object(zval ** dbx_object, zval *** pdbx_handle, zval *** pdbx_module, zval *** pdbx_database) {
    convert_to_object_ex(dbx_object);
    if (zend_hash_find((*dbx_object)->value.obj.properties, "handle", 7, (void **) pdbx_handle)==FAILURE
    || zend_hash_find((*dbx_object)->value.obj.properties, "module", 7, (void **) pdbx_module)==FAILURE
    || zend_hash_find((*dbx_object)->value.obj.properties, "database", 9, (void **) pdbx_database)==FAILURE) {
        return 0;
        }
    return 1;
    }

/*/ from dbx.h, to be used in support-files (dbx_mysql.c etc...) /*/
void dbx_call_any_function(INTERNAL_FUNCTION_PARAMETERS, char * function_name, zval ** returnvalue, int number_of_arguments, zval *** params) {
    zval * zval_function_name;
    MAKE_STD_ZVAL(zval_function_name);
    ZVAL_STRING(zval_function_name, function_name, 1);
    if (call_user_function_ex(EG(function_table), NULL, zval_function_name, returnvalue, number_of_arguments, params, 0, NULL) == FAILURE) {
        zend_error(E_ERROR, "function '%s' not found", zval_function_name->value.str.val);
        }
    zval_dtor(zval_function_name); /*/ to free stringvalue memory /*/
    FREE_ZVAL(zval_function_name);
    }

/*/ switch_dbx functions declarations
/ / each must be supported in the x/dbx_module files as dbx_module_function,
/ /   e.g. switch_dbx_connect expects a dbx_mysql_connect in de x/dbx_mysql files
/ /   all params except the dbx_module param are passed on
/ / each must return the expected zval * 's in the rv parameter, which are passed on unmodified
/ / do NOT use the return_value parameter from INTERNAL_FUNCTION_PARAMETERS
/ / you can additionally return 0 or 1 for failure or success which will also be returned by the switches
/*/
int switch_dbx_connect(zval ** rv, zval ** host, zval ** db, zval ** username, zval ** password, INTERNAL_FUNCTION_PARAMETERS, zval ** dbx_module);
    /*/ returns connection handle as resource on success or 0 as long on failure /*/
int switch_dbx_pconnect(zval ** rv, zval ** host, zval ** db, zval ** username, zval ** password, INTERNAL_FUNCTION_PARAMETERS, zval ** dbx_module);
    /*/ returns persistent connection handle as resource on success or 0 as long on failure /*/
int switch_dbx_close(zval ** rv, zval ** dbx_handle, INTERNAL_FUNCTION_PARAMETERS, zval ** dbx_module);
    /*/ returns 1 as long on success or 0 as long on failure /*/
int switch_dbx_query(zval ** rv, zval ** dbx_handle, zval ** db_name, zval ** sql_statement, INTERNAL_FUNCTION_PARAMETERS, zval ** dbx_module);
    /*/ returns 1 as long or result identifier as resource on success or 0 as long on failure /*/
int switch_dbx_getcolumncount(zval ** rv, zval ** result_handle, INTERNAL_FUNCTION_PARAMETERS, zval ** dbx_module);
    /*/ returns column-count as long on success or 0 as long on failure /*/
int switch_dbx_getcolumnname(zval ** rv, zval ** result_handle, long column_index, INTERNAL_FUNCTION_PARAMETERS, zval ** dbx_module);
    /*/ returns column-name as string on success or 0 as long on failure /*/
int switch_dbx_getcolumntype(zval ** rv, zval ** result_handle, long column_index, INTERNAL_FUNCTION_PARAMETERS, zval ** dbx_module);
    /*/ returns column-type as string on success or 0 as long on failure /*/
int switch_dbx_getrow(zval ** rv, zval ** result_handle, long row_number, INTERNAL_FUNCTION_PARAMETERS, zval ** dbx_module);
    /*/ returns array[0..columncount-1] as strings on success or 0 as long on failure /*/
int switch_dbx_error(zval ** rv, zval ** dbx_handle, INTERNAL_FUNCTION_PARAMETERS, zval ** dbx_module);
    /*/ returns string /*/

/* Every user visible function must have an entry in dbx_functions[].
*/
function_entry dbx_functions[] = {
    ZEND_FE(dbx_connect,	NULL)
    ZEND_FE(dbx_close,  	NULL)
    ZEND_FE(dbx_query,  	NULL)
    ZEND_FE(dbx_error,	    NULL)

    ZEND_FE(dbx_sort,    NULL)
    ZEND_FE(dbx_cmp_asc,    NULL)
    ZEND_FE(dbx_cmp_desc,    NULL)

	{NULL, NULL, NULL}	/* Must be the last line in dbx_functions[] */
    };

zend_module_entry dbx_module_entry = {
	"dbx",
    dbx_functions,
    ZEND_MINIT(dbx),
    ZEND_MSHUTDOWN(dbx),
    NULL, /*ZEND_RINIT(dbx),		/* Replace with NULL if there's nothing to do at request start */
	NULL, /*ZEND_RSHUTDOWN(dbx),	/* Replace with NULL if there's nothing to do at request end */
	ZEND_MINFO(dbx),
	STANDARD_MODULE_PROPERTIES
    };

#ifdef COMPILE_DL_DBX
ZEND_GET_MODULE(dbx)
#endif

ZEND_MINIT_FUNCTION(dbx)
{
/*/	REGISTER_INI_ENTRIES(); /*/

    REGISTER_LONG_CONSTANT("DBX_MYSQL", DBX_MYSQL, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DBX_ODBC", DBX_ODBC, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DBX_PGSQL", DBX_PGSQL, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DBX_MSSQL", DBX_MSSQL, CONST_CS | CONST_PERSISTENT);

    REGISTER_LONG_CONSTANT("DBX_PERSISTENT", DBX_PERSISTENT, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DBX_RESULT_INFO", DBX_RESULT_INFO, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DBX_RESULT_INDEX", DBX_RESULT_INDEX, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DBX_RESULT_ASSOC", DBX_RESULT_ASSOC, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DBX_CMP_TEXT", DBX_CMP_TEXT, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("DBX_CMP_NUMBER", DBX_CMP_NUMBER, CONST_CS | CONST_PERSISTENT);

    return SUCCESS;
    }

ZEND_MSHUTDOWN_FUNCTION(dbx)
{
/*/    UNREGISTER_INI_ENTRIES(); /*/
	return SUCCESS;
}
    
/* Remove if there's nothing to do at request start */
/*ZEND_RINIT_FUNCTION(dbx)
{	return SUCCESS;
}*/

/* Remove if there's nothing to do at request end */
/*ZEND_RSHUTDOWN_FUNCTION(dbx)
{   return SUCCESS;
}*/

ZEND_MINFO_FUNCTION(dbx)
{
    php_info_print_table_start();
    php_info_print_table_row(2, "dbx support", "enabled");
    php_info_print_table_row(2, "dbx support for MySQL", "enabled");
    php_info_print_table_row(2, "dbx support for ODBC", "enabled");
    php_info_print_table_row(2, "dbx support for PostgreSQL", "enabled");
    php_info_print_table_row(2, "dbx support for Microsoft SQL Server", "enabled");
    php_info_print_table_end();
/*/    DISPLAY_INI_ENTRIES(); /*/
}
/*/
/ /
/ / actual implementation of the dbx functions
/ /
/ /
/ /
/ /
/*/
/* {{{ proto dbx_handle_object dbx_connect(string module_name, string host, string db, string username, string password [, bool persistent])
   returns a dbx_handle_object on success
   returns 0 on failure
*/
ZEND_FUNCTION(dbx_connect)
{
    int number_of_arguments=5;
    zval ** arguments[6];

    int result;
    long module_identifier;
    zval * dbx_module;
    zval * db_name;
    zval * rv_dbx_handle;
    int persistent=0;

    if ( !(ZEND_NUM_ARGS()==number_of_arguments+1 || ZEND_NUM_ARGS()==number_of_arguments) || zend_get_parameters_array_ex(ZEND_NUM_ARGS(), arguments) == FAILURE) {
		WRONG_PARAM_COUNT;
    	}
    if (ZEND_NUM_ARGS()==number_of_arguments+1) {
        convert_to_long_ex(arguments[5]);
        if ((*arguments[5])->value.lval!=0) persistent=1;
        }

    if ((*arguments[0])->type == IS_LONG) {
        if (!module_identifier_exists((*arguments[0])->value.lval)) {
            zend_error(E_WARNING, "dbx: module '%ld' not loaded or not supported.\n", (*arguments[0])->value.lval);
            return;
            }
        module_identifier = (*arguments[0])->value.lval;
        }
    else {
        convert_to_string_ex(arguments[0]);
        if (!module_exists((*arguments[0])->value.str.val)) {
            zend_error(E_WARNING, "dbx: module '%s' not loaded.\n", (*arguments[0])->value.str.val);
            return;
            }
        module_identifier=get_module_identifier((*arguments[0])->value.str.val);
        if (!module_identifier) {
            zend_error(E_WARNING, "dbx: unsupported module '%s'.\n", (*arguments[0])->value.str.val);
            return;
            }
        }

    MAKE_STD_ZVAL(dbx_module); 
    ZVAL_LONG(dbx_module, module_identifier);
    MAKE_STD_ZVAL(rv_dbx_handle); 
    ZVAL_LONG(rv_dbx_handle, 0);
    convert_to_string_ex(arguments[1]);
    convert_to_string_ex(arguments[2]);
    convert_to_string_ex(arguments[3]);
    convert_to_string_ex(arguments[4]);
    MAKE_STD_ZVAL(db_name); 
    ZVAL_STRING(db_name, (*arguments[2])->value.str.val, 1);
    if (persistent) {
        result = switch_dbx_pconnect(&rv_dbx_handle, arguments[1], arguments[2], arguments[3], arguments[4], INTERNAL_FUNCTION_PARAM_PASSTHRU, &dbx_module);
        }
    else {
        result = switch_dbx_connect(&rv_dbx_handle, arguments[1], arguments[2], arguments[3], arguments[4], INTERNAL_FUNCTION_PARAM_PASSTHRU, &dbx_module);
        }
    if (!result) {
        FREE_ZVAL(dbx_module);
        FREE_ZVAL(db_name);
        FREE_ZVAL(rv_dbx_handle);
        RETURN_LONG(0);
        }

    if (object_init(return_value) != SUCCESS) {
        zend_error(E_ERROR, "dbx: unable to create resulting object...");
        FREE_ZVAL(dbx_module);
        FREE_ZVAL(db_name);
        FREE_ZVAL(rv_dbx_handle);
        RETURN_LONG(0);
        }

    zend_hash_update(return_value->value.obj.properties, "handle", 7, (void *)&(rv_dbx_handle), sizeof(zval *), NULL);
    zend_hash_update(return_value->value.obj.properties, "module", 7, (void *)&(dbx_module), sizeof(zval *), NULL);
    zend_hash_update(return_value->value.obj.properties, "database", 9, (void *)&(db_name), sizeof(zval *), NULL);
}
/* }}} */

/* {{{ proto bool dbx_close(dbx_handle_object dbx_handle)
   Returns success or failure */
ZEND_FUNCTION(dbx_close)
{
    int number_of_arguments=1;
    zval ** arguments[1];

    int result;
    zval ** dbx_handle;
    zval ** dbx_module;
    zval ** dbx_database;
    zval * rv_success;

    if (ZEND_NUM_ARGS() !=number_of_arguments || zend_get_parameters_array_ex(number_of_arguments, arguments) == FAILURE) {
		WRONG_PARAM_COUNT;
	    }
    if (!split_dbx_handle_object(arguments[0], &dbx_handle, &dbx_module, &dbx_database)) {
        zend_error(E_WARNING, "dbx_close: not a valid dbx_handle-object...");
        RETURN_LONG(0);
        }

    MAKE_STD_ZVAL(rv_success); 
    ZVAL_LONG(rv_success, 0);

    result = switch_dbx_close(&rv_success, dbx_handle, INTERNAL_FUNCTION_PARAM_PASSTHRU, dbx_module);

    result = (result && rv_success->value.lval)?1:0;

    FREE_ZVAL(rv_success);

    RETURN_LONG(result?1:0);
}
/* }}} */

/* {{{ proto data[rowinfo+rows][colinfo+cols] dbx_query(dbx_handle_object dbx_handle, string sql_statement [, long flags])
   Returns results combined with query-information or false for failure or true for success on execution query
   flags parameters is not implemented yet and if specified generates a WRONG_PARAM_COUNT 
   it will be used to indicate what column info should be returned, and if fieldnames should be used
   as assoc column indicators in the result-set */
ZEND_FUNCTION(dbx_query)
{
    int min_number_of_arguments=2;
    int number_of_arguments=3;
    zval ** arguments[3];

    int result;
    zval ** dbx_handle;
    zval ** dbx_module;
    zval ** dbx_database;
    zval * rv_result_handle;
    zval * rv_column_count;
    long col_index;
    long row_count;
    zval * info;
    long info_flags;
    zval * data;

    if (ZEND_NUM_ARGS()<min_number_of_arguments || ZEND_NUM_ARGS()>number_of_arguments || zend_get_parameters_array_ex(ZEND_NUM_ARGS(), arguments) == FAILURE) {
		WRONG_PARAM_COUNT;
	    }
    if (!split_dbx_handle_object(arguments[0], &dbx_handle, &dbx_module, &dbx_database)) {
        zend_error(E_WARNING, "dbx_query: not a valid dbx_handle-object...");
        RETURN_LONG(0);
        }
    /*/ default values /*/
    info_flags = DBX_RESULT_INFO | DBX_RESULT_INDEX | DBX_RESULT_ASSOC;
    /*/ parameter overrides /*/
    if (ZEND_NUM_ARGS()>2) {
        convert_to_long_ex(arguments[2]);
        info_flags = (*arguments[2])->value.lval;
        /*/ fieldnames are needed for association! /*/
        if (info_flags & DBX_RESULT_ASSOC) {
            info_flags |= DBX_RESULT_INFO;
            }
        }
    MAKE_STD_ZVAL(rv_result_handle); 
    ZVAL_LONG(rv_result_handle, 0);
    convert_to_string_ex(arguments[1]);
    result = switch_dbx_query(&rv_result_handle, dbx_handle, dbx_database, arguments[1], INTERNAL_FUNCTION_PARAM_PASSTHRU, dbx_module);
    /*/ boolean return value means either failure for any query or success for queries that don't return anything  /*/
    if (!result || (rv_result_handle && rv_result_handle->type==IS_BOOL)) {
        result = (result && rv_result_handle->value.lval)?1:0;
        FREE_ZVAL(rv_result_handle);
        RETURN_LONG(result?1:0);
        }
    /*/ if you get here, the query succeeded and returned results, so we'll return them
    / / rv_result_handle holds a resource
    /*/
    /*/ init return_value as object (of rows) /*/
    if (object_init(return_value) != SUCCESS) {
        zend_error(E_ERROR, "dbx_query: unable to create resulting object...");
        FREE_ZVAL(rv_result_handle);
        RETURN_LONG(0);
        }
    /*/ add result_handle property to return_value /*/
    zend_hash_update(return_value->value.obj.properties, "handle", 7, (void *)&(rv_result_handle), sizeof(zval *), NULL);
    /*/ init info property as array and add to return_value as a property /*/
    if (info_flags & DBX_RESULT_INFO) {
        MAKE_STD_ZVAL(info); 
        if (array_init(info) != SUCCESS) {
            zend_error(E_ERROR, "dbx_query: unable to create info-array for results...");
            FREE_ZVAL(info);
            RETURN_LONG(0);
            }
        zend_hash_update(return_value->value.obj.properties, "info", 5, (void *)&(info), sizeof(zval *), NULL);
        }
    /*/ init data property as array and add to return_value as a property /*/
    MAKE_STD_ZVAL(data); 
    if (array_init(data) != SUCCESS) {
        zend_error(E_ERROR, "dbx_query: unable to create data-array for results...");
        FREE_ZVAL(data);
        RETURN_LONG(0);
        }
    zend_hash_update(return_value->value.obj.properties, "data", 5, (void *)&(data), sizeof(zval *), NULL);
    /*/ get columncount and add to returnvalue as property /*/
    MAKE_STD_ZVAL(rv_column_count); 
    ZVAL_LONG(rv_column_count, 0);
    result = switch_dbx_getcolumncount(&rv_column_count, &rv_result_handle, INTERNAL_FUNCTION_PARAM_PASSTHRU, dbx_module);
    if (!result) { 
        zend_error(E_ERROR, "dbx_query: get column_count failed...");
        FREE_ZVAL(rv_column_count);
        RETURN_LONG(0); 
        }
    zend_hash_update(return_value->value.obj.properties, "cols", 5, (void *)&(rv_column_count), sizeof(zval *), NULL);
    /*/ fill the info array with columnnames (only indexed (maybe assoc)) /*/
    if (info_flags & DBX_RESULT_INFO) {
        zval * info_row;
        MAKE_STD_ZVAL(info_row);
        if (array_init(info_row) != SUCCESS) {
            zend_error(E_ERROR, "dbx_query: unable to create info_row-array for results...");
            FREE_ZVAL(info_row);
            RETURN_LONG(0);
            }
        for (col_index=0; col_index<rv_column_count->value.lval; ++col_index) {
            zval * rv_column_name;
            MAKE_STD_ZVAL(rv_column_name);
            ZVAL_LONG(rv_column_name, 0);
            result = switch_dbx_getcolumnname(&rv_column_name, &rv_result_handle, col_index, INTERNAL_FUNCTION_PARAM_PASSTHRU, dbx_module);
            if (result) { 
                zend_hash_index_update(info_row->value.ht, col_index, (void *)&(rv_column_name), sizeof(zval *), NULL);
                }
            else {
                FREE_ZVAL(rv_column_name);
                }
            }
        zend_hash_update(info->value.ht, "name", 5, (void *) &info_row, sizeof(zval *), NULL);
        }
    /*/ fill the info array with columntypes (indexed and assoc) /*/
    if (info_flags & DBX_RESULT_INFO) {
        zval * info_row;
        MAKE_STD_ZVAL(info_row);
        if (array_init(info_row) != SUCCESS) {
            zend_error(E_ERROR, "dbx_query: unable to create info_row-array for results...");
            FREE_ZVAL(info_row);
            RETURN_LONG(0);
            }
        for (col_index=0; col_index<rv_column_count->value.lval; ++col_index) {
            zval * rv_column_type;
            MAKE_STD_ZVAL(rv_column_type);
            ZVAL_LONG(rv_column_type, 0);
            result = switch_dbx_getcolumntype(&rv_column_type, &rv_result_handle, col_index, INTERNAL_FUNCTION_PARAM_PASSTHRU, dbx_module);
            if (result) { 
                zend_hash_index_update(info_row->value.ht, col_index, (void *)&(rv_column_type), sizeof(zval *), NULL);
                }
            else {
                FREE_ZVAL(rv_column_type);
                }
            }
        zend_hash_update(info->value.ht, "type", 5, (void *) &info_row, sizeof(zval *), NULL);
        }
    /*/ fill each row array with fieldvalues (indexed and assoc) /*/
    row_count=0;
    result=1;
    while (result) {
        zval * rv_row;
        MAKE_STD_ZVAL(rv_row);
        ZVAL_LONG(rv_row, 0);
        result = switch_dbx_getrow(&rv_row, &rv_result_handle, row_count, INTERNAL_FUNCTION_PARAM_PASSTHRU, dbx_module);
        if (result) {
                zval ** row_ptr;
                zend_hash_index_update(data->value.ht, row_count, (void *)&(rv_row), sizeof(zval *), (void **) &row_ptr);
                /*/ associate results with fieldnames /*/
                if (info_flags & DBX_RESULT_ASSOC) {
                    zval **columnname_ptr, **actual_ptr, **reference_ptr;
                    zval *dummy, **inforow_ptr;
                    ALLOC_ZVAL(dummy);
                    INIT_ZVAL(*dummy);	
                    zend_hash_find(info->value.ht, "name", 5, (void **) &inforow_ptr);
                    for (col_index=0; col_index<rv_column_count->value.lval; ++col_index) {
                        zend_hash_index_find((*inforow_ptr)->value.ht, col_index, (void **) &columnname_ptr);
                        zend_hash_index_find((*row_ptr)->value.ht, col_index, (void **) &actual_ptr);
                        zend_hash_update((*row_ptr)->value.ht, (*columnname_ptr)->value.str.val, (*columnname_ptr)->value.str.len + 1, &dummy, sizeof(zval *), (void **) &reference_ptr);
                        zend_assign_to_variable_reference(NULL, reference_ptr, actual_ptr, NULL ELS_CC);
                        }
                    }
            ++row_count;
            }
        else {
            FREE_ZVAL(rv_row);
            }
        }
    /*/ add row_count property /*/
    add_property_long(return_value, "rows", row_count);
    /*/ thank you for watching. /*/
}
/* }}} */

/* {{{ proto void dbx_error()
   Returns success or failure */
ZEND_FUNCTION(dbx_error)
{
    int number_of_arguments=1;
    zval ** arguments[1];

    int result;
    zval ** dbx_handle;
    zval ** dbx_module;
    zval ** dbx_database;
    zval * rv_errormsg;

    if (ZEND_NUM_ARGS() !=number_of_arguments || zend_get_parameters_array_ex(number_of_arguments, arguments) == FAILURE) {
		WRONG_PARAM_COUNT;
	    }
    if (!split_dbx_handle_object(arguments[0], &dbx_handle, &dbx_module, &dbx_database)) {
        zend_error(E_WARNING, "dbx_error: not a valid dbx_handle-object...");
        RETURN_LONG(0);
        }

    MAKE_STD_ZVAL(rv_errormsg); 
    ZVAL_LONG(rv_errormsg, 0);
    result = switch_dbx_error(&rv_errormsg, dbx_handle, INTERNAL_FUNCTION_PARAM_PASSTHRU, dbx_module);
    if (!result) {
        FREE_ZVAL(rv_errormsg);
        RETURN_STRING("", 1); 
        }
    MOVE_RETURNED_TO_RV(&return_value, rv_errormsg);
}
/* }}} */

/*/
/ /       dbx functions that are database independent... like sorting result_objects!
/*/
/* {{{ proto long dbx_cmp_asc(array row_x, array row_y, string columnname)
   returns row_x[columnname] - row_y[columnname], converted to -1, 0 or 1
*/
ZEND_FUNCTION(dbx_cmp_asc)
{
    int min_number_of_arguments=3;
    int max_number_of_arguments=4;
    int number_of_arguments=-1;
    long comparison_type;
    double dtemp;
    long ltemp;
    zval ** arguments[4];
    zval ** zv_a;
    zval ** zv_b;
    int result=0;
    number_of_arguments=ZEND_NUM_ARGS();
    if (number_of_arguments<min_number_of_arguments || number_of_arguments>max_number_of_arguments || zend_get_parameters_array_ex(number_of_arguments, arguments) == FAILURE) {
		WRONG_PARAM_COUNT;
	    }

    if ((*arguments[0])->type != IS_ARRAY
    || (*arguments[1])->type != IS_ARRAY) {
        zend_error(E_WARNING, "Wrong argument type for compare");
        RETURN_LONG(0);
        }
    convert_to_string_ex(arguments[2]); /*/ field name /*/
    comparison_type = DBX_CMP_TEXT;
    if (number_of_arguments>3) {
        convert_to_long_ex(arguments[3]); /*/ comparison type /*/
        comparison_type=(*arguments[3])->value.lval;
        }

    if (zend_hash_find((*arguments[0])->value.ht, (*arguments[2])->value.str.val, (*arguments[2])->value.str.len+1, (void **) &zv_a)==FAILURE
    || zend_hash_find((*arguments[1])->value.ht, (*arguments[2])->value.str.val, (*arguments[2])->value.str.len+1, (void **) &zv_b)==FAILURE)  {
        zend_error(E_WARNING, "Field '%s' not available in result-object", (*arguments[2])->value.str.val);
        RETURN_LONG(0);
        }

    switch (comparison_type) {
        case DBX_CMP_TEXT:
            convert_to_string_ex(zv_a);
            convert_to_string_ex(zv_b);
            ltemp = strcmp((*zv_a)->value.str.val, (*zv_b)->value.str.val);
            result = (ltemp==0?0: (ltemp>0?1:-1));
            break;
        case DBX_CMP_NUMBER:
            convert_to_double_ex(zv_a);
            convert_to_double_ex(zv_b);
            dtemp = ((*zv_a)->value.dval - (*zv_b)->value.dval);
            result = (dtemp==0?0: (dtemp>0?1:-1));
            break;
        default:
            result=0;
            break;
        }

    RETURN_LONG(result);
}

/* {{{ proto long dbx_cmp_desc(array row_x, array row_y, string columnname)
   returns row_y[columnname] - row_x[columnname], converted to -1, 0 or 1
*/
ZEND_FUNCTION(dbx_cmp_desc)
{
    int min_number_of_arguments=3;
    int max_number_of_arguments=4;
    int number_of_arguments=-1;
    long comparison_type;
    double dtemp;
    long ltemp;
    zval ** arguments[4];
    zval ** zv_a;
    zval ** zv_b;
    int result=0;
    number_of_arguments=ZEND_NUM_ARGS();
    if (number_of_arguments<min_number_of_arguments || number_of_arguments>max_number_of_arguments || zend_get_parameters_array_ex(number_of_arguments, arguments) == FAILURE) {
		WRONG_PARAM_COUNT;
	    }

    if ((*arguments[0])->type != IS_ARRAY
    || (*arguments[1])->type != IS_ARRAY) {
        zend_error(E_WARNING, "Wrong argument type for compare");
        RETURN_LONG(0);
        }
    convert_to_string_ex(arguments[2]); /*/ field name /*/
    comparison_type = DBX_CMP_TEXT;
    if (number_of_arguments>3) {
        convert_to_long_ex(arguments[3]); /*/ comparison type /*/
        comparison_type=(*arguments[3])->value.lval;
        }

    if (zend_hash_find((*arguments[0])->value.ht, (*arguments[2])->value.str.val, (*arguments[2])->value.str.len+1, (void **) &zv_a)==FAILURE
    || zend_hash_find((*arguments[1])->value.ht, (*arguments[2])->value.str.val, (*arguments[2])->value.str.len+1, (void **) &zv_b)==FAILURE)  {
        zend_error(E_WARNING, "Field '%s' not available in result-object", (*arguments[2])->value.str.val);
        RETURN_LONG(0);
        }

    switch (comparison_type) {
        case DBX_CMP_TEXT:
            convert_to_string_ex(zv_a);
            convert_to_string_ex(zv_b);
            ltemp = strcmp((*zv_b)->value.str.val, (*zv_a)->value.str.val);
            result = (ltemp==0?0: (ltemp>0?1:-1));
            break;
        case DBX_CMP_NUMBER:
            convert_to_double_ex(zv_a);
            convert_to_double_ex(zv_b);
            dtemp = ((*zv_b)->value.dval - (*zv_a)->value.dval);
            result = (dtemp==0?0: (dtemp>0?1:-1));
            break;
        default:
            result=0;
            break;
        }

    RETURN_LONG(result);
}

/* {{{ proto long dbx_sort(dbx_result_object stn_search_keywords_result, string compare_function_name)
   returns 0 on failure, 1 on success
*/
ZEND_FUNCTION(dbx_sort)
{
    int number_of_arguments=2;
    zval ** arguments[2];
    zval ** zval_data;
    zval * returned_zval;
    int result=0;
    if (ZEND_NUM_ARGS() !=number_of_arguments || zend_get_parameters_array_ex(number_of_arguments, arguments) == FAILURE) {
		WRONG_PARAM_COUNT;
	    }

    if ((*arguments[0])->type != IS_OBJECT
    || (*arguments[1])->type != IS_STRING) {
        zend_error(E_WARNING, "Wrong argument type for sort");
        RETURN_LONG(0);
        }

    if (zend_hash_find((*arguments[0])->value.obj.properties, "data", 5, (void **) &zval_data)==FAILURE
    || (*zval_data)->type != IS_ARRAY) {
        zend_error(E_WARNING, "Wrong argument type for sort");
        RETURN_LONG(0);
        }

    arguments[0] = zval_data;
    dbx_call_any_function(INTERNAL_FUNCTION_PARAM_PASSTHRU, "usort", &returned_zval, number_of_arguments, arguments);
    zval_dtor(returned_zval);
    FREE_ZVAL(returned_zval);

    RETURN_LONG(1);
}

/***********************************/

/*/
/ / switch_dbx functions
/*/
int switch_dbx_connect(zval ** rv, zval ** host, zval ** db, zval ** username, zval ** password, INTERNAL_FUNCTION_PARAMETERS, zval ** dbx_module) {
    /*/ returns connection handle as resource on success or 0 as long on failure /*/
    switch ((*dbx_module)->value.lval) {
        case DBX_MYSQL: return dbx_mysql_connect(rv, host, db, username, password, INTERNAL_FUNCTION_PARAM_PASSTHRU);        
        case DBX_ODBC: return dbx_odbc_connect(rv, host, db, username, password, INTERNAL_FUNCTION_PARAM_PASSTHRU);        
        case DBX_PGSQL: return dbx_pgsql_connect(rv, host, db, username, password, INTERNAL_FUNCTION_PARAM_PASSTHRU);        
        case DBX_MSSQL: return dbx_mssql_connect(rv, host, db, username, password, INTERNAL_FUNCTION_PARAM_PASSTHRU);        
        }
    zend_error(E_WARNING, "dbx_connect: not supported in this module");
    return 0;
    }

int switch_dbx_pconnect(zval ** rv, zval ** host, zval ** db, zval ** username, zval ** password, INTERNAL_FUNCTION_PARAMETERS, zval ** dbx_module) {
    /*/ returns persistent connection handle as resource on success or 0 as long on failure /*/
    switch ((*dbx_module)->value.lval) {
        case DBX_MYSQL: return dbx_mysql_pconnect(rv, host, db, username, password, INTERNAL_FUNCTION_PARAM_PASSTHRU);        
        case DBX_ODBC: return dbx_odbc_pconnect(rv, host, db, username, password, INTERNAL_FUNCTION_PARAM_PASSTHRU);        
        case DBX_PGSQL: return dbx_pgsql_pconnect(rv, host, db, username, password, INTERNAL_FUNCTION_PARAM_PASSTHRU);        
        case DBX_MSSQL: return dbx_mssql_pconnect(rv, host, db, username, password, INTERNAL_FUNCTION_PARAM_PASSTHRU);        
        }
    zend_error(E_WARNING, "dbx_pconnect: not supported in this module");
    return 0;
    }

int switch_dbx_close(zval ** rv, zval ** dbx_handle, INTERNAL_FUNCTION_PARAMETERS, zval ** dbx_module) {
    /*/ returns 1 as long on success or 0 as long on failure /*/
    switch ((*dbx_module)->value.lval) {
        case DBX_MYSQL: return dbx_mysql_close(rv, dbx_handle, INTERNAL_FUNCTION_PARAM_PASSTHRU);        
        case DBX_ODBC: return dbx_odbc_close(rv, dbx_handle, INTERNAL_FUNCTION_PARAM_PASSTHRU);        
        case DBX_PGSQL: return dbx_pgsql_close(rv, dbx_handle, INTERNAL_FUNCTION_PARAM_PASSTHRU);        
        case DBX_MSSQL: return dbx_mssql_close(rv, dbx_handle, INTERNAL_FUNCTION_PARAM_PASSTHRU);        
        }
    zend_error(E_WARNING, "dbx_close: not supported in this module");
    return 0;
    }

int switch_dbx_query(zval ** rv, zval ** dbx_handle, zval ** db_name, zval ** sql_statement, INTERNAL_FUNCTION_PARAMETERS, zval ** dbx_module) {
    /*/ returns 1 as long or result identifier as resource on success or 0 as long on failure /*/
    switch ((*dbx_module)->value.lval) {
        case DBX_MYSQL: return dbx_mysql_query(rv, dbx_handle, db_name, sql_statement, INTERNAL_FUNCTION_PARAM_PASSTHRU);        
        case DBX_ODBC: return dbx_odbc_query(rv, dbx_handle, db_name, sql_statement, INTERNAL_FUNCTION_PARAM_PASSTHRU);        
        case DBX_PGSQL: return dbx_pgsql_query(rv, dbx_handle, db_name, sql_statement, INTERNAL_FUNCTION_PARAM_PASSTHRU);        
        case DBX_MSSQL: return dbx_mssql_query(rv, dbx_handle, db_name, sql_statement, INTERNAL_FUNCTION_PARAM_PASSTHRU);        
        }
    zend_error(E_WARNING, "dbx_query: not supported in this module");
    return 0;
    }

int switch_dbx_getcolumncount(zval ** rv, zval ** result_handle, INTERNAL_FUNCTION_PARAMETERS, zval ** dbx_module) {
    /*/ returns column-count as long on success or 0 as long on failure /*/
    switch ((*dbx_module)->value.lval) {
        case DBX_MYSQL: return dbx_mysql_getcolumncount(rv, result_handle, INTERNAL_FUNCTION_PARAM_PASSTHRU);        
        case DBX_ODBC: return dbx_odbc_getcolumncount(rv, result_handle, INTERNAL_FUNCTION_PARAM_PASSTHRU);        
        case DBX_PGSQL: return dbx_pgsql_getcolumncount(rv, result_handle, INTERNAL_FUNCTION_PARAM_PASSTHRU);        
        case DBX_MSSQL: return dbx_mssql_getcolumncount(rv, result_handle, INTERNAL_FUNCTION_PARAM_PASSTHRU);        
        }
    zend_error(E_WARNING, "dbx_getcolumncount: not supported in this module");
    return 0;
    }

int switch_dbx_getcolumnname(zval ** rv, zval ** result_handle, long column_index, INTERNAL_FUNCTION_PARAMETERS, zval ** dbx_module) {
    /*/ returns column-name as string on success or 0 as long on failure /*/
    switch ((*dbx_module)->value.lval) {
        case DBX_MYSQL: return dbx_mysql_getcolumnname(rv, result_handle, column_index, INTERNAL_FUNCTION_PARAM_PASSTHRU);        
        case DBX_ODBC: return dbx_odbc_getcolumnname(rv, result_handle, column_index, INTERNAL_FUNCTION_PARAM_PASSTHRU);        
        case DBX_PGSQL: return dbx_pgsql_getcolumnname(rv, result_handle, column_index, INTERNAL_FUNCTION_PARAM_PASSTHRU);        
        case DBX_MSSQL: return dbx_mssql_getcolumnname(rv, result_handle, column_index, INTERNAL_FUNCTION_PARAM_PASSTHRU);        
        }
    zend_error(E_WARNING, "dbx_getcolumnname: not supported in this module");
    return 0;
    }

int switch_dbx_getcolumntype(zval ** rv, zval ** result_handle, long column_index, INTERNAL_FUNCTION_PARAMETERS, zval ** dbx_module) {
    /*/ returns column-type as string on success or 0 as long on failure /*/
    switch ((*dbx_module)->value.lval) {
        case DBX_MYSQL: return dbx_mysql_getcolumntype(rv, result_handle, column_index, INTERNAL_FUNCTION_PARAM_PASSTHRU);        
        case DBX_ODBC: return dbx_odbc_getcolumntype(rv, result_handle, column_index, INTERNAL_FUNCTION_PARAM_PASSTHRU);        
        case DBX_PGSQL: return dbx_pgsql_getcolumntype(rv, result_handle, column_index, INTERNAL_FUNCTION_PARAM_PASSTHRU);        
        case DBX_MSSQL: return dbx_mssql_getcolumntype(rv, result_handle, column_index, INTERNAL_FUNCTION_PARAM_PASSTHRU);        
       }
    zend_error(E_WARNING, "dbx_getcolumntype: not supported in this module");
    return 0;
    }

int switch_dbx_getrow(zval ** rv, zval ** result_handle, long row_number, INTERNAL_FUNCTION_PARAMETERS, zval ** dbx_module) {
    /*/ returns array[0..columncount-1] as strings on success or 0 as long on failure /*/
    switch ((*dbx_module)->value.lval) {
        case DBX_MYSQL: return dbx_mysql_getrow(rv, result_handle, row_number, INTERNAL_FUNCTION_PARAM_PASSTHRU);        
        case DBX_ODBC: return dbx_odbc_getrow(rv, result_handle, row_number, INTERNAL_FUNCTION_PARAM_PASSTHRU);        
        case DBX_PGSQL: return dbx_pgsql_getrow(rv, result_handle, row_number, INTERNAL_FUNCTION_PARAM_PASSTHRU);        
        case DBX_MSSQL: return dbx_mssql_getrow(rv, result_handle, row_number, INTERNAL_FUNCTION_PARAM_PASSTHRU);        
        }
    zend_error(E_WARNING, "dbx_getrow: not supported in this module");
    return 0;
    }

int switch_dbx_error(zval ** rv, zval ** dbx_handle, INTERNAL_FUNCTION_PARAMETERS, zval ** dbx_module) {
    /*/ returns string /*/
    switch ((*dbx_module)->value.lval) {
        case DBX_MYSQL: return dbx_mysql_error(rv, dbx_handle, INTERNAL_FUNCTION_PARAM_PASSTHRU);        
        case DBX_ODBC: return dbx_odbc_error(rv, dbx_handle, INTERNAL_FUNCTION_PARAM_PASSTHRU);        
        case DBX_PGSQL: return dbx_pgsql_error(rv, dbx_handle, INTERNAL_FUNCTION_PARAM_PASSTHRU);        
        case DBX_MSSQL: return dbx_mssql_error(rv, dbx_handle, INTERNAL_FUNCTION_PARAM_PASSTHRU);        
        }
    zend_error(E_WARNING, "dbx_error: not supported in this module");
    return 0;
    }

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */
