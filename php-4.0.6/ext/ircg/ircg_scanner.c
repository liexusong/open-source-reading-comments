/* Generated by re2c 0.5 on Wed Apr 25 15:49:43 2001 */
#line 1 "/home/sas/src/php4/ext/ircg/ircg_scanner.re"
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
   | Authors: Sascha Schumann <sascha@schumann.cx>                        |
   +----------------------------------------------------------------------+
 */

/* $Id: ircg_scanner.c,v 1.10 2001/04/25 13:50:35 sas Exp $ */

#include <ext/standard/php_smart_str.h>
#include <stdio.h>
#include <string.h>

static const char *color_list[] = {
    "white",
    "black",
    "blue",
    "green",
    "red",
    "brown",
    "purple",
    "orange",
    "yellow",
    "lightgreen",
    "teal",
    "lightcyan",
    "lightblue",
    "pink",
    "gray",
    "lightgrey"
};


enum {
	STATE_PLAIN,
	STATE_URL,
	STATE_COLOR_FG,
	STATE_COLOR_COMMA,
	STATE_COLOR_BG
};

typedef struct {
	int bg_code;
	int fg_code;
	int font_tag_open;
	int bold_tag_open;
	int underline_tag_open;
	
	smart_str scheme;
	smart_str *result;
} ircg_msg_scanner;

#line 75


#define YYFILL(n) { }
#define YYCTYPE unsigned char
#define YYCURSOR xp
#define YYLIMIT end
#define YYMARKER q
#define STATE mode

#define STD_PARA ircg_msg_scanner *ctx, const char *start, const char *YYCURSOR
#define STD_ARGS ctx, start, YYCURSOR

static void handle_scheme(STD_PARA)
{
	ctx->scheme.len = 0;
	smart_str_appendl_ex(&ctx->scheme, start, YYCURSOR - start, 1);
	smart_str_0(&ctx->scheme);
}

static void handle_url(STD_PARA)
{
	smart_str_appends_ex(ctx->result, "<a target=blank href=\"", 1);
	smart_str_append_ex(ctx->result, &ctx->scheme, 1);
	smart_str_appendl_ex(ctx->result, start, YYCURSOR - start, 1);
	smart_str_appends_ex(ctx->result, "\">", 1);
	smart_str_append_ex(ctx->result, &ctx->scheme, 1);
	smart_str_appendl_ex(ctx->result, start, YYCURSOR - start, 1);
	smart_str_appends_ex(ctx->result, "</a>", 1);
}

static void handle_color_digit(STD_PARA, int mode)
{
	int len;
	int nr;

	len = YYCURSOR - start;
	switch (len) {
		case 2:
			nr = (start[0] - '0') * 10 + (start[1] - '0');
			break;
		case 1:
			nr = start[0] - '0';
			break;
	}
	
	switch (mode) {
		case 0: ctx->fg_code = nr; break;
		case 1: ctx->bg_code = nr; break;
	}
}

#define IS_VALID_CODE(n) (n >= 0 && n <= 15)

static void finish_color_stuff(STD_PARA)
{
	if (ctx->font_tag_open) {
		smart_str_appends_ex(ctx->result, "</font>", 1);
		ctx->font_tag_open = 0;
	}
}

static void handle_bold(STD_PARA, int final)
{
	switch (ctx->bold_tag_open) {
	case 0:
		if (!final) smart_str_appends_ex(ctx->result, "<b>", 1);
		break;
	case 1:
		smart_str_appends_ex(ctx->result, "</b>", 1);
		break;
	}

	ctx->bold_tag_open = 1 - ctx->bold_tag_open;
}

static void handle_underline(STD_PARA, int final)
{
	switch (ctx->underline_tag_open) {
	case 0:
		if (!final) smart_str_appends_ex(ctx->result, "<u>", 1);
		break;
	case 1:
		smart_str_appends_ex(ctx->result, "</u>", 1);
		break;
	}

	ctx->underline_tag_open = 1 - ctx->underline_tag_open;
}

static void commit_color_stuff(STD_PARA)
{
	finish_color_stuff(STD_ARGS);

	if (IS_VALID_CODE(ctx->fg_code)) {
		smart_str_appends_ex(ctx->result, "<font color=\"", 1);
		smart_str_appends_ex(ctx->result, color_list[ctx->fg_code], 1);
		smart_str_appends_ex(ctx->result, "\">", 1);
		ctx->font_tag_open = 1;
	}
}

static void passthru(STD_PARA)
{
	smart_str_appendl_ex(ctx->result, start, YYCURSOR - start, 1);
}

static void add_entity(STD_PARA, const char *entity)
{
	smart_str_appends_ex(ctx->result, entity, 1);
}

void ircg_mirc_color(const char *msg, smart_str *result, size_t msg_len) {
	int mode = STATE_PLAIN;
	const char *end, *xp, *q, *start;
	ircg_msg_scanner mctx, *ctx = &mctx;

	mctx.result = result;
	mctx.scheme.c = NULL;
	mctx.font_tag_open = mctx.bold_tag_open = mctx.underline_tag_open = 0;
	
	if (msg_len == -1)
		msg_len = strlen(msg);
	end = msg + msg_len;
	xp = msg;
	
	while (1) {
		start = YYCURSOR;

		switch (STATE) {

		case STATE_PLAIN:
{
	YYCTYPE yych;
	unsigned int yyaccept;
	static unsigned char yybm[] = {
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	128, 128, 128, 128, 128, 128, 128, 128, 
	128, 128,   0,   0,   0,   0,   0,   0, 
	  0, 128, 128, 128, 128, 128, 128, 128, 
	128, 128, 128, 128, 128, 128, 128, 128, 
	128, 128, 128, 128, 128, 128, 128, 128, 
	128, 128, 128,   0,   0,   0,   0,   0, 
	  0, 128, 128, 128, 128, 128, 128, 128, 
	128, 128, 128, 128, 128, 128, 128, 128, 
	128, 128, 128, 128, 128, 128, 128, 128, 
	128, 128, 128,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	};
	goto yy0;
yy1:	++YYCURSOR;
yy0:
	if((YYLIMIT - YYCURSOR) < 4) YYFILL(4);
	yych = *YYCURSOR;
	if(yych <= '&'){
		if(yych <= '\003'){
			if(yych <= '\000')	goto yy17;
			if(yych <= '\001')	goto yy16;
			if(yych <= '\002')	goto yy12;
			goto yy4;
		} else {
			if(yych == '\037')	goto yy14;
			if(yych <= '%')	goto yy16;
			goto yy10;
		}
	} else {
		if(yych <= '>'){
			if(yych == '<')	goto yy6;
			if(yych <= '=')	goto yy16;
			goto yy8;
		} else {
			if(yych <= 'Z'){
				if(yych <= '@')	goto yy16;
			} else {
				if(yych <= '`')	goto yy16;
				if(yych >= '{')	goto yy16;
			}
		}
	}
yy2:	yyaccept = 0;
	yych = *(YYMARKER = ++YYCURSOR);
	if(yych <= '@'){
		if(yych <= '/')	goto yy3;
		if(yych <= ':')	goto yy20;
	} else {
		if(yych <= 'Z')	goto yy20;
		if(yych <= '`')	goto yy3;
		if(yych <= 'z')	goto yy20;
	}
yy3:
#line 214
	{ passthru(STD_ARGS); continue; }
yy4:	yych = *++YYCURSOR;
yy5:
#line 208
	{ mctx.fg_code = mctx.bg_code = -1; STATE = STATE_COLOR_FG; continue; }
yy6:	yych = *++YYCURSOR;
yy7:
#line 209
	{ add_entity(STD_ARGS, "&lt;"); continue; }
yy8:	yych = *++YYCURSOR;
yy9:
#line 210
	{ add_entity(STD_ARGS, "&gt;"); continue; }
yy10:	yych = *++YYCURSOR;
yy11:
#line 211
	{ add_entity(STD_ARGS, "&amp;"); continue; }
yy12:	yych = *++YYCURSOR;
yy13:
#line 212
	{ handle_bold(STD_ARGS, 0); continue; }
yy14:	yych = *++YYCURSOR;
yy15:
#line 213
	{ handle_underline(STD_ARGS, 0); continue; }
yy16:	yych = *++YYCURSOR;
	goto yy3;
yy17:	yych = *++YYCURSOR;
yy18:
#line 215
	{ goto stop; }
yy19:	++YYCURSOR;
	if(YYLIMIT == YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
yy20:	if(yybm[0+yych] & 128)	goto yy19;
	if(yych == ':')	goto yy22;
yy21:	YYCURSOR = YYMARKER;
	switch(yyaccept){
	case 0:	goto yy3;
	}
yy22:	yych = *++YYCURSOR;
	if(yych != '/')	goto yy21;
yy23:	yych = *++YYCURSOR;
	if(yych != '/')	goto yy21;
yy24:	yych = *++YYCURSOR;
yy25:
#line 207
	{ handle_scheme(STD_ARGS); STATE = STATE_URL; continue; }
}
#line 216


			break;

		case STATE_URL:	

			
{
	YYCTYPE yych;
	unsigned int yyaccept;
	static unsigned char yybm[] = {
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0, 128,   0, 128, 128, 128, 128, 128, 
	128, 128, 128, 128, 128, 128, 128, 128, 
	128, 128, 128, 128, 128, 128, 128, 128, 
	128, 128, 128, 128,   0, 128,   0, 128, 
	128, 128, 128, 128, 128, 128, 128, 128, 
	128, 128, 128, 128, 128, 128, 128, 128, 
	128, 128, 128, 128, 128, 128, 128, 128, 
	128, 128, 128,   0,   0,   0,   0, 128, 
	  0, 128, 128, 128, 128, 128, 128, 128, 
	128, 128, 128, 128, 128, 128, 128, 128, 
	128, 128, 128, 128, 128, 128, 128, 128, 
	128, 128, 128,   0,   0,   0, 128,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	};
	goto yy26;
yy27:	++YYCURSOR;
yy26:
	if((YYLIMIT - YYCURSOR) < 2) YYFILL(2);
	yych = *YYCURSOR;
	if(yych <= '>'){
		if(yych <= '"'){
			if(yych != '!')	goto yy30;
		} else {
			if(yych == '<')	goto yy30;
			if(yych >= '>')	goto yy30;
		}
	} else {
		if(yych <= '`'){
			if(yych <= 'Z')	goto yy28;
			if(yych != '_')	goto yy30;
		} else {
			if(yych <= 'z')	goto yy28;
			if(yych != '~')	goto yy30;
		}
	}
yy28:	yych = *++YYCURSOR;
	goto yy33;
yy29:
#line 224
	{ handle_url(STD_ARGS); STATE = STATE_PLAIN; continue; }
yy30:	yych = *++YYCURSOR;
yy31:
#line 225
	{ passthru(STD_ARGS); STATE = STATE_PLAIN; continue; }
yy32:	++YYCURSOR;
	if(YYLIMIT == YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
yy33:	if(yybm[0+yych] & 128)	goto yy32;
	goto yy29;
}
#line 226


			break;

		case STATE_COLOR_FG:
		
{
	YYCTYPE yych;
	unsigned int yyaccept;
	goto yy34;
yy35:	++YYCURSOR;
yy34:
	if((YYLIMIT - YYCURSOR) < 2) YYFILL(2);
	yych = *YYCURSOR;
	if(yych <= '/')	goto yy38;
	if(yych >= ':')	goto yy38;
yy36:	yych = *++YYCURSOR;
	if(yych <= '/')	goto yy37;
	if(yych <= '9')	goto yy40;
yy37:
#line 233
	{ handle_color_digit(STD_ARGS, 0); STATE = STATE_COLOR_COMMA; continue; }
yy38:	yych = *++YYCURSOR;
yy39:
#line 234
	{ finish_color_stuff(STD_ARGS); passthru(STD_ARGS); STATE = STATE_PLAIN; continue; }
yy40:	yych = *++YYCURSOR;
	goto yy37;
}
#line 235


			break;
		
		case STATE_COLOR_COMMA:
		
{
	YYCTYPE yych;
	unsigned int yyaccept;
	goto yy41;
yy42:	++YYCURSOR;
yy41:
	if(YYLIMIT == YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	if(yych != ',')	goto yy45;
yy43:	yych = *++YYCURSOR;
yy44:
#line 242
	{ STATE = STATE_COLOR_BG; continue; }
yy45:	yych = *++YYCURSOR;
yy46:
#line 243
	{ YYCURSOR--; commit_color_stuff(STD_ARGS); STATE = STATE_PLAIN; continue; }
}
#line 244


			break;

		case STATE_COLOR_BG:

{
	YYCTYPE yych;
	unsigned int yyaccept;
	goto yy47;
yy48:	++YYCURSOR;
yy47:
	if((YYLIMIT - YYCURSOR) < 2) YYFILL(2);
	yych = *YYCURSOR;
	if(yych <= '/')	goto yy51;
	if(yych >= ':')	goto yy51;
yy49:	yych = *++YYCURSOR;
	if(yych <= '/')	goto yy50;
	if(yych <= '9')	goto yy53;
yy50:
#line 251
	{ handle_color_digit(STD_ARGS, 1); commit_color_stuff(STD_ARGS); STATE = STATE_PLAIN; continue; }
yy51:	yych = *++YYCURSOR;
yy52:
#line 252
	{ commit_color_stuff(STD_ARGS); STATE = STATE_PLAIN; continue; }
yy53:	yych = *++YYCURSOR;
	goto yy50;
}
#line 253

			break;
		}
	}
stop:
	smart_str_free_ex(&ctx->scheme, 1);

	finish_color_stuff(STD_ARGS);
	handle_bold(STD_ARGS, 1);
	handle_underline(STD_ARGS, 1);
}
