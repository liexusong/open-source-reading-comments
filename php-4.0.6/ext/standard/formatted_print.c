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
   | Authors: Stig S�ther Bakken <ssb@guardian.no>                        |
   +----------------------------------------------------------------------+
 */

/* $Id: formatted_print.c,v 1.31.2.1 2001/05/20 00:31:45 derick Exp $ */

#include <math.h>				/* modf() */
#include "php.h"
#include "ext/standard/head.h"
#include "php_string.h"
#include "zend_execute.h"
#include <stdio.h>

#define ALIGN_LEFT 0
#define ALIGN_RIGHT 1
#define ADJ_WIDTH 1
#define ADJ_PRECISION 2
#define NUM_BUF_SIZE 500
#define	NDIG 80
#define FLOAT_DIGITS 6
#define FLOAT_PRECISION 6
#define MAX_FLOAT_DIGITS 38
#define MAX_FLOAT_PRECISION 40

#if 0
/* trick to control varargs functions through cpp */
# define PRINTF_DEBUG(arg) php_printf arg
#else
# define PRINTF_DEBUG(arg)
#endif

static char hexchars[] = "0123456789abcdef";
static char HEXCHARS[] = "0123456789ABCDEF";


/*
 * cvt.c - IEEE floating point formatting routines for FreeBSD
 * from GNU libc-4.6.27
 */

/*
 *    php_convert_to_decimal converts to decimal
 *      the number of digits is specified by ndigit
 *      decpt is set to the position of the decimal point
 *      sign is set to 0 for positive, 1 for negative
 */
static char *php_convert_to_decimal(double arg, int ndigits, int *decpt, int *sign, int eflag)
{
	register int r2;
	double fi, fj;
	register char *p, *p1;
	/*THREADX*/
#ifndef THREAD_SAFE
	static char cvt_buf[NDIG];
#endif

	if (ndigits >= NDIG - 1)
		ndigits = NDIG - 2;
	r2 = 0;
	*sign = 0;
	p = &cvt_buf[0];
	if (arg < 0) {
		*sign = 1;
		arg = -arg;
	}
	arg = modf(arg, &fi);
	p1 = &cvt_buf[NDIG];
	/*
	 * Do integer part
	 */
	if (fi != 0) {
		p1 = &cvt_buf[NDIG];
		while (fi != 0) {
			fj = modf(fi / 10, &fi);
			*--p1 = (int) ((fj + .03) * 10) + '0';
			r2++;
		}
		while (p1 < &cvt_buf[NDIG])
			*p++ = *p1++;
	} else if (arg > 0) {
		while ((fj = arg * 10.0) < 0.9999999) {
			arg = fj;
			r2--;
		}
	}
	p1 = &cvt_buf[ndigits];
	if (eflag == 0)
		p1 += r2;
	*decpt = r2;
	if (p1 < &cvt_buf[0]) {
		cvt_buf[0] = '\0';
		return (cvt_buf);
	}
	while (p <= p1 && p < &cvt_buf[NDIG]) {
		arg *= 10;
		arg = modf(arg, &fj);
		*p++ = (int) fj + '0';
	}
	if (p1 >= &cvt_buf[NDIG]) {
		cvt_buf[NDIG - 1] = '\0';
		return (cvt_buf);
	}
	p = p1;
	*p1 += 5;
	while (*p1 > '9') {
		*p1 = '0';
		if (p1 > cvt_buf)
			++ * --p1;
		else {
			*p1 = '1';
			(*decpt)++;
			if (eflag == 0) {
				if (p > cvt_buf)
					*p = '0';
				p++;
			}
		}
	}
	*p = '\0';
	return (cvt_buf);
}


inline static void
php_sprintf_appendchar(char **buffer, int *pos, int *size, char add)
{
	if ((*pos + 1) >= *size) {
		*size <<= 1;
		PRINTF_DEBUG(("%s: ereallocing buffer to %d bytes\n", get_active_function_name(), *size));
		*buffer = erealloc(*buffer, *size);
	}
	PRINTF_DEBUG(("sprintf: appending '%c', pos=\n", add, *pos));
	(*buffer)[(*pos)++] = add;
}


inline static void
php_sprintf_appendstring(char **buffer, int *pos, int *size, char *add,
						   int min_width, int max_width, char padding,
						   int alignment, int len, int sign, int expprec)
{
	register int npad;

	npad = min_width - MIN(len, (expprec ? max_width : len));

	if (npad < 0) {
		npad = 0;
	}
	
	PRINTF_DEBUG(("sprintf: appendstring(%x, %d, %d, \"%s\", %d, '%c', %d)\n",
				  *buffer, *pos, *size, add, min_width, padding, alignment));
	if ((max_width == 0) && (! expprec)) {
		max_width = MAX(min_width, len);
	}
	if ((*pos + max_width) >= *size) {
		while ((*pos + max_width) >= *size) {
			*size <<= 1;
		}
		PRINTF_DEBUG(("sprintf ereallocing buffer to %d bytes\n", *size));
		*buffer = erealloc(*buffer, *size);
	}
	if (alignment == ALIGN_RIGHT) {
		if (sign && padding=='0') {
			(*buffer)[(*pos)++] = '-';
			add++;
			len--;
		}
		while (npad-- > 0) {
			(*buffer)[(*pos)++] = padding;
		}
	}
	PRINTF_DEBUG(("sprintf: appending \"%s\"\n", add));
	memcpy(&(*buffer)[*pos], add, MIN(max_width, len)+1);
	*pos += MIN(max_width, len);
	if (alignment == ALIGN_LEFT) {
		while (npad--) {
			(*buffer)[(*pos)++] = padding;
		}
	}
}


inline static void
php_sprintf_appendint(char **buffer, int *pos, int *size, int number,
						int width, char padding, int alignment)
{
	char numbuf[NUM_BUF_SIZE];
	register unsigned int magn, nmagn, i = NUM_BUF_SIZE - 1, neg = 0;

	PRINTF_DEBUG(("sprintf: appendint(%x, %x, %x, %d, %d, '%c', %d)\n",
				  *buffer, pos, size, number, width, padding, alignment));
	if (number < 0) {
		neg = 1;
		magn = ((unsigned int) -(number + 1)) + 1;
	} else {
		magn = (unsigned int) number;
	}

	/* Can't right-pad 0's on integers */
	if(alignment==0 && padding=='0') padding=' ';

	numbuf[i] = '\0';

	do {
		nmagn = magn / 10;

		numbuf[--i] = (magn - (nmagn * 10)) + '0';
		magn = nmagn;
	}
	while (magn > 0 && i > 0);
	if (neg) {
		numbuf[--i] = '-';
	}
	PRINTF_DEBUG(("sprintf: appending %d as \"%s\", i=%d\n",
				  number, &numbuf[i], i));
	php_sprintf_appendstring(buffer, pos, size, &numbuf[i], width, 0,
							 padding, alignment, (NUM_BUF_SIZE - 1) - i,
							 neg, 0);
}

inline static void
php_sprintf_appenduint(char **buffer, int *pos, int *size, int number,
						int width, char padding, int alignment)
{
	char numbuf[NUM_BUF_SIZE];
	register unsigned int magn, nmagn, i = NUM_BUF_SIZE - 1;

	PRINTF_DEBUG(("sprintf: appenduint(%x, %x, %x, %d, %d, '%c', %d)\n",
				  *buffer, pos, size, number, width, padding, alignment));
	magn = (unsigned int) number;

	/* Can't right-pad 0's on integers */
	if (alignment == 0 && padding == '0') padding = ' ';

	numbuf[i] = '\0';

	do {
		nmagn = magn / 10;

		numbuf[--i] = (magn - (nmagn * 10)) + '0';
		magn = nmagn;
	}
	while (magn > 0 && i > 0);
	PRINTF_DEBUG(("sprintf: appending %d as \"%s\", i=%d\n", number, &numbuf[i], i));
	php_sprintf_appendstring(buffer, pos, size, &numbuf[i], width, 0,
							 padding, alignment, (NUM_BUF_SIZE - 1) - i, 0, 0);
}

inline static void
php_sprintf_appenddouble(char **buffer, int *pos,
						 int *size, double number,
						 int width, char padding,
						 int alignment, int precision,
						 int adjust, char fmt)
{
	char numbuf[NUM_BUF_SIZE];
	char *cvt;
	register int i = 0, j = 0;
	int sign, decpt;

	PRINTF_DEBUG(("sprintf: appenddouble(%x, %x, %x, %f, %d, '%c', %d, %c)\n",
				  *buffer, pos, size, number, width, padding, alignment, fmt));
	if ((adjust & ADJ_PRECISION) == 0) {
		precision = FLOAT_PRECISION;
	} else if (precision > MAX_FLOAT_PRECISION) {
		precision = MAX_FLOAT_PRECISION;
	}
	
	if (zend_isnan(number)) {
		sign = (number<0);
		php_sprintf_appendstring(buffer, pos, size, "NaN", 3, 0, padding,
								 alignment, precision, sign, 0);
		return;
	}

	if (zend_isinf(number)) {
		sign = (number<0);
		php_sprintf_appendstring(buffer, pos, size, "INF", 3, 0, padding,
								 alignment, precision, sign, 0);
		return;
	}

	cvt = php_convert_to_decimal(number, precision, &decpt, &sign, (fmt == 'e'));

	if (sign) {
		numbuf[i++] = '-';
	}

	if (fmt == 'f') {
		if (decpt <= 0) {
			numbuf[i++] = '0';
			if (precision > 0) {
				int k = precision;
				numbuf[i++] = '.';
				while ((decpt++ < 0) && k--) {
					numbuf[i++] = '0';
				}
			}
		} else {
			while (decpt-- > 0)
				numbuf[i++] = cvt[j++];
			if (precision > 0)
				numbuf[i++] = '.';
		}
	} else {
		numbuf[i++] = cvt[j++];
		if (precision > 0)
			numbuf[i++] = '.';
	}

	while (cvt[j]) {
		numbuf[i++] = cvt[j++];
	}

	numbuf[i] = '\0';

	if (precision > 0) {
		width += (precision + 1);
	}
	php_sprintf_appendstring(buffer, pos, size, numbuf, width, 0, padding,
							 alignment, i, sign, 0);
}


inline static void
php_sprintf_append2n(char **buffer, int *pos, int *size, int number,
					 int width, char padding, int alignment, int n,
					 char *chartable, int expprec)
{
	char numbuf[NUM_BUF_SIZE];
	register unsigned int num, i = NUM_BUF_SIZE - 1, neg = 0;
	register int andbits = (1 << n) - 1;

	PRINTF_DEBUG(("sprintf: append2n(%x, %x, %x, %d, %d, '%c', %d, %d, %x)\n",
				  *buffer, pos, size, number, width, padding, alignment, n,
				  chartable));
	PRINTF_DEBUG(("sprintf: append2n 2^%d andbits=%x\n", n, andbits));

	if (number < 0) {
		neg = 1;
		num = ((unsigned int) -(number + 1)) + 1;
	} else {
		num = (unsigned int) number;
	}

	numbuf[i] = '\0';

	do {
		numbuf[--i] = chartable[(num & andbits)];
		num >>= n;
	}
	while (num > 0);

	if (neg) {
		numbuf[--i] = '-';
	}
	php_sprintf_appendstring(buffer, pos, size, &numbuf[i], width, 0,
							 padding, alignment, (NUM_BUF_SIZE - 1) - i,
							 neg, expprec);
}


inline static int
php_sprintf_getnumber(char *buffer, int *pos)
{
	char *endptr;
	register int num = strtol(&buffer[*pos], &endptr, 10);
	register int i = 0;

	if (endptr != NULL) {
		i = (endptr - &buffer[*pos]);
	}
	PRINTF_DEBUG(("sprintf_getnumber: number was %d bytes long\n", i));
	*pos += i;
	return num;
}


/*
 * New sprintf implementation for PHP.
 *
 * Modifiers:
 *
 *  " "   pad integers with spaces
 *  "-"   left adjusted field
 *   n    field size
 *  "."n  precision (floats only)
 *
 * Type specifiers:
 *
 *  "%"   literal "%", modifiers are ignored.
 *  "b"   integer argument is printed as binary
 *  "c"   integer argument is printed as a single character
 *  "d"   argument is an integer
 *  "f"   the argument is a float
 *  "o"   integer argument is printed as octal
 *  "s"   argument is a string
 *  "x"   integer argument is printed as lowercase hexadecimal
 *  "X"   integer argument is printed as uppercase hexadecimal
 *
 */
static char *
php_formatted_print(int ht, int *len)
{
	pval ***args;
	int argc, size = 240, inpos = 0, outpos = 0, temppos;
	int alignment, width, precision, currarg, adjusting, argnum;
	char *format, *result, padding;

	argc = ZEND_NUM_ARGS();

	if (argc < 1) {
		WRONG_PARAM_COUNT_WITH_RETVAL(NULL);
	}
	args = (pval ***)emalloc(argc * sizeof(pval *));

	if (zend_get_parameters_array_ex(argc, args) == FAILURE) {
		efree(args);
		WRONG_PARAM_COUNT_WITH_RETVAL(NULL);
	}
	convert_to_string_ex(args[0]);
	format = (*args[0])->value.str.val;
	result = emalloc(size);

	currarg = 1;

	while (inpos<(*args[0])->value.str.len) {
		int expprec = 0;

		PRINTF_DEBUG(("sprintf: format[%d]='%c'\n", inpos, format[inpos]));
		PRINTF_DEBUG(("sprintf: outpos=%d\n", outpos));
		if (format[inpos] != '%') {
			php_sprintf_appendchar(&result, &outpos, &size, format[inpos++]);
		} else if (format[inpos + 1] == '%') {
			php_sprintf_appendchar(&result, &outpos, &size, '%');
			inpos += 2;
		} else {
			if (currarg >= argc && format[inpos + 1] != '%') {
				efree(result);
				efree(args);
				php_error(E_WARNING, "%s(): too few arguments",get_active_function_name());
				return NULL;
			}
			/* starting a new format specifier, reset variables */
			alignment = ALIGN_RIGHT;
			adjusting = 0;
			padding = ' ';
			inpos++;			/* skip the '%' */

			PRINTF_DEBUG(("sprintf: first looking at '%c', inpos=%d\n",
						  format[inpos], inpos));
			if (isascii((int)format[inpos]) && !isalpha((int)format[inpos])) {
				/* first look for argnum */
				temppos = inpos;
				while (isdigit((int)format[temppos])) temppos++;
				if (format[temppos] == '$') {
					argnum = php_sprintf_getnumber(format, &inpos);
					inpos++;  /* skip the '$' */
				} else {
					argnum = currarg++;
				}
				if (argnum >= argc) {
					efree(result);
					efree(args);
					php_error(E_WARNING, "%s(): too few arguments",get_active_function_name());
					return NULL;
				}

				/* after argnum comes modifiers */
				PRINTF_DEBUG(("sprintf: looking for modifiers\n"
							  "sprintf: now looking at '%c', inpos=%d\n",
							  format[inpos], inpos));
				for (;; inpos++) {
					if (format[inpos] == ' ' || format[inpos] == '0') {
						padding = format[inpos];
					} else if (format[inpos] == '-') {
						alignment = ALIGN_LEFT;
						/* space padding, the default */
					} else if (format[inpos] == '\'') {
						padding = format[++inpos];
					} else {
						PRINTF_DEBUG(("sprintf: end of modifiers\n"));
						break;
					}
				}
				PRINTF_DEBUG(("sprintf: padding='%c'\n", padding));
				PRINTF_DEBUG(("sprintf: alignment=%s\n",
							  (alignment == ALIGN_LEFT) ? "left" : "right"));


				/* after modifiers comes width */
				if (isdigit((int)format[inpos])) {
					PRINTF_DEBUG(("sprintf: getting width\n"));
					width = php_sprintf_getnumber(format, &inpos);
					adjusting |= ADJ_WIDTH;
				} else {
					width = 0;
				}
				PRINTF_DEBUG(("sprintf: width=%d\n", width));

				/* after width and argnum comes precision */
				if (format[inpos] == '.') {
					inpos++;
					PRINTF_DEBUG(("sprintf: getting precision\n"));
					if (isdigit((int)format[inpos])) {
						precision = php_sprintf_getnumber(format, &inpos);
						adjusting |= ADJ_PRECISION;
						expprec = 1;
					} else {
						precision = 0;
					}
				} else {
					precision = 0;
				}
				PRINTF_DEBUG(("sprintf: precision=%d\n", precision));
			} else {
				width = precision = 0;
				argnum = currarg++;
			}

			if (format[inpos] == 'l') {
				inpos++;
			}
			PRINTF_DEBUG(("sprintf: format character='%c'\n", format[inpos]));
			/* now we expect to find a type specifier */
			switch (format[inpos]) {
				case 's':
					convert_to_string_ex(args[argnum]);
					php_sprintf_appendstring(&result, &outpos, &size,
											 (*args[argnum])->value.str.val,
											 width, precision, padding,
											 alignment,
											 (*args[argnum])->value.str.len,
											 0, expprec);
					break;

				case 'd':
					convert_to_long_ex(args[argnum]);
					php_sprintf_appendint(&result, &outpos, &size,
										  (*args[argnum])->value.lval,
										  width, padding, alignment);
					break;

				case 'u':
					convert_to_long_ex(args[argnum]);
					php_sprintf_appenduint(&result, &outpos, &size,
										  (*args[argnum])->value.lval,
										  width, padding, alignment);
					break;

				case 'e':
				case 'f':
					/* XXX not done */
					convert_to_double_ex(args[argnum]);
					php_sprintf_appenddouble(&result, &outpos, &size,
											 (*args[argnum])->value.dval,
											 width, padding, alignment,
											 precision, adjusting,
											 format[inpos]);
					break;

				case 'c':
					convert_to_long_ex(args[argnum]);
					php_sprintf_appendchar(&result, &outpos, &size,
										(char) (*args[argnum])->value.lval);
					break;

				case 'o':
					convert_to_long_ex(args[argnum]);
					php_sprintf_append2n(&result, &outpos, &size,
										 (*args[argnum])->value.lval,
										 width, padding, alignment, 3,
										 hexchars, expprec);
					break;

				case 'x':
					convert_to_long_ex(args[argnum]);
					php_sprintf_append2n(&result, &outpos, &size,
										 (*args[argnum])->value.lval,
										 width, padding, alignment, 4,
										 hexchars, expprec);
					break;

				case 'X':
					convert_to_long_ex(args[argnum]);
					php_sprintf_append2n(&result, &outpos, &size,
										 (*args[argnum])->value.lval,
										 width, padding, alignment, 4,
										 HEXCHARS, expprec);
					break;

				case 'b':
					convert_to_long_ex(args[argnum]);
					php_sprintf_append2n(&result, &outpos, &size,
										 (*args[argnum])->value.lval,
										 width, padding, alignment, 1,
										 hexchars, expprec);
					break;

				case '%':
					php_sprintf_appendchar(&result, &outpos, &size, '%');

					break;
				default:
					break;
			}
			inpos++;
		}
	}
	
	efree(args);
	
	/* possibly, we have to make sure we have room for the terminating null? */
	result[outpos]=0;
	*len = outpos;	
	return result;
}

/* {{{ proto string sprintf(string format [, mixed arg1 [, mixed ...]])
   Return a formatted string */
PHP_FUNCTION(user_sprintf)
{
	char *result;
	int len;
	
	if ((result=php_formatted_print(ht,&len))==NULL) {
		RETURN_FALSE;
	}
	RETVAL_STRINGL(result,len,1);
	efree(result);
}
/* }}} */

/* {{{ proto int printf(string format [, mixed arg1 [, mixed ...]])
   Output a formatted string */
PHP_FUNCTION(user_printf)
{
	char *result;
	int len;
	
	if ((result=php_formatted_print(ht,&len))==NULL) {
		RETURN_FALSE;
	}
	PHPWRITE(result,len);
	efree(result);
}
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */
