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

#ifndef ZEND_DBX_H
#define ZEND_DBX_H

#ifndef INIT_FUNC_ARGS
#include "zend_modules.h"
#endif

#include "php.h"

#define DBX_PERSISTENT        1
#define DBX_RESULT_INFO      1
#define DBX_RESULT_INDEX    2
#define DBX_RESULT_ASSOC   4
#define DBX_CMP_TEXT          0
#define DBX_CMP_NUMBER      1

#define MOVE_RETURNED_TO_RV(rv, returned_zval) { **rv = *returned_zval; zval_copy_ctor(*rv); zval_ptr_dtor(&returned_zval); }

void dbx_call_any_function(INTERNAL_FUNCTION_PARAMETERS, char * function_name, zval ** returnvalue, int number_of_arguments, zval *** params);

#endif	/* ZEND_DBX_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */
