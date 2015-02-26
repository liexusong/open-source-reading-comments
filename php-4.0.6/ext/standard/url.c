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
   | Author: Jim Winstead (jimw@php.net)                                  |
   +----------------------------------------------------------------------+
 */
/* $Id: url.c,v 1.33.4.1 2001/05/15 02:04:38 sniper Exp $ */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>

#include "php.h"

#include "url.h"
#ifdef _OSD_POSIX
#ifndef APACHE
#error On this EBCDIC platform, PHP is only supported as an Apache module.
#else /*APACHE*/
#ifndef CHARSET_EBCDIC
#define CHARSET_EBCDIC /* this machine uses EBCDIC, not ASCII! */
#endif
#include "ebcdic.h"
#endif /*APACHE*/
#endif /*_OSD_POSIX*/


void free_url(php_url * theurl)
{
	if (theurl->scheme)
		efree(theurl->scheme);
	if (theurl->user)
		efree(theurl->user);
	if (theurl->pass)
		efree(theurl->pass);
	if (theurl->host)
		efree(theurl->host);
	if (theurl->path)
		efree(theurl->path);
	if (theurl->query)
		efree(theurl->query);
	if (theurl->fragment)
		efree(theurl->fragment);
	efree(theurl);
}

php_url *url_parse(char *str)
{
	regex_t re;
	regmatch_t subs[11];
	int err;
	int length = strlen(str);
	char *result;

	php_url *ret = (php_url *) emalloc(sizeof(php_url));
	if (!ret) {
		/*php_error(E_WARNING,"Unable to allocate memory\n");*/
		return NULL;
	}
	memset(ret, 0, sizeof(php_url));

	/* from Appendix B of draft-fielding-url-syntax-09,
	   http://www.ics.uci.edu/~fielding/url/url.txt */
	err = regcomp(&re, "^(([^:/?#]+):)?(//([^/?#]*))?([^?#]*)(\\?([^#]*))?(#(.*))?", REG_EXTENDED);
	if (err) {
		/*php_error(E_WARNING,"Unable to compile regex: %d\n", err);*/
		efree(ret);
		return NULL;
	}
	err = regexec(&re, str, 10, subs, 0);
	if (err) {
		/*php_error(E_WARNING,"Error with regex\n");*/
		efree(ret);
		regfree(&re);
		return NULL;
	}
	/* no processing necessary on the scheme */
	if (subs[2].rm_so != -1 && subs[2].rm_so <= length) {
		ret->scheme = estrndup(str + subs[2].rm_so, subs[2].rm_eo - subs[2].rm_so);
	}

	/* the path to the resource */
	if (subs[5].rm_so != -1 && subs[5].rm_so <= length) {
		ret->path = estrndup(str + subs[5].rm_so, subs[5].rm_eo - subs[5].rm_so);
	}

	/* the query part */
	if (subs[7].rm_so != -1 && subs[7].rm_so <= length) {
		ret->query = estrndup(str + subs[7].rm_so, subs[7].rm_eo - subs[7].rm_so);
	}

	/* the fragment */
	if (subs[9].rm_so != -1 && subs[9].rm_so <= length) {
		ret->fragment = estrndup(str + subs[9].rm_so, subs[9].rm_eo - subs[9].rm_so);
	}

	/* extract the username, pass, and port from the hostname */
	if (subs[4].rm_so != -1 && subs[4].rm_so <= length) {

		int cerr;
		/* extract username:pass@host:port from regex results */
		result = estrndup(str + subs[4].rm_so, subs[4].rm_eo - subs[4].rm_so);
		length = strlen(result);

		regfree(&re);			/* free the old regex */
		
		if ((cerr=regcomp(&re, "^(([^@:]+)(:([^@:]+))?@)?((\\[([^]]+)\\])|([^:@]+))(:([^:@]+))?", REG_EXTENDED))
			|| (err=regexec(&re, result, 11, subs, 0))) {
			STR_FREE(ret->scheme);
			STR_FREE(ret->path);
			STR_FREE(ret->query);
			STR_FREE(ret->fragment);
			efree(ret);
			efree(result);
			/*php_error(E_WARNING,"Unable to compile regex: %d\n", err);*/
			if (!cerr) regfree(&re); 
			return NULL;
		}
		/* now deal with all of the results */
		if (subs[2].rm_so != -1 && subs[2].rm_so < length) {
			ret->user = estrndup(result + subs[2].rm_so, subs[2].rm_eo - subs[2].rm_so);
		}
		if (subs[4].rm_so != -1 && subs[4].rm_so < length) {
			ret->pass = estrndup(result + subs[4].rm_so, subs[4].rm_eo - subs[4].rm_so);
		}
		if (subs[7].rm_so != -1 && subs[7].rm_so < length) {
			ret->host = estrndup(result + subs[7].rm_so, subs[7].rm_eo - subs[7].rm_so);
		} else if (subs[8].rm_so != -1 && subs[8].rm_so < length) {
			ret->host = estrndup(result + subs[8].rm_so, subs[8].rm_eo - subs[8].rm_so);
		}
		if (subs[10].rm_so != -1 && subs[10].rm_so < length) {
			ret->port = (unsigned short) strtol(result + subs[10].rm_so, NULL, 10);
		}
		efree(result);
	}
	else if (ret->scheme && !strcmp(ret->scheme, "http")) {
		STR_FREE(ret->scheme);
		STR_FREE(ret->path);
		STR_FREE(ret->query);
		STR_FREE(ret->fragment);
		efree(ret);
		regfree(&re);
		return NULL;
	}
	regfree(&re);
	return ret;
}

/* {{{ proto array parse_url(string url)
   Parse a URL and return its components */
PHP_FUNCTION(parse_url)
{
	pval **str;
	php_url *resource;

	if (ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &str) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(str);

	resource = url_parse((*str)->value.str.val);

	if (resource == NULL) {
		php_error(E_WARNING, "unable to parse url (%s)", (*str)->value.str.val);
		RETURN_FALSE;
	}
	/* allocate an array for return */
	if (array_init(return_value) == FAILURE) {
		free_url(resource);
		RETURN_FALSE;
	}
	/* add the various elements to the array */
	if (resource->scheme != NULL)
		add_assoc_string(return_value, "scheme", resource->scheme, 1);
	if (resource->host != NULL)
		add_assoc_string(return_value, "host", resource->host, 1);
	if (resource->port != 0)
		add_assoc_long(return_value, "port", resource->port);
	if (resource->user != NULL)
		add_assoc_string(return_value, "user", resource->user, 1);
	if (resource->pass != NULL)
		add_assoc_string(return_value, "pass", resource->pass, 1);
	if (resource->path != NULL)
		add_assoc_string(return_value, "path", resource->path, 1);
	if (resource->query != NULL)
		add_assoc_string(return_value, "query", resource->query, 1);
	if (resource->fragment != NULL)
		add_assoc_string(return_value, "fragment", resource->fragment, 1);
	free_url(resource);
}
/* }}} */

static int php_htoi(char *s)
{
	int value;
	int c;

	c = s[0];
	if (isupper(c))
		c = tolower(c);
	value = (c >= '0' && c <= '9' ? c - '0' : c - 'a' + 10) * 16;

	c = s[1];
	if (isupper(c))
		c = tolower(c);
	value += c >= '0' && c <= '9' ? c - '0' : c - 'a' + 10;

	return (value);
}

/* rfc1738:

   ...The characters ";",
   "/", "?", ":", "@", "=" and "&" are the characters which may be
   reserved for special meaning within a scheme...

   ...Thus, only alphanumerics, the special characters "$-_.+!*'(),", and
   reserved characters used for their reserved purposes may be used
   unencoded within a URL...

   For added safety, we only leave -_. unencoded.
 */

static unsigned char hexchars[] = "0123456789ABCDEF";

char *php_url_encode(char *s, int len)
{
	register int x, y;
	unsigned char *str;

	str = (unsigned char *) emalloc(3 * strlen(s) + 1);
	for (x = 0, y = 0; len--; x++, y++) {
		str[y] = (unsigned char) s[x];
		if (str[y] == ' ') {
			str[y] = '+';
#ifndef CHARSET_EBCDIC
		} else if ((str[y] < '0' && str[y] != '-' && str[y] != '.') ||
				   (str[y] < 'A' && str[y] > '9') ||
				   (str[y] > 'Z' && str[y] < 'a' && str[y] != '_') ||
				   (str[y] > 'z')) {
			str[y++] = '%';
			str[y++] = hexchars[(unsigned char) s[x] >> 4];
			str[y] = hexchars[(unsigned char) s[x] & 15];
		}
#else /*CHARSET_EBCDIC*/
		} else if (!isalnum(str[y]) && strchr("_-.", str[y]) == NULL) {
			/* Allow only alphanumeric chars and '_', '-', '.'; escape the rest */
			str[y++] = '%';
			str[y++] = hexchars[os_toascii[(unsigned char) s[x]] >> 4];
			str[y] = hexchars[os_toascii[(unsigned char) s[x]] & 0x0F];
		}
#endif /*CHARSET_EBCDIC*/
	}
	str[y] = '\0';
	return ((char *) str);
}

/* {{{ proto string urlencode(string str)
   URL-encodes string */
PHP_FUNCTION(urlencode)
{
	pval **arg;
	char *str;

	if (ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &arg) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(arg);

	if (!(*arg)->value.str.len) {
		var_reset(return_value);
		return;
	}
	str = php_url_encode((*arg)->value.str.val, (*arg)->value.str.len);
	RETVAL_STRING(str, 1);
	efree(str);
}
/* }}} */

/* {{{ proto string urldecode(string str)
   Decodes URL-encoded string */
PHP_FUNCTION(urldecode)
{
	pval **arg;
	int len;

	if (ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &arg) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(arg);

	if (!(*arg)->value.str.len) {
		var_reset(return_value);
		return;
	}

	*return_value = **arg;
	zval_copy_ctor(return_value);
	
	len = php_url_decode(return_value->value.str.val, return_value->value.str.len);
	return_value->value.str.len = len;
}
/* }}} */

int php_url_decode(char *str, int len)
{
	char *dest = str;
	char *data = str;

	while (len--) {
		if (*data == '+')
			*dest = ' ';
		else if (*data == '%' && len >= 2 && isxdigit((int) *(data + 1)) && isxdigit((int) *(data + 2))) {
#ifndef CHARSET_EBCDIC
			*dest = (char) php_htoi(data + 1);
#else
			*dest = os_toebcdic[(char) php_htoi(data + 1)];
#endif
			data += 2;
			len -= 2;
		} else
			*dest = *data;
		data++;
		dest++;
	}
	*dest = '\0';
	return dest - str;
}

char *php_raw_url_encode(char *s, int len)
{
	register int x, y;
	unsigned char *str;

	str = (unsigned char *) emalloc(3 * len + 1);
	for (x = 0, y = 0; len--; x++, y++) {
		str[y] = (unsigned char) s[x];
#ifndef CHARSET_EBCDIC
		if ((str[y] < '0' && str[y] != '-' && str[y] != '.') ||
			(str[y] < 'A' && str[y] > '9') ||
			(str[y] > 'Z' && str[y] < 'a' && str[y] != '_') ||
			(str[y] > 'z')) {
			str[y++] = '%';
			str[y++] = hexchars[(unsigned char) s[x] >> 4];
			str[y] = hexchars[(unsigned char) s[x] & 15];
#else /*CHARSET_EBCDIC*/
		if (!isalnum(str[y]) && strchr("_-.", str[y]) != NULL) {
			str[y++] = '%';
			str[y++] = hexchars[os_toascii[(unsigned char) s[x]] >> 4];
			str[y] = hexchars[os_toascii[(unsigned char) s[x]] & 15];
#endif /*CHARSET_EBCDIC*/
		}
	}
	str[y] = '\0';
	return ((char *) str);
}

/* {{{ proto string rawurlencode(string str)
   URL-encodes string */
PHP_FUNCTION(rawurlencode)
{
	pval **arg;
	char *str;

	if (ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &arg) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(arg);

	if (!(*arg)->value.str.len) {
		RETURN_FALSE;
	}
	str = php_raw_url_encode((*arg)->value.str.val, (*arg)->value.str.len);
	RETVAL_STRING(str, 1);
	efree(str);
}
/* }}} */

/* {{{ proto string rawurldecode(string str)
   Decodes URL-encodes string */
PHP_FUNCTION(rawurldecode)
{
	pval **arg;
	int len;
	char *str;

	if (ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &arg) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(arg);

	if (!(*arg)->value.str.len) {
		RETURN_FALSE;
	}
	str = estrndup(Z_STRVAL_PP(arg), Z_STRLEN_PP(arg));
	len = php_raw_url_decode(str, Z_STRLEN_PP(arg));

	RETVAL_STRINGL(str, len, 0);
}
/* }}} */

int php_raw_url_decode(char *str, int len)
{
	char *dest = str;
	char *data = str;

	while (len--) {
		if (*data == '%' && len >= 2 && isxdigit((int) *(data + 1)) && isxdigit((int) *(data + 2))) {
#ifndef CHARSET_EBCDIC
			*dest = (char) php_htoi(data + 1);
#else
			*dest = os_toebcdic[(char) php_htoi(data + 1)];
#endif
			data += 2;
			len -= 2;
		} else
			*dest = *data;
		data++;
		dest++;
	}
	*dest = '\0';
	return dest - str;
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */
