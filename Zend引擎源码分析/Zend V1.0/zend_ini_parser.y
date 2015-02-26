%{
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
   | Author: Zeev Suraski <zeev@zend.com>                                 |
   +----------------------------------------------------------------------+
*/


/* $Id: zend_ini_parser.y,v 1.9 2001/02/26 05:43:27 andi Exp $ */

#define DEBUG_CFG_PARSER 0
#include "zend.h"
#include "zend_API.h"
#include "zend_ini.h"
#include "zend_constants.h"
#include "zend_ini_scanner.h"
#include "zend_extensions.h"


#if WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winbase.h>
#endif


typedef struct _zend_ini_parser_param {
	zend_ini_parser_cb_t ini_parser_cb;
	void *arg;
} zend_ini_parser_param;

#define YYSTYPE zval
#define YYPARSE_PARAM ini_parser_param

#define ZEND_INI_PARSER_CB	((zend_ini_parser_param *) ini_parser_param)->ini_parser_cb
#define ZEND_INI_PARSER_ARG	((zend_ini_parser_param *) ini_parser_param)->arg

int ini_lex(zval *ini_lval);
int ini_parse(void *ini_parser_param);

zval yylval;

#ifndef ZTS
extern int ini_lex(zval *ini_lval);
extern FILE *ini_in;
extern int ini_lineno;
extern void init_cfg_scanner(void);
#endif

void zend_ini_do_op(char type, zval *result, zval *op1, zval *op2)
{
	int i_result;
	int i_op1, i_op2;
	char str_result[MAX_LENGTH_OF_LONG];

	i_op1 = atoi(op1->value.str.val);
	free(op1->value.str.val);
	if (op2) {
		i_op2 = atoi(op2->value.str.val);
		free(op2->value.str.val);
	} else {
		i_op2 = 0;
	}

	switch (type) {
		case '|':
			i_result = i_op1 | i_op2;
			break;
		case '&':
			i_result = i_op1 & i_op2;
			break;
		case '~':
			i_result = ~i_op1;
			break;
		case '!':
			i_result = !i_op1;
			break;
		default:
			i_result = 0;
			break;
	}

	result->value.str.len = zend_sprintf(str_result, "%d", i_result);
	result->value.str.val = (char *) malloc(result->value.str.len+1);
	memcpy(result->value.str.val, str_result, result->value.str.len);
	result->value.str.val[result->value.str.len] = 0;
	result->type = IS_STRING;
}


void zend_ini_get_constant(zval *result, zval *name)
{
	zval z_constant;

	if (zend_get_constant(name->value.str.val, name->value.str.len, &z_constant)) {
		/* z_constant is emalloc()'d */
		convert_to_string(&z_constant);
		result->value.str.val = zend_strndup(z_constant.value.str.val, z_constant.value.str.len);
		result->value.str.len = z_constant.value.str.len;
		result->type = z_constant.type;
		zval_dtor(&z_constant);
		free(name->value.str.val);	
	} else {
		*result = *name;
	}
}


static void ini_error(char *str)
{
	char *error_buf;
	int error_buf_len;
	char *currently_parsed_filename = zend_ini_scanner_get_filename();
	CLS_FETCH();

	error_buf_len = 128+strlen(currently_parsed_filename); /* should be more than enough */
	error_buf = (char *) emalloc(error_buf_len);

	sprintf(error_buf, "Error parsing %s on line %d\n", currently_parsed_filename, zend_ini_scanner_get_lineno());

	if (CG(ini_parser_unbuffered_errors)) {
#ifdef PHP_WIN32
		MessageBox(NULL, error_buf, "PHP Error", MB_OK|MB_TOPMOST|0x00200000L);
#else
		fprintf(stderr, "PHP:  %s", error_buf);
#endif
	} else {
		zend_error(E_CORE_WARNING, error_buf);
	}
	efree(error_buf);
}


int zend_parse_ini_file(zend_file_handle *fh, zend_bool unbuffered_errors, zend_ini_parser_cb_t ini_parser_cb, void *arg)
{
	zend_ini_parser_param ini_parser_param;
	int retval;
	CLS_FETCH();

	ini_parser_param.ini_parser_cb = ini_parser_cb;
	ini_parser_param.arg = arg;

	if (zend_ini_open_file_for_scanning(fh)==FAILURE) {
		return FAILURE;
	}

	CG(ini_parser_unbuffered_errors) = unbuffered_errors;
	retval = ini_parse(&ini_parser_param);

	zend_ini_close_file(fh);

	if (retval==0) {
		return SUCCESS;
	} else {
		return FAILURE;
	}
}


%}

%pure_parser
%token TC_STRING
%token TC_ENCAPSULATED_STRING
%token SECTION
%token CFG_TRUE
%token CFG_FALSE
%left '|' '&'
%right '~' '!'

%%

statement_list:
		statement_list statement
	|	/* empty */
;

statement:
		TC_STRING '=' string_or_value {
#if DEBUG_CFG_PARSER
			printf("'%s' = '%s'\n", $1.value.str.val, $3.value.str.val);
#endif
			ZEND_INI_PARSER_CB(&$1, &$3, ZEND_INI_PARSER_ENTRY, ZEND_INI_PARSER_ARG);
			free($1.value.str.val);
			free($3.value.str.val);
		}
	|	TC_STRING { ZEND_INI_PARSER_CB(&$1, NULL, ZEND_INI_PARSER_ENTRY, ZEND_INI_PARSER_ARG); free($1.value.str.val); }
	|	SECTION { ZEND_INI_PARSER_CB(&$1, NULL, ZEND_INI_PARSER_SECTION, ZEND_INI_PARSER_ARG); free($1.value.str.val); }
	|	'\n'
;


string_or_value:
		expr { $$ = $1; }
	|	TC_ENCAPSULATED_STRING { $$ = $1; }
	|	CFG_TRUE { $$ = $1; }
	|	CFG_FALSE { $$ = $1; }
	|	'\n' { $$.value.str.val = strdup(""); $$.value.str.len=0; $$.type = IS_STRING; }
	|	'\0' { $$.value.str.val = strdup(""); $$.value.str.len=0; $$.type = IS_STRING; }
;

expr:
		constant_string			{ $$ = $1; }
	|	expr '|' expr			{ zend_ini_do_op('|', &$$, &$1, &$3); }
	|	expr '&' expr			{ zend_ini_do_op('&', &$$, &$1, &$3); }
	|	'~' expr				{ zend_ini_do_op('~', &$$, &$2, NULL); }
	|	'!'	expr				{ zend_ini_do_op('!', &$$, &$2, NULL); }
	|	'(' expr ')'			{ $$ = $2; }
;

constant_string:
		TC_STRING { zend_ini_get_constant(&$$, &$1); }
;
/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */
