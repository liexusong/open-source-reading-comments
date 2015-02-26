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
   | Authors:                                                             |
   +----------------------------------------------------------------------+
*/

/* $Id: php_content_types.c,v 1.9 2001/02/26 06:07:31 andi Exp $ */

#include "php.h"
#include "SAPI.h"
#include "rfc1867.h"

#include "php_content_types.h"

static sapi_post_entry php_post_entries[] = {
	{ DEFAULT_POST_CONTENT_TYPE,	sizeof(DEFAULT_POST_CONTENT_TYPE)-1,	sapi_read_standard_form_data,	php_std_post_handler },
	{ MULTIPART_CONTENT_TYPE,		sizeof(MULTIPART_CONTENT_TYPE)-1,		sapi_read_standard_form_data,	rfc1867_post_handler },
	{ NULL, 0, NULL }
};


SAPI_POST_READER_FUNC(php_default_post_reader)
{
	char *data;
	ELS_FETCH();

	sapi_read_standard_form_data(SLS_C);
	data = estrndup(SG(request_info).post_data,SG(request_info).post_data_length);
	SET_VAR_STRINGL("HTTP_RAW_POST_DATA", data, SG(request_info).post_data_length);
}


int php_startup_sapi_content_types(void)
{
	sapi_register_post_entries(php_post_entries);
	sapi_register_default_post_reader(php_default_post_reader);
	return SUCCESS;
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */
