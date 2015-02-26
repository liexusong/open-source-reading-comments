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
   | Authors: Tom May <tom@go2net.com>                                    |
   +----------------------------------------------------------------------+
*/

/* $Id: php_sysvsem.h,v 1.8 2001/02/26 06:07:24 andi Exp $ */

#ifndef PHP_SYSVSEM_H
#define PHP_SYSVSEM_H

#if HAVE_SYSVSEM

extern zend_module_entry sysvsem_module_entry;
#define sysvsem_module_ptr &sysvsem_module_entry

PHP_MINIT_FUNCTION(sysvsem);
PHP_FUNCTION(sem_get);
PHP_FUNCTION(sem_acquire);
PHP_FUNCTION(sem_release);

typedef struct {
	int le_sem;
} sysvsem_module;

typedef struct {
	int id;						/* For error reporting. */
	int key;					/* For error reporting. */
	int semid;					/* Returned by semget(). */
	int count;					/* Acquire count for auto-release. */
} sysvsem_sem;

extern sysvsem_module php_sysvsem_module;

#else

#define sysvsem_module_ptr NULL

#endif

#define phpext_sysvsem_ptr sysvsem_module_ptr

#endif /* PHP_SYSVSEM_H */
