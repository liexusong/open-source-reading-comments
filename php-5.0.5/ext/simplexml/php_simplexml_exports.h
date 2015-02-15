/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2004 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.0 of the PHP license,       |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_0.txt.                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: Sterling Hughes <sterling@php.net>                           |
  |         Marcus Boerger <helly@php.net>                               |
  |         Rob Richards <rrichards@php.net>                             |
  +----------------------------------------------------------------------+
*/

/* $Id: php_simplexml_exports.h,v 1.1.2.1 2004/08/05 23:57:53 iliaa Exp $ */

#ifndef PHP_SIMPLEXML_EXPORTS_H
#define PHP_SIMPLEXML_EXPORTS_H

#include "php_simplexml.h"

#define SKIP_TEXT(__p) \
	if ((__p)->type == XML_TEXT_NODE) { \
		goto next_iter; \
	}

#define GET_NODE(__s, __n) { \
	if ((__s)->node && (__s)->node->node) { \
		__n = (__s)->node->node; \
	} else { \
		__n = NULL; \
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Node no longer exists"); \
	} \
}

ZEND_API zend_object_value sxe_object_new(zend_class_entry *ce TSRMLS_DC);
/* {{{ php_sxe_fetch_object()
 */
static inline php_sxe_object *
php_sxe_fetch_object(zval *object TSRMLS_DC)
{
	return (php_sxe_object *) zend_object_store_get_object(object TSRMLS_CC);
}
/* }}} */

ZEND_API void php_sxe_reset_iterator(php_sxe_object *sxe TSRMLS_DC);
ZEND_API void php_sxe_move_forward_iterator(php_sxe_object *sxe TSRMLS_DC);

#endif /* PHP_SIMPLEXML_EXPORTS_H */

/**
 * Local Variables:
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: t
 * End:
 * vim600: fdm=marker
 * vim: noet sw=4 ts=4
 */
