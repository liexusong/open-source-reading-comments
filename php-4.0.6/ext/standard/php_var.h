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
   | Authors: Jani Lehtim�ki <jkl@njet.net>                               |
   +----------------------------------------------------------------------+
*/

/* $Id: php_var.h,v 1.6 2001/02/26 06:07:23 andi Exp $ */

#ifndef PHP_VAR_H
#define PHP_VAR_H

PHP_FUNCTION(var_dump);
PHP_FUNCTION(serialize);
PHP_FUNCTION(unserialize);

void php_var_dump(pval **struc, int level);

/* typdef HashTable php_serialize_data_t; */
#define php_serialize_data_t HashTable

void php_var_serialize(pval *buf, pval **struc, php_serialize_data_t *var_hash);
int php_var_unserialize(pval **rval, const char **p, const char *max, php_serialize_data_t *var_hash);

#define PHP_VAR_SERIALIZE_INIT(var_hash) \
   zend_hash_init(&(var_hash),10,NULL,NULL,0)
#define PHP_VAR_SERIALIZE_DESTROY(var_hash) \
   zend_hash_destroy(&(var_hash))

#define PHP_VAR_UNSERIALIZE_INIT(var_hash) \
   zend_hash_init(&(var_hash),10,NULL,NULL,0)
#define PHP_VAR_UNSERIALIZE_DESTROY(var_hash) \
   zend_hash_destroy(&(var_hash))

PHPAPI zend_class_entry *php_create_empty_class(char *class_name,int len);

#endif /* PHP_VAR_H */
