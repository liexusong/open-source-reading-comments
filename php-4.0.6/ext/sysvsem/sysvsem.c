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
 
/* $Id: sysvsem.c,v 1.27.4.1 2001/05/24 12:42:09 ssb Exp $ */

/* This has been built and tested on Solaris 2.6 and Linux 2.1.122.
 * It may not compile or execute correctly on other systems.
 *
 * sas: Works for me on Linux 2.0.36 and FreeBSD 3.0-current
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"

#if HAVE_SYSVSEM

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <errno.h>

#include "php_sysvsem.h"

#if !HAVE_SEMUN

union semun {
	int val;                    /* value for SETVAL */
	struct semid_ds *buf;       /* buffer for IPC_STAT, IPC_SET */
	unsigned short int *array;  /* array for GETALL, SETALL */
	struct seminfo *__buf;      /* buffer for IPC_INFO */
};

#undef HAVE_SEMUN
#define HAVE_SEMUN 1

#endif

function_entry sysvsem_functions[] = {
	PHP_FE(sem_get,			NULL)
	PHP_FE(sem_acquire,		NULL)
	PHP_FE(sem_release,		NULL)
	{NULL, NULL, NULL}
};

zend_module_entry sysvsem_module_entry = {
	"sysvsem", sysvsem_functions, PHP_MINIT(sysvsem), NULL, NULL, NULL, NULL, STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_SYSVSEM
ZEND_GET_MODULE(sysvsem)
#endif


THREAD_LS sysvsem_module php_sysvsem_module;

/* Semaphore functions using System V semaphores.  Each semaphore
 * actually consists of three semaphores allocated as a unit under the
 * same key.  Semaphore 0 (SYSVSEM_SEM) is the actual semaphore, it is
 * initialized to max_acquire and decremented as processes acquire it.
 * The value of semaphore 1 (SYSVSEM_USAGE) is a count of the number
 * of processes using the semaphore.  After calling semget(), if a
 * process finds that the usage count is 1, it will set the value of
 * SYSVSEM_SEM to max_acquire.  This allows max_acquire to be set and
 * track the PHP code without having a global init routine or external
 * semaphore init code.  Except see the bug regarding a race condition
 * php_sysvsem_get().  Semaphore 2 (SYSVSEM_SETVAL) serializes the
 * calls to GETVAL SYSVSEM_USAGE and SETVAL SYSVSEM_SEM.  It can be
 * acquired only when it is zero.
 */

#define SYSVSEM_SEM		0
#define SYSVSEM_USAGE	1
#define SYSVSEM_SETVAL	2


static void release_sysvsem_sem(zend_rsrc_list_entry *rsrc)
{
	sysvsem_sem *sem_ptr = (sysvsem_sem *)rsrc->ptr;
	struct sembuf sop[2];

	/* Decrement the usage count. */

	sop[0].sem_num = SYSVSEM_USAGE;
	sop[0].sem_op  = -1;
	sop[0].sem_flg = SEM_UNDO;

	/* Release the semaphore if it has been acquired but not released. */

	if (sem_ptr->count) {
		php_error(E_WARNING, "Releasing SysV semaphore id %d key 0x%x in request cleanup", sem_ptr->id, sem_ptr->key);

		sop[1].sem_num = SYSVSEM_SEM;
		sop[1].sem_op  = sem_ptr->count;
		sop[1].sem_flg = SEM_UNDO;
	}
	if (semop(sem_ptr->semid, sop, sem_ptr->count ? 2 : 1) == -1) {
		php_error(E_WARNING, "semop() failed in release_sysvsem_sem for key 0x%x: %s", sem_ptr->key, strerror(errno));
	}

	efree(sem_ptr);
}


PHP_MINIT_FUNCTION(sysvsem)
{
	php_sysvsem_module.le_sem = zend_register_list_destructors_ex(release_sysvsem_sem, NULL, "sysvsem", module_number);

	return SUCCESS;
}

#define SETVAL_WANTS_PTR

#if defined(_AIX)
#undef SETVAL_WANTS_PTR
#endif

/* {{{ proto int sem_get(int key [, int max_acquire [, int perm]])
   Return an id for the semaphore with the given key, and allow max_acquire (default 1) processes to acquire it simultaneously */
PHP_FUNCTION(sem_get)
{
	pval **arg_key, **arg_max_acquire, **arg_perm;
	int key, max_acquire, perm;
    int semid;
	struct sembuf sop[3];
	int count;
	sysvsem_sem *sem_ptr;
#if HAVE_SEMUN
	union semun un;
#endif

	max_acquire = 1;
	perm = 0666;

	switch (ZEND_NUM_ARGS()) {
		case 1:
			if (zend_get_parameters_ex(1, &arg_key)==FAILURE) {
				RETURN_FALSE;
			}
			convert_to_long_ex(arg_key);
			key = (int)(*arg_key)->value.lval;
			break;
		case 2:
			if (zend_get_parameters_ex(2, &arg_key, &arg_max_acquire)==FAILURE) {
				RETURN_FALSE;
			}
			convert_to_long_ex(arg_key);
			key = (int)(*arg_key)->value.lval;
			convert_to_long_ex(arg_max_acquire);
			max_acquire = (int)(*arg_max_acquire)->value.lval;
			break;
		case 3:
			if (zend_get_parameters_ex(3, &arg_key, &arg_max_acquire, &arg_perm)==FAILURE) {
				RETURN_FALSE;
			}
			convert_to_long_ex(arg_key);
			convert_to_long_ex(arg_max_acquire);
			convert_to_long_ex(arg_perm);
			key = (int)(*arg_key)->value.lval;
			max_acquire = (int)(*arg_max_acquire)->value.lval;
			perm = (int)(*arg_perm)->value.lval;
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}

	/* Get/create the semaphore.  Note that we rely on the semaphores
	 * being zeroed when they are created.  Despite the fact that
	 * the(?)  Linux semget() man page says they are not initialized,
	 * the kernel versions 2.0.x and 2.1.z do in fact zero them.
	 */

    semid = semget(key, 3, perm|IPC_CREAT);
	if (semid == -1) {
		php_error(E_WARNING, "semget() failed for key 0x%x: %s", key, strerror(errno));
		RETURN_FALSE;
	}

	/* Find out how many processes are using this semaphore.  Note
	 * that on Linux (at least) there is a race condition here because
	 * semaphore undo on process exit is not atomic, so we could
	 * acquire SYSVSEM_SETVAL before a crashed process has decremented
	 * SYSVSEM_USAGE in which case count will be greater than it
	 * should be and we won't set max_acquire.  Fortunately this
	 * doesn't actually matter in practice.
	 */

	/* Wait for sem 1 to be zero . . . */

	sop[0].sem_num = SYSVSEM_SETVAL;
	sop[0].sem_op  = 0;
	sop[0].sem_flg = 0;

	/* . . . and increment it so it becomes non-zero . . . */

	sop[1].sem_num = SYSVSEM_SETVAL;
	sop[1].sem_op  = 1;
	sop[1].sem_flg = SEM_UNDO;

	/* . . . and increment the usage count. */

	sop[2].sem_num = SYSVSEM_USAGE;
	sop[2].sem_op  = 1;
	sop[2].sem_flg = SEM_UNDO;
	while (semop(semid, sop, 3) == -1) {
		if (errno != EINTR) {
			php_error(E_WARNING, "semop() failed acquiring SYSVSEM_SETVAL for key 0x%x: %s", key, strerror(errno));
			break;
		}
	}

	/* Get the usage count. */
#if HAVE_SEMUN
	count = semctl(semid, SYSVSEM_USAGE, GETVAL, un);
#else
	count = semctl(semid, SYSVSEM_USAGE, GETVAL, NULL);
#endif
	if (count == -1) {
		php_error(E_WARNING, "semctl(GETVAL) failed for key 0x%x: %s", key, strerror(errno));
	}

	/* If we are the only user, then take this opportunity to set the max. */

	if (count == 1) {
#if HAVE_SEMUN
		/* This is correct for Linux which has union semun. */
		union semun semarg;
		semarg.val = max_acquire;
		if (semctl(semid, SYSVSEM_SEM, SETVAL, semarg) == -1) {
			php_error(E_WARNING, "semctl(SETVAL) failed for key 0x%x: %s", key, strerror(errno));
		}
#elif defined(SETVAL_WANTS_PTR)
		/* This is correct for Solaris 2.6 which does not have union semun. */
		if (semctl(semid, SYSVSEM_SEM, SETVAL, &max_acquire) == -1) {
			php_error(E_WARNING, "semctl(SETVAL) failed for key 0x%x: %s", key, strerror(errno));
		}
#else
		/* This works for i.e. AIX */
		if (semctl(semid, SYSVSEM_SEM, SETVAL, max_acquire) == -1) {
			php_error(E_WARNING, "semctl(SETVAL) failed for key 0x%x: %s", key, strerror(errno));
		}
#endif
	}

	/* Set semaphore 1 back to zero. */

	sop[0].sem_num = SYSVSEM_SETVAL;
	sop[0].sem_op  = -1;
	sop[0].sem_flg = SEM_UNDO;
	while (semop(semid, sop, 1) == -1) {
		if (errno != EINTR) {
			php_error(E_WARNING, "semop() failed releasing SYSVSEM_SETVAL for key 0x%x: %s", key, strerror(errno));
			break;
		}
	}

	sem_ptr = (sysvsem_sem *) emalloc(sizeof(sysvsem_sem));
	sem_ptr->key   = key;
	sem_ptr->semid = semid;
	sem_ptr->count = 0;

	return_value->value.lval = zend_list_insert(sem_ptr, php_sysvsem_module.le_sem);
	return_value->type = IS_LONG;

	sem_ptr->id = (int)return_value->value.lval;
}
/* }}} */


static void php_sysvsem_semop(INTERNAL_FUNCTION_PARAMETERS, int acquire)
{
	pval **arg_id;
	int id, type;
	sysvsem_sem *sem_ptr;
    struct sembuf sop;

	switch(ZEND_NUM_ARGS()) {
		case 1:
			if (zend_get_parameters_ex(1, &arg_id)==FAILURE) {
				RETURN_FALSE;
			}
			convert_to_long_ex(arg_id);
			id = (int)(*arg_id)->value.lval;
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}

	sem_ptr = (sysvsem_sem *) zend_list_find(id, &type);
	if (type!=php_sysvsem_module.le_sem) {
		php_error(E_WARNING, "%d is not a SysV semaphore index", id);
		RETURN_FALSE;
	}

	if (!acquire && sem_ptr->count == 0) {
		php_error(E_WARNING, "SysV semaphore index %d (key 0x%x) is not currently acquired", id, sem_ptr->key);
		RETURN_FALSE;
	}

    sop.sem_num = SYSVSEM_SEM;
    sop.sem_op  = acquire ? -1 : 1;
    sop.sem_flg = SEM_UNDO;

    while (semop(sem_ptr->semid, &sop, 1) == -1) {
		if (errno != EINTR) {
			php_error(E_WARNING, "semop(%s) failed for key 0x%x: %s",
					   acquire ? "acquire" : "release",  sem_ptr->key, strerror(errno));
			RETURN_FALSE;
		}
	}

	sem_ptr->count -= acquire ? -1 : 1;
	RETURN_TRUE;
}


/* {{{ proto int sem_acquire(int id)
   Acquires the semaphore with the given id, blocking if necessary */
PHP_FUNCTION(sem_acquire)
{
	php_sysvsem_semop(INTERNAL_FUNCTION_PARAM_PASSTHRU, 1);
}
/* }}} */

/* {{{ proto int sem_release(int id)
   Releases the semaphore with the given id */
PHP_FUNCTION(sem_release)
{
	php_sysvsem_semop(INTERNAL_FUNCTION_PARAM_PASSTHRU, 0);
}
/* }}} */

#endif /* HAVE_SYSVSEM */
/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */
