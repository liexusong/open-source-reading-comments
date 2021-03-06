%{
/*
   +----------------------------------------------------------------------+
   | PHP version 4.0                                                      |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2001 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.0 of the PHP license,       |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_0.txt.                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: James Moore <jmoore@php.net>                                |
   +----------------------------------------------------------------------+
 */

/* $Id: w32api_function_definition_scanner.l,v 1.3 2003/06/10 20:03:40 imajes Exp $ */


#include <stdio.h>
#include <stdlib.h>
#define  WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string.h>

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "ext/standard/php_string.h"
#include "php_w32api.h"
#include "w32api_function_definition_parser.h"

#ifdef YYSTYPE
#undef YYSTYPE
#endif

#define YYSTYPE w32api_parser_function_definition_union 

#define YY_DECL int w32api_function_definition_lex(w32api_parser_function_definition_union *funcdef_lval)


%}

ID [A-Za-z][A-Za-z0-9_]*
FILENAME {ID}\.{ID}

%option noyywrap

%%

"from"      {return tFROM;}
"alias"		{return tALIAS;}
"&"         {return tBYREF;}
{FILENAME}  {funcdef_lval->s = estrdup(yytext); return tFILENAME;}
{ID}        {funcdef_lval->s = estrdup(yytext); return tIDENTIFIER;}
[ \r\t\n]   /* Ignore Whitespace */
.           {return *yytext;}

%%

