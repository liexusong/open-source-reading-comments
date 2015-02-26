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
#include "dbx_mysql.h"

#define MYSQL_ASSOC		1<<0
#define MYSQL_NUM		1<<1

int dbx_mysql_connect(zval ** rv, zval ** host, zval ** db, zval ** username, zval ** password, INTERNAL_FUNCTION_PARAMETERS) {
    /*/ returns connection handle as resource on success or 0 as long on failure /*/
    int number_of_arguments=3;
    zval ** arguments[3];
    zval * returned_zval=NULL;
    zval * select_db_zval=NULL;

    arguments[0]=host;
    arguments[1]=username;
    arguments[2]=password;
    dbx_call_any_function(INTERNAL_FUNCTION_PARAM_PASSTHRU, "mysql_connect", &returned_zval, number_of_arguments, arguments);
    if (!returned_zval || returned_zval->type!=IS_RESOURCE) {
        if (returned_zval) zval_ptr_dtor(&returned_zval);
        return 0;
        }
    MOVE_RETURNED_TO_RV(rv, returned_zval);

    number_of_arguments=2;
    arguments[0]=db;
    arguments[1]=rv;
    dbx_call_any_function(INTERNAL_FUNCTION_PARAM_PASSTHRU, "mysql_select_db", &select_db_zval, number_of_arguments, arguments);
    zval_ptr_dtor(&select_db_zval);

    return 1;
    }

int dbx_mysql_pconnect(zval ** rv, zval ** host, zval ** db, zval ** username, zval ** password, INTERNAL_FUNCTION_PARAMETERS) {
    /*/ returns persistent connection handle as resource on success or 0 as long on failure /*/
    int number_of_arguments=3;
    zval ** arguments[3];
    zval * returned_zval=NULL;
    zval * select_db_zval=NULL;

    arguments[0]=host;
    arguments[1]=username;
    arguments[2]=password;
    dbx_call_any_function(INTERNAL_FUNCTION_PARAM_PASSTHRU, "mysql_pconnect", &returned_zval, number_of_arguments, arguments);
    if (!returned_zval || returned_zval->type!=IS_RESOURCE) {
        if (returned_zval) zval_ptr_dtor(&returned_zval);
        return 0;
        }
    MOVE_RETURNED_TO_RV(rv, returned_zval);

    number_of_arguments=2;
    arguments[0]=db;
    arguments[1]=rv;
    dbx_call_any_function(INTERNAL_FUNCTION_PARAM_PASSTHRU, "mysql_select_db", &select_db_zval, number_of_arguments, arguments);
    zval_ptr_dtor(&select_db_zval);

    return 1;
    }

int dbx_mysql_close(zval ** rv, zval ** dbx_handle, INTERNAL_FUNCTION_PARAMETERS) {
    /*/ returns 1 as long on success or 0 as long on failure /*/
    int number_of_arguments=1;
    zval ** arguments[1];
    zval * returned_zval=NULL;

    arguments[0]=dbx_handle;
    dbx_call_any_function(INTERNAL_FUNCTION_PARAM_PASSTHRU, "mysql_close", &returned_zval, number_of_arguments, arguments);
    if (!returned_zval || returned_zval->type!=IS_BOOL) {
        if (returned_zval) zval_ptr_dtor(&returned_zval);
        return 0;
        }
    MOVE_RETURNED_TO_RV(rv, returned_zval);
    return 1;
    }

int dbx_mysql_query(zval ** rv, zval ** dbx_handle, zval ** db_name, zval ** sql_statement, INTERNAL_FUNCTION_PARAMETERS) {
    /*/ returns 1 as long or a result identifier as resource on success  or 0 as long on failure /*/
    int number_of_arguments=3;
    zval ** arguments[3];
    zval * returned_zval=NULL;

    arguments[0]=db_name;
    arguments[1]=sql_statement;
    arguments[2]=dbx_handle;
    dbx_call_any_function(INTERNAL_FUNCTION_PARAM_PASSTHRU, "mysql_db_query", &returned_zval, number_of_arguments, arguments);
    /*/ mysql_query returns a bool for success or failure, or a result_identifier for select statements /*/
    if (!returned_zval || (returned_zval->type!=IS_BOOL && returned_zval->type!=IS_RESOURCE)) {
        if (returned_zval) zval_ptr_dtor(&returned_zval);
        return 0;
        }
    MOVE_RETURNED_TO_RV(rv, returned_zval);
    return 1;
    }

int dbx_mysql_getcolumncount(zval ** rv, zval ** result_handle, INTERNAL_FUNCTION_PARAMETERS) {
    /*/ returns column-count as long on success or 0 as long on failure /*/
    int number_of_arguments=1;
    zval ** arguments[1];
    zval * returned_zval=NULL;

    arguments[0]=result_handle;
    dbx_call_any_function(INTERNAL_FUNCTION_PARAM_PASSTHRU, "mysql_num_fields", &returned_zval, number_of_arguments, arguments);
    if (!returned_zval || returned_zval->type!=IS_LONG) {
        if (returned_zval) zval_ptr_dtor(&returned_zval);
        return 0;
        }
    MOVE_RETURNED_TO_RV(rv, returned_zval);
    return 1;
    }

int dbx_mysql_getcolumnname(zval ** rv, zval ** result_handle, long column_index, INTERNAL_FUNCTION_PARAMETERS) {
    /*/ returns column-name as string on success or 0 as long on failure /*/
    int number_of_arguments=2;
    zval ** arguments[2];
    zval * zval_column_index;
    zval * returned_zval=NULL;

    MAKE_STD_ZVAL(zval_column_index);
    ZVAL_LONG(zval_column_index, column_index);
    arguments[0]=result_handle;
    arguments[1]=&zval_column_index;
    dbx_call_any_function(INTERNAL_FUNCTION_PARAM_PASSTHRU, "mysql_field_name", &returned_zval, number_of_arguments, arguments);
    /*/ mysql_field_name returns a string /*/
    if (!returned_zval || returned_zval->type!=IS_STRING) {
        if (returned_zval) zval_ptr_dtor(&returned_zval);
        FREE_ZVAL(zval_column_index);
        return 0;
        }
    FREE_ZVAL(zval_column_index);
    MOVE_RETURNED_TO_RV(rv, returned_zval);
    return 1;
    }

int dbx_mysql_getcolumntype(zval ** rv, zval ** result_handle, long column_index, INTERNAL_FUNCTION_PARAMETERS) {
    /*/ returns column-type as string on success or 0 as long on failure /*/
    int number_of_arguments=2;
    zval ** arguments[2];
    zval * zval_column_index;
    zval * returned_zval=NULL;

    MAKE_STD_ZVAL(zval_column_index);
    ZVAL_LONG(zval_column_index, column_index);
    arguments[0]=result_handle;
    arguments[1]=&zval_column_index;
    dbx_call_any_function(INTERNAL_FUNCTION_PARAM_PASSTHRU, "mysql_field_type", &returned_zval, number_of_arguments, arguments);
    /*/ mysql_field_name returns a string /*/
    if (!returned_zval || returned_zval->type!=IS_STRING) {
        if (returned_zval) zval_ptr_dtor(&returned_zval);
        FREE_ZVAL(zval_column_index);
        return 0;
        }
    FREE_ZVAL(zval_column_index);
    MOVE_RETURNED_TO_RV(rv, returned_zval);
    return 1;
    }

int dbx_mysql_getrow(zval ** rv, zval ** result_handle, long row_number, INTERNAL_FUNCTION_PARAMETERS) {
    /*/ returns array[0..columncount-1] as strings on success or 0 as long on failure /*/
    int number_of_arguments=2;
    zval ** arguments[2];
    zval * zval_resulttype=NULL;
    zval * returned_zval=NULL;

    MAKE_STD_ZVAL(zval_resulttype);
    ZVAL_LONG(zval_resulttype, MYSQL_NUM);
    arguments[0]=result_handle;
    arguments[1]=&zval_resulttype;
    dbx_call_any_function(INTERNAL_FUNCTION_PARAM_PASSTHRU, "mysql_fetch_array", &returned_zval, number_of_arguments, arguments);
    if (!returned_zval || returned_zval->type!=IS_ARRAY) {
        if (returned_zval) zval_ptr_dtor(&returned_zval);
        FREE_ZVAL(zval_resulttype);
        return 0;
        }
    FREE_ZVAL(zval_resulttype);
    MOVE_RETURNED_TO_RV(rv, returned_zval);
    return 1;
    }

int dbx_mysql_error(zval ** rv, zval ** dbx_handle, INTERNAL_FUNCTION_PARAMETERS) {
    /*/ returns string /*/
    int number_of_arguments=1;
    zval ** arguments[1];
    zval * returned_zval=NULL;

    arguments[0]=dbx_handle;
    if (!dbx_handle) number_of_arguments=0;
    dbx_call_any_function(INTERNAL_FUNCTION_PARAM_PASSTHRU, "mysql_error", &returned_zval, number_of_arguments, arguments);
    if (!returned_zval || returned_zval->type!=IS_STRING) {
        if (returned_zval) zval_ptr_dtor(&returned_zval);
        return 0;
        }
    MOVE_RETURNED_TO_RV(rv, returned_zval);
    return 1;
    }

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */
