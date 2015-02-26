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

#include "dbx.h"
#include "dbx_odbc.h"

#define ODBC_ASSOC	1
#define ODBC_NUM	 2 

int dbx_odbc_connect(zval ** rv, zval ** host, zval ** db, zval ** username, zval ** password, INTERNAL_FUNCTION_PARAMETERS) {
    /*/ returns connection handle as resource on success or 0 as long on failure /*/
    int number_of_arguments=3;
    zval ** arguments[3];
    zval * returned_zval=NULL;

    arguments[0]=db;
    arguments[1]=username;
    arguments[2]=password;
    dbx_call_any_function(INTERNAL_FUNCTION_PARAM_PASSTHRU, "odbc_connect", &returned_zval, number_of_arguments, arguments);
    if (!returned_zval || returned_zval->type!=IS_RESOURCE) {
        if (returned_zval) zval_ptr_dtor(&returned_zval);
        return 0;
        }
    MOVE_RETURNED_TO_RV(rv, returned_zval);
    return 1;
    }

int dbx_odbc_pconnect(zval ** rv, zval ** host, zval ** db, zval ** username, zval ** password, INTERNAL_FUNCTION_PARAMETERS) {
    /*/ returns connection handle as resource on success or 0 as long on failure /*/
    int number_of_arguments=3;
    zval ** arguments[3];
    zval * returned_zval=NULL;

    arguments[0]=db;
    arguments[1]=username;
    arguments[2]=password;
    dbx_call_any_function(INTERNAL_FUNCTION_PARAM_PASSTHRU, "odbc_pconnect", &returned_zval, number_of_arguments, arguments);
    if (!returned_zval || returned_zval->type!=IS_RESOURCE) {
        if (returned_zval) zval_ptr_dtor(&returned_zval);
        return 0;
        }
    MOVE_RETURNED_TO_RV(rv, returned_zval);
    return 1;
    }

int dbx_odbc_close(zval ** rv, zval ** dbx_handle, INTERNAL_FUNCTION_PARAMETERS) {
    /*/ returns 1 as long on success or 0 as long on failure /*/
    int number_of_arguments=1;
    zval ** arguments[1];
    zval * returned_zval=NULL;

    arguments[0]=dbx_handle;
    dbx_call_any_function(INTERNAL_FUNCTION_PARAM_PASSTHRU, "odbc_close", &returned_zval, number_of_arguments, arguments);
    if (!returned_zval || returned_zval->type!=IS_BOOL) {
        if (returned_zval) zval_ptr_dtor(&returned_zval);
        return 0;
        }
    MOVE_RETURNED_TO_RV(rv, returned_zval);
    return 1;
    }

int dbx_odbc_query(zval ** rv, zval ** dbx_handle, zval ** db_name, zval ** sql_statement, INTERNAL_FUNCTION_PARAMETERS) {
    /*/ returns 1 as long or a result identifier as resource on success  or 0 as long on failure /*/
    int number_of_arguments=2;
    zval ** arguments[2];
    zval * queryresult_zval=NULL;
    zval * num_fields_zval=NULL;

    // db_name is not used in this function
    arguments[0]=dbx_handle;
    arguments[1]=sql_statement;
    dbx_call_any_function(INTERNAL_FUNCTION_PARAM_PASSTHRU, "odbc_exec", &queryresult_zval, number_of_arguments, arguments);
    /*/ odbc_query returns a bool for failure, or a result_identifier for success /*/
    if (!queryresult_zval || queryresult_zval->type!=IS_RESOURCE) {
        if (queryresult_zval) zval_ptr_dtor(&queryresult_zval);
        return 0;
        }
    MAKE_STD_ZVAL(num_fields_zval);
    ZVAL_LONG(num_fields_zval, 0);
    if (!dbx_odbc_getcolumncount(&num_fields_zval, &queryresult_zval, INTERNAL_FUNCTION_PARAM_PASSTHRU)) {
        FREE_ZVAL(num_fields_zval);
        if (queryresult_zval) zval_ptr_dtor(&queryresult_zval);
        return 0;
        }
    if (num_fields_zval->value.lval==0) {
        (*rv)->type=IS_BOOL;
        (*rv)->value.lval=1; /*/ success, but no data /*/
        FREE_ZVAL(num_fields_zval);
        if (queryresult_zval) zval_ptr_dtor(&queryresult_zval);
        return 1;
        }
    FREE_ZVAL(num_fields_zval);
    MOVE_RETURNED_TO_RV(rv, queryresult_zval);
    return 1;
    }

int dbx_odbc_getcolumncount(zval ** rv, zval ** result_handle, INTERNAL_FUNCTION_PARAMETERS) {
    /*/ returns column-count as long on success or 0 as long on failure /*/
    int number_of_arguments=1;
    zval ** arguments[1];
    zval * returned_zval=NULL;

    arguments[0]=result_handle;
    dbx_call_any_function(INTERNAL_FUNCTION_PARAM_PASSTHRU, "odbc_num_fields", &returned_zval, number_of_arguments, arguments);
    if (!returned_zval || returned_zval->type!=IS_LONG || returned_zval->value.lval<0) {
        if (returned_zval) zval_ptr_dtor(&returned_zval);
        return 0;
        }
    MOVE_RETURNED_TO_RV(rv, returned_zval);
    return 1;
    }

int dbx_odbc_getcolumnname(zval ** rv, zval ** result_handle, long column_index, INTERNAL_FUNCTION_PARAMETERS) {
    /*/ returns column-name as string on success or 0 as long on failure /*/
    int number_of_arguments=2;
    zval ** arguments[2];
    zval * zval_column_index;
    zval * returned_zval=NULL;

    MAKE_STD_ZVAL(zval_column_index);
    ZVAL_LONG(zval_column_index, column_index+1);
    arguments[0]=result_handle;
    arguments[1]=&zval_column_index;
    dbx_call_any_function(INTERNAL_FUNCTION_PARAM_PASSTHRU, "odbc_field_name", &returned_zval, number_of_arguments, arguments);
    /*/ odbc_field_name returns a string /*/
    if (!returned_zval || returned_zval->type!=IS_STRING) {
        if (returned_zval) zval_ptr_dtor(&returned_zval);
        FREE_ZVAL(zval_column_index);
        return 0;
        }
    FREE_ZVAL(zval_column_index);
    MOVE_RETURNED_TO_RV(rv, returned_zval);
    return 1;
    }

int dbx_odbc_getcolumntype(zval ** rv, zval ** result_handle, long column_index, INTERNAL_FUNCTION_PARAMETERS) {
    /*/ returns column-type as string on success or 0 as long on failure /*/
    int number_of_arguments=2;
    zval ** arguments[2];
    zval * zval_column_index;
    zval * returned_zval=NULL;

    MAKE_STD_ZVAL(zval_column_index);
    ZVAL_LONG(zval_column_index, column_index+1);
    arguments[0]=result_handle;
    arguments[1]=&zval_column_index;
    dbx_call_any_function(INTERNAL_FUNCTION_PARAM_PASSTHRU, "odbc_field_type", &returned_zval, number_of_arguments, arguments);
    /*/ odbc_field_name returns a string /*/
    if (!returned_zval || returned_zval->type!=IS_STRING) {
        if (returned_zval) zval_ptr_dtor(&returned_zval);
        FREE_ZVAL(zval_column_index);
        return 0;
        }
    FREE_ZVAL(zval_column_index);
    MOVE_RETURNED_TO_RV(rv, returned_zval);
    return 1;
    }

int dbx_odbc_getrow(zval ** rv, zval ** result_handle, long row_number, INTERNAL_FUNCTION_PARAMETERS) {
    /*/ returns array[0..columncount-1] as strings on success or 0 as long on failure /*/
    int number_of_arguments;
    zval ** arguments[2];
    zval * num_fields_zval=NULL;
    zval * fetch_row_result_zval=NULL;
    zval * field_result_zval=NULL;
    zval * field_index_zval;
    zval * returned_zval=NULL;
    long field_index;
    long field_count=-1;

    /*/ get # fields /*/
    MAKE_STD_ZVAL(num_fields_zval);
    ZVAL_LONG(num_fields_zval, 0);
    if (!dbx_odbc_getcolumncount(&num_fields_zval, result_handle, INTERNAL_FUNCTION_PARAM_PASSTHRU)) {
        return 0;
        }
    field_count=num_fields_zval->value.lval;
    FREE_ZVAL(num_fields_zval);
    /*/ fetch row /*/
    number_of_arguments=1;
    arguments[0]=result_handle;
    dbx_call_any_function(INTERNAL_FUNCTION_PARAM_PASSTHRU, "odbc_fetch_row", &fetch_row_result_zval, number_of_arguments, arguments);
    if (!fetch_row_result_zval || fetch_row_result_zval->type!=IS_BOOL) {
        if (fetch_row_result_zval) zval_ptr_dtor(&fetch_row_result_zval);
        return 0;
        }
    if (fetch_row_result_zval->value.lval==0) {
        (*rv)->type=IS_LONG;
        (*rv)->value.lval=0; /*/ ok, no more rows /*/
        zval_ptr_dtor(&fetch_row_result_zval);
        return 0;
        }
    zval_ptr_dtor(&fetch_row_result_zval);
    /*/ fill array with field results... /*/
    MAKE_STD_ZVAL(returned_zval);
    if (array_init(returned_zval) != SUCCESS) {
        zend_error(E_ERROR, "dbx_odbc_getrow: unable to create result-array...");
        FREE_ZVAL(returned_zval);
        return 0;
        }
    MAKE_STD_ZVAL(field_index_zval);
    ZVAL_LONG(field_index_zval, 0);
    number_of_arguments=2;
    for (field_index=0; field_index<field_count; ++field_index) {
        ZVAL_LONG(field_index_zval, field_index+1);
        arguments[0]=result_handle;
        arguments[1]=&field_index_zval;
        dbx_call_any_function(INTERNAL_FUNCTION_PARAM_PASSTHRU, "odbc_result", &field_result_zval, number_of_arguments, arguments);
        zend_hash_index_update(returned_zval->value.ht, field_index, (void *)&(field_result_zval), sizeof(zval *), NULL);
        }
    FREE_ZVAL(field_index_zval);

    MOVE_RETURNED_TO_RV(rv, returned_zval);
    return 1;
    }

int dbx_odbc_error(zval ** rv, zval ** dbx_handle, INTERNAL_FUNCTION_PARAMETERS) {
    /*/ returns empty string, no equivalent in odbc module (yet???) /*/
    ZVAL_EMPTY_STRING((*rv));
    return 1;
    }

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */
