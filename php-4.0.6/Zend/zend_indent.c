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


/* This indenter doesn't really work, it's here for no particular reason. */


#include "zend.h"
#include "zend_language_parser.h"
#include "zend_compile.h"
#include "zend_indent.h"

#ifndef ZTS
extern char *zendtext;
extern int zendleng;
#else
#define zendtext ((char *) zend_get_zendtext(CLS_C))
#define zendleng zend_get_zendleng(CLS_C)
#endif


static void handle_whitespace(int *emit_whitespace)
{
	unsigned char c;
	int i;

	for (c=0; c<128; c++) {
		if (emit_whitespace[c]>0) {
			for (i=0; i<emit_whitespace[c]; i++) {
				zend_write((char *) &c, 1);
			}
		}
	}
	memset(emit_whitespace, 0, sizeof(int)*256);
}


ZEND_API void zend_indent()
{
	zval token;
	int token_type;
	int in_string=0;
	int nest_level=0;
	int emit_whitespace[256];
	int i;
	CLS_FETCH();

	memset(emit_whitespace, 0, sizeof(int)*256);

	/* highlight stuff coming back from zendlex() */
	token.type = 0;
	while ((token_type=lex_scan(&token CLS_CC))) {
		switch (token_type) {
			case T_INLINE_HTML:
				zend_write(zendtext, zendleng);
				break;
			case T_WHITESPACE: {
					token.type = 0;
					/* eat whitespace, emit newlines */
					for (i=0; i<zendleng; i++) {
						emit_whitespace[(unsigned char) zendtext[i]]++;
					}
					continue;
				}
				break;
			case '"':
				in_string = !in_string;
				/* break missing intentionally */
			default:
				if (token.type==0) {
					/* keyword */
					switch(token_type) {
						case ',':
							ZEND_PUTS(", ");
							goto dflt_printout;
							break;
						case '{':
							nest_level++;
							if (emit_whitespace['\n']>0) {
								ZEND_PUTS(" {\n");
								memset(emit_whitespace, 0, sizeof(int)*256);
							} else {
								ZEND_PUTS("{");
							}
							break;
						case '}':
							nest_level--;
							if (emit_whitespace['\n']==0) {
								ZEND_PUTS("\n");
							}
							for (i=0; i<nest_level; i++) {
								ZEND_PUTS("    ");
							}
							goto dflt_printout;
							break;
dflt_printout:
						default:
							if (emit_whitespace['\n']>0) {
								for (i=0; i<emit_whitespace['\n']; i++) {
									ZEND_PUTS("\n");
								}
								memset(emit_whitespace, 0, sizeof(int)*256);
								for (i=0; i<nest_level; i++) {
									ZEND_PUTS("    ");
								}
							} else {
								handle_whitespace(emit_whitespace);
							}
							zend_write(zendtext, zendleng);
							break;
					}
				} else {
					handle_whitespace(emit_whitespace);
					if (in_string) {
						zend_write(zendtext, zendleng);
						/* a part of a string */
					} else {
						zend_write(zendtext, zendleng);
					}
				}
				break;
		}
		if (token.type == IS_STRING) {
			switch (token_type) {
			case T_OPEN_TAG:
			case T_CLOSE_TAG:
			case T_WHITESPACE:
				break;
			default:
				efree(token.value.str.val);
				break;
			}
		}
		token.type = 0;
	}
}
