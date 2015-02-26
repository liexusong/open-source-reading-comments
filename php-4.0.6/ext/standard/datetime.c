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
   | Authors: Andi Gutmans <andi@zend.com>                                |
   |          Zeev Suraski <zeev@zend.com>                                |
   |          Rasmus Lerdorf <rasmus@php.net>                             |
   +----------------------------------------------------------------------+
 */


/* $Id: datetime.c,v 1.69 2001/04/29 15:48:07 derick Exp $ */


#include "php.h"
#include "zend_operators.h"
#include "datetime.h"
#include "php_globals.h"

#include <time.h>
#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif
#include <stdio.h>

#include "php_parsedate.h"

char *mon_full_names[] =
{
	"January", "February", "March", "April",
	"May", "June", "July", "August",
	"September", "October", "November", "December"
};
char *mon_short_names[] =
{
	"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};
char *day_full_names[] =
{
	"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"
};
char *day_short_names[] =
{
	"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

#if !defined(HAVE_TM_ZONE) && !defined(_TIMEZONE) && !defined(HAVE_DECLARED_TIMEZONE)
extern time_t timezone;
extern int daylight;
#endif

static int phpday_tab[2][12] =
{
	{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
	{31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
};

#define isleap(year) (((year%4) == 0 && (year%100)!=0) || (year%400)==0)

/* {{{ proto int time(void)
   Return current UNIX timestamp */
PHP_FUNCTION(time)
{
	RETURN_LONG((long)time(NULL));
}
/* }}} */

void php_mktime(INTERNAL_FUNCTION_PARAMETERS, int gm)
{
	pval **arguments[7];
	struct tm *ta, tmbuf;
	time_t t;
	int i, gmadjust, seconds, arg_count = ZEND_NUM_ARGS();
	int is_dst = -1;

	if (arg_count > 7 || zend_get_parameters_array_ex(arg_count,arguments) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	/* convert supplied arguments to longs */
	for (i = 0; i < arg_count; i++) {
		convert_to_long_ex(arguments[i]);
	}
	t = time(NULL);
#ifdef HAVE_TZSET
	tzset();
#endif
	/*
	** Set default time parameters with local time values,
	** EVEN when some GMT time parameters are specified!
	** This may give strange result, with PHP gmmktime(0,0,0),
	** which is assumed to return GMT midnight time
	** for today (in localtime), so that the result time may be
	** AFTER or BEFORE the current time.
	** May be we should initialize tn using gmtime(), so that
	** default parameters for PHP gmmktime would be the current
	** GMT time values...
	*/
	ta = php_localtime_r(&t, &tmbuf);

	/* Let DST be unknown. mktime() should compute the right value
	** and behave correctly. Unless the user overrides this.
	*/
	ta->tm_isdst = -1;

	/*
	** Now change date values with supplied parameters.
	*/
	switch(arg_count) {
	case 7:
		ta->tm_isdst = is_dst = (*arguments[6])->value.lval;
		/* fall-through */
	case 6:
		/* special case: 
		   a zero in year, month and day is considered illegal
		   as it would be interpreted as 30.11.1999 otherwise
		*/
		if (  (  (*arguments[5])->value.lval==0)
			  &&((*arguments[4])->value.lval==0)
			  &&((*arguments[3])->value.lval==0)
			  ) {
			RETURN_LONG(-1);
		}

		/*
		** Accept parameter in range 0..1000 interpreted as 1900..2900
		** (if 100 is given, it means year 2000)
		** or in range 1001..9999 interpreted as is (this will store
		** negative tm_year for years in range 1001..1899)
		** This function is then Y2K ready, and accepts a wide range of
		** dates including the whole gregorian calendar.
		** But it cannot represent ancestral dates prior to year 1001.
		** Additionally, input parameters of 0..70 are mapped to 100..170
		*/
		if ((*arguments[5])->value.lval < 70)
			ta->tm_year = (*arguments[5])->value.lval + 100;
		else
			ta->tm_year = (*arguments[5])->value.lval
			  - (((*arguments[5])->value.lval > 1000) ? 1900 : 0);
		/* fall-through */
	case 5:
		ta->tm_mday = (*arguments[4])->value.lval;
		/* fall-through */
	case 4:
		ta->tm_mon = (*arguments[3])->value.lval - 1;
		/* fall-through */
	case 3:
		ta->tm_sec = (*arguments[2])->value.lval;
		/* fall-through */
	case 2:
		ta->tm_min = (*arguments[1])->value.lval;
		/* fall-through */
	case 1:
		ta->tm_hour = (*arguments[0])->value.lval;
		/* fall-through */
	case 0:
		break;
	}

	seconds = mktime(ta);
	if (is_dst == -1)
		is_dst = ta->tm_isdst;

	if (gm) {
#if HAVE_TM_GMTOFF
	    /*
		** mktime(ta) very nicely just filled ta->tm_gmtoff with
		** the exactly right value for adjustment if we want GMT.
	    */
	    gmadjust = ta->tm_gmtoff;
#else
	    /*
	    ** If correcting for daylight savings time, we set the adjustment to
		** the value of timezone - 3600 seconds. Otherwise, we need to overcorrect and
		** set the adjustment to the main timezone + 3600 seconds.
	    */
	    gmadjust = -(is_dst ? timezone - 3600 : timezone + 3600);
#endif
		seconds += gmadjust;
	}

	RETURN_LONG(seconds);
}

/* {{{ proto int mktime(int hour, int min, int sec, int mon, int day, int year)
   Get UNIX timestamp for a date */
PHP_FUNCTION(mktime)
{
	php_mktime(INTERNAL_FUNCTION_PARAM_PASSTHRU, 0);
}
/* }}} */

/* {{{ proto int gmmktime(int hour, int min, int sec, int mon, int day, int year)
   Get UNIX timestamp for a GMT date */
PHP_FUNCTION(gmmktime)
{
	php_mktime(INTERNAL_FUNCTION_PARAM_PASSTHRU, 1);
}
/* }}} */

static void
php_date(INTERNAL_FUNCTION_PARAMETERS, int gm)
{
	pval **format, **timestamp;
	time_t the_time;
	struct tm *ta, tmbuf;
	int i, size = 0, length, h, beat;
	char tmp_buff[32];

	switch(ZEND_NUM_ARGS()) {
	case 1:
		if (zend_get_parameters_ex(1, &format) == FAILURE) {
			WRONG_PARAM_COUNT;
		}
		the_time = time(NULL);
		break;
	case 2:
		if (zend_get_parameters_ex(2, &format, &timestamp) == FAILURE) {
			WRONG_PARAM_COUNT;
		}
		convert_to_long_ex(timestamp);
		the_time = (*timestamp)->value.lval;
		break;
	default:
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(format);

	if (gm) {
		ta = php_gmtime_r(&the_time, &tmbuf);
	} else {
		ta = php_localtime_r(&the_time, &tmbuf);
	}

	if (!ta) {			/* that really shouldn't happen... */
		php_error(E_WARNING, "unexpected error in date()");
		RETURN_FALSE;
	}
	for (i = 0; i < (*format)->value.str.len; i++) {
		switch ((*format)->value.str.val[i]) {
			case 'r':		/* rfc822 format */
				size += 31;
				break;
			case 'U':		/* seconds since the epoch */
				size += 10;
				break;
			case 'F':		/* month, textual, full */
			case 'l':		/* day (of the week), textual */
				size += 28;
				break;
			case 'T':		/* timezone name */
#if HAVE_TM_ZONE
				size += strlen(ta->tm_zone);
#elif HAVE_TZNAME
				size += strlen(tzname[0]);
#endif
				break;
			case 'Z':		/* timezone offset in seconds */
				size += 6;
				break;
			case 'O':		/* GMT offset in [+-]HHMM format */
				size += 5;
				break;
			case 'Y':		/* year, numeric, 4 digits */
				size += 4;
				break;
			case 'M':		/* month, textual, 3 letters */
			case 'D':		/* day, textual, 3 letters */
			case 'z':		/* day of the year, 1 to 366 */
			case 'B':		/* Swatch Beat, 3 digits */
				size += 3;
				break;
			case 'y':		/* year, numeric, 2 digits */
			case 'm':		/* month, numeric */
			case 'n':		/* month, numeric, no leading zeroes */
			case 'd':		/* day of the month, numeric */
			case 'j':		/* day of the month, numeric, no leading zeros */
			case 'H':		/* hour, numeric, 24 hour format */
			case 'h':		/* hour, numeric, 12 hour format */
			case 'G':		/* hour, numeric, 24 hour format, no leading zeroes */
			case 'g':		/* hour, numeric, 12 hour format, no leading zeroes */
			case 'i':		/* minutes, numeric */
			case 's':		/* seconds, numeric */
			case 'A':		/* AM/PM */
			case 'a':		/* am/pm */
			case 'S':		/* standard english suffix for the day of the month (e.g. 3rd, 2nd, etc) */
			case 't':		/* days in current month */
				size += 2;
				break;
			case '\\':
				if(i < (*format)->value.str.len-1) {
					i++;
				}
				size ++;
				break;
			case 'L':		/* boolean for leap year */
			case 'w':		/* day of the week, numeric */
			case 'I':		/* DST? */
			default:
				size++;
				break;
		}
	}

	return_value->value.str.val = (char *) emalloc(size + 1);
	return_value->value.str.val[0] = '\0';

	for (i = 0; i < (*format)->value.str.len; i++) {
		switch ((*format)->value.str.val[i]) {
			case '\\':
				if(i < (*format)->value.str.len-1) {
					char ch[2];
					ch[0]=(*format)->value.str.val[i+1];
					ch[1]='\0';
					strcat(return_value->value.str.val, ch);
					i++;
				}
				break;
			case 'U':		/* seconds since the epoch */
				sprintf(tmp_buff, "%ld", (long)the_time); /* SAFE */
				strcat(return_value->value.str.val, tmp_buff);
				break;
			case 'F':		/* month, textual, full */
				strcat(return_value->value.str.val, mon_full_names[ta->tm_mon]);
				break;
			case 'l':		/* day (of the week), textual, full */
				strcat(return_value->value.str.val, day_full_names[ta->tm_wday]);
				break;
			case 'Y':		/* year, numeric, 4 digits */
				sprintf(tmp_buff, "%d", ta->tm_year + 1900);  /* SAFE */
				strcat(return_value->value.str.val, tmp_buff);
				break;
			case 'M':		/* month, textual, 3 letters */
				strcat(return_value->value.str.val, mon_short_names[ta->tm_mon]);
				break;
			case 'D':		/* day (of the week), textual, 3 letters */
				strcat(return_value->value.str.val, day_short_names[ta->tm_wday]);
				break;
			case 'z':		/* day (of the year) */
				sprintf(tmp_buff, "%d", ta->tm_yday);  /* SAFE */
				strcat(return_value->value.str.val, tmp_buff);
				break;
			case 'y':		/* year, numeric, 2 digits */
				sprintf(tmp_buff, "%02d", ((ta->tm_year)%100));  /* SAFE */
				strcat(return_value->value.str.val, tmp_buff);
				break;
			case 'm':		/* month, numeric */
				sprintf(tmp_buff, "%02d", ta->tm_mon + 1);  /* SAFE */
				strcat(return_value->value.str.val, tmp_buff);
				break;
			case 'n':      /* month, numeric, no leading zeros */
				sprintf(tmp_buff, "%d", ta->tm_mon + 1);  /* SAFE */
				strcat(return_value->value.str.val, tmp_buff);
				break;
			case 'd':		/* day of the month, numeric */
				sprintf(tmp_buff, "%02d", ta->tm_mday);  /* SAFE */
				strcat(return_value->value.str.val, tmp_buff);
				break;
			case 'j':
				sprintf(tmp_buff, "%d", ta->tm_mday); /* SAFE */
				strcat(return_value->value.str.val, tmp_buff);
				break;
			case 'H':		/* hour, numeric, 24 hour format */
				sprintf(tmp_buff, "%02d", ta->tm_hour);  /* SAFE */
				strcat(return_value->value.str.val, tmp_buff);
				break;
			case 'h':		/* hour, numeric, 12 hour format */
				h = ta->tm_hour % 12; if (h==0) h = 12;
				sprintf(tmp_buff, "%02d", h);  /* SAFE */
				strcat(return_value->value.str.val, tmp_buff);
				break;
			case 'G':      /* hour, numeric, 24 hour format, no leading zeros */
				sprintf(tmp_buff, "%d", ta->tm_hour);  /* SAFE */
				strcat(return_value->value.str.val, tmp_buff);
				break;
			case 'g':      /* hour, numeric, 12 hour format, no leading zeros */
				h = ta->tm_hour % 12; if (h==0) h = 12;
				sprintf(tmp_buff, "%d", h);  /* SAFE */
				strcat(return_value->value.str.val, tmp_buff);
				break;
			case 'i':		/* minutes, numeric */
				sprintf(tmp_buff, "%02d", ta->tm_min);  /* SAFE */
				strcat(return_value->value.str.val, tmp_buff);
				break;
			case 's':		/* seconds, numeric */
				sprintf(tmp_buff, "%02d", ta->tm_sec);  /* SAFE */ 
				strcat(return_value->value.str.val, tmp_buff);
				break;
			case 'A':		/* AM/PM */
				strcat(return_value->value.str.val, (ta->tm_hour >= 12 ? "PM" : "AM"));
				break;
			case 'a':		/* am/pm */
				strcat(return_value->value.str.val, (ta->tm_hour >= 12 ? "pm" : "am"));
				break;
			case 'S':		/* standard english suffix, e.g. 2nd/3rd for the day of the month */
				if (ta->tm_mday >= 10 && ta->tm_mday <= 19) {
					strcat(return_value->value.str.val, "th");
				} else {
					switch (ta->tm_mday % 10) {
						case 1:
							strcat(return_value->value.str.val, "st");
							break;
						case 2:
							strcat(return_value->value.str.val, "nd");
							break;
						case 3:
							strcat(return_value->value.str.val, "rd");
							break;
						default:
							strcat(return_value->value.str.val, "th");
							break;
					}
				}
				break;
			case 't':		/* days in current month */
				sprintf(tmp_buff, "%2d", phpday_tab[isleap((ta->tm_year+1900))][ta->tm_mon] );
				strcat(return_value->value.str.val, tmp_buff);
				break;
			case 'w':		/* day of the week, numeric EXTENSION */
				sprintf(tmp_buff, "%01d", ta->tm_wday);  /* SAFE */
				strcat(return_value->value.str.val, tmp_buff);
				break;
			case 'O':		/* GMT offset in [+-]HHMM format */
#if HAVE_TM_GMTOFF				
				sprintf(tmp_buff, "%c%02d%02d", (ta->tm_gmtoff < 0) ? '-' : '+', abs(ta->tm_gmtoff / 3600), abs( ta->tm_gmtoff % 3600));
#else
				sprintf(tmp_buff, "%c%02d%02d", ((ta->tm_isdst ? timezone - 3600:timezone)>0)?'-':'+',abs((ta->tm_isdst ? timezone - 3600 : timezone) / 3600), abs((ta->tm_isdst ? timezone - 3600 : timezone) % 3600));
#endif
				strcat(return_value->value.str.val, tmp_buff);
				break;
			case 'Z':		/* timezone offset in seconds */
#if HAVE_TM_GMTOFF
				sprintf(tmp_buff, "%ld", ta->tm_gmtoff);
#else
				sprintf(tmp_buff, "%ld", ta->tm_isdst ? -(timezone - 3600) : -timezone);
#endif
				strcat(return_value->value.str.val, tmp_buff);
				break;
			case 'L':		/* boolean for leapyear */
				sprintf(tmp_buff, "%d", (isleap((ta->tm_year+1900)) ? 1 : 0 ) );
				strcat(return_value->value.str.val, tmp_buff);
				break;
			case 'T':		/* timezone name */
#if HAVE_TM_ZONE
				strcat(return_value->value.str.val, ta->tm_zone);
#elif HAVE_TZNAME
				strcat(return_value->value.str.val, tzname[0]);
#endif
				break;
			case 'B':	/* Swatch Beat a.k.a. Internet Time */
				beat =  (((((long)the_time)-(((long)the_time) -
					((((long)the_time) % 86400) + 3600))) * 10) / 864);
				while (beat < 0) {
					beat += 1000;
				}
				beat = beat % 1000;
				sprintf(tmp_buff, "%03d", beat); /* SAFE */
				strcat(return_value->value.str.val, tmp_buff);
				break;
			case 'I':
				sprintf(tmp_buff, "%d", ta->tm_isdst);
				strcat(return_value->value.str.val, tmp_buff);
				break;
			case 'r':
#if HAVE_TM_GMTOFF				
				sprintf(tmp_buff, "%3s, %2d %3s %04d %02d:%02d:%02d %c%02d%02d", 
					day_short_names[ta->tm_wday],
					ta->tm_mday,
					mon_short_names[ta->tm_mon],
					ta->tm_year + 1900,
					ta->tm_hour,
					ta->tm_min,
					ta->tm_sec,
					(ta->tm_gmtoff < 0) ? '-' : '+',
					abs(ta->tm_gmtoff / 3600),
					abs( ta->tm_gmtoff % 3600)
				);
#else
				sprintf(tmp_buff, "%3s, %2d %3s %04d %02d:%02d:%02d %c%02d%02d", 
					day_short_names[ta->tm_wday],
					ta->tm_mday,
					mon_short_names[ta->tm_mon],
					ta->tm_year + 1900,
					ta->tm_hour,
					ta->tm_min,
					ta->tm_sec,
					((ta->tm_isdst ? timezone - 3600 : timezone) > 0) ? '-' : '+',
					abs((ta->tm_isdst ? timezone - 3600 : timezone) / 3600),
					abs((ta->tm_isdst ? timezone - 3600 : timezone) % 3600)
				);
#endif
				strcat(return_value->value.str.val, tmp_buff);
				break;

			default:
				length = strlen(return_value->value.str.val);
				return_value->value.str.val[length] = (*format)->value.str.val[i];
				return_value->value.str.val[length + 1] = '\0';
				break;
		}
	}
	return_value->value.str.len = strlen(return_value->value.str.val);
	return_value->type = IS_STRING;
}

/* {{{ proto string date(string format [, int timestamp])
   Format a local time/date */
PHP_FUNCTION(date)
{
	php_date(INTERNAL_FUNCTION_PARAM_PASSTHRU, 0);
}
/* }}} */

/* {{{ proto string gmdate(string format [, int timestamp])
   Format a GMT/UTC date/time */
PHP_FUNCTION(gmdate)
{
	php_date(INTERNAL_FUNCTION_PARAM_PASSTHRU, 1);
}
/* }}} */

/* {{{ proto array localtime([int timestamp [, bool associative_array]])
   Returns the results of the C system call localtime as an associative array if the associative_array argument is set to 1 other wise it is a regular array */
PHP_FUNCTION(localtime)
{
	zval **timestamp_arg, **assoc_array_arg;
	struct tm *ta, tmbuf;
	time_t timestamp;
	int assoc_array = 0;
	int arg_count = ZEND_NUM_ARGS();

	if (arg_count < 0 || arg_count > 2 ||
		zend_get_parameters_ex(arg_count, &timestamp_arg, &assoc_array_arg) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	switch (arg_count) {
		case 0:
			timestamp = (long)time(NULL);
			break;
		case 1:
			convert_to_long_ex(timestamp_arg);
			timestamp = (*timestamp_arg)->value.lval;
			break;
		case 2:
			convert_to_long_ex(timestamp_arg);
			convert_to_long_ex(assoc_array_arg);
			timestamp = (*timestamp_arg)->value.lval;
			assoc_array = (*assoc_array_arg)->value.lval;
			break;
	}
	ta = php_localtime_r(&timestamp, &tmbuf);
	if (array_init(return_value) == FAILURE) {
		php_error(E_ERROR, "Cannot prepare return array from localtime");
		RETURN_FALSE;
	}

	if (assoc_array) {
		add_assoc_long(return_value, "tm_sec",   ta->tm_sec);
		add_assoc_long(return_value, "tm_min",   ta->tm_min);
		add_assoc_long(return_value, "tm_hour",  ta->tm_hour);
		add_assoc_long(return_value, "tm_mday",  ta->tm_mday);
		add_assoc_long(return_value, "tm_mon",   ta->tm_mon);
		add_assoc_long(return_value, "tm_year",  ta->tm_year);
		add_assoc_long(return_value, "tm_wday",  ta->tm_wday);
		add_assoc_long(return_value, "tm_yday",  ta->tm_yday);
		add_assoc_long(return_value, "tm_isdst", ta->tm_isdst);
	} else {
		add_next_index_long(return_value, ta->tm_sec);
		add_next_index_long(return_value, ta->tm_min);
		add_next_index_long(return_value, ta->tm_hour);
		add_next_index_long(return_value, ta->tm_mday);
		add_next_index_long(return_value, ta->tm_mon);
		add_next_index_long(return_value, ta->tm_year);
		add_next_index_long(return_value, ta->tm_wday);
		add_next_index_long(return_value, ta->tm_yday);
		add_next_index_long(return_value, ta->tm_isdst);
	}
}
/* }}} */

/* {{{ proto array getdate([int timestamp])
   Get date/time information */
PHP_FUNCTION(getdate)
{
	pval **timestamp_arg;
	struct tm *ta, tmbuf;
	time_t timestamp;

	if (ZEND_NUM_ARGS() == 0) {
		timestamp = time(NULL);
	} else if (ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1,&timestamp_arg) == FAILURE) {
		WRONG_PARAM_COUNT;
	} else {
		convert_to_long_ex(timestamp_arg);
		timestamp = (*timestamp_arg)->value.lval;
	}

	ta = php_localtime_r(&timestamp, &tmbuf);
	if (!ta) {
		php_error(E_WARNING, "Cannot perform date calculation");
		return;
	}
	if (array_init(return_value) == FAILURE) {
		php_error(E_ERROR, "Unable to initialize array");
		return;
	}
	add_assoc_long(return_value, "seconds", ta->tm_sec);
	add_assoc_long(return_value, "minutes", ta->tm_min);
	add_assoc_long(return_value, "hours", ta->tm_hour);
	add_assoc_long(return_value, "mday", ta->tm_mday);
	add_assoc_long(return_value, "wday", ta->tm_wday);
	add_assoc_long(return_value, "mon", ta->tm_mon + 1);
	add_assoc_long(return_value, "year", ta->tm_year + 1900);
	add_assoc_long(return_value, "yday", ta->tm_yday);
	add_assoc_string(return_value, "weekday", day_full_names[ta->tm_wday], 1);
	add_assoc_string(return_value, "month", mon_full_names[ta->tm_mon], 1);
	add_index_long(return_value, 0, timestamp);
}
/* }}} */

/* Return date string in standard format for http headers */
char *php_std_date(time_t t)
{
	struct tm *tm1, tmbuf;
	char *str;
	PLS_FETCH();

	tm1 = php_gmtime_r(&t, &tmbuf);
	str = emalloc(81);
	if (PG(y2k_compliance)) {
		snprintf(str, 80, "%s, %02d-%s-%04d %02d:%02d:%02d GMT",
				day_short_names[tm1->tm_wday],
				tm1->tm_mday,
				mon_short_names[tm1->tm_mon],
				tm1->tm_year+1900,
				tm1->tm_hour, tm1->tm_min, tm1->tm_sec);
	} else {
		snprintf(str, 80, "%s, %02d-%s-%02d %02d:%02d:%02d GMT",
				day_short_names[tm1->tm_wday],
				tm1->tm_mday,
				mon_short_names[tm1->tm_mon],
				((tm1->tm_year)%100),
				tm1->tm_hour, tm1->tm_min, tm1->tm_sec);
	}
	
	str[79]=0;
	return (str);
}


/* {{{ proto bool checkdate(int month, int day, int year)
   Returns true(1) if it is a valid date in gregorian calendar */
PHP_FUNCTION(checkdate)
{
	pval **month, **day, **year;
	int m, d, y, res=0;
	
	if (ZEND_NUM_ARGS() != 3 ||
		zend_get_parameters_ex(3, &month, &day, &year) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	if((*year)->type == IS_STRING) {
		res = is_numeric_string((*year)->value.str.val, (*year)->value.str.len, NULL, NULL);
		if(res!=IS_LONG && res !=IS_DOUBLE) {
			RETURN_FALSE;	
		}
	}
	convert_to_long_ex(day);
	convert_to_long_ex(month);
	convert_to_long_ex(year);
	y = (*year)->value.lval;
	m = (*month)->value.lval;
	d = (*day)->value.lval;

	if (y < 1 || y > 32767) {
		RETURN_FALSE;
	}
	if (m < 1 || m > 12) {
		RETURN_FALSE;
	}
	if (d < 1 || d > phpday_tab[isleap(y)][m - 1]) {
		RETURN_FALSE;
	}
	RETURN_TRUE;				/* True : This month,day,year arguments are valid */
}
/* }}} */

#if HAVE_STRFTIME
void _php_strftime(INTERNAL_FUNCTION_PARAMETERS, int gm)
{
	pval **format_arg, **timestamp_arg;
	char *format,*buf;
	time_t timestamp;
	struct tm *ta, tmbuf;
	int max_reallocs = 5;
	size_t buf_len=64, real_len;

	switch (ZEND_NUM_ARGS()) {
		case 1:
			if (zend_get_parameters_ex(1,&format_arg)==FAILURE) {
				RETURN_FALSE;
			}
			time(&timestamp);
			break;
		case 2:
			if (zend_get_parameters_ex(2, &format_arg,&timestamp_arg)==FAILURE) {
				RETURN_FALSE;
			}
			convert_to_long_ex(timestamp_arg);
			timestamp = (*timestamp_arg)->value.lval;
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}

	convert_to_string_ex(format_arg);
	if ((*format_arg)->value.str.len==0) {
		RETURN_FALSE;
	}
	if (timestamp < 0) {
		RETURN_FALSE;
	}
	format = (*format_arg)->value.str.val;
	if (gm) {
		ta = php_gmtime_r(&timestamp, &tmbuf);
	} else {
		ta = php_localtime_r(&timestamp, &tmbuf);
	}

	buf = (char *) emalloc(buf_len);
	while ((real_len=strftime(buf,buf_len,format,ta))==buf_len || real_len==0) {
		buf_len *= 2;
		buf = (char *) erealloc(buf, buf_len);
		if(!--max_reallocs) break;
	}
	
	if(real_len && real_len != buf_len) {
		buf = (char *) erealloc(buf,real_len+1);
		RETURN_STRINGL(buf, real_len, 0);
	}
	efree(buf);
	RETURN_FALSE;
}
/* {{{ proto string strftime(string format [, int timestamp])
   Format a local time/date according to locale settings */
PHP_FUNCTION(strftime)
{
	_php_strftime(INTERNAL_FUNCTION_PARAM_PASSTHRU, 0);
}
/* }}} */

/* {{{ proto string gmstrftime(string format [, int timestamp])
   Format a GMT/UCT time/date according to locale settings */
PHP_FUNCTION(gmstrftime)
{
	_php_strftime(INTERNAL_FUNCTION_PARAM_PASSTHRU, 1);
}
/* }}} */

#endif

/* {{{ proto int strtotime(string time, int now)
   Convert string representation of date and time to a timestamp */
PHP_FUNCTION(strtotime)
{
	zval	**z_time, **z_now;
	int 	 argc;
	time_t now;

	argc = ZEND_NUM_ARGS();

	if (argc < 1 || argc > 2 || zend_get_parameters_ex(argc, &z_time, &z_now)==FAILURE) {
		WRONG_PARAM_COUNT;
	}

	convert_to_string_ex(z_time);
	if (Z_STRLEN_PP(z_time) == 0)
		php_error (E_NOTICE, "strtotime() called with empty time parameter");
	if (argc == 2) {
		convert_to_long_ex(z_now);
		now = Z_LVAL_PP(z_now);
		RETURN_LONG(php_parse_date(Z_STRVAL_PP(z_time), &now));
	} else {
		RETURN_LONG(php_parse_date(Z_STRVAL_PP(z_time), NULL));
	}
}
/* }}} */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */
