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
   | Authors: Rasmus Lerdorf <rasmus@lerdorf.on.ca>                       |
   |          Zeev Suraski <zeev@zend.com>                                |
   |          Pedro Melo <melo@ip.pt>                                     |
   |                                                                      |
   | Based on code from: Shawn Cokus <Cokus@math.washington.edu>          |
   +----------------------------------------------------------------------+
 */
/* $Id: rand.c,v 1.26 2001/03/20 20:48:42 ohrn Exp $ */

#include <stdlib.h>

#include "php.h"
#include "php_math.h"
#include "php_rand.h"

#include "basic_functions.h"

/*
   This is the ``Mersenne Twister'' random number generator MT19937, which
   generates pseudorandom integers uniformly distributed in 0..(2^32 - 1)
   starting from any odd seed in 0..(2^32 - 1).  This version is a recode
   by Shawn Cokus (Cokus@math.washington.edu) on March 8, 1998 of a version by
   Takuji Nishimura (who had suggestions from Topher Cooper and Marc Rieffel in
   July-August 1997).
  
   Effectiveness of the recoding (on Goedel2.math.washington.edu, a DEC Alpha
   running OSF/1) using GCC -O3 as a compiler: before recoding: 51.6 sec. to
   generate 300 million random numbers; after recoding: 24.0 sec. for the same
   (i.e., 46.5% of original time), so speed is now about 12.5 million random
   number generations per second on this machine.
  
   According to the URL <http://www.math.keio.ac.jp/~matumoto/emt.html>
   (and paraphrasing a bit in places), the Mersenne Twister is ``designed
   with consideration of the flaws of various existing generators,'' has
   a period of 2^19937 - 1, gives a sequence that is 623-dimensionally
   equidistributed, and ``has passed many stringent tests, including the
   die-hard test of G. Marsaglia and the load test of P. Hellekalek and
   S. Wegenkittl.''  It is efficient in memory usage (typically using 2506
   to 5012 bytes of static data, depending on data type sizes, and the code
   is quite short as well).  It generates random numbers in batches of 624
   at a time, so the caching and pipelining of modern systems is exploited.
   It is also divide- and mod-free.
  
   This library is free software; you can redistribute it and/or modify it
   under the terms of the GNU Library General Public License as published by
   the Free Software Foundation (either version 2 of the License or, at your
   option, any later version).  This library is distributed in the hope that
   it will be useful, but WITHOUT ANY WARRANTY, without even the implied
   warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
   the GNU Library General Public License for more details.  You should have
   received a copy of the GNU Library General Public License along with this
   library; if not, write to the Free Software Foundation, Inc., 59 Temple
   Place, Suite 330, Boston, MA 02111-1307, USA.
  
   The code as Shawn received it included the following notice:
  
     Copyright (C) 1997 Makoto Matsumoto and Takuji Nishimura.  When
     you use this, send an e-mail to <matumoto@math.keio.ac.jp> with
     an appropriate reference to your work.
  
   It would be nice to CC: <Cokus@math.washington.edu> when you write.
  

  
   php_uint32 must be an unsigned integer type capable of holding at least 32
   bits; exactly 32 should be fastest, but 64 is better on an Alpha with
   GCC at -O3 optimization so try your options and see what's best for you

   Melo: we should put some ifdefs here to catch those alphas...
*/


#define N             MT_N                 /* length of state vector */
#define M             (397)                /* a period parameter */
#define K             (0x9908B0DFU)        /* a magic constant */
#define hiBit(u)      ((u) & 0x80000000U)  /* mask all but highest   bit of u */
#define loBit(u)      ((u) & 0x00000001U)  /* mask all but lowest    bit of u */
#define loBits(u)     ((u) & 0x7FFFFFFFU)  /* mask     the highest   bit of u */
#define mixBits(u, v) (hiBit(u)|loBits(v)) /* move hi bit of u to hi bit of v */

#define MT_RAND_MAX ((long)(0x7FFFFFFF)) /* (1<<31) - 1 */

static void seedMT(php_uint32 seed BLS_DC)
{
    /*
       We initialize state[0..(N-1)] via the generator
      
         x_new = (69069 * x_old) mod 2^32
      
       from Line 15 of Table 1, p. 106, Sec. 3.3.4 of Knuth's
       _The Art of Computer Programming_, Volume 2, 3rd ed.
      
       Notes (SJC): I do not know what the initial state requirements
       of the Mersenne Twister are, but it seems this seeding generator
       could be better.  It achieves the maximum period for its modulus
       (2^30) iff x_initial is odd (p. 20-21, Sec. 3.2.1.2, Knuth); if
       x_initial can be even, you have sequences like 0, 0, 0, ...;
       2^31, 2^31, 2^31, ...; 2^30, 2^30, 2^30, ...; 2^29, 2^29 + 2^31,
       2^29, 2^29 + 2^31, ..., etc. so I force seed to be odd below.
      
       Even if x_initial is odd, if x_initial is 1 mod 4 then
      
         the          lowest bit of x is always 1,
         the  next-to-lowest bit of x is always 0,
         the 2nd-from-lowest bit of x alternates      ... 0 1 0 1 0 1 0 1 ... ,
         the 3rd-from-lowest bit of x 4-cycles        ... 0 1 1 0 0 1 1 0 ... ,
         the 4th-from-lowest bit of x has the 8-cycle ... 0 0 0 1 1 1 1 0 ... ,
          ...
      
       and if x_initial is 3 mod 4 then
      
         the          lowest bit of x is always 1,
         the  next-to-lowest bit of x is always 1,
         the 2nd-from-lowest bit of x alternates      ... 0 1 0 1 0 1 0 1 ... ,
         the 3rd-from-lowest bit of x 4-cycles        ... 0 0 1 1 0 0 1 1 ... ,
         the 4th-from-lowest bit of x has the 8-cycle ... 0 0 1 1 1 1 0 0 ... ,
          ...
      
       The generator's potency (min. s>=0 with (69069-1)^s = 0 mod 2^32) is
       16, which seems to be alright by p. 25, Sec. 3.2.1.3 of Knuth.  It
       also does well in the dimension 2..5 spectral tests, but it could be
       better in dimension 6 (Line 15, Table 1, p. 106, Sec. 3.3.4, Knuth).
      
       Note that the random number user does not see the values generated
       here directly since reloadMT() will always munge them first, so maybe
       none of all of this matters.  In fact, the seed values made here could
       even be extra-special desirable if the Mersenne Twister theory says
       so-- that's why the only change I made is to restrict to odd seeds.
    */

    register php_uint32 x = (seed | 1U) & 0xFFFFFFFFU, *s = BG(state);
    register int    j;

    for(BG(left)=0, *s++=x, j=N; --j;
        *s++ = (x*=69069U) & 0xFFFFFFFFU);
}


static php_uint32 reloadMT(BLS_D)
{
    register php_uint32 *p0=BG(state), *p2=BG(state)+2, *pM=BG(state)+M, s0, s1;
    register int    j;

    if(BG(left) < -1)
        seedMT(4357U BLS_CC);

    BG(left)=N-1, BG(next)=BG(state)+1;

    for(s0=BG(state)[0], s1=BG(state)[1], j=N-M+1; --j; s0=s1, s1=*p2++)
        *p0++ = *pM++ ^ (mixBits(s0, s1) >> 1) ^ (loBit(s1) ? K : 0U);

    for(pM=BG(state), j=M; --j; s0=s1, s1=*p2++)
        *p0++ = *pM++ ^ (mixBits(s0, s1) >> 1) ^ (loBit(s1) ? K : 0U);

    s1=BG(state)[0], *p0 = *pM ^ (mixBits(s0, s1) >> 1) ^ (loBit(s1) ? K : 0U);
    s1 ^= (s1 >> 11);
    s1 ^= (s1 <<  7) & 0x9D2C5680U;
    s1 ^= (s1 << 15) & 0xEFC60000U;
    return(s1 ^ (s1 >> 18));
}


static inline php_uint32 randomMT(void)
{
    php_uint32 y;
	BLS_FETCH();

    if(--BG(left) < 0)
        return(reloadMT(BLS_C));

    y  = *BG(next)++;
    y ^= (y >> 11);
    y ^= (y <<  7) & 0x9D2C5680U;
    y ^= (y << 15) & 0xEFC60000U;
    return(y ^ (y >> 18));
}

/* {{{ proto void srand(int seed)
   Seeds random number generator */
PHP_FUNCTION(srand)
{
	pval **arg;

	if (ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &arg) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_long_ex(arg);
	php_srand((*arg)->value.lval);
}
/* }}} */

/* {{{ proto void mt_srand(int seed)
   Seeds Mersenne Twister random number generator */
PHP_FUNCTION(mt_srand)
{
	pval **arg;
	BLS_FETCH();

	if (ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &arg) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_long_ex(arg);
	seedMT((*arg)->value.lval BLS_CC);
}
/* }}} */

/* {{{ proto int rand([int min, int max])
   Returns a random number */
PHP_FUNCTION(rand)
{
	pval **p_min=NULL, **p_max=NULL;
	
	switch (ZEND_NUM_ARGS()) {
		case 0:
			break;
		case 2:
			if (zend_get_parameters_ex(2, &p_min, &p_max)==FAILURE) {
				RETURN_FALSE;
			}
			convert_to_long_ex(p_min);
			convert_to_long_ex(p_max);
			if ((*p_max)->value.lval-(*p_min)->value.lval < 0) {
				php_error(E_WARNING,"rand():  Invalid range:  %ld..%ld", (*p_min)->value.lval, (*p_max)->value.lval);
			}else if ((*p_max)->value.lval-(*p_min)->value.lval > PHP_RAND_MAX){
				php3_error(E_WARNING,"rand():  Invalid range:  %ld..%ld", (*p_min)->value.lval, (*p_max)->value.lval);
			}
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}
			
	return_value->type = IS_LONG;

	return_value->value.lval = php_rand();

    /*
     * A bit of tricky math here.  We want to avoid using a modulus because
     * that simply tosses the high-order bits and might skew the distribution
     * of random values over the range.  Instead we map the range directly.
     *
     * We need to map the range from 0...M evenly to the range a...b
     * Let n = the random number and n' = the mapped random number
     *
     * Then we have: n' = a + n(b-a)/M
     *
     * We have a problem here in that only n==M will get mapped to b which
     # means the chances of getting b is much much less than getting any of
     # the other values in the range.  We can fix this by increasing our range
     # artifically and using:
     #
     #               n' = a + n(b-a+1)/M
     *
     # Now we only have a problem if n==M which would cause us to produce a
     # number of b+1 which would be bad.  So we bump M up by one to make sure
     # this will never happen, and the final algorithm looks like this:
     #
     #               n' = a + n(b-a+1)/(M+1) 
     *
     * -RL
     */
	if (p_min && p_max) { /* implement range */
		return_value->value.lval = (*p_min)->value.lval +
			(int)((double)((*p_max)->value.lval - (*p_min)->value.lval + 1.0) * return_value->value.lval/(PHP_RAND_MAX+1.0));     
	}
}
/* }}} */

/* {{{ proto int mt_rand([int min, int max])
   Returns a random number from Mersenne Twister */
PHP_FUNCTION(mt_rand)
{
	pval **p_min=NULL, **p_max=NULL;
	
	switch (ZEND_NUM_ARGS()) {
		case 0:
			break;
		case 2:
			if (zend_get_parameters_ex(2, &p_min, &p_max)==FAILURE) {
				RETURN_FALSE;
			}
			convert_to_long_ex(p_min);
			convert_to_long_ex(p_max);
			if ((*p_max)->value.lval-(*p_min)->value.lval <= 0) {
				php_error(E_WARNING,"mt_rand():  Invalid range:  %ld..%ld", (*p_min)->value.lval, (*p_max)->value.lval);
			}else if ((*p_max)->value.lval-(*p_min)->value.lval > MT_RAND_MAX){
				php3_error(E_WARNING,"mt_rand():  Invalid range:  %ld..%ld",(*p_min)->value.lval, (*p_max)->value.lval);
			}
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}
			
	return_value->type = IS_LONG;
	/*
	 * Melo: hmms.. randomMT() returns 32 random bits...
	 * Yet, the previous php_rand only returns 31 at most.
	 * So I put a right shift to loose the lsb. It *seems*
	 * better than clearing the msb. 
	 * Update: 
	 * I talked with Cokus via email and it won't ruin the algorithm
	 */
	return_value->value.lval = (long)(randomMT() >> 1);

	if (p_min && p_max) { /* implement range */
		return_value->value.lval = (*p_min)->value.lval +
			(long)((double)((*p_max)->value.lval - (*p_min)->value.lval + 1.0) * return_value->value.lval/(MT_RAND_MAX+1.0));
	}
}
/* }}} */

/* {{{ proto int getrandmax(void)
   Returns the maximum value a random number can have */
PHP_FUNCTION(getrandmax)
{
	return_value->type = IS_LONG;
	return_value->value.lval = PHP_RAND_MAX;
}
/* }}} */


/* {{{ proto int mt_getrandmax(void)
   Returns the maximum value a random number from Mersenne Twister can have */
PHP_FUNCTION(mt_getrandmax)
{
	return_value->type = IS_LONG;
	/*
	 * Melo: it could be 2^^32 but we only use 2^^31 to maintain
	 * compatibility with the previous php_rand
	 */
  	return_value->value.lval = MT_RAND_MAX;	/* 2^^31 */
}
/* }}} */
/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */
