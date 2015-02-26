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


#include <stdio.h>
#include <signal.h>

#include "zend.h"
#include "zend_compile.h"
#include "zend_execute.h"
#include "zend_API.h"
#include "zend_ptr_stack.h"
#include "zend_constants.h"
#include "zend_extensions.h"
#include "zend_execute_locks.h"
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif


ZEND_API void (*zend_execute)(zend_op_array *op_array ELS_DC);

#ifdef ZEND_WIN32
#include <process.h>
/* true global */
static WNDCLASS wc;
static HWND timeout_window;
static HANDLE timeout_thread_event;
static DWORD timeout_thread_id;
static int timeout_thread_initialized=0;
#endif


#if ZEND_DEBUG
static void (*original_sigsegv_handler)(int);
static void zend_handle_sigsegv(int dummy)
{
	fflush(stdout);
	fflush(stderr);
	if (original_sigsegv_handler==zend_handle_sigsegv) {
		signal(SIGSEGV, original_sigsegv_handler);
	} else {
		signal(SIGSEGV, SIG_DFL);
	}
	{
		ELS_FETCH();

		fprintf(stderr, "SIGSEGV caught on opcode %d on opline %d of %s() at %s:%d\n\n",
				active_opline->opcode,
				active_opline-EG(active_op_array)->opcodes,
				get_active_function_name(),
				zend_get_executed_filename(ELS_C),
				zend_get_executed_lineno(ELS_C));
	}
	if (original_sigsegv_handler!=zend_handle_sigsegv) {
		original_sigsegv_handler(dummy);
	}
}
#endif


static void zend_extension_activator(zend_extension *extension)
{
	if (extension->activate) {
		extension->activate();
	}
}


static void zend_extension_deactivator(zend_extension *extension)
{
	if (extension->deactivate) {
		extension->deactivate();
	}
}


static int is_not_internal_function(zend_function *function)
{
	return(function->type != ZEND_INTERNAL_FUNCTION);
}


static int is_not_internal_class(zend_class_entry *ce)
{
	return(ce->type != ZEND_INTERNAL_CLASS);
}


void init_executor(CLS_D ELS_DC)
{
	INIT_ZVAL(EG(uninitialized_zval));
	INIT_ZVAL(EG(error_zval));
	EG(uninitialized_zval_ptr)=&EG(uninitialized_zval);
	EG(error_zval_ptr)=&EG(error_zval);
	zend_ptr_stack_init(&EG(arg_types_stack));
/* destroys stack frame, therefore makes core dumps worthless */
#if 0&&ZEND_DEBUG
	original_sigsegv_handler = signal(SIGSEGV, zend_handle_sigsegv);
#endif
	EG(return_value_ptr_ptr) = &EG(global_return_value_ptr);
	EG(global_return_value_ptr) = NULL;

	EG(symtable_cache_ptr) = EG(symtable_cache)-1;
	EG(symtable_cache_limit)=EG(symtable_cache)+SYMTABLE_CACHE_SIZE-1;
	EG(no_extensions)=0;

	EG(function_table) = CG(function_table);
	EG(class_table) = CG(class_table);

	EG(in_execution) = 0;

	zend_ptr_stack_init(&EG(argument_stack));

	zend_hash_init(&EG(symbol_table), 50, NULL, ZVAL_PTR_DTOR, 0);
	EG(active_symbol_table) = &EG(symbol_table);

	zend_llist_apply(&zend_extensions, (void (*)(void *)) zend_extension_activator);
	EG(opline_ptr) = NULL;
	EG(garbage_ptr) = 0;

	zend_hash_init(&EG(included_files), 5, NULL, NULL, 0);

	EG(ticks_count) = 0;

	EG(user_error_handler) = NULL;

	zend_ptr_stack_init(&EG(user_error_handlers));

	EG(orig_error_reporting) = EG(error_reporting);

#ifdef ZEND_WIN32
	EG(timed_out) = 0;
#endif
}


void shutdown_executor(ELS_D)
{
	zend_ptr_stack_destroy(&EG(arg_types_stack));
			
	while (EG(symtable_cache_ptr)>=EG(symtable_cache)) {
		zend_hash_destroy(*EG(symtable_cache_ptr));
		efree(*EG(symtable_cache_ptr));
		EG(symtable_cache_ptr)--;
	}
	zend_llist_apply(&zend_extensions, (void (*)(void *)) zend_extension_deactivator);

	zend_hash_destroy(&EG(symbol_table));

	while (EG(garbage_ptr)--) {
		if (EG(garbage)[EG(garbage_ptr)]->refcount==1) {
			zval_ptr_dtor(&EG(garbage)[EG(garbage_ptr)]);
		}
	}

	zend_ptr_stack_destroy(&EG(argument_stack));

	/* Destroy all op arrays */
	zend_hash_apply(EG(function_table), (int (*)(void *)) is_not_internal_function);
	zend_hash_apply(EG(class_table), (int (*)(void *)) is_not_internal_class);

	zend_destroy_rsrc_list(ELS_C); /* must be destroyed after the main symbol table and
									* op arrays are destroyed.
									*/

	clean_non_persistent_constants();
#if ZEND_DEBUG
	signal(SIGSEGV, original_sigsegv_handler);
#endif

	zend_hash_destroy(&EG(included_files));

	if (EG(user_error_handler)) {
		zval_dtor(EG(user_error_handler));
		FREE_ZVAL(EG(user_error_handler));
	}

	zend_ptr_stack_clean(&EG(user_error_handlers), ZVAL_DESTRUCTOR, 1);
	zend_ptr_stack_destroy(&EG(user_error_handlers));

	EG(error_reporting) = EG(orig_error_reporting);
}


ZEND_API char *get_active_function_name()
{
	ELS_FETCH();

	switch(EG(function_state_ptr)->function->type) {
		case ZEND_USER_FUNCTION: {
				char *function_name = ((zend_op_array *) EG(function_state_ptr)->function)->function_name;
			
				if (function_name) {
					return function_name;
				} else {
					return "main";
				}
			}
			break;
		case ZEND_INTERNAL_FUNCTION:
			return ((zend_internal_function *) EG(function_state_ptr)->function)->function_name;
			break;
		default:
			return NULL;
	}
}


ZEND_API char *zend_get_executed_filename(ELS_D)
{
	if (EG(active_op_array)) {
		return EG(active_op_array)->filename;
	} else {
		return "[no active file]";
	}
}


ZEND_API uint zend_get_executed_lineno(ELS_D)
{
	if (EG(opline_ptr)) {
		return active_opline->lineno;
	} else {
		return 0;
	}
}


ZEND_API zend_bool zend_is_executing()
{
	ELS_FETCH();

	return EG(in_execution);
}


ZEND_API void _zval_ptr_dtor(zval **zval_ptr ZEND_FILE_LINE_DC)
{
#if DEBUG_ZEND>=2
	printf("Reducing refcount for %x (%x):  %d->%d\n", *zval_ptr, zval_ptr, (*zval_ptr)->refcount, (*zval_ptr)->refcount-1);
#endif
	(*zval_ptr)->refcount--;
	if ((*zval_ptr)->refcount==0) {
		zval_dtor(*zval_ptr);
		safe_free_zval_ptr(*zval_ptr);
	} else if (((*zval_ptr)->refcount == 1) && ((*zval_ptr)->type != IS_OBJECT)) {
		(*zval_ptr)->is_ref = 0;
	}
}
	

ZEND_API int zend_is_true(zval *op)
{
	return i_zend_is_true(op);
}


ZEND_API int zval_update_constant(zval **pp, void *arg)
{
	zval *p = *pp;
	zend_bool inline_change = (zend_bool) (unsigned long) arg;

	if (p->type == IS_CONSTANT) {
		zval c;
		int refcount;

		SEPARATE_ZVAL(pp);
		p = *pp;

		refcount = p->refcount;

		if (!zend_get_constant(p->value.str.val, p->value.str.len, &c)) {
			zend_error(E_NOTICE, "Use of undefined constant %s - assumed '%s'",
						p->value.str.val,
						p->value.str.val);
			p->type = IS_STRING;
			if (!inline_change) {
				zval_copy_ctor(p);
			}
		} else {
			if (inline_change) {
				STR_FREE(p->value.str.val);
			}
			*p = c;
		}
		INIT_PZVAL(p);
		p->refcount = refcount;
	} else if (p->type == IS_CONSTANT_ARRAY) {
		SEPARATE_ZVAL(pp);
		p = *pp;
		p->type = IS_ARRAY;
		zend_hash_apply_with_argument(p->value.ht, (int (*)(void *,void *)) zval_update_constant, (void *) 1);
	}
	return 0;
}


int call_user_function(HashTable *function_table, zval **object_pp, zval *function_name, zval *retval_ptr, int param_count, zval *params[])
{
	zval ***params_array = (zval ***) emalloc(sizeof(zval **)*param_count);
	int i;
	int ex_retval;
	zval *local_retval_ptr;

	for (i=0; i<param_count; i++) {
		params_array[i] = &params[i];
	}
	ex_retval = call_user_function_ex(function_table, object_pp, function_name, &local_retval_ptr, param_count, params_array, 1, NULL);
	if (local_retval_ptr) {
		COPY_PZVAL_TO_ZVAL(*retval_ptr, local_retval_ptr);
	} else {
		INIT_ZVAL(*retval_ptr);
	}
	efree(params_array);
	return ex_retval;
}


int call_user_function_ex(HashTable *function_table, zval **object_pp, zval *function_name, zval **retval_ptr_ptr, int param_count, zval **params[], int no_separation, HashTable *symbol_table)
{
	int i;
	zval **original_return_value;
	HashTable *calling_symbol_table;
	zend_function_state function_state;
	zend_function_state *original_function_state_ptr;
	zend_op_array *original_op_array;
	zend_op **original_opline_ptr;
	int orig_free_op1, orig_free_op2;
	int (*orig_unary_op)(zval *result, zval *op1);
	int (*orig_binary_op)(zval *result, zval *op1, zval *op2);
	ELS_FETCH();

	*retval_ptr_ptr = NULL;

	if (function_name->type==IS_ARRAY) { /* assume array($obj, $name) couple */
		zval **tmp_object_ptr, **tmp_real_function_name;

		if (zend_hash_index_find(function_name->value.ht, 0, (void **) &tmp_object_ptr)==FAILURE) {
			return FAILURE;
		}
		if (zend_hash_index_find(function_name->value.ht, 1, (void **) &tmp_real_function_name)==FAILURE) {
			return FAILURE;
		}
		function_name = *tmp_real_function_name;
		SEPARATE_ZVAL_IF_NOT_REF(tmp_object_ptr);
		object_pp = tmp_object_ptr;
		(*object_pp)->is_ref = 1;
	}

	if (object_pp && !*object_pp) {
		object_pp = NULL;
	}
	if (object_pp) {
		if (Z_TYPE_PP(object_pp) == IS_OBJECT) {
			function_table = &(*object_pp)->value.obj.ce->function_table;
		} else if (Z_TYPE_PP(object_pp) == IS_STRING) {
			zend_class_entry *ce;
			char *lc_class;
			int found;

			lc_class = estrndup(Z_STRVAL_PP(object_pp), Z_STRLEN_PP(object_pp));
			zend_str_tolower(lc_class, Z_STRLEN_PP(object_pp));
			found = zend_hash_find(EG(class_table), lc_class, Z_STRLEN_PP(object_pp) + 1, (void **) &ce);
			efree(lc_class);
			if (found == FAILURE)
				return FAILURE;

			function_table = &ce->function_table;
			object_pp = NULL;
		} else
			return FAILURE;
	}

	if (function_name->type!=IS_STRING) {
		return FAILURE;
	}

	original_function_state_ptr = EG(function_state_ptr);
	zend_str_tolower(function_name->value.str.val, function_name->value.str.len);
	if (zend_hash_find(function_table, function_name->value.str.val, function_name->value.str.len+1, (void **) &function_state.function)==FAILURE) {
		return FAILURE;
	}

	for (i=0; i<param_count; i++) {
		zval *param;

		if (function_state.function->common.arg_types
			&& i<function_state.function->common.arg_types[0]
			&& function_state.function->common.arg_types[i+1]==BYREF_FORCE
			&& !PZVAL_IS_REF(*params[i])) {
			if ((*params[i])->refcount>1) {
				zval *new_zval;

				if (no_separation) {
					return FAILURE;
				}
				ALLOC_ZVAL(new_zval);
				*new_zval = **params[i];
				zval_copy_ctor(new_zval);
				new_zval->refcount = 1;
				(*params[i])->refcount--;
				*params[i] = new_zval;
			}
			(*params[i])->refcount++;
			(*params[i])->is_ref = 1;
			param = *params[i];
		} else if (*params[i] != &EG(uninitialized_zval)) {
			(*params[i])->refcount++;
			param = *params[i];
		} else {
			ALLOC_ZVAL(param);
			*param = **(params[i]);
			INIT_PZVAL(param);
		}
		zend_ptr_stack_push(&EG(argument_stack), param);
	}

	zend_ptr_stack_n_push(&EG(argument_stack), 2, (void *) (long) param_count, NULL);

	EG(function_state_ptr) = &function_state;

	if (function_state.function->type == ZEND_USER_FUNCTION) {
		calling_symbol_table = EG(active_symbol_table);
		if (symbol_table) {
			EG(active_symbol_table) = symbol_table;
		} else {
			ALLOC_HASHTABLE(EG(active_symbol_table));
			zend_hash_init(EG(active_symbol_table), 0, NULL, ZVAL_PTR_DTOR, 0);
		}
		if (object_pp) {
			zval *dummy, **this_ptr;
				
			ALLOC_ZVAL(dummy);
			INIT_ZVAL(*dummy);	
			zend_hash_update(EG(active_symbol_table), "this", sizeof("this"), &dummy, sizeof(zval *), (void **) &this_ptr);
			zend_assign_to_variable_reference(NULL, this_ptr, object_pp, NULL ELS_CC);
		}
		original_return_value = EG(return_value_ptr_ptr);
		original_op_array = EG(active_op_array);
		EG(return_value_ptr_ptr) = retval_ptr_ptr;
		EG(active_op_array) = (zend_op_array *) function_state.function;
		original_opline_ptr = EG(opline_ptr);
		orig_free_op1 = EG(free_op1);
		orig_free_op2 = EG(free_op2);
		orig_unary_op = EG(unary_op);
		orig_binary_op = EG(binary_op);
		zend_execute(EG(active_op_array) ELS_CC);
		if (!symbol_table) {
			zend_hash_destroy(EG(active_symbol_table));
			FREE_HASHTABLE(EG(active_symbol_table));
		}
		EG(active_symbol_table) = calling_symbol_table;
		EG(active_op_array) = original_op_array;
		EG(return_value_ptr_ptr)=original_return_value;
		EG(opline_ptr) = original_opline_ptr;
		EG(free_op1) = orig_free_op1;
		EG(free_op2) = orig_free_op2;
		EG(unary_op) = orig_unary_op;
		EG(binary_op) = orig_binary_op;
	} else {
		ALLOC_INIT_ZVAL(*retval_ptr_ptr);
		((zend_internal_function *) function_state.function)->handler(param_count, *retval_ptr_ptr, (object_pp?*object_pp:NULL), 1 ELS_CC);
		INIT_PZVAL(*retval_ptr_ptr);
	}
	zend_ptr_stack_clear_multiple(ELS_C);
	EG(function_state_ptr) = original_function_state_ptr;

	return SUCCESS;
}


ZEND_API int zend_eval_string(char *str, zval *retval_ptr, char *string_name CLS_DC ELS_DC)
{
	zval pv;
	zend_op_array *new_op_array;
	zend_op_array *original_active_op_array = EG(active_op_array);
	zend_function_state *original_function_state_ptr = EG(function_state_ptr);
	int original_handle_op_arrays;
	int retval;

	if (retval_ptr) {
		pv.value.str.len = strlen(str)+sizeof("return  ;")-1;
		pv.value.str.val = emalloc(pv.value.str.len+1);
		strcpy(pv.value.str.val, "return ");
		strcat(pv.value.str.val, str);
		strcat(pv.value.str.val, " ;");
	} else {
		pv.value.str.len = strlen(str);
		pv.value.str.val = estrndup(str, pv.value.str.len);
	}
	pv.type = IS_STRING;

	/*printf("Evaluating '%s'\n", pv.value.str.val);*/

	original_handle_op_arrays = CG(handle_op_arrays);
	CG(handle_op_arrays) = 0;
	new_op_array = compile_string(&pv, string_name CLS_CC);
	CG(handle_op_arrays) = original_handle_op_arrays;

	if (new_op_array) {
		zval *local_retval_ptr=NULL;
		zval **original_return_value_ptr_ptr = EG(return_value_ptr_ptr);
		zend_op **original_opline_ptr = EG(opline_ptr);
		
		EG(return_value_ptr_ptr) = &local_retval_ptr;
		EG(active_op_array) = new_op_array;
		EG(no_extensions)=1;

		zend_execute(new_op_array ELS_CC);

		if (local_retval_ptr) {
			if (retval_ptr) {
				COPY_PZVAL_TO_ZVAL(*retval_ptr, local_retval_ptr);
			} else {
				zval_ptr_dtor(&local_retval_ptr);
			}
		} else {
			if (retval_ptr) {
				INIT_ZVAL(*retval_ptr);
			}
		}

		EG(no_extensions)=0;
		EG(opline_ptr) = original_opline_ptr;
		EG(active_op_array) = original_active_op_array;
		EG(function_state_ptr) = original_function_state_ptr;
		destroy_op_array(new_op_array);
		efree(new_op_array);
		EG(return_value_ptr_ptr) = original_return_value_ptr_ptr;
		retval = SUCCESS;
	} else {
		retval = FAILURE;
	}
	zval_dtor(&pv);
	return retval;
}



#if SUPPORT_INTERACTIVE
void execute_new_code(CLS_D)
{
    zend_op *opline, *end;
	ELS_FETCH();

	if (!EG(interactive)
		|| CG(active_op_array)->backpatch_count>0
		|| CG(active_op_array)->function_name
		|| CG(active_op_array)->type!=ZEND_USER_FUNCTION) {
		return;
	}

    opline=CG(active_op_array)->opcodes + CG(active_op_array)->start_op_number;
	end=CG(active_op_array)->opcodes+CG(active_op_array)->last;

    while (opline<end) {
        if (opline->op1.op_type==IS_CONST) {
            opline->op1.u.constant.is_ref = 1;
            opline->op1.u.constant.refcount = 2; /* Make sure is_ref won't be reset */
        }
        if (opline->op2.op_type==IS_CONST) {
            opline->op2.u.constant.is_ref = 1;
            opline->op2.u.constant.refcount = 2;
        }
        opline++;
    }

	CG(active_op_array)->start_op_number = CG(active_op_array)->last_executed_op_number;
	CG(active_op_array)->end_op_number = CG(active_op_array)->last;
	EG(active_op_array) = CG(active_op_array);
	zend_execute(CG(active_op_array) ELS_CC);
	zval_ptr_dtor(EG(return_value_ptr_ptr));
	CG(active_op_array)->start_op_number = CG(active_op_array)->last_executed_op_number;
}
#endif


ZEND_API void zend_timeout(int dummy)
{
	ELS_FETCH();

	/* is there any point in this?  we're terminating the request anyway...
	PLS_FETCH();

	PG(connection_status) |= PHP_CONNECTION_TIMEOUT;
	*/
	zend_error(E_ERROR, "Maximum execution time of %d second%s exceeded",
			  EG(timeout_seconds), EG(timeout_seconds) == 1 ? "" : "s");
}


#ifdef ZEND_WIN32
static LRESULT CALLBACK zend_timeout_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) 
{
	switch (message) {
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		case WM_REGISTER_ZEND_TIMEOUT:
			/* wParam is the thread id pointer, lParam is the timeout amount in seconds */
			if (lParam==0) {
				KillTimer(timeout_window, wParam);
			} else {
				SetTimer(timeout_window, wParam, lParam*1000, NULL);
			}
			break;
		case WM_UNREGISTER_ZEND_TIMEOUT:
			/* wParam is the thread id pointer */
			KillTimer(timeout_window, wParam);
			break;
		case WM_TIMER: {
#ifdef ZTS
				zend_executor_globals *executor_globals;

				executor_globals = ts_resource_ex(executor_globals_id, &wParam);
				if (!executor_globals) {
					/* Thread died before receiving its timeout? */
					break;
				}
#endif
				KillTimer(timeout_window, wParam);
				EG(timed_out) = 1;
			}
			break;
		default:
			return DefWindowProc(hWnd,message,wParam,lParam);
	}
	return 0;
}



static unsigned __stdcall timeout_thread_proc(void *pArgs)
{
	MSG message;

	wc.style=0;
	wc.lpfnWndProc = zend_timeout_WndProc;
	wc.cbClsExtra=0;
	wc.cbWndExtra=0;
	wc.hInstance=NULL;
	wc.hIcon=NULL;
	wc.hCursor=NULL;
	wc.hbrBackground=(HBRUSH)(COLOR_BACKGROUND + 5);
	wc.lpszMenuName=NULL;
	wc.lpszClassName = "Zend Timeout Window";
	if (!RegisterClass(&wc)) {
		return -1;
	}
	timeout_window = CreateWindow(wc.lpszClassName, wc.lpszClassName, 0, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, NULL, NULL);
	SetEvent(timeout_thread_event);
	while (GetMessage(&message, NULL, 0, 0)) {
		SendMessage(timeout_window, message.message, message.wParam, message.lParam);
		if (message.message == WM_QUIT) {
			break;
		}
	}
	DestroyWindow(timeout_window);
	UnregisterClass(wc.lpszClassName, NULL);
	return 0;
}


void zend_init_timeout_thread()
{
	timeout_thread_event = CreateEvent(NULL, FALSE, FALSE, NULL);
	_beginthreadex(NULL, 0, timeout_thread_proc, NULL, 0, &timeout_thread_id);
	WaitForSingleObject(timeout_thread_event, INFINITE);
}


void zend_shutdown_timeout_thread()
{
	if (!timeout_thread_initialized) {
		return;
	}
	PostThreadMessage(timeout_thread_id, WM_QUIT, 0, 0);
}

#endif

/* This one doesn't exists on QNX */
#ifndef SIGPROF
#define SIGPROF 27
#endif

void zend_set_timeout(long seconds)
{
	ELS_FETCH();

	EG(timeout_seconds) = seconds;
#ifdef ZEND_WIN32
	if (timeout_thread_initialized==0 && InterlockedIncrement(&timeout_thread_initialized)==1) {
		/* We start up this process-wide thread here and not in zend_startup(), because if Zend
		 * is initialized inside a DllMain(), you're not supposed to start threads from it.
		 */
		zend_init_timeout_thread();
	}
	PostThreadMessage(timeout_thread_id, WM_REGISTER_ZEND_TIMEOUT, (WPARAM) GetCurrentThreadId(), (LPARAM) seconds);
#else
#	ifdef HAVE_SETITIMER
	{
		struct itimerval t_r;		/* timeout requested */
		sigset_t sigset;

		t_r.it_value.tv_sec = seconds;
		t_r.it_value.tv_usec = t_r.it_interval.tv_sec = t_r.it_interval.tv_usec = 0;

		setitimer(ITIMER_PROF, &t_r, NULL);
		signal(SIGPROF, zend_timeout);
		sigemptyset(&sigset);
		sigaddset(&sigset, SIGPROF);
		sigprocmask(SIG_UNBLOCK,&sigset,NULL);
	}
#	endif
#endif
}


void zend_unset_timeout(void)
{
	ELS_FETCH();

#ifdef ZEND_WIN32
	PostThreadMessage(timeout_thread_id, WM_UNREGISTER_ZEND_TIMEOUT, (WPARAM) GetCurrentThreadId(), (LPARAM) 0);
#else
#	ifdef HAVE_SETITIMER
	{
		struct itimerval no_timeout;

		no_timeout.it_value.tv_sec = no_timeout.it_value.tv_usec = no_timeout.it_interval.tv_sec = no_timeout.it_interval.tv_usec = 0;

		setitimer(ITIMER_PROF, &no_timeout, NULL);
	}
#	endif
#endif
}
