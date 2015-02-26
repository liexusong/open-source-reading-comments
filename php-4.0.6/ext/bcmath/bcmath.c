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
   | Authors: Andi Gutmans <andi@zend.com>                                |
   +----------------------------------------------------------------------+
*/

/* $Id: bcmath.c,v 1.27.4.1 2001/05/24 12:41:36 ssb Exp $ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"

#if WITH_BCMATH

#include "ext/standard/info.h"
#include "php_bcmath.h"
#include "ext/bcmath/libbcmath/src/bcmath.h"

function_entry bcmath_functions[] = {
	PHP_FE(bcadd,									NULL)
	PHP_FE(bcsub,									NULL)
	PHP_FE(bcmul,									NULL)
	PHP_FE(bcdiv,									NULL)
	PHP_FE(bcmod,									NULL)
	PHP_FE(bcpow,									NULL)
	PHP_FE(bcsqrt,									NULL)
	PHP_FE(bcscale,									NULL)
	PHP_FE(bccomp,									NULL)
	{NULL, NULL, NULL}
};

zend_module_entry bcmath_module_entry = {
	"bcmath",
    bcmath_functions,
	PHP_MINIT(bcmath),
	PHP_MSHUTDOWN(bcmath),
	PHP_RINIT(bcmath),
	NULL,
	PHP_MINFO(bcmath),
	STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_BCMATH
ZEND_GET_MODULE(bcmath)
#endif

#ifndef THREAD_SAFE
static long bc_precision;
#endif

/* Storage used for special numbers. */
bc_num _zero_;
bc_num _one_;
bc_num _two_;


/* Make a copy of a number!  Just increments the reference count! */
bc_num copy_num (bc_num num)
{
  num->n_refs++;
  return num;
}


/* Initialize a number NUM by making it a copy of zero. */
void init_num (bc_num *num)
{
	*num = copy_num (_zero_);
}


PHP_MINIT_FUNCTION(bcmath)
{
	extern bc_num _zero_;
	extern bc_num _one_;
	extern bc_num _two_;

	_zero_ = bc_new_num (1,0);
	_one_  = bc_new_num (1,0);
	_one_->n_value[0] = 1;
	_two_  = bc_new_num (1,0);
	_two_->n_value[0] = 2;
	persist_alloc(_zero_);
	persist_alloc(_one_);
	persist_alloc(_two_);
	persist_alloc(_zero_->n_ptr);
	persist_alloc(_one_->n_ptr);
	persist_alloc(_two_->n_ptr);

	return SUCCESS;
}



PHP_MSHUTDOWN_FUNCTION(bcmath)
{
	bc_free_num(&_zero_);
	bc_free_num(&_one_);
	bc_free_num(&_two_);
	return SUCCESS;
}


PHP_RINIT_FUNCTION(bcmath)
{
	if (cfg_get_long("bcmath.scale",&bc_precision)==FAILURE) {
		bc_precision=0;
	}
	return SUCCESS;
}


PHP_MINFO_FUNCTION(bcmath)
{
  php_info_print_table_start();
  php_info_print_table_row(2, "BCMath support", "enabled");
  php_info_print_table_end();
}

/* {{{ proto string bcadd(string left_operand, string right_operand [, int scale])
   Returns the sum of two arbitrary precision numbers */
PHP_FUNCTION(bcadd)
{
	pval **left, **right,**scale_param;
	bc_num first, second, result;
	int scale=bc_precision;

	switch (ZEND_NUM_ARGS()) {
		case 2:
				if (zend_get_parameters_ex(2, &left,&right) == FAILURE) {
		        	WRONG_PARAM_COUNT;
    			}
				break;
		case 3:
				if (zend_get_parameters_ex(3, &left,&right, &scale_param) == FAILURE) {
		        	WRONG_PARAM_COUNT;
    			}
				convert_to_long_ex(scale_param);
				scale = (int) (*scale_param)->value.lval;
				break;
		default:
				WRONG_PARAM_COUNT;
				break;
	}
	convert_to_string_ex(left);
	convert_to_string_ex(right);
	bc_init_num(&first);
	bc_init_num(&second);
	bc_init_num(&result);
	bc_str2num(&first,(*left)->value.str.val,scale);
	bc_str2num(&second,(*right)->value.str.val,scale);
	bc_add (first,second,&result, scale);
	return_value->value.str.val = bc_num2str(result);
	return_value->value.str.len = strlen(return_value->value.str.val);
	return_value->type = IS_STRING;
	bc_free_num(&first);
	bc_free_num(&second);
	bc_free_num(&result);
	return;
}
/* }}} */

/* {{{ proto string bcsub(string left_operand, string right_operand [, int scale])
   Returns the difference between two arbitrary precision numbers */
PHP_FUNCTION(bcsub)
{
	pval **left, **right,**scale_param;
	bc_num first, second, result;
	int scale=bc_precision;

	switch (ZEND_NUM_ARGS()) {
		case 2:
				if (zend_get_parameters_ex(2, &left,&right) == FAILURE) {
		        	WRONG_PARAM_COUNT;
    			}
				break;
		case 3:
				if (zend_get_parameters_ex(3, &left,&right, &scale_param) == FAILURE) {
		        	WRONG_PARAM_COUNT;
    			}
				convert_to_long_ex(scale_param);
				scale = (int) (*scale_param)->value.lval;
				break;
		default:
				WRONG_PARAM_COUNT;
				break;
	}
	convert_to_string_ex(left);
	convert_to_string_ex(right);
	bc_init_num(&first);
	bc_init_num(&second);
	bc_init_num(&result);
	bc_str2num(&first,(*left)->value.str.val,scale);
	bc_str2num(&second,(*right)->value.str.val,scale);
	bc_sub (first,second,&result, scale);
	return_value->value.str.val = bc_num2str(result);
	return_value->value.str.len = strlen(return_value->value.str.val);
	return_value->type = IS_STRING;
	bc_free_num(&first);
	bc_free_num(&second);
	bc_free_num(&result);
	return;
}
/* }}} */

/* {{{ proto string bcmul(string left_operand, string right_operand [, int scale])
   Returns the multiplication of two arbitrary precision numbers */
PHP_FUNCTION(bcmul)
{
	pval **left, **right,**scale_param;
	bc_num first, second, result;
	int scale=bc_precision;

	switch (ZEND_NUM_ARGS()) {
		case 2:
				if (zend_get_parameters_ex(2, &left,&right) == FAILURE) {
		        	WRONG_PARAM_COUNT;
    			}
				break;
		case 3:
				if (zend_get_parameters_ex(3, &left,&right, &scale_param) == FAILURE) {
		        	WRONG_PARAM_COUNT;
    			}
				convert_to_long_ex(scale_param);
				scale = (int) (*scale_param)->value.lval;
				break;
		default:
				WRONG_PARAM_COUNT;
				break;
	}
	convert_to_string_ex(left);
	convert_to_string_ex(right);
	bc_init_num(&first);
	bc_init_num(&second);
	bc_init_num(&result);
	bc_str2num(&first,(*left)->value.str.val,scale);
	bc_str2num(&second,(*right)->value.str.val,scale);
	bc_multiply (first,second,&result, scale);
	return_value->value.str.val = bc_num2str(result);
	return_value->value.str.len = strlen(return_value->value.str.val);
	return_value->type = IS_STRING;
	bc_free_num(&first);
	bc_free_num(&second);
	bc_free_num(&result);
	return;
}
/* }}} */

/* {{{ proto string bcdiv(string left_operand, string right_operand [, int scale])
   Returns the quotient of two arbitrary precision numbers (division) */
PHP_FUNCTION(bcdiv)
{
	pval **left, **right,**scale_param;
	bc_num first, second, result;
	int scale=bc_precision;

	switch (ZEND_NUM_ARGS()) {
		case 2:
				if (zend_get_parameters_ex(2, &left,&right) == FAILURE) {
		        	WRONG_PARAM_COUNT;
    			}
				break;
		case 3:
				if (zend_get_parameters_ex(3, &left,&right, &scale_param) == FAILURE) {
		        	WRONG_PARAM_COUNT;
    			}
				convert_to_long_ex(scale_param);
				scale = (int) (*scale_param)->value.lval;
				break;
		default:
				WRONG_PARAM_COUNT;
				break;
	}
	convert_to_string_ex(left);
	convert_to_string_ex(right);
	bc_init_num(&first);
	bc_init_num(&second);
	bc_init_num(&result);
	bc_str2num(&first,(*left)->value.str.val,scale);
	bc_str2num(&second,(*right)->value.str.val,scale);
	switch (bc_divide (first,second,&result, scale)) {
		case 0: /* OK */
			return_value->value.str.val = bc_num2str(result);
			return_value->value.str.len = strlen(return_value->value.str.val);
			return_value->type = IS_STRING;
			break;
		case -1: /* division by zero */
			php_error(E_WARNING,"Division by zero");
			break;
	}
	bc_free_num(&first);
	bc_free_num(&second);
	bc_free_num(&result);
	return;
}
/* }}} */

/* {{{ proto string bcmod(string left_operand, string right_operand)
   Returns the modulus of the two arbitrary precision operands */
PHP_FUNCTION(bcmod)
{
	pval **left, **right;
	bc_num first, second, result;

	switch (ZEND_NUM_ARGS()) {
		case 2:
				if (zend_get_parameters_ex(2, &left,&right) == FAILURE) {
		        	WRONG_PARAM_COUNT;
    			}
				break;
		default:
				WRONG_PARAM_COUNT;
				break;
	}
	convert_to_string_ex(left);
	convert_to_string_ex(right);
	bc_init_num(&first);
	bc_init_num(&second);
	bc_init_num(&result);
	bc_str2num(&first,(*left)->value.str.val,0);
	bc_str2num(&second,(*right)->value.str.val,0);
	switch (bc_modulo(first,second,&result, 0)) {
		case 0:
			return_value->value.str.val = bc_num2str(result);
			return_value->value.str.len = strlen(return_value->value.str.val);
			return_value->type = IS_STRING;
			break;
		case -1:
			php_error(E_WARNING,"Division by zero");
			break;
	}
	bc_free_num(&first);
	bc_free_num(&second);
	bc_free_num(&result);
	return;
}
/* }}} */

/* {{{ proto string bcpow(string x, string y [, int scale])
   Returns the value of an arbitrary precision number raised to the power of another */
PHP_FUNCTION(bcpow)
{
	pval **left, **right,**scale_param;
	bc_num first, second, result;
	int scale=bc_precision;

	switch (ZEND_NUM_ARGS()) {
		case 2:
				if (zend_get_parameters_ex(2, &left,&right) == FAILURE) {
		        	WRONG_PARAM_COUNT;
    			}
				break;
		case 3:
				if (zend_get_parameters_ex(3, &left,&right, &scale_param) == FAILURE) {
		        	WRONG_PARAM_COUNT;
    			}
				convert_to_long_ex(scale_param);
				scale = (int) (*scale_param)->value.lval;
				break;
		default:
				WRONG_PARAM_COUNT;
				break;
	}
	convert_to_string_ex(left);
	convert_to_string_ex(right);
	bc_init_num(&first);
	bc_init_num(&second);
	bc_init_num(&result);
	bc_str2num(&first,(*left)->value.str.val,scale);
	bc_str2num(&second,(*right)->value.str.val,scale);
	bc_raise (first,second,&result, scale); 
	return_value->value.str.val = bc_num2str(result);
	return_value->value.str.len = strlen(return_value->value.str.val);
	return_value->type = IS_STRING;
	bc_free_num(&first);
	bc_free_num(&second);
	bc_free_num(&result);
	return;
}
/* }}} */

/* {{{ proto string bcsqrt(string operand [, int scale])
   Returns the square root of an arbitray precision number */
PHP_FUNCTION(bcsqrt)
{
	pval **left,**scale_param;
	bc_num result;
	int scale=bc_precision;

	switch (ZEND_NUM_ARGS()) {
		case 1:
				if (zend_get_parameters_ex(1, &left)== FAILURE) {
		        	WRONG_PARAM_COUNT;
    			}
				break;
		case 2:
				if (zend_get_parameters_ex(2, &left,&scale_param) == FAILURE) {
		        	WRONG_PARAM_COUNT;
    			}
				convert_to_long_ex(scale_param);
				scale = (int) (*scale_param)->value.lval;
				break;
		default:
				WRONG_PARAM_COUNT;
				break;
	}
	convert_to_string_ex(left);
	bc_init_num(&result);
	bc_str2num(&result,(*left)->value.str.val,scale);
	if (bc_sqrt (&result, scale) != 0) {
		return_value->value.str.val = bc_num2str(result);
		return_value->value.str.len = strlen(return_value->value.str.val);
		return_value->type = IS_STRING;
	} else {
		php_error(E_WARNING,"Square root of negative number");
	}
	bc_free_num(&result);
	return;
}
/* }}} */

/* {{{ proto string bccomp(string left_operand, string right_operand [, int scale])
   Compares two arbitrary precision numbers */
PHP_FUNCTION(bccomp)
{
	pval **left, **right, **scale_param;
	bc_num first, second;
	int scale=bc_precision;

	switch (ZEND_NUM_ARGS()) {
		case 2:
				if (zend_get_parameters_ex(2, &left,&right) == FAILURE) {
		        	WRONG_PARAM_COUNT;
    			}
				break;
		case 3:
				if (zend_get_parameters_ex(3, &left,&right, &scale_param) == FAILURE) {
		        	WRONG_PARAM_COUNT;
    			}
				convert_to_long_ex(scale_param);
				scale = (int) (*scale_param)->value.lval;
				break;
		default:
				WRONG_PARAM_COUNT;
				break;
	}

	convert_to_string_ex(left);
	convert_to_string_ex(right);
	bc_init_num(&first);
	bc_init_num(&second);

	bc_str2num(&first,(*left)->value.str.val,scale);
	bc_str2num(&second,(*right)->value.str.val,scale);
	return_value->value.lval = bc_compare(first,second);
	return_value->type = IS_LONG;

	bc_free_num(&first);
	bc_free_num(&second);
	return;
}
/* }}} */

/* {{{ proto string bcscale(int scale)
   Sets default scale parameter for all bc math functions */
PHP_FUNCTION(bcscale)
{
	pval **new_scale;
	
	if (ZEND_NUM_ARGS()!=1 || zend_get_parameters_ex(1,&new_scale)==FAILURE) {
		WRONG_PARAM_COUNT;
	}
	
	convert_to_long_ex(new_scale);
	bc_precision = (*new_scale)->value.lval;
	RETURN_TRUE;
}
/* }}} */


#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */
