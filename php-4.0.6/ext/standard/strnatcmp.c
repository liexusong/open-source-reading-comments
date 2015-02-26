/* -*- mode: c; c-file-style: "k&r" -*-

  Modified for PHP by Andrei Zmievski <andrei@ispi.net>

  strnatcmp.c -- Perform 'natural order' comparisons of strings in C.
  Copyright (C) 2000 by Martin Pool <mbp@humbug.org.au>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#include <ctype.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

#include "php.h"
#include "php_string.h"

#if defined(__GNUC__)
#  define UNUSED __attribute__((__unused__))
#else
#  define UNUSED
#endif

static char const *version UNUSED =
    "$Id: strnatcmp.c,v 1.4 2000/04/29 18:57:06 andrei Exp $";

static int
compare_right(char const **a, char const *aend, char const **b, char const *bend)
{
	int bias = 0;

	/* The longest run of digits wins.  That aside, the greatest
	   value wins, but we can't know that it will until we've scanned
	   both numbers to know that they have the same magnitude, so we
	   remember it in BIAS. */
	for(;; (*a)++, (*b)++) {
		if ((*a == aend || !isdigit((int)**a)) &&
			(*b == bend || !isdigit((int)**b)))
			return bias;
		else if (*a == aend || !isdigit((int)**a))
			return -1;
		else if (*b == bend || !isdigit((int)**b))
			return +1;
		else if (**a < **b) {
			if (!bias)
				bias = -1;
		} else if (**a > **b) {
			if (!bias)
				bias = +1;
		}
     }

     return 0;
}


static int
compare_left(char const **a, char const *aend, char const **b, char const *bend)
{
     /* Compare two left-aligned numbers: the first to have a
        different value wins. */
	for(;; (*a)++, (*b)++) {
		if ((*a == aend || !isdigit((int)**a)) &&
			(*b == bend || !isdigit((int)**b)))
			return 0;
		else if (*a == aend || !isdigit((int)**a))
			return -1;
		else if (*b == bend || !isdigit((int)**b))
			return +1;
		 else if (**a < **b)
			 return -1;
		 else if (**a > **b)
			 return +1;
     }
	  
     return 0;
}


PHPAPI int strnatcmp_ex(char const *a, size_t a_len, char const *b, size_t b_len, int fold_case)
{
	char ca, cb;
	char const *ap, *bp;
	char const *aend = a + a_len,
			   *bend = b + b_len;
	int fractional, result;

	if (a_len == 0 || b_len == 0)
		return a_len - b_len;

	ap = a;
	bp = b;
	while (1) {
		ca = *ap; cb = *bp;

		/* skip over leading spaces or zeros */
		while (isspace((int)ca))
			ca = *++ap;

		while (isspace((int)cb))
			cb = *++bp;

		/* process run of digits */
		if (isdigit((int)ca)  &&  isdigit((int)cb)) {
			fractional = (ca == '0' || cb == '0');

			if (fractional)
				result = compare_left(&ap, aend, &bp, bend);
			else
				result = compare_right(&ap, aend, &bp, bend);

			if (result != 0)
				return result;
			else if (ap == aend && bp == bend)
				/* End of the strings. Let caller sort them out. */
				return 0;
			else {
				/* Keep on comparing from the current point. */
				ca = *ap; cb = *bp;
			}
		}

		if (fold_case) {
			ca = toupper(ca);
			cb = toupper(cb);
		}

		if (ca < cb)
			return -1;
		else if (ca > cb)
			return +1;

		++ap; ++bp;
		if (ap == aend && bp == bend)
			/* The strings compare the same.  Perhaps the caller
			   will want to call strcmp to break the tie. */
			return 0;
		else if (ap == aend)
			return -1;
		else if (bp == bend)
			return 1;
	}
}
