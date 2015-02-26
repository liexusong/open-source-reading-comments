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
   | Authors: Rasmus Lerdorf                                              |
   +----------------------------------------------------------------------+
*/

/* $Id: type.c,v 1.9 2001/02/26 06:07:23 andi Exp $ */

#include "php.h"
#include "type.h"

/*
 * Determines if 'str' is an integer (long), real number or a string
 *
 * Note that leading zeroes automatically force a STRING type
 */
int php_check_type(char *str)
{
	char *s;
	int type = IS_LONG;

	s = str;
	if (*s == '0' && *(s + 1) && *(s + 1) != '.')
		return (IS_STRING);
	if (*s == '+' || *s == '-' || (*s >= '0' && *s <= '9') || *s == '.') {
		if (*s == '.')
			type = IS_DOUBLE;
		s++;
		while (*s) {
			if (*s >= '0' && *s <= '9') {
				s++;
				continue;
			} else if (*s == '.' && type == IS_LONG) {
				type = IS_DOUBLE;
				s++;
				continue;
			} else
				return (IS_STRING);
		}
	} else
		return (IS_STRING);

	return (type);
}								/* php_check_type */

/*
 * 0 - simple variable
 * 1 - non-index array
 * 2 - index array
 */
int php_check_ident_type(char *str)
{
	char *s;

	if (!(s = (char *) strchr(str, '[')))
		return (GPC_REGULAR);
	s++;
	while (*s == ' ' || *s == '\t' || *s == '\n') {
		s++;
	}
	if (*s == ']') {
		return (GPC_NON_INDEXED_ARRAY);
	}
	return (GPC_INDEXED_ARRAY);
}

char *php_get_ident_index(char *str)
{
	char *temp;
	char *s, *t;
	char o;

	temp = emalloc(strlen(str));
	temp[0] = '\0';
	s = (char *) strchr(str, '[');
	if (s) {
		t = (char *) strrchr(str, ']');
		if (t) {
			o = *t;
			*t = '\0';
			strcpy(temp, s + 1);
			*t = o;
		}
	}
	return (temp);
}
