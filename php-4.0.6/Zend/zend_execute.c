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

#define ZEND_INTENSIVE_DEBUGGING 0

#include <stdio.h>
#include <signal.h>

#include "zend.h"
#include "zend_compile.h"
#include "zend_execute.h"
#include "zend_API.h"
#include "zend_ptr_stack.h"
#include "zend_constants.h"
#include "zend_extensions.h"
#include "zend_fast_cache.h"
#include "zend_execute_locks.h"

#define get_zval_ptr(node, Ts, should_free, type) _get_zval_ptr(node, Ts, should_free ELS_CC)
#define get_zval_ptr_ptr(node, Ts, type) _get_zval_ptr_ptr(node, Ts ELS_CC)

#define get_incdec_op(op, opcode) \
	switch (opcode) { \
		case ZEND_PRE_INC: \
		case ZEND_POST_INC: \
			(op) = increment_function; \
			break; \
		case ZEND_PRE_DEC: \
		case ZEND_POST_DEC: \
			(op) = decrement_function; \
			break; \
		default: \
			(op) = NULL; \
			break; \
	} \

/* These globals don't have to be thread safe since they're never modified */


/* Prototypes */
static zval get_overloaded_property(temp_variable *T ELS_DC);
static void set_overloaded_property(temp_variable *T, zval *value ELS_DC);
static void call_overloaded_function(temp_variable *T, int arg_count, zval *return_value ELS_DC);
static void zend_fetch_var_address(znode *result, znode *op1, znode *op2, temp_variable *Ts, int type ELS_DC);
static void zend_fetch_dimension_address(znode *result, znode *op1, znode *op2, temp_variable *Ts, int type ELS_DC);
static void zend_fetch_property_address(znode *result, znode *op1, znode *op2, temp_variable *Ts, int type ELS_DC);
static void zend_fetch_dimension_address_from_tmp_var(znode *result, znode *op1, znode *op2, temp_variable *Ts ELS_DC);
static void zend_extension_statement_handler(zend_extension *extension, zend_op_array *op_array);
static void zend_extension_fcall_begin_handler(zend_extension *extension, zend_op_array *op_array);
static void zend_extension_fcall_end_handler(zend_extension *extension, zend_op_array *op_array);

#define RETURN_VALUE_USED(opline) (!((opline)->result.u.EA.type & EXT_TYPE_UNUSED))

static inline zval *_get_zval_ptr(znode *node, temp_variable *Ts, int *should_free ELS_DC)
{
	switch(node->op_type) {
		case IS_CONST:
			*should_free = 0;
			return &node->u.constant;
			break;
		case IS_TMP_VAR:
			*should_free = 1;
			return &Ts[node->u.var].tmp_var;
			break;
		case IS_VAR:
			if (Ts[node->u.var].var.ptr) {
				PZVAL_UNLOCK(Ts[node->u.var].var.ptr);
				*should_free = 0;
				return Ts[node->u.var].var.ptr;
			} else {
				*should_free = 1;

				switch (Ts[node->u.var].EA.type) {
					case IS_OVERLOADED_OBJECT:
						Ts[node->u.var].tmp_var = get_overloaded_property(&Ts[node->u.var] ELS_CC);
						Ts[node->u.var].tmp_var.refcount=1;
						Ts[node->u.var].tmp_var.is_ref=1;
						return &Ts[node->u.var].tmp_var;
						break;
					case IS_STRING_OFFSET: {
							temp_variable *T = &Ts[node->u.var];
							zval *str = T->EA.data.str_offset.str;

							if (T->EA.data.str_offset.str->type != IS_STRING
								|| (T->EA.data.str_offset.offset<0)
								|| (T->EA.data.str_offset.str->value.str.len <= T->EA.data.str_offset.offset)) {
								T->tmp_var.value.str.val = empty_string;
								T->tmp_var.value.str.len = 0;
							} else {
								char c = str->value.str.val[T->EA.data.str_offset.offset];

								T->tmp_var.value.str.val = estrndup(&c, 1);
								T->tmp_var.value.str.len = 1;
							}
							PZVAL_UNLOCK(str);
							T->tmp_var.refcount=1;
							T->tmp_var.is_ref=1;
							T->tmp_var.type = IS_STRING;
							return &T->tmp_var;
						}
						break;
				}
			}
			break;
		case IS_UNUSED:
			*should_free = 0;
			return NULL;
			break;
		EMPTY_SWITCH_DEFAULT_CASE()
	}
	return NULL;
}

static inline zval *_get_object_zval_ptr(znode *node, temp_variable *Ts, int *should_free ELS_DC)
{
	switch(node->op_type) {
		case IS_TMP_VAR:
			*should_free = 1;
			return &Ts[node->u.var].tmp_var;
			break;
		case IS_VAR:
			if (Ts[node->u.var].var.ptr_ptr) {
				PZVAL_UNLOCK(*Ts[node->u.var].var.ptr_ptr);
				*should_free = 0;
				SEPARATE_ZVAL_IF_NOT_REF(Ts[node->u.var].var.ptr_ptr);
				return *Ts[node->u.var].var.ptr_ptr;
			} else {
				if (Ts[node->u.var].EA.type==IS_STRING_OFFSET) {
					PZVAL_UNLOCK(Ts[node->u.var].EA.data.str_offset.str);
				}
				*should_free = 1;
				return NULL;
			}
			break;
		case IS_UNUSED:
			return NULL;
			break;
		EMPTY_SWITCH_DEFAULT_CASE()
	}
	return NULL;
}


static inline zval **_get_zval_ptr_ptr(znode *node, temp_variable *Ts ELS_DC)
{
	if (node->op_type==IS_VAR) {
		if (Ts[node->u.var].var.ptr_ptr) {
			PZVAL_UNLOCK(*Ts[node->u.var].var.ptr_ptr);
		} else if (Ts[node->u.var].EA.type==IS_STRING_OFFSET) {
			PZVAL_UNLOCK(Ts[node->u.var].EA.data.str_offset.str);
		}
		return Ts[node->u.var].var.ptr_ptr;
	} else {
		return NULL;
	}
}


static inline zval **zend_fetch_property_address_inner(HashTable *ht, znode *op2, temp_variable *Ts, int type ELS_DC)
{
	zval *prop_ptr = get_zval_ptr(op2, Ts, &EG(free_op2), BP_VAR_R);
	zval **retval;
	zval tmp;


	switch (op2->op_type) {
		case IS_CONST:
			/* already a constant string */
			break;
		case IS_VAR:
			tmp = *prop_ptr;
			zval_copy_ctor(&tmp);
			convert_to_string(&tmp);
			prop_ptr = &tmp;
			break;
		case IS_TMP_VAR:
			convert_to_string(prop_ptr);
			break;
	}

	if (zend_hash_find(ht, prop_ptr->value.str.val, prop_ptr->value.str.len+1, (void **) &retval) == FAILURE) {
		switch (type) {
			case BP_VAR_R: 
				zend_error(E_NOTICE,"Undefined property:  %s", prop_ptr->value.str.val);
				/* break missing intentionally */
			case BP_VAR_IS:
				retval = &EG(uninitialized_zval_ptr);
				break;
			case BP_VAR_RW:
				zend_error(E_NOTICE,"Undefined property:  %s", prop_ptr->value.str.val);
				/* break missing intentionally */
			case BP_VAR_W: {
					zval *new_zval = &EG(uninitialized_zval);

					new_zval->refcount++;
					zend_hash_update(ht, prop_ptr->value.str.val, prop_ptr->value.str.len+1, &new_zval, sizeof(zval *), (void **) &retval);
				}
				break;
			EMPTY_SWITCH_DEFAULT_CASE()
		}
	}

	if (prop_ptr == &tmp) {
		zval_dtor(prop_ptr);
	}
	FREE_OP(op2, EG(free_op2));
	return retval;
}



static inline void zend_switch_free(zend_op *opline, temp_variable *Ts ELS_DC)
{
	switch (opline->op1.op_type) {
		case IS_VAR:
			if (!Ts[opline->op1.u.var].var.ptr_ptr) {
				get_zval_ptr(&opline->op1, Ts, &EG(free_op1), BP_VAR_R);
				FREE_OP(&opline->op1, EG(free_op1));
			} else {
				zval_ptr_dtor(&Ts[opline->op1.u.var].var.ptr);
				if (opline->extended_value) { /* foreach() free */
					zval_ptr_dtor(&Ts[opline->op1.u.var].var.ptr);
				}
			}
			break;
		case IS_TMP_VAR:
			zendi_zval_dtor(Ts[opline->op1.u.var].tmp_var);
			break;
		EMPTY_SWITCH_DEFAULT_CASE()
	}
}


static inline void zend_assign_to_variable(znode *result, znode *op1, znode *op2, zval *value, int type, temp_variable *Ts ELS_DC)
{
	zval **variable_ptr_ptr = get_zval_ptr_ptr(op1, Ts, BP_VAR_W);
	zval *variable_ptr;
	
	if (!variable_ptr_ptr) {
		switch (Ts[op1->u.var].EA.type) {
			case IS_OVERLOADED_OBJECT:
				set_overloaded_property(&Ts[op1->u.var], value ELS_CC);
				if (type == IS_TMP_VAR) {
					zval_dtor(value);
				}
				break;
			case IS_STRING_OFFSET: {
					temp_variable *T = &Ts[op1->u.var];

					if (T->EA.data.str_offset.str->type == IS_STRING
						&& (T->EA.data.str_offset.offset >= 0)
						&& (T->EA.data.str_offset.offset < T->EA.data.str_offset.str->value.str.len)) {
						zval tmp;
						zval *final_value = value;

						if (value->type!=IS_STRING) {
							tmp = *value;
							if (op2 && op2->op_type == IS_VAR) {
								zval_copy_ctor(&tmp);
							}
							convert_to_string(&tmp);
							final_value = &tmp;
						}

						T->EA.data.str_offset.str->value.str.val[T->EA.data.str_offset.offset] = final_value->value.str.val[0];
						if (op2
							&& op2->op_type == IS_VAR
							&& value==&Ts[op2->u.var].tmp_var) {
							STR_FREE(value->value.str.val);
						}
						if (final_value == &tmp) {
							zval_dtor(final_value);
						}
						/*
						 * the value of an assignment to a string offset is undefined
						Ts[result->u.var].var = &T->EA.data.str_offset.str;
						*/
					}
					/* zval_ptr_dtor(&T->EA.data.str_offset.str); Nuke this line if it doesn't cause a leak */
					T->tmp_var.type = IS_STRING;
				}
				break;
			EMPTY_SWITCH_DEFAULT_CASE()
		}
		Ts[result->u.var].var.ptr_ptr = &EG(uninitialized_zval_ptr);
		SELECTIVE_PZVAL_LOCK(*Ts[result->u.var].var.ptr_ptr, result);
		AI_USE_PTR(Ts[result->u.var].var);
		return;
	}

	variable_ptr = *variable_ptr_ptr;

	if (variable_ptr == EG(error_zval_ptr)) {
		if (result) {
			Ts[result->u.var].var.ptr_ptr = &EG(uninitialized_zval_ptr);
			SELECTIVE_PZVAL_LOCK(*Ts[result->u.var].var.ptr_ptr, result);
			AI_USE_PTR(Ts[result->u.var].var);
		}
		if (type==IS_TMP_VAR) {
			zval_dtor(value);
		}
		return;
	}
	
	if (PZVAL_IS_REF(variable_ptr)) {
		if (variable_ptr!=value) {
			short refcount=variable_ptr->refcount;
			zval garbage;
	
			if (type!=IS_TMP_VAR) {
				value->refcount++;
			}
			garbage = *variable_ptr;
			*variable_ptr = *value;
			variable_ptr->refcount = refcount;
			variable_ptr->is_ref = 1;
			if (type!=IS_TMP_VAR) {
				zendi_zval_copy_ctor(*variable_ptr);
				value->refcount--;
			}
			zendi_zval_dtor(garbage);
		}
	} else {
		variable_ptr->refcount--;
		if (variable_ptr->refcount==0) {
			switch (type) {
				case IS_VAR:
					/* break missing intentionally */
				case IS_CONST:
					if (variable_ptr==value) {
						variable_ptr->refcount++;
					} else if (PZVAL_IS_REF(value)) {
						zval tmp;

						tmp = *value;
						zval_copy_ctor(&tmp);
						tmp.refcount=1;
						zendi_zval_dtor(*variable_ptr);
						*variable_ptr = tmp;
					} else {
						value->refcount++;
						zendi_zval_dtor(*variable_ptr);
						safe_free_zval_ptr(variable_ptr);
						*variable_ptr_ptr = value;
					}
					break;
				case IS_TMP_VAR:
					zendi_zval_dtor(*variable_ptr);
					value->refcount=1;
					*variable_ptr = *value;
					break;
				EMPTY_SWITCH_DEFAULT_CASE()
			}
		} else { /* we need to split */
			switch (type) {
				case IS_VAR:
					/* break missing intentionally */
				case IS_CONST:
					if (PZVAL_IS_REF(value) && value->refcount > 0) {
						ALLOC_ZVAL(variable_ptr);
						*variable_ptr_ptr = variable_ptr;
						*variable_ptr = *value;
						zval_copy_ctor(variable_ptr);
						variable_ptr->refcount=1;
						break;
					}
					*variable_ptr_ptr = value;
					value->refcount++;
					break;
				case IS_TMP_VAR:
					ALLOC_ZVAL(*variable_ptr_ptr);
					value->refcount=1;
					**variable_ptr_ptr = *value;
					break;
				EMPTY_SWITCH_DEFAULT_CASE()
			}
		}
		(*variable_ptr_ptr)->is_ref=0;
	}
	if (result) {
		Ts[result->u.var].var.ptr_ptr = variable_ptr_ptr;
		SELECTIVE_PZVAL_LOCK(*variable_ptr_ptr, result);
		AI_USE_PTR(Ts[result->u.var].var);
	} 
}


/* Utility Functions for Extensions */
static void zend_extension_statement_handler(zend_extension *extension, zend_op_array *op_array)
{
	if (extension->statement_handler) {
		extension->statement_handler(op_array);
	}
}


static void zend_extension_fcall_begin_handler(zend_extension *extension, zend_op_array *op_array)
{
	if (extension->fcall_begin_handler) {
		extension->fcall_begin_handler(op_array);
	}
}


static void zend_extension_fcall_end_handler(zend_extension *extension, zend_op_array *op_array)
{
	if (extension->fcall_end_handler) {
		extension->fcall_end_handler(op_array);
	}
}


static void print_refcount(zval *p, char *str)
{
	print_refcount(NULL, NULL);
}


static void zend_fetch_var_address(znode *result, znode *op1, znode *op2, temp_variable *Ts, int type ELS_DC)
{
	int free_op1;
	zval *varname = get_zval_ptr(op1, Ts, &free_op1, BP_VAR_R);
	zval **retval;
	zval tmp_varname;
	HashTable *target_symbol_table=0;

	switch (op2->u.fetch_type) {
		case ZEND_FETCH_LOCAL:
			target_symbol_table = EG(active_symbol_table);
			break;
		case ZEND_FETCH_GLOBAL:
			if (op1->op_type == IS_VAR) {
				PZVAL_LOCK(varname);
			}
			target_symbol_table = &EG(symbol_table);
			break;
		case ZEND_FETCH_STATIC:
			if (!EG(active_op_array)->static_variables) {
				ALLOC_HASHTABLE(EG(active_op_array)->static_variables);
				zend_hash_init(EG(active_op_array)->static_variables, 2, NULL, ZVAL_PTR_DTOR, 0);
			}
			target_symbol_table = EG(active_op_array)->static_variables;
			break;
		EMPTY_SWITCH_DEFAULT_CASE()
	}

	if (varname->type != IS_STRING) {
		tmp_varname = *varname;
		zval_copy_ctor(&tmp_varname);
		convert_to_string(&tmp_varname);
		varname = &tmp_varname;
	}
	if (zend_hash_find(target_symbol_table, varname->value.str.val, varname->value.str.len+1, (void **) &retval) == FAILURE) {
		switch (type) {
			case BP_VAR_R: 
				zend_error(E_NOTICE,"Undefined variable:  %s", varname->value.str.val);
				/* break missing intentionally */
			case BP_VAR_IS:
				retval = &EG(uninitialized_zval_ptr);
				break;
			case BP_VAR_RW:
				zend_error(E_NOTICE,"Undefined variable:  %s", varname->value.str.val);
				/* break missing intentionally */
			case BP_VAR_W: {
					zval *new_zval = &EG(uninitialized_zval);

					new_zval->refcount++;
					zend_hash_update(target_symbol_table, varname->value.str.val, varname->value.str.len+1, &new_zval, sizeof(zval *), (void **) &retval);
				}
				break;
			EMPTY_SWITCH_DEFAULT_CASE()
		}
	}
	if (op2->u.fetch_type == ZEND_FETCH_LOCAL) {
		FREE_OP(op1, free_op1);
	} else if (op2->u.fetch_type == ZEND_FETCH_STATIC) {
		zval_update_constant(retval, (void *) 1);
	}

	if (varname == &tmp_varname) {
		zval_dtor(varname);
	}
	Ts[result->u.var].var.ptr_ptr = retval;
	SELECTIVE_PZVAL_LOCK(*retval, result);
}


static inline zval **zend_fetch_dimension_address_inner(HashTable *ht, znode *op2, temp_variable *Ts, int type ELS_DC)
{
	zval *dim = get_zval_ptr(op2, Ts, &EG(free_op2), BP_VAR_R);
	zval **retval;
	char *offset_key;
	int offset_key_length;

	switch (dim->type) {
		case IS_NULL:
			offset_key = "";
			offset_key_length = 0;
			goto fetch_string_dim;
		case IS_STRING:
			offset_key = dim->value.str.val;
			offset_key_length = dim->value.str.len;

fetch_string_dim:
			if (zend_hash_find(ht, offset_key, offset_key_length+1, (void **) &retval) == FAILURE) {
				switch (type) {
					case BP_VAR_R: 
						zend_error(E_NOTICE,"Undefined index:  %s", offset_key);
						/* break missing intentionally */
					case BP_VAR_IS:
						retval = &EG(uninitialized_zval_ptr);
						break;
					case BP_VAR_RW:
						zend_error(E_NOTICE,"Undefined index:  %s", offset_key);
						/* break missing intentionally */
					case BP_VAR_W: {
							zval *new_zval = &EG(uninitialized_zval);

							new_zval->refcount++;
							zend_hash_update(ht, offset_key, offset_key_length+1, &new_zval, sizeof(zval *), (void **) &retval);
						}
						break;
				}
			}
			break;
		case IS_DOUBLE:
		case IS_RESOURCE:
		case IS_BOOL: 
		case IS_LONG: {
				long index;

				if (dim->type == IS_DOUBLE) {
					index = (long)dim->value.dval;
				} else {
					index = dim->value.lval;
				}
				if (zend_hash_index_find(ht, index, (void **) &retval) == FAILURE) {
					switch (type) {
						case BP_VAR_R: 
							zend_error(E_NOTICE,"Undefined offset:  %d", index);
							/* break missing intentionally */
						case BP_VAR_IS:
							retval = &EG(uninitialized_zval_ptr);
							break;
						case BP_VAR_RW:
							zend_error(E_NOTICE,"Undefined offset:  %d", index);
							/* break missing intentionally */
						case BP_VAR_W: {
								zval *new_zval = &EG(uninitialized_zval);

								new_zval->refcount++;
								zend_hash_index_update(ht, index, &new_zval, sizeof(zval *), (void **) &retval);
							}
							break;
					}
				}
			}
			break;
		default: 
			zend_error(E_WARNING, "Illegal offset type");
			if (type == BP_VAR_R || type == BP_VAR_IS) {
				retval = &EG(uninitialized_zval_ptr);
			} else {
				retval = &EG(error_zval_ptr);
			}
			break;
	}
	FREE_OP(op2, EG(free_op2));
	return retval;
}

static void fetch_overloaded_element(znode *result, znode *op1, znode *op2, temp_variable *Ts, int type, zval ***retval, int overloaded_element_type ELS_DC)
{
	zend_overloaded_element overloaded_element;

	if (Ts[op1->u.var].EA.type == IS_STRING_OFFSET) {
		zval_ptr_dtor(&Ts[op1->u.var].EA.data.str_offset.str);
		switch (type) {
			case BP_VAR_R:
			case BP_VAR_IS:
				*retval = &EG(uninitialized_zval_ptr);
				break;
			case BP_VAR_W:
			case BP_VAR_RW:
				*retval = &EG(error_zval_ptr);
				break;
			EMPTY_SWITCH_DEFAULT_CASE()
		}
		SELECTIVE_PZVAL_LOCK(**retval, result);
		return;
	}

	overloaded_element.element = *get_zval_ptr(op2, Ts, &EG(free_op2), type);
	overloaded_element.type = overloaded_element_type;
	if (!EG(free_op2)) {
		zval_copy_ctor(&overloaded_element.element);
	}

	Ts[result->u.var].EA = Ts[op1->u.var].EA;
	zend_llist_add_element(Ts[result->u.var].EA.data.overloaded_element.elements_list, &overloaded_element);

	Ts[result->u.var].EA.type = IS_OVERLOADED_OBJECT;
	*retval = NULL;
	return;
}


static void zend_fetch_dimension_address(znode *result, znode *op1, znode *op2, temp_variable *Ts, int type ELS_DC)
{
	zval **container_ptr = get_zval_ptr_ptr(op1, Ts, type);
	zval *container;
	zval ***retval = &Ts[result->u.var].var.ptr_ptr;


	if (container_ptr == NULL) {
		fetch_overloaded_element(result, op1, op2, Ts, type, retval, OE_IS_ARRAY ELS_CC);
		return;
	}

	container = *container_ptr;

	if (container == EG(error_zval_ptr)) {
		*retval = &EG(error_zval_ptr);
		SELECTIVE_PZVAL_LOCK(**retval, result);
		return;
	}

	if (container->type==IS_NULL
		|| (container->type==IS_BOOL && container->value.lval==0)
		|| (container->type==IS_STRING && container->value.str.len==0)) {
		switch (type) {
			case BP_VAR_RW:
			case BP_VAR_W:
				if (!PZVAL_IS_REF(container)) {
					SEPARATE_ZVAL(container_ptr);
					container = *container_ptr;
				}
				array_init(container);
				break;
		}
	}

	switch (container->type) {
		case IS_ARRAY:
			if ((type==BP_VAR_W || type==BP_VAR_RW) && container->refcount>1 && !PZVAL_IS_REF(container)) {
				SEPARATE_ZVAL(container_ptr);
				container = *container_ptr;
			}
			if (op2->op_type == IS_UNUSED) {
				zval *new_zval = &EG(uninitialized_zval);

				new_zval->refcount++;
				zend_hash_next_index_insert(container->value.ht, &new_zval, sizeof(zval *), (void **) retval);
			} else {
				*retval = zend_fetch_dimension_address_inner(container->value.ht, op2, Ts, type ELS_CC);
			}
			SELECTIVE_PZVAL_LOCK(**retval, result);
			break;
		case IS_NULL:
			/* for read-mode only */
			get_zval_ptr(op2, Ts, &EG(free_op2), BP_VAR_R);
			*retval = &EG(uninitialized_zval_ptr);
			SELECTIVE_PZVAL_LOCK(**retval, result);
			FREE_OP(op2, EG(free_op2));
			if (type==BP_VAR_W || type==BP_VAR_RW) {
				zend_error(E_WARNING, "Cannot use a NULL value as an array");
			}
			break;
		case IS_STRING: {
				zval *offset;
				zval tmp;

				if (op2->op_type==IS_UNUSED) {
					zend_error(E_ERROR, "[] operator not supported for strings");
				}

				offset = get_zval_ptr(op2, Ts, &EG(free_op2), BP_VAR_R);

				if (offset->type != IS_LONG) {
					tmp = *offset;
					zval_copy_ctor(&tmp);
					convert_to_long(&tmp);
					offset = &tmp;
				}
				if (type!=BP_VAR_R && type!=BP_VAR_IS) {
					SEPARATE_ZVAL_IF_NOT_REF(container_ptr);
				}
				container = *container_ptr;
				Ts[result->u.var].EA.data.str_offset.str = container;
				PZVAL_LOCK(container);
				Ts[result->u.var].EA.data.str_offset.offset = offset->value.lval;
				Ts[result->u.var].EA.type = IS_STRING_OFFSET;
				FREE_OP(op2, EG(free_op2));
				*retval = NULL;
				return;
			}
			break;
		default: {
				zval *offset;

				offset = get_zval_ptr(op2, Ts, &EG(free_op2), BP_VAR_R);
				if (type==BP_VAR_R || type==BP_VAR_IS) {
					*retval = &EG(uninitialized_zval_ptr);
				} else {
					*retval = &EG(error_zval_ptr);
				}
				FREE_OP(op2, EG(free_op2));
				SELECTIVE_PZVAL_LOCK(**retval, result);
				if (type==BP_VAR_W || type==BP_VAR_RW) {
					zend_error(E_WARNING, "Cannot use a scalar value as an array");
				}
			}
			break;
	}
}


static void zend_fetch_dimension_address_from_tmp_var(znode *result, znode *op1, znode *op2, temp_variable *Ts ELS_DC)
{
	int free_op1;
	zval *container = get_zval_ptr(op1, Ts, &free_op1, BP_VAR_R);

	if (container->type != IS_ARRAY) {
		Ts[result->u.var].var.ptr_ptr = &EG(uninitialized_zval_ptr);
		SELECTIVE_PZVAL_LOCK(*Ts[result->u.var].var.ptr_ptr, result);
		return;
	}

	Ts[result->u.var].var.ptr_ptr = zend_fetch_dimension_address_inner(container->value.ht, op2, Ts, BP_VAR_R ELS_CC);
	SELECTIVE_PZVAL_LOCK(*Ts[result->u.var].var.ptr_ptr, result);
}


static void zend_fetch_property_address(znode *result, znode *op1, znode *op2, temp_variable *Ts, int type ELS_DC)
{
	zval **container_ptr = get_zval_ptr_ptr(op1, Ts, type);
	zval *container;
	zval ***retval = &Ts[result->u.var].var.ptr_ptr;


	if (container_ptr == NULL) {
		fetch_overloaded_element(result, op1, op2, Ts, type, retval, OE_IS_OBJECT ELS_CC);
		return;
	}

	container = *container_ptr;
	if (container == EG(error_zval_ptr)) {
		*retval = &EG(error_zval_ptr);
		SELECTIVE_PZVAL_LOCK(**retval, result);
		return;
	}

	if (container->type == IS_OBJECT
		&& container->value.obj.ce->handle_property_get) {
		zend_overloaded_element overloaded_element;

		Ts[result->u.var].EA.data.overloaded_element.object = container;
		Ts[result->u.var].EA.data.overloaded_element.type = type;
		Ts[result->u.var].EA.data.overloaded_element.elements_list = (zend_llist *) emalloc(sizeof(zend_llist));
		zend_llist_init(Ts[result->u.var].EA.data.overloaded_element.elements_list, sizeof(zend_overloaded_element), NULL, 0);
		overloaded_element.element = *get_zval_ptr(op2, Ts, &EG(free_op2), type);
		overloaded_element.type = OE_IS_OBJECT;
		if (!EG(free_op2)) {
			zval_copy_ctor(&overloaded_element.element);
		}
		zend_llist_add_element(Ts[result->u.var].EA.data.overloaded_element.elements_list, &overloaded_element);
		Ts[result->u.var].EA.type = IS_OVERLOADED_OBJECT;
		*retval = NULL;
		return;
	}


	if (container->type==IS_NULL
		|| (container->type==IS_BOOL && container->value.lval==0)
		|| (container->type==IS_STRING && container->value.str.len==0)) {
		switch (type) {
			case BP_VAR_RW:
			case BP_VAR_W:
				if (!PZVAL_IS_REF(container)) {
					SEPARATE_ZVAL(container_ptr);
					container = *container_ptr;
				}
				object_init(container);
				break;
		}
	}
		
	if (container->type != IS_OBJECT) {
		zval *offset;

		offset = get_zval_ptr(op2, Ts, &EG(free_op2), BP_VAR_R);
		FREE_OP(op2, EG(free_op2));
		if (type==BP_VAR_R || type==BP_VAR_IS) {
			*retval = &EG(uninitialized_zval_ptr);
		} else {
			*retval = &EG(error_zval_ptr);
		}
		SELECTIVE_PZVAL_LOCK(**retval, result);
		return;
	}


	if ((type==BP_VAR_W || type==BP_VAR_RW) && container->refcount>1 && !PZVAL_IS_REF(container)) {
		SEPARATE_ZVAL(container_ptr);
		container = *container_ptr;
	}
	*retval = zend_fetch_property_address_inner(container->value.obj.properties, op2, Ts, type ELS_CC);
	SELECTIVE_PZVAL_LOCK(**retval, result);
}


static zval get_overloaded_property(temp_variable *T ELS_DC)
{
	zval result;

	result = (T->EA.data.overloaded_element.object)->value.obj.ce->handle_property_get(&T->EA.data.overloaded_element);

	zend_llist_destroy(T->EA.data.overloaded_element.elements_list);
	efree(T->EA.data.overloaded_element.elements_list);
	return result;
}


static void set_overloaded_property(temp_variable *T, zval *value ELS_DC)
{
	if ((T->EA.data.overloaded_element.object)->value.obj.ce->handle_property_set) {
		(T->EA.data.overloaded_element.object)->value.obj.ce->handle_property_set(&T->EA.data.overloaded_element, value);
	} else {
		zend_error(E_ERROR, "Class '%s' does not support setting overloaded properties",
			(T->EA.data.overloaded_element.object)->value.obj.ce->name);
	}
	zend_llist_destroy(T->EA.data.overloaded_element.elements_list);
	efree(T->EA.data.overloaded_element.elements_list);
}


static void call_overloaded_function(temp_variable *T, int arg_count, zval *return_value ELS_DC)
{
	if ((T->EA.data.overloaded_element.object)->value.obj.ce->handle_function_call) {
		(T->EA.data.overloaded_element.object)->value.obj.ce->handle_function_call(arg_count, return_value, T->EA.data.overloaded_element.object, 1 ELS_CC, &T->EA.data.overloaded_element);
	} else {
		zend_error(E_ERROR, "Class '%s' does not support overloaded method calls",
			(T->EA.data.overloaded_element.object)->value.obj.ce->name);
	}
	zend_llist_destroy(T->EA.data.overloaded_element.elements_list);
	efree(T->EA.data.overloaded_element.elements_list);
}


#if ZEND_INTENSIVE_DEBUGGING

#define CHECK_SYMBOL_TABLES()														\
	zend_hash_apply(&EG(symbol_table), (int (*)()) zend_check_symbol);				\
	if (&EG(symbol_table)!=EG(active_symbol_table)) {								\
		zend_hash_apply(EG(active_symbol_table), (int (*)()) zend_check_symbol);	\
	}

static int zend_check_symbol(zval **pz)
{
	if ((*pz)->type>9) {
		fprintf(stderr, "Warning!  %x has invalid type!\n", *pz);
	} else if ((*pz)->type==IS_ARRAY) {
		zend_hash_apply((*pz)->value.ht, (int (*)()) zend_check_symbol);
	} else if ((*pz)->type==IS_OBJECT) {
		zend_hash_apply((*pz)->value.obj.properties, (int (*)()) zend_check_symbol);
	}

	return 0;
}


#else
#define CHECK_SYMBOL_TABLES()
#endif

#define NEXT_OPCODE()		\
	CHECK_SYMBOL_TABLES()	\
	opline++;				\
	continue;

typedef struct _object_info {
	zval *ptr;
} object_info;

ZEND_API void execute(zend_op_array *op_array ELS_DC)
{
	zend_op *opline = op_array->opcodes;
#if SUPPORT_INTERACTIVE
	zend_op *end = op_array->opcodes + op_array->last;
#endif
	zend_function_state function_state;
	zend_function *fbc=NULL;  /* Function Being Called */
	object_info object = {NULL};
#if ZEND_DEBUG || (!defined (__GNUC__) || __GNUC__ < 2)
	temp_variable *Ts = (temp_variable *) do_alloca(sizeof(temp_variable)*op_array->T);
#else
	temp_variable Ts[op_array->T];
#endif
	zend_bool original_in_execution=EG(in_execution);

	EG(in_execution) = 1;
#if SUPPORT_INTERACTIVE
	if (EG(interactive)) {
		opline = op_array->opcodes + op_array->start_op_number;
		end = op_array->opcodes + op_array->end_op_number;
	}
#endif

	EG(opline_ptr) = &opline;

	function_state.function = (zend_function *) op_array;
	EG(function_state_ptr) = &function_state;
#if ZEND_DEBUG
	/* function_state.function_symbol_table is saved as-is to a stack,
	 * which is an intentional UMR.  Shut it up if we're in DEBUG.
	 */
	function_state.function_symbol_table = NULL;
#endif
	
	if (op_array->uses_globals) {
		zval *globals;

		ALLOC_ZVAL(globals);
		globals->refcount=1;
		globals->is_ref=1;
		globals->type = IS_ARRAY;
		globals->value.ht = &EG(symbol_table);
		if (zend_hash_add(EG(active_symbol_table), "GLOBALS", sizeof("GLOBALS"), &globals, sizeof(zval *), NULL)==FAILURE) {
			FREE_ZVAL(globals);
		}
	}

#if SUPPORT_INTERACTIVE
	while (opline<end) {
#else
	while (1) {
#endif
#ifdef ZEND_WIN32
		if (EG(timed_out)) {
			zend_timeout(0);
		}
#endif
		switch(opline->opcode) {
			case ZEND_ADD:
				EG(binary_op) = add_function;
				goto binary_op_addr;
			case ZEND_SUB:
				EG(binary_op) = sub_function;
				goto binary_op_addr;
			case ZEND_MUL:
				EG(binary_op) = mul_function;
				goto binary_op_addr;
			case ZEND_DIV:
				EG(binary_op) = div_function;
				goto binary_op_addr;
			case ZEND_MOD:
				EG(binary_op) = mod_function;
				goto binary_op_addr;
			case ZEND_SL:
				EG(binary_op) = shift_left_function;
				goto binary_op_addr;
			case ZEND_SR:
				EG(binary_op) = shift_right_function;
				goto binary_op_addr;
			case ZEND_CONCAT:
				EG(binary_op) = concat_function;
				goto binary_op_addr;
			case ZEND_IS_IDENTICAL:
				EG(binary_op) = is_identical_function;
				goto binary_op_addr;
			case ZEND_IS_NOT_IDENTICAL:
				EG(binary_op) = is_not_identical_function;
				goto binary_op_addr;
			case ZEND_IS_EQUAL:
				EG(binary_op) = is_equal_function;
				goto binary_op_addr;
			case ZEND_IS_NOT_EQUAL:
				EG(binary_op) = is_not_equal_function;
				goto binary_op_addr;
			case ZEND_IS_SMALLER:
				EG(binary_op) = is_smaller_function;
				goto binary_op_addr;
			case ZEND_IS_SMALLER_OR_EQUAL:
				EG(binary_op) = is_smaller_or_equal_function;
				goto binary_op_addr;
			case ZEND_BW_OR:
				EG(binary_op) = bitwise_or_function;
				goto binary_op_addr;
			case ZEND_BW_AND:
				EG(binary_op) = bitwise_and_function;
				goto binary_op_addr;
			case ZEND_BW_XOR:
				EG(binary_op) = bitwise_xor_function;
				goto binary_op_addr;
			case ZEND_BOOL_XOR:
				EG(binary_op) = boolean_xor_function;
		  	    /* Fall through */
binary_op_addr:
				EG(binary_op)(&Ts[opline->result.u.var].tmp_var, 
							 get_zval_ptr(&opline->op1, Ts, &EG(free_op1), BP_VAR_R),
							 get_zval_ptr(&opline->op2, Ts, &EG(free_op2), BP_VAR_R) );
				FREE_OP(&opline->op1, EG(free_op1));
				FREE_OP(&opline->op2, EG(free_op2));
				NEXT_OPCODE();
			case ZEND_BW_NOT:
			case ZEND_BOOL_NOT:
				EG(unary_op) = get_unary_op(opline->opcode);
				EG(unary_op)(&Ts[opline->result.u.var].tmp_var,
							get_zval_ptr(&opline->op1, Ts, &EG(free_op1), BP_VAR_R) );
				FREE_OP(&opline->op1, EG(free_op1));
				NEXT_OPCODE();
			case ZEND_ASSIGN_ADD:
				EG(binary_op) = add_function;
				goto binary_assign_op_addr;
			case ZEND_ASSIGN_SUB:
				EG(binary_op) = sub_function;
				goto binary_assign_op_addr;
			case ZEND_ASSIGN_MUL:
				EG(binary_op) = mul_function;
				goto binary_assign_op_addr;
			case ZEND_ASSIGN_DIV:
				EG(binary_op) = div_function;
				goto binary_assign_op_addr;
			case ZEND_ASSIGN_MOD:
				EG(binary_op) = mod_function;
				goto binary_assign_op_addr;
			case ZEND_ASSIGN_SL:
				EG(binary_op) = shift_left_function;
				goto binary_assign_op_addr;
			case ZEND_ASSIGN_SR:
				EG(binary_op) = shift_right_function;
				goto binary_assign_op_addr;
			case ZEND_ASSIGN_CONCAT:
				EG(binary_op) = concat_function;
				goto binary_assign_op_addr;
			case ZEND_ASSIGN_BW_OR:
				EG(binary_op) = bitwise_or_function;
				goto binary_assign_op_addr;
			case ZEND_ASSIGN_BW_AND:
				EG(binary_op) = bitwise_and_function;
				goto binary_assign_op_addr;
			case ZEND_ASSIGN_BW_XOR:
				EG(binary_op) = bitwise_xor_function;
				/* Fall through */
binary_assign_op_addr: {
					zval **var_ptr = get_zval_ptr_ptr(&opline->op1, Ts, BP_VAR_RW);
				
					if (!var_ptr) {
						zend_error(E_ERROR, "Cannot use assign-op operators with overloaded objects nor string offsets");
					}
					if (*var_ptr == EG(error_zval_ptr)) {
						Ts[opline->result.u.var].var.ptr_ptr = &EG(uninitialized_zval_ptr);
						SELECTIVE_PZVAL_LOCK(*Ts[opline->result.u.var].var.ptr_ptr, &opline->result);
						AI_USE_PTR(Ts[opline->result.u.var].var);
						opline++;
						continue;
					}
					
					SEPARATE_ZVAL_IF_NOT_REF(var_ptr);

					EG(binary_op)(*var_ptr, *var_ptr, get_zval_ptr(&opline->op2, Ts, &EG(free_op2), BP_VAR_R));
					Ts[opline->result.u.var].var.ptr_ptr = var_ptr;
					SELECTIVE_PZVAL_LOCK(*var_ptr, &opline->result);
					FREE_OP(&opline->op2, EG(free_op2));
					AI_USE_PTR(Ts[opline->result.u.var].var);
				}
				NEXT_OPCODE();
			case ZEND_PRE_INC:
			case ZEND_PRE_DEC:
			case ZEND_POST_INC:
			case ZEND_POST_DEC: {
					int (*incdec_op)(zval *op);
					zval **var_ptr = get_zval_ptr_ptr(&opline->op1, Ts, BP_VAR_RW);

					if (!var_ptr) {
						zend_error(E_ERROR, "Cannot increment/decrement overloaded objects nor string offsets");
					}
					if (*var_ptr == EG(error_zval_ptr)) {
						Ts[opline->result.u.var].var.ptr_ptr = &EG(uninitialized_zval_ptr);
						SELECTIVE_PZVAL_LOCK(*Ts[opline->result.u.var].var.ptr_ptr, &opline->result);
						AI_USE_PTR(Ts[opline->result.u.var].var);
						opline++;
						continue;
					}

					get_incdec_op(incdec_op, opline->opcode);

					switch (opline->opcode) {
						case ZEND_POST_INC:
						case ZEND_POST_DEC:
							Ts[opline->result.u.var].tmp_var = **var_ptr;
							zendi_zval_copy_ctor(Ts[opline->result.u.var].tmp_var);
							break;
					}
					
					SEPARATE_ZVAL_IF_NOT_REF(var_ptr);

					incdec_op(*var_ptr);
					switch (opline->opcode) {
						case ZEND_PRE_INC:
						case ZEND_PRE_DEC:
							Ts[opline->result.u.var].var.ptr_ptr = var_ptr;
							SELECTIVE_PZVAL_LOCK(*var_ptr, &opline->result);
							AI_USE_PTR(Ts[opline->result.u.var].var);
							break;
					}
				}
				NEXT_OPCODE();
			case ZEND_PRINT:
				zend_print_variable(get_zval_ptr(&opline->op1, Ts, &EG(free_op1), BP_VAR_R));
				Ts[opline->result.u.var].tmp_var.value.lval = 1;
				Ts[opline->result.u.var].tmp_var.type = IS_LONG;
				FREE_OP(&opline->op1, EG(free_op1));
				NEXT_OPCODE();
			case ZEND_ECHO:
				zend_print_variable(get_zval_ptr(&opline->op1, Ts, &EG(free_op1), BP_VAR_R));
				FREE_OP(&opline->op1, EG(free_op1));
				NEXT_OPCODE();
			case ZEND_FETCH_R:
				zend_fetch_var_address(&opline->result, &opline->op1, &opline->op2, Ts, BP_VAR_R ELS_CC);
				AI_USE_PTR(Ts[opline->result.u.var].var);
				NEXT_OPCODE();
			case ZEND_FETCH_W:
				zend_fetch_var_address(&opline->result, &opline->op1, &opline->op2, Ts, BP_VAR_W ELS_CC);
				NEXT_OPCODE();
			case ZEND_FETCH_RW:
				zend_fetch_var_address(&opline->result, &opline->op1, &opline->op2, Ts, BP_VAR_RW ELS_CC);
				NEXT_OPCODE();
			case ZEND_FETCH_FUNC_ARG:
				if (ARG_SHOULD_BE_SENT_BY_REF(opline->extended_value, fbc, fbc->common.arg_types)) {
					/* Behave like FETCH_W */
					zend_fetch_var_address(&opline->result, &opline->op1, &opline->op2, Ts, BP_VAR_W ELS_CC);
				} else {
					/* Behave like FETCH_R */
					zend_fetch_var_address(&opline->result, &opline->op1, &opline->op2, Ts, BP_VAR_R ELS_CC);
					AI_USE_PTR(Ts[opline->result.u.var].var);
				}
				NEXT_OPCODE();
			case ZEND_FETCH_UNSET:
				zend_fetch_var_address(&opline->result, &opline->op1, &opline->op2, Ts, BP_VAR_R ELS_CC);
				PZVAL_UNLOCK(*Ts[opline->result.u.var].var.ptr_ptr);
				if (Ts[opline->result.u.var].var.ptr_ptr != &EG(uninitialized_zval_ptr)) {
					SEPARATE_ZVAL_IF_NOT_REF(Ts[opline->result.u.var].var.ptr_ptr);
				}
				PZVAL_LOCK(*Ts[opline->result.u.var].var.ptr_ptr);
				NEXT_OPCODE();
			case ZEND_FETCH_IS:
				zend_fetch_var_address(&opline->result, &opline->op1, &opline->op2, Ts, BP_VAR_IS ELS_CC);
				AI_USE_PTR(Ts[opline->result.u.var].var);
				NEXT_OPCODE();
			case ZEND_FETCH_DIM_R:
				if (opline->extended_value == ZEND_FETCH_ADD_LOCK) {
					PZVAL_LOCK(*Ts[opline->op1.u.var].var.ptr_ptr);
				}
				zend_fetch_dimension_address(&opline->result, &opline->op1, &opline->op2, Ts, BP_VAR_R ELS_CC);
				AI_USE_PTR(Ts[opline->result.u.var].var);
				NEXT_OPCODE();
			case ZEND_FETCH_DIM_W:
				zend_fetch_dimension_address(&opline->result, &opline->op1, &opline->op2, Ts, BP_VAR_W ELS_CC);
				NEXT_OPCODE();
			case ZEND_FETCH_DIM_RW:
				zend_fetch_dimension_address(&opline->result, &opline->op1, &opline->op2, Ts, BP_VAR_RW ELS_CC);
				NEXT_OPCODE();
			case ZEND_FETCH_DIM_IS:
				zend_fetch_dimension_address(&opline->result, &opline->op1, &opline->op2, Ts, BP_VAR_IS ELS_CC);
				AI_USE_PTR(Ts[opline->result.u.var].var);
				NEXT_OPCODE();
			case ZEND_FETCH_DIM_FUNC_ARG:
				if (ARG_SHOULD_BE_SENT_BY_REF(opline->extended_value, fbc, fbc->common.arg_types)) {
					/* Behave like FETCH_DIM_W */
					zend_fetch_dimension_address(&opline->result, &opline->op1, &opline->op2, Ts, BP_VAR_W ELS_CC);
				} else {
					/* Behave like FETCH_DIM_R, except for locking used for list() */
					zend_fetch_dimension_address(&opline->result, &opline->op1, &opline->op2, Ts, BP_VAR_R ELS_CC);
					AI_USE_PTR(Ts[opline->result.u.var].var);
				}
				NEXT_OPCODE();
			case ZEND_FETCH_DIM_UNSET:
				/* Not needed in DIM_UNSET
				if (opline->extended_value == ZEND_FETCH_ADD_LOCK) {
					PZVAL_LOCK(*Ts[opline->op1.u.var].var.ptr_ptr);
				}
				*/
				zend_fetch_dimension_address(&opline->result, &opline->op1, &opline->op2, Ts, BP_VAR_R ELS_CC);
				PZVAL_UNLOCK(*Ts[opline->result.u.var].var.ptr_ptr);
				if (Ts[opline->result.u.var].var.ptr_ptr != &EG(uninitialized_zval_ptr)) {
					SEPARATE_ZVAL_IF_NOT_REF(Ts[opline->result.u.var].var.ptr_ptr);
				}
				PZVAL_LOCK(*Ts[opline->result.u.var].var.ptr_ptr);
				NEXT_OPCODE();
			case ZEND_FETCH_OBJ_R:
				zend_fetch_property_address(&opline->result, &opline->op1, &opline->op2, Ts, BP_VAR_R ELS_CC);
				AI_USE_PTR(Ts[opline->result.u.var].var);
				NEXT_OPCODE();
			case ZEND_FETCH_OBJ_W:
				zend_fetch_property_address(&opline->result, &opline->op1, &opline->op2, Ts, BP_VAR_W ELS_CC);
				NEXT_OPCODE();
			case ZEND_FETCH_OBJ_RW:
				zend_fetch_property_address(&opline->result, &opline->op1, &opline->op2, Ts, BP_VAR_RW ELS_CC);
				NEXT_OPCODE();
			case ZEND_FETCH_OBJ_IS:
				zend_fetch_property_address(&opline->result, &opline->op1, &opline->op2, Ts, BP_VAR_IS ELS_CC);
				AI_USE_PTR(Ts[opline->result.u.var].var);
				NEXT_OPCODE();
			case ZEND_FETCH_OBJ_FUNC_ARG:
				if (ARG_SHOULD_BE_SENT_BY_REF(opline->extended_value, fbc, fbc->common.arg_types)) {
					/* Behave like FETCH_OBJ_W */
					zend_fetch_property_address(&opline->result, &opline->op1, &opline->op2, Ts, BP_VAR_W ELS_CC);
				} else {
					zend_fetch_property_address(&opline->result, &opline->op1, &opline->op2, Ts, BP_VAR_R ELS_CC);
					AI_USE_PTR(Ts[opline->result.u.var].var);
				}
				NEXT_OPCODE();
			case ZEND_FETCH_OBJ_UNSET:
				zend_fetch_property_address(&opline->result, &opline->op1, &opline->op2, Ts, BP_VAR_R ELS_CC);

				PZVAL_UNLOCK(*Ts[opline->result.u.var].var.ptr_ptr);
				if (Ts[opline->result.u.var].var.ptr_ptr != &EG(uninitialized_zval_ptr)) {
					SEPARATE_ZVAL_IF_NOT_REF(Ts[opline->result.u.var].var.ptr_ptr);
				}
				PZVAL_LOCK(*Ts[opline->result.u.var].var.ptr_ptr);
				NEXT_OPCODE();
			case ZEND_FETCH_DIM_TMP_VAR:
				zend_fetch_dimension_address_from_tmp_var(&opline->result, &opline->op1, &opline->op2, Ts ELS_CC);
				AI_USE_PTR(Ts[opline->result.u.var].var);
				NEXT_OPCODE();
			case ZEND_ASSIGN: {
					zval *value;
					value = get_zval_ptr(&opline->op2, Ts, &EG(free_op2), BP_VAR_R);

 					zend_assign_to_variable(&opline->result, &opline->op1, &opline->op2, value, (EG(free_op2)?IS_TMP_VAR:opline->op2.op_type), Ts ELS_CC);
					/* zend_assign_to_variable() always takes care of op2, never free it! */
				}
				NEXT_OPCODE();
			case ZEND_ASSIGN_REF:
				zend_assign_to_variable_reference(&opline->result, get_zval_ptr_ptr(&opline->op1, Ts, BP_VAR_W), get_zval_ptr_ptr(&opline->op2, Ts, BP_VAR_W), Ts ELS_CC);
				NEXT_OPCODE();
			case ZEND_JMP:
#if DEBUG_ZEND>=2
				printf("Jumping to %d\n", opline->op1.u.opline_num);
#endif
				opline = &op_array->opcodes[opline->op1.u.opline_num];
				continue;
				break;
			case ZEND_JMPZ: {
					znode *op1 = &opline->op1;
					
					if (!i_zend_is_true(get_zval_ptr(op1, Ts, &EG(free_op1), BP_VAR_R))) {
#if DEBUG_ZEND>=2
						printf("Conditional jmp to %d\n", opline->op2.u.opline_num);
#endif
						opline = &op_array->opcodes[opline->op2.u.opline_num];
						FREE_OP(op1, EG(free_op1));
						continue;
					}
					FREE_OP(op1, EG(free_op1));
				}
				NEXT_OPCODE();
			case ZEND_JMPNZ: {
					znode *op1 = &opline->op1;
					
					if (zend_is_true(get_zval_ptr(op1, Ts, &EG(free_op1), BP_VAR_R))) {
#if DEBUG_ZEND>=2
						printf("Conditional jmp to %d\n", opline->op2.u.opline_num);
#endif
						opline = &op_array->opcodes[opline->op2.u.opline_num];
						FREE_OP(op1, EG(free_op1));
						continue;
					}
					FREE_OP(op1, EG(free_op1));
				}
				NEXT_OPCODE();
			case ZEND_JMPZNZ: {
					znode *res = &opline->op1;
					
					if (zend_is_true(get_zval_ptr(res, Ts, &EG(free_op1), BP_VAR_R))) {
#if DEBUG_ZEND>=2
						printf("Conditional jmp on true to %d\n", opline->extended_value);
#endif
						opline = &op_array->opcodes[opline->extended_value];
					} else {
#if DEBUG_ZEND>=2
						printf("Conditional jmp on false to %d\n", opline->op2.u.opline_num);
#endif
						opline = &op_array->opcodes[opline->op2.u.opline_num];
					}
					FREE_OP(res, EG(free_op1));
				}
				continue;
				break;
			case ZEND_JMPZ_EX: {
					zend_op *original_opline = opline;
					int retval = zend_is_true(get_zval_ptr(&original_opline->op1, Ts, &EG(free_op1), BP_VAR_R));
					
					FREE_OP(&original_opline->op1, EG(free_op1));
					Ts[original_opline->result.u.var].tmp_var.value.lval = retval;
					Ts[original_opline->result.u.var].tmp_var.type = IS_LONG;
					if (!retval) {
#if DEBUG_ZEND>=2
						printf("Conditional jmp to %d\n", opline->op2.u.opline_num);
#endif
						opline = &op_array->opcodes[opline->op2.u.opline_num];
						continue;
					}
				}
				NEXT_OPCODE();
			case ZEND_JMPNZ_EX: {
					zend_op *original_opline = opline;
					int retval = zend_is_true(get_zval_ptr(&original_opline->op1, Ts, &EG(free_op1), BP_VAR_R));
					
					FREE_OP(&original_opline->op1, EG(free_op1));
					Ts[original_opline->result.u.var].tmp_var.value.lval = retval;
					Ts[original_opline->result.u.var].tmp_var.type = IS_LONG;
					if (retval) {
#if DEBUG_ZEND>=2
						printf("Conditional jmp to %d\n", opline->op2.u.opline_num);
#endif
						opline = &op_array->opcodes[opline->op2.u.opline_num];
						continue;
					}
				}
				NEXT_OPCODE();
			case ZEND_FREE:
				zendi_zval_dtor(Ts[opline->op1.u.var].tmp_var);
				NEXT_OPCODE();
			case ZEND_INIT_STRING:
				Ts[opline->result.u.var].tmp_var.value.str.val = emalloc(1);
				Ts[opline->result.u.var].tmp_var.value.str.val[0] = 0;
				Ts[opline->result.u.var].tmp_var.value.str.len = 0;
				Ts[opline->result.u.var].tmp_var.refcount = 1;
				NEXT_OPCODE();
			case ZEND_ADD_CHAR:
				add_char_to_string(	&Ts[opline->result.u.var].tmp_var,
									get_zval_ptr(&opline->op1, Ts, &EG(free_op1), BP_VAR_NA),
									&opline->op2.u.constant);
				/* FREE_OP is missing intentionally here - we're always working on the same temporary variable */
				NEXT_OPCODE();
			case ZEND_ADD_STRING:
				add_string_to_string(	&Ts[opline->result.u.var].tmp_var,
										get_zval_ptr(&opline->op1, Ts, &EG(free_op1), BP_VAR_NA),
										&opline->op2.u.constant);
				/* FREE_OP is missing intentionally here - we're always working on the same temporary variable */
				NEXT_OPCODE();
			case ZEND_ADD_VAR: {
					zval *var = get_zval_ptr(&opline->op2, Ts, &EG(free_op2), BP_VAR_R);
					zval var_copy;
					int use_copy;

					zend_make_printable_zval(var, &var_copy, &use_copy);
					if (use_copy) {
						var = &var_copy;
					}
					add_string_to_string(	&Ts[opline->result.u.var].tmp_var,
											get_zval_ptr(&opline->op1, Ts, &EG(free_op1), BP_VAR_NA),
											var);
					if (use_copy) {
						zval_dtor(var);
					}
					/* original comment, possibly problematic:
					 * FREE_OP is missing intentionally here - we're always working on the same temporary variable
					 * (Zeev):  I don't think it's problematic, we only use variables
					 * which aren't affected by FREE_OP()'s anyway, unless they're
					 * string offsets or overloaded objects
					 */
					FREE_OP(&opline->op2, EG(free_op2));
				}
				NEXT_OPCODE();
			case ZEND_INIT_FCALL_BY_NAME: {
					zval *function_name;
					zend_function *function;
					HashTable *active_function_table;
					zval tmp;

					zend_ptr_stack_n_push(&EG(arg_types_stack), 2, fbc, object.ptr);
					if (opline->extended_value & ZEND_CTOR_CALL) {
						/* constructor call */

						if (opline->op1.op_type == IS_VAR) {
							SELECTIVE_PZVAL_LOCK(*Ts[opline->op1.u.var].var.ptr_ptr, &opline->op1);
						}
						if (opline->op2.op_type == IS_VAR) {
							PZVAL_LOCK(*Ts[opline->op2.u.var].var.ptr_ptr);
						}
					}
					function_name = get_zval_ptr(&opline->op2, Ts, &EG(free_op2), BP_VAR_R);

					tmp = *function_name;
					zval_copy_ctor(&tmp);
					convert_to_string(&tmp);
					function_name = &tmp;
					zend_str_tolower(tmp.value.str.val, tmp.value.str.len);
						
					if (opline->op1.op_type != IS_UNUSED) {
						if (opline->op1.op_type==IS_CONST) { /* used for class_name::function() */
							zend_class_entry *ce;
							zval **object_ptr_ptr;

							if (zend_hash_find(EG(active_symbol_table), "this", sizeof("this"), (void **) &object_ptr_ptr)==FAILURE) {
								object.ptr=NULL;
							} else {
								/* We assume that "this" is already is_ref and pointing to the object.
								   If it isn't then tough */
								object.ptr = *object_ptr_ptr;
								object.ptr->refcount++; /* For this pointer */
							}
							if (zend_hash_find(EG(class_table), opline->op1.u.constant.value.str.val, opline->op1.u.constant.value.str.len+1, (void **) &ce)==FAILURE) { /* class doesn't exist */
								/* test for parent:: special case - if it doesn't exist - error out */
								if (opline->op1.u.constant.value.str.len!=(sizeof("parent")-1)
									|| memcmp(opline->op1.u.constant.value.str.val, "parent", sizeof("parent")-1)!=0
									|| !(ce = object.ptr->value.obj.ce->parent)) {
									zend_error(E_ERROR, "Undefined class name '%s'", opline->op1.u.constant.value.str.val);
								}
							}
							active_function_table = &ce->function_table;
						} else { /* used for member function calls */
							object.ptr = _get_object_zval_ptr(&opline->op1, Ts, &EG(free_op1) ELS_CC);
							
							if ((!object.ptr && Ts[opline->op1.u.var].EA.type==IS_OVERLOADED_OBJECT)								
								|| ((object.ptr && object.ptr->type==IS_OBJECT) && (object.ptr->value.obj.ce->handle_function_call))) { /* overloaded function call */
								zend_overloaded_element overloaded_element;

								overloaded_element.element = *function_name;
								overloaded_element.type = OE_IS_METHOD;

								if (object.ptr) {
									Ts[opline->op1.u.var].EA.data.overloaded_element.object = object.ptr;
									Ts[opline->op1.u.var].EA.data.overloaded_element.type = BP_VAR_NA;
									Ts[opline->op1.u.var].EA.data.overloaded_element.elements_list = (zend_llist *) emalloc(sizeof(zend_llist));
									zend_llist_init(Ts[opline->op1.u.var].EA.data.overloaded_element.elements_list, sizeof(zend_overloaded_element), NULL, 0);
								}
								zend_llist_add_element(Ts[opline->op1.u.var].EA.data.overloaded_element.elements_list, &overloaded_element);
								fbc = (zend_function *) emalloc(sizeof(zend_function));
								fbc->type = ZEND_OVERLOADED_FUNCTION;
								fbc->common.arg_types = NULL;
								fbc->overloaded_function.var = opline->op1.u.var;
								goto overloaded_function_call_cont;
							}

							if (!object.ptr || object.ptr->type != IS_OBJECT) {
								zend_error(E_ERROR, "Call to a member function on a non-object");
							}
							if (!object.ptr->is_ref && object.ptr->refcount > 1) {
								zend_error(E_ERROR, "Bug: Problem in method call\n");
							}
							object.ptr->is_ref=1;
							object.ptr->refcount++; /* For $this pointer */
							active_function_table = &(object.ptr->value.obj.ce->function_table);
						}
					} else { /* function pointer */
						object.ptr = NULL;
						active_function_table = EG(function_table);
					}
					if (zend_hash_find(active_function_table, function_name->value.str.val, function_name->value.str.len+1, (void **) &function)==FAILURE) {
						zend_error(E_ERROR, "Call to undefined function:  %s()", function_name->value.str.val);
					}
					zval_dtor(&tmp);
					fbc = function;
overloaded_function_call_cont:
					FREE_OP(&opline->op2, EG(free_op2));
				}
				NEXT_OPCODE();
			case ZEND_DO_FCALL_BY_NAME:
				function_state.function = fbc;
				goto do_fcall_common;
			case ZEND_DO_FCALL: {
					zval *fname = get_zval_ptr(&opline->op1, Ts, &EG(free_op1), BP_VAR_R);

					if (zend_hash_find(EG(function_table), fname->value.str.val, fname->value.str.len+1, (void **) &function_state.function)==FAILURE) {
						zend_error(E_ERROR, "Unknown function:  %s()\n", fname->value.str.val);
					}
					FREE_OP(&opline->op1, EG(free_op1));
					zend_ptr_stack_push(&EG(arg_types_stack), object.ptr);
					object.ptr = NULL;
					goto do_fcall_common;
				}
do_fcall_common:
				{
					zval **original_return_value;
					int return_value_used = RETURN_VALUE_USED(opline);

					zend_ptr_stack_n_push(&EG(argument_stack), 2, (void *) opline->extended_value, NULL);

					Ts[opline->result.u.var].var.ptr_ptr = &Ts[opline->result.u.var].var.ptr;

					if (function_state.function->type==ZEND_INTERNAL_FUNCTION) {	
						ALLOC_ZVAL(Ts[opline->result.u.var].var.ptr);
						INIT_ZVAL(*(Ts[opline->result.u.var].var.ptr));
						((zend_internal_function *) function_state.function)->handler(opline->extended_value, Ts[opline->result.u.var].var.ptr, object.ptr, return_value_used ELS_CC);
						if (object.ptr) {
							object.ptr->refcount--;
						}
						Ts[opline->result.u.var].var.ptr->is_ref = 0;
						Ts[opline->result.u.var].var.ptr->refcount = 1;
						if (!return_value_used) {
							zval_ptr_dtor(&Ts[opline->result.u.var].var.ptr);
						}
					} else if (function_state.function->type==ZEND_USER_FUNCTION) {
						HashTable *calling_symbol_table;

						Ts[opline->result.u.var].var.ptr = NULL;
						if (EG(symtable_cache_ptr)>=EG(symtable_cache)) {
							/*printf("Cache hit!  Reusing %x\n", symtable_cache[symtable_cache_ptr]);*/
							function_state.function_symbol_table = *(EG(symtable_cache_ptr)--);
						} else {
							ALLOC_HASHTABLE(function_state.function_symbol_table);
							zend_hash_init(function_state.function_symbol_table, 0, NULL, ZVAL_PTR_DTOR, 0);
							/*printf("Cache miss!  Initialized %x\n", function_state.function_symbol_table);*/
						}
						calling_symbol_table = EG(active_symbol_table);
						EG(active_symbol_table) = function_state.function_symbol_table;
						if (opline->opcode==ZEND_DO_FCALL_BY_NAME
							&& object.ptr
							&& fbc->type!=ZEND_OVERLOADED_FUNCTION) {
							zval **this_ptr;
							zval *null_ptr = NULL;

							zend_hash_update(function_state.function_symbol_table, "this", sizeof("this"), &null_ptr, sizeof(zval *), (void **) &this_ptr);
							if (!PZVAL_IS_REF(object.ptr)) {
								zend_error(E_WARNING, "Problem with method call - please report this bug");
                			}
							*this_ptr = object.ptr;
							object.ptr = NULL;
						}
						original_return_value = EG(return_value_ptr_ptr);
						EG(return_value_ptr_ptr) = Ts[opline->result.u.var].var.ptr_ptr;
						EG(active_op_array) = (zend_op_array *) function_state.function;

						zend_execute(EG(active_op_array) ELS_CC);

						if (return_value_used && !Ts[opline->result.u.var].var.ptr) {
							ALLOC_ZVAL(Ts[opline->result.u.var].var.ptr);
							INIT_ZVAL(*Ts[opline->result.u.var].var.ptr);
						} else if (!return_value_used && Ts[opline->result.u.var].var.ptr) {
							zval_ptr_dtor(&Ts[opline->result.u.var].var.ptr);
						}
						EG(opline_ptr) = &opline;
						EG(active_op_array) = op_array;
						EG(return_value_ptr_ptr)=original_return_value;
						if (EG(symtable_cache_ptr)>=EG(symtable_cache_limit)) {
							zend_hash_destroy(function_state.function_symbol_table);
							FREE_HASHTABLE(function_state.function_symbol_table);
						} else {
							*(++EG(symtable_cache_ptr)) = function_state.function_symbol_table;
							zend_hash_clean(*EG(symtable_cache_ptr));
						}
						EG(active_symbol_table) = calling_symbol_table;
					} else { /* ZEND_OVERLOADED_FUNCTION */
						ALLOC_ZVAL(Ts[opline->result.u.var].var.ptr);
						INIT_ZVAL(*(Ts[opline->result.u.var].var.ptr));
						call_overloaded_function(&Ts[fbc->overloaded_function.var], opline->extended_value, Ts[opline->result.u.var].var.ptr ELS_CC);
						efree(fbc);
						if (!return_value_used) {
							zval_ptr_dtor(&Ts[opline->result.u.var].var.ptr);
						}
					}
					if (opline->opcode == ZEND_DO_FCALL_BY_NAME) {
						zend_ptr_stack_n_pop(&EG(arg_types_stack), 2, &object.ptr, &fbc);
					} else {
						object.ptr = zend_ptr_stack_pop(&EG(arg_types_stack));
					}
					function_state.function = (zend_function *) op_array;
					EG(function_state_ptr) = &function_state;
					zend_ptr_stack_clear_multiple(ELS_C);
				}
				NEXT_OPCODE();
			case ZEND_RETURN: {
					zval *retval_ptr;
					zval **retval_ptr_ptr;
					
					if ((EG(active_op_array)->return_reference == ZEND_RETURN_REF) &&
						(opline->op1.op_type != IS_CONST) && 
						(opline->op1.op_type != IS_TMP_VAR)) {
						
						retval_ptr_ptr = get_zval_ptr_ptr(&opline->op1, Ts, BP_VAR_W);

						SEPARATE_ZVAL_TO_MAKE_IS_REF(retval_ptr_ptr);
						
						(*retval_ptr_ptr)->refcount++;
						(*EG(return_value_ptr_ptr)) = (*retval_ptr_ptr);
					} else {
						retval_ptr = get_zval_ptr(&opline->op1, Ts, &EG(free_op1), BP_VAR_R);
					
						if (!EG(free_op1)) { /* Not a temp var */
							if (PZVAL_IS_REF(retval_ptr) && retval_ptr->refcount > 0) {
								ALLOC_ZVAL(*(EG(return_value_ptr_ptr)));
								**EG(return_value_ptr_ptr) = *retval_ptr;
								(*EG(return_value_ptr_ptr))->is_ref = 0;
								(*EG(return_value_ptr_ptr))->refcount = 1;
								zval_copy_ctor(*EG(return_value_ptr_ptr));
							} else {
								*EG(return_value_ptr_ptr) = retval_ptr;
								retval_ptr->refcount++;
							}
						} else {
							ALLOC_ZVAL(*(EG(return_value_ptr_ptr)));
							**EG(return_value_ptr_ptr) = *retval_ptr;
							(*EG(return_value_ptr_ptr))->refcount = 1;
							(*EG(return_value_ptr_ptr))->is_ref = 0;
						}
					}
#if SUPPORT_INTERACTIVE
					op_array->last_executed_op_number = opline-op_array->opcodes;
#endif
					free_alloca(Ts);
					EG(in_execution) = original_in_execution;
					return;
				}
				break;
			case ZEND_SEND_VAL: 
				if (opline->extended_value==ZEND_DO_FCALL_BY_NAME
					&& ARG_SHOULD_BE_SENT_BY_REF(opline->op2.u.opline_num, fbc, fbc->common.arg_types)) {
						zend_error(E_ERROR, "Cannot pass parameter %d by reference", opline->op2.u.opline_num);
				}
				{
					zval *valptr;
					zval *value;

					value = get_zval_ptr(&opline->op1, Ts, &EG(free_op1), BP_VAR_R);

					ALLOC_ZVAL(valptr);
					*valptr = *value;
					if (!EG(free_op1)) {
						zval_copy_ctor(valptr);
					}
					INIT_PZVAL(valptr);
					zend_ptr_stack_push(&EG(argument_stack), valptr);
				}
				NEXT_OPCODE();
			case ZEND_SEND_VAR_NO_REF:
				if (opline->extended_value & ZEND_ARG_COMPILE_TIME_BOUND) { /* Had function_ptr at compile_time */
					if (!(opline->extended_value & ZEND_ARG_SEND_BY_REF)) {
						goto send_by_var;
					}
				} else if (!ARG_SHOULD_BE_SENT_BY_REF(opline->op2.u.opline_num, fbc, fbc->common.arg_types)) {
					goto send_by_var;
				}
				{
					zval *varptr;
					varptr = get_zval_ptr(&opline->op1, Ts, &EG(free_op1), BP_VAR_R);

					if (varptr != &EG(uninitialized_zval) && (PZVAL_IS_REF(varptr) || varptr->refcount == 1)) {
						varptr->is_ref = 1;
						varptr->refcount++;
						zend_ptr_stack_push(&EG(argument_stack), varptr);
						NEXT_OPCODE();
					}
					zend_error(E_ERROR, "Only variables can be passed by reference");
				}
				NEXT_OPCODE();
			case ZEND_SEND_VAR:
					if ((opline->extended_value == ZEND_DO_FCALL_BY_NAME)
						&& ARG_SHOULD_BE_SENT_BY_REF(opline->op2.u.opline_num, fbc, fbc->common.arg_types)) {
						goto send_by_ref;
					}
send_by_var:
					{
						zval *varptr;
						varptr = get_zval_ptr(&opline->op1, Ts, &EG(free_op1), BP_VAR_R);

						if (varptr == &EG(uninitialized_zval)) {
							ALLOC_ZVAL(varptr);
							INIT_ZVAL(*varptr);
							varptr->refcount = 0;
						} else if (PZVAL_IS_REF(varptr)) {
							zval *original_var = varptr;

							ALLOC_ZVAL(varptr);
							*varptr = *original_var;
							varptr->is_ref = 0;
							varptr->refcount = 0;
							zval_copy_ctor(varptr);
						}
						varptr->refcount++;
						zend_ptr_stack_push(&EG(argument_stack), varptr);
						FREE_OP(&opline->op1, EG(free_op1));  /* for string offsets */
					}
					NEXT_OPCODE();
send_by_ref:
			case ZEND_SEND_REF: {
					zval **varptr_ptr;
					zval *varptr;
					varptr_ptr = get_zval_ptr_ptr(&opline->op1, Ts, BP_VAR_W);

					if (!varptr_ptr) {
						zend_error(E_ERROR, "Only variables can be passed by reference");
					}

					SEPARATE_ZVAL_TO_MAKE_IS_REF(varptr_ptr);
					varptr = *varptr_ptr;
					varptr->refcount++;
					zend_ptr_stack_push(&EG(argument_stack), varptr);
				}
				NEXT_OPCODE();
			case ZEND_RECV: {
					zval **param;

					if (zend_ptr_stack_get_arg(opline->op1.u.constant.value.lval, (void **) &param ELS_CC)==FAILURE) {
						zend_error(E_WARNING, "Missing argument %d for %s()\n", opline->op1.u.constant.value.lval, get_active_function_name());
						if (opline->result.op_type == IS_VAR) {
							PZVAL_UNLOCK(*Ts[opline->result.u.var].var.ptr_ptr);
						}
					} else if (PZVAL_IS_REF(*param)) {
						zend_assign_to_variable_reference(NULL, get_zval_ptr_ptr(&opline->result, Ts, BP_VAR_W), param, NULL ELS_CC);
					} else {
						zend_assign_to_variable(NULL, &opline->result, NULL, *param, IS_VAR, Ts ELS_CC);
					}
				}
				NEXT_OPCODE();
			case ZEND_RECV_INIT: {
					zval **param, *assignment_value;

					if (zend_ptr_stack_get_arg(opline->op1.u.constant.value.lval, (void **) &param ELS_CC)==FAILURE) {
						if (opline->op2.u.constant.type == IS_CONSTANT || opline->op2.u.constant.type==IS_CONSTANT_ARRAY) {
							zval *default_value;

							ALLOC_ZVAL(default_value);
							*default_value = opline->op2.u.constant;
							if (opline->op2.u.constant.type==IS_CONSTANT_ARRAY) {
								zval_copy_ctor(default_value);
							}
							default_value->refcount=1;
							zval_update_constant(&default_value, 0);
							default_value->refcount=0;
							default_value->is_ref=0;
							param = &default_value;
							assignment_value = default_value;
						} else {
							param = NULL;
							assignment_value = &opline->op2.u.constant;
						}
					} else {
						assignment_value = *param;
					}

					if (PZVAL_IS_REF(assignment_value) && param) {
						zend_assign_to_variable_reference(NULL, get_zval_ptr_ptr(&opline->result, Ts, BP_VAR_W), param, NULL ELS_CC);
					} else {
						zend_assign_to_variable(NULL, &opline->result, NULL, assignment_value, IS_VAR, Ts ELS_CC);
					}
				}
				NEXT_OPCODE();
			case ZEND_BOOL:
				/* PHP 3.0 returned "" for false and 1 for true, here we use 0 and 1 for now */
				Ts[opline->result.u.var].tmp_var.value.lval = zend_is_true(get_zval_ptr(&opline->op1, Ts, &EG(free_op1), BP_VAR_R));
				Ts[opline->result.u.var].tmp_var.type = IS_LONG;
				FREE_OP(&opline->op1, EG(free_op1));
				NEXT_OPCODE();
			case ZEND_BRK:
			case ZEND_CONT: {
					zval *nest_levels_zval = get_zval_ptr(&opline->op2, Ts, &EG(free_op2), BP_VAR_R);
					zval tmp;
					int array_offset, nest_levels, original_nest_levels;
					zend_brk_cont_element *jmp_to;

					if (nest_levels_zval->type != IS_LONG) {
						tmp = *nest_levels_zval;
						zval_copy_ctor(&tmp);
						convert_to_long(&tmp);
						nest_levels = tmp.value.lval;
					} else {
						nest_levels = nest_levels_zval->value.lval;
					}
					original_nest_levels = nest_levels;
					array_offset = opline->op1.u.opline_num;
					do {
						if (array_offset==-1) {
							zend_error(E_ERROR, "Cannot break/continue %d levels\n", original_nest_levels);
						}
						jmp_to = &op_array->brk_cont_array[array_offset];
						if (nest_levels>1) {
							zend_op *brk_opline = &op_array->opcodes[jmp_to->brk];

							switch (brk_opline->opcode) {
								case ZEND_SWITCH_FREE:
									zend_switch_free(brk_opline, Ts ELS_CC);
									break;
								case ZEND_FREE:
									zendi_zval_dtor(Ts[brk_opline->op1.u.var].tmp_var);
									break;
							}
						}
						array_offset = jmp_to->parent;
					} while (--nest_levels > 0);

					if (opline->opcode == ZEND_BRK) {
						opline = op_array->opcodes+jmp_to->brk;
					} else {
						opline = op_array->opcodes+jmp_to->cont;
					}
					FREE_OP(&opline->op2, EG(free_op2));
					continue;
				}
				/* Never reaches this point */
			case ZEND_CASE: {
					int switch_expr_is_overloaded=0;

					if (opline->op1.op_type==IS_VAR) {
						if (Ts[opline->op1.u.var].var.ptr_ptr) {
							PZVAL_LOCK(*Ts[opline->op1.u.var].var.ptr_ptr);
						} else {
							switch_expr_is_overloaded = 1;
							if (Ts[opline->op1.u.var].EA.type==IS_STRING_OFFSET) {
								Ts[opline->op1.u.var].EA.data.str_offset.str->refcount++;
							}
						}
					}
					is_equal_function(&Ts[opline->result.u.var].tmp_var, 
								 get_zval_ptr(&opline->op1, Ts, &EG(free_op1), BP_VAR_R),
								 get_zval_ptr(&opline->op2, Ts, &EG(free_op2), BP_VAR_R));

					FREE_OP(&opline->op2, EG(free_op2));
					if (switch_expr_is_overloaded) {
						/* We only free op1 if this is a string offset,
						 * Since if it is a TMP_VAR, it'll be reused by
						 * other CASE opcodes (whereas string offsets
						 * are allocated at each get_zval_ptr())
						 */
						FREE_OP(&opline->op1, EG(free_op1));
						Ts[opline->op1.u.var].var.ptr_ptr = NULL;
						AI_USE_PTR(Ts[opline->op1.u.var].var);
					}
				}
				NEXT_OPCODE();
			case ZEND_SWITCH_FREE:
				zend_switch_free(opline, Ts ELS_CC);
				NEXT_OPCODE();
			case ZEND_NEW: {
					zval *tmp = get_zval_ptr(&opline->op1, Ts, &EG(free_op1), BP_VAR_R);
					zval class_name;
					zend_class_entry *ce;

					class_name = *tmp;
					zval_copy_ctor(&class_name);
					convert_to_string(&class_name);
					zend_str_tolower(class_name.value.str.val, class_name.value.str.len);

					if (zend_hash_find(EG(class_table), class_name.value.str.val, class_name.value.str.len+1, (void **) &ce)==FAILURE) {
						zend_error(E_ERROR, "Cannot instantiate non-existent class:  %s", class_name.value.str.val);
					}
					Ts[opline->result.u.var].var.ptr_ptr = &Ts[opline->result.u.var].var.ptr;
					ALLOC_ZVAL(Ts[opline->result.u.var].var.ptr);
					object_init_ex(Ts[opline->result.u.var].var.ptr, ce);
					Ts[opline->result.u.var].var.ptr->refcount=1;
					Ts[opline->result.u.var].var.ptr->is_ref=1;
					zval_dtor(&class_name);
					FREE_OP(&opline->op1, EG(free_op1));
				}
				NEXT_OPCODE();
			case ZEND_FETCH_CONSTANT:
				if (!zend_get_constant(opline->op1.u.constant.value.str.val, opline->op1.u.constant.value.str.len, &Ts[opline->result.u.var].tmp_var)) {
					zend_error(E_NOTICE, "Use of undefined constant %s - assumed '%s'",
								opline->op1.u.constant.value.str.val,
								opline->op1.u.constant.value.str.val);
					Ts[opline->result.u.var].tmp_var = opline->op1.u.constant;
					zval_copy_ctor(&Ts[opline->result.u.var].tmp_var);
				}
				NEXT_OPCODE();
			case ZEND_INIT_ARRAY:
			case ZEND_ADD_ARRAY_ELEMENT: {
					zval *array_ptr = &Ts[opline->result.u.var].tmp_var;
					zval *expr_ptr, **expr_ptr_ptr = NULL;
					zval *offset=get_zval_ptr(&opline->op2, Ts, &EG(free_op2), BP_VAR_R);

					if (opline->extended_value) {
						expr_ptr_ptr=get_zval_ptr_ptr(&opline->op1, Ts, BP_VAR_R);
						expr_ptr = *expr_ptr_ptr;
					} else {
						expr_ptr=get_zval_ptr(&opline->op1, Ts, &EG(free_op1), BP_VAR_R);
					}
					
					if (opline->opcode==ZEND_INIT_ARRAY) {
						array_init(array_ptr);
						if (!expr_ptr) {
							NEXT_OPCODE();
						}
					}
					if (!opline->extended_value && EG(free_op1)) { /* temporary variable */
						zval *new_expr;

						ALLOC_ZVAL(new_expr);
						*new_expr = *expr_ptr;
						expr_ptr = new_expr;
						INIT_PZVAL(expr_ptr);
					} else {
						if (opline->extended_value) {
							SEPARATE_ZVAL_TO_MAKE_IS_REF(expr_ptr_ptr);
							expr_ptr = *expr_ptr_ptr;
							expr_ptr->refcount++;
						} else if (PZVAL_IS_REF(expr_ptr)) {
							zval *new_expr;

							ALLOC_ZVAL(new_expr);
							*new_expr = *expr_ptr;
							expr_ptr = new_expr;
							zendi_zval_copy_ctor(*expr_ptr);
							INIT_PZVAL(expr_ptr);
						} else {
							expr_ptr->refcount++;
						}
					}
					if (offset) {
						switch(offset->type) {
							case IS_DOUBLE:
								zend_hash_index_update(array_ptr->value.ht, (long) offset->value.dval, &expr_ptr, sizeof(zval *), NULL);
								break;
							case IS_LONG:
								zend_hash_index_update(array_ptr->value.ht, offset->value.lval, &expr_ptr, sizeof(zval *), NULL);
								break;
							case IS_STRING:
								zend_hash_update(array_ptr->value.ht, offset->value.str.val, offset->value.str.len+1, &expr_ptr, sizeof(zval *), NULL);
								break;
							case IS_NULL:
								zend_hash_update(array_ptr->value.ht, "", sizeof(""), &expr_ptr, sizeof(zval *), NULL);
								break;
							default:
								zval_ptr_dtor(&expr_ptr);
								/* do nothing */
								break;
						}
						FREE_OP(&opline->op2, EG(free_op2));
					} else {
						zend_hash_next_index_insert(array_ptr->value.ht, &expr_ptr, sizeof(zval *), NULL);
					}
				}
				NEXT_OPCODE();
			case ZEND_CAST: {
					zval *expr = get_zval_ptr(&opline->op1, Ts, &EG(free_op1), BP_VAR_R);
					zval *result = &Ts[opline->result.u.var].tmp_var;

					*result = *expr;
					if (!EG(free_op1)) {
						zendi_zval_copy_ctor(*result);
					}					
					switch (opline->extended_value) {
						case IS_NULL:
							convert_to_null(result);
							break;
						case IS_BOOL:
							convert_to_boolean(result);
							break;
						case IS_LONG:
							convert_to_long(result);
							break;
						case IS_DOUBLE:
							convert_to_double(result);
							break;
						case IS_STRING:
							convert_to_string(result);
							break;
						case IS_ARRAY:
							convert_to_array(result);
							break;
						case IS_OBJECT:
							convert_to_object(result);
							break;
					}
				}
				NEXT_OPCODE();
			case ZEND_INCLUDE_OR_EVAL: {
					zend_op_array *new_op_array=NULL;
					zval **original_return_value = EG(return_value_ptr_ptr);
					int return_value_used;
					zval *inc_filename = get_zval_ptr(&opline->op1, Ts, &EG(free_op1), BP_VAR_R);
					zval tmp_inc_filename;
					CLS_FETCH();

					if (inc_filename->type!=IS_STRING) {
						tmp_inc_filename = *inc_filename;
						zval_copy_ctor(&tmp_inc_filename);
						convert_to_string(&tmp_inc_filename);
						inc_filename = &tmp_inc_filename;
					}
					
					return_value_used = RETURN_VALUE_USED(opline);

					switch (opline->op2.u.constant.value.lval) {
						case ZEND_INCLUDE_ONCE:
						case ZEND_REQUIRE_ONCE: {
								char *opened_path=NULL;
								int dummy = 1;
								zend_file_handle file_handle;

								file_handle.handle.fp = zend_fopen(inc_filename->value.str.val, &opened_path);
								file_handle.type = ZEND_HANDLE_FP;
								file_handle.filename = inc_filename->value.str.val;
								file_handle.opened_path = opened_path;
								file_handle.free_filename = 0;

								if (file_handle.handle.fp) {
									if (!opened_path || zend_hash_add(&EG(included_files), opened_path, strlen(opened_path)+1, (void *)&dummy, sizeof(int), NULL)==SUCCESS) {
										new_op_array = zend_compile_file(&file_handle, (opline->op2.u.constant.value.lval==ZEND_INCLUDE_ONCE?ZEND_INCLUDE:ZEND_REQUIRE) CLS_CC);
										zend_destroy_file_handle(&file_handle CLS_CC);
										opened_path = NULL; /* zend_destroy_file_handle() already frees it */
									} else {
										fclose(file_handle.handle.fp);
									}
								} else {
									if (opline->op2.u.constant.value.lval==ZEND_INCLUDE_ONCE) {
										zend_message_dispatcher(ZMSG_FAILED_INCLUDE_FOPEN, file_handle.filename);
									} else {
										zend_message_dispatcher(ZMSG_FAILED_REQUIRE_FOPEN, file_handle.filename);
									}
								}
								if (opened_path) {
									efree(opened_path);
								}
								break;
							}
							break;
						case ZEND_INCLUDE:
						case ZEND_REQUIRE:
							new_op_array = compile_filename(opline->op2.u.constant.value.lval, inc_filename CLS_CC ELS_CC);
							break;
						case ZEND_EVAL: {
								char *eval_desc = zend_make_compiled_string_description("eval()'d code");

								new_op_array = compile_string(inc_filename, eval_desc CLS_CC);
								efree(eval_desc);
							}
							break;
						EMPTY_SWITCH_DEFAULT_CASE()
					}
					if (inc_filename==&tmp_inc_filename) {
						zval_dtor(&tmp_inc_filename);
					}
					FREE_OP(&opline->op1, EG(free_op1));
					Ts[opline->result.u.var].var.ptr = NULL;
					Ts[opline->result.u.var].var.ptr_ptr = &Ts[opline->result.u.var].var.ptr;
					if (new_op_array) {
						EG(return_value_ptr_ptr) = Ts[opline->result.u.var].var.ptr_ptr;
						EG(active_op_array) = new_op_array;

						zend_execute(new_op_array ELS_CC);
						
						if (!return_value_used) {
							if (Ts[opline->result.u.var].var.ptr) {
								zval_ptr_dtor(&Ts[opline->result.u.var].var.ptr);
							} 
						} else { /* return value is used */
							if (!Ts[opline->result.u.var].var.ptr) { /* there was no return statement */
								ALLOC_ZVAL(Ts[opline->result.u.var].var.ptr);
								INIT_PZVAL(Ts[opline->result.u.var].var.ptr);
								Ts[opline->result.u.var].var.ptr->value.lval = 1;
								Ts[opline->result.u.var].var.ptr->type = IS_LONG;
							}
						}

						EG(opline_ptr) = &opline;
						EG(active_op_array) = op_array;
						EG(function_state_ptr) = &function_state;
						destroy_op_array(new_op_array);
						efree(new_op_array);
					} else {
						if (return_value_used) {
							ALLOC_ZVAL(Ts[opline->result.u.var].var.ptr);
							INIT_ZVAL(*Ts[opline->result.u.var].var.ptr);
						}
					}
					EG(return_value_ptr_ptr) = original_return_value;
				}
				NEXT_OPCODE();
			case ZEND_UNSET_VAR: {
					zval tmp, *variable = get_zval_ptr(&opline->op1, Ts, &EG(free_op1), BP_VAR_R);

					if (variable->type != IS_STRING) {
						tmp = *variable;
						zval_copy_ctor(&tmp);
						convert_to_string(&tmp);
						variable = &tmp;
					}

					zend_hash_del(EG(active_symbol_table), variable->value.str.val, variable->value.str.len+1);

					if (variable == &tmp) {
						zval_dtor(&tmp);
					}
					FREE_OP(&opline->op1, EG(free_op1));
				}
				NEXT_OPCODE();
			case ZEND_UNSET_DIM_OBJ: {
					zval **container = get_zval_ptr_ptr(&opline->op1, Ts, BP_VAR_R);
					zval *offset = get_zval_ptr(&opline->op2, Ts, &EG(free_op2), BP_VAR_R);

					if (container) {
						HashTable *ht;

						switch ((*container)->type) {
							case IS_ARRAY:
								ht = (*container)->value.ht;
								break;
							case IS_OBJECT:
								ht = (*container)->value.obj.properties;
								break;
							default:
								ht = NULL;
								break;
						}
						if (ht)	{
							switch (offset->type) {
								case IS_DOUBLE:
								case IS_RESOURCE:
								case IS_BOOL: 
								case IS_LONG:
									{
										long index;

										if (offset->type == IS_DOUBLE) {
											index = (long) offset->value.lval;
										} else {
											index = offset->value.lval;
										}
										zend_hash_index_del(ht, index);
										break;
									}
								case IS_STRING:
									zend_hash_del(ht, offset->value.str.val, offset->value.str.len+1);
									break;
								case IS_NULL:
									zend_hash_del(ht, "", sizeof(""));
									break;
								default: 
									zend_error(E_WARNING, "Illegal offset type in unset");
									break;
							}
						}
					} else {
						/* overloaded element */
					}
					FREE_OP(&opline->op2, EG(free_op2));
				}
				NEXT_OPCODE();
			case ZEND_FE_RESET: {
					zval *array_ptr, **array_ptr_ptr;
					HashTable *fe_ht;
					
					if (opline->extended_value) {
						array_ptr_ptr = get_zval_ptr_ptr(&opline->op1, Ts, BP_VAR_R);
						if (array_ptr_ptr == NULL) {
							MAKE_STD_ZVAL(array_ptr);
						} else {
							SEPARATE_ZVAL_IF_NOT_REF(array_ptr_ptr);
							array_ptr = *array_ptr_ptr;
							array_ptr->refcount++;
						}
					} else {
						array_ptr = get_zval_ptr(&opline->op1, Ts, &EG(free_op1), BP_VAR_R);
						if (EG(free_op1)) { /* IS_TMP_VAR */
							zval *tmp;

							ALLOC_ZVAL(tmp);
							*tmp = *array_ptr;
							INIT_PZVAL(tmp);
							array_ptr = tmp;
						} else {
							array_ptr->refcount++;
						}
					}
					PZVAL_LOCK(array_ptr);
					Ts[opline->result.u.var].var.ptr = array_ptr;
					Ts[opline->result.u.var].var.ptr_ptr = &Ts[opline->result.u.var].var.ptr;	

					if ((fe_ht = HASH_OF(array_ptr)) != NULL) {
						/* probably redundant */
						zend_hash_internal_pointer_reset(fe_ht);
					} else {
						/* JMP to the end of foreach - TBD */
					}
				}
				NEXT_OPCODE();
			case ZEND_FE_FETCH: {
					zval *array = get_zval_ptr(&opline->op1, Ts, &EG(free_op1), BP_VAR_R);
					zval *result = &Ts[opline->result.u.var].tmp_var;
					zval **value, *key;
					char *str_key;
					ulong int_key;
					HashTable *fe_ht;

					PZVAL_LOCK(array);

					fe_ht = HASH_OF(array);

					if (!fe_ht) {
						zend_error(E_WARNING, "Invalid argument supplied for foreach()");
						opline = op_array->opcodes+opline->op2.u.opline_num;
						continue;
					} else if (zend_hash_get_current_data(fe_ht, (void **) &value)==FAILURE) {
						opline = op_array->opcodes+opline->op2.u.opline_num;
						continue;
					}
					array_init(result);


					(*value)->refcount++;
					zend_hash_index_update(result->value.ht, 0, value, sizeof(zval *), NULL);

					ALLOC_ZVAL(key);
					INIT_PZVAL(key);
					switch (zend_hash_get_current_key(fe_ht, &str_key, &int_key, 1)) {
						case HASH_KEY_IS_STRING:
							key->value.str.val = str_key;
							key->value.str.len = strlen(str_key);
							key->type = IS_STRING;
							break;
						case HASH_KEY_IS_LONG:
							key->value.lval = int_key;
							key->type = IS_LONG;
							break;
						EMPTY_SWITCH_DEFAULT_CASE()
					}
					zend_hash_index_update(result->value.ht, 1, &key, sizeof(zval *), NULL);
					zend_hash_move_forward(fe_ht);
				}
				NEXT_OPCODE();
			case ZEND_JMP_NO_CTOR: {
					zval *object;

					if (opline->op1.op_type == IS_VAR) {
						PZVAL_LOCK(*Ts[opline->op1.u.var].var.ptr_ptr);
					}
					
					object = get_zval_ptr(&opline->op1, Ts, &EG(free_op1), BP_VAR_R);
					if (!object->value.obj.ce->handle_function_call
						&& !zend_hash_exists(&object->value.obj.ce->function_table, object->value.obj.ce->name, object->value.obj.ce->name_length+1)) {
						opline = op_array->opcodes + opline->op2.u.opline_num;
						continue;
					}
				}
				NEXT_OPCODE();
			case ZEND_ISSET_ISEMPTY: {
					zval **var = get_zval_ptr_ptr(&opline->op1, Ts, BP_VAR_IS);
					int isset;

					if (!var) {
						if (Ts[opline->op1.u.var].EA.type==IS_STRING_OFFSET) {
							if (Ts[opline->op1.u.var].EA.data.str_offset.offset>=0
								&& Ts[opline->op1.u.var].EA.data.str_offset.offset<Ts[opline->op1.u.var].EA.data.str_offset.str->value.str.len) {
								isset = 1;
							} else {
								isset = 0;
							}
						} else {
							isset = 1;
						}
					} else if (*var==EG(uninitialized_zval_ptr)
						|| ((*var)->type == IS_NULL)) {
						isset = 0;
					} else {
						isset = 1;
					}

					switch (opline->op2.u.constant.value.lval) {
						case ZEND_ISSET:
							Ts[opline->result.u.var].tmp_var.value.lval = isset;
							break;
						case ZEND_ISEMPTY:
							if (!var) {
								if (!isset
									|| Ts[opline->op1.u.var].EA.data.str_offset.str->value.str.val[Ts[opline->op1.u.var].EA.data.str_offset.offset]=='0') {
									Ts[opline->result.u.var].tmp_var.value.lval = 1;
								} else {
									Ts[opline->result.u.var].tmp_var.value.lval = 0;
								}
							} else if (!isset || !zend_is_true((*var))) {
								Ts[opline->result.u.var].tmp_var.value.lval = 1;
							} else {
								Ts[opline->result.u.var].tmp_var.value.lval = 0;
							}
							break;
					}
					Ts[opline->result.u.var].tmp_var.type = IS_BOOL;
				}
				NEXT_OPCODE();
			case ZEND_EXIT:
				if (opline->op1.op_type != IS_UNUSED) {
					zval *ptr;

					ptr = get_zval_ptr(&opline->op1, Ts, &EG(free_op1), BP_VAR_R);
					if (Z_TYPE_P(ptr) == IS_LONG) {
						EG(exit_status) = Z_LVAL_P(ptr);
					}
					zend_print_variable(ptr);
					FREE_OP(&opline->op1, EG(free_op1));
				}
				zend_bailout();
				NEXT_OPCODE();
			case ZEND_BEGIN_SILENCE:
				Ts[opline->result.u.var].tmp_var.value.lval = EG(error_reporting);
				Ts[opline->result.u.var].tmp_var.type = IS_LONG;  /* shouldn't be necessary */
				EG(error_reporting) = 0;
				NEXT_OPCODE();
			case ZEND_END_SILENCE:
				EG(error_reporting) = Ts[opline->op1.u.var].tmp_var.value.lval;
				NEXT_OPCODE();
			case ZEND_QM_ASSIGN: {
					zval *value = get_zval_ptr(&opline->op1, Ts, &EG(free_op1), BP_VAR_R);

					Ts[opline->result.u.var].tmp_var = *value;
					if (!EG(free_op1)) {
						zval_copy_ctor(&Ts[opline->result.u.var].tmp_var);
					}
				}
				NEXT_OPCODE();
			case ZEND_EXT_STMT: 
				if (!EG(no_extensions)) {
					zend_llist_apply_with_argument(&zend_extensions, (void (*)(void *, void *)) zend_extension_statement_handler, op_array);
				}
				NEXT_OPCODE();
			case ZEND_EXT_FCALL_BEGIN:
				if (!EG(no_extensions)) {
					zend_llist_apply_with_argument(&zend_extensions, (void (*)(void *, void *)) zend_extension_fcall_begin_handler, op_array);
				}
				NEXT_OPCODE();
			case ZEND_EXT_FCALL_END:
				if (!EG(no_extensions)) {
					zend_llist_apply_with_argument(&zend_extensions, (void (*)(void *, void *)) zend_extension_fcall_end_handler, op_array);
				}
				NEXT_OPCODE();
			case ZEND_DECLARE_FUNCTION_OR_CLASS:
				do_bind_function_or_class(opline, EG(function_table), EG(class_table), 0);
				NEXT_OPCODE();
			case ZEND_TICKS:
				if (++EG(ticks_count)==opline->op1.u.constant.value.lval) {
					EG(ticks_count)=0;
					if (zend_ticks_function) {
						zend_ticks_function(opline->op1.u.constant.value.lval);
					}
				}
				NEXT_OPCODE();
			case ZEND_EXT_NOP:
			case ZEND_NOP:
				NEXT_OPCODE();
			EMPTY_SWITCH_DEFAULT_CASE()

		}
	}
#if SUPPORT_INTERACTIVE
	ALLOC_INIT_ZVAL(*(EG(return_value_ptr_ptr)));
	op_array->last_executed_op_number = opline-op_array->opcodes;
	EG(in_execution) = original_in_execution;
	free_alloca(Ts);
#else
	zend_error(E_ERROR,"Arrived at end of main loop which shouldn't happen");
#endif
}
