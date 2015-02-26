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
   | Authors: Stig S�ther Bakken <ssb@fast.no>                            |
   +----------------------------------------------------------------------+
 */

/* $Id: syslog.c,v 1.26 2001/02/26 06:07:23 andi Exp $ */

#include "php.h"

#ifdef HAVE_SYSLOG_H
#include "php_ini.h"
#include "zend_globals.h"

#include <stdlib.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <string.h>
#include <errno.h>

#include <stdio.h>
#include "basic_functions.h"
#include "php_ext_syslog.h"

static void start_syslog(BLS_D);

PHP_MINIT_FUNCTION(syslog)
{
	/* error levels */
	REGISTER_LONG_CONSTANT("LOG_EMERG", LOG_EMERG, CONST_CS | CONST_PERSISTENT); /* system unusable */
	REGISTER_LONG_CONSTANT("LOG_ALERT", LOG_ALERT, CONST_CS | CONST_PERSISTENT); /* immediate action required */
	REGISTER_LONG_CONSTANT("LOG_CRIT", LOG_CRIT, CONST_CS | CONST_PERSISTENT); /* critical conditions */
	REGISTER_LONG_CONSTANT("LOG_ERR", LOG_ERR, CONST_CS | CONST_PERSISTENT); 
	REGISTER_LONG_CONSTANT("LOG_WARNING", LOG_WARNING, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("LOG_NOTICE", LOG_NOTICE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("LOG_INFO", LOG_INFO, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("LOG_DEBUG", LOG_DEBUG, CONST_CS | CONST_PERSISTENT);
	/* facility: type of program logging the message */
	REGISTER_LONG_CONSTANT("LOG_KERN", LOG_KERN, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("LOG_USER", LOG_USER, CONST_CS | CONST_PERSISTENT); /* generic user level */
	REGISTER_LONG_CONSTANT("LOG_MAIL", LOG_MAIL, CONST_CS | CONST_PERSISTENT); /* log to email */
	REGISTER_LONG_CONSTANT("LOG_DAEMON", LOG_DAEMON, CONST_CS | CONST_PERSISTENT); /* other system daemons */
	REGISTER_LONG_CONSTANT("LOG_AUTH", LOG_AUTH, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("LOG_SYSLOG", LOG_SYSLOG, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("LOG_LPR", LOG_LPR, CONST_CS | CONST_PERSISTENT);
#ifdef LOG_NEWS
	/* No LOG_NEWS on HP-UX */
	REGISTER_LONG_CONSTANT("LOG_NEWS", LOG_NEWS, CONST_CS | CONST_PERSISTENT); /* usenet new */
#endif
#ifdef LOG_UUCP
	/* No LOG_UUCP on HP-UX */
	REGISTER_LONG_CONSTANT("LOG_UUCP", LOG_UUCP, CONST_CS | CONST_PERSISTENT);
#endif
#ifdef LOG_CRON
	/* apparently some systems don't have this one */
	REGISTER_LONG_CONSTANT("LOG_CRON", LOG_CRON, CONST_CS | CONST_PERSISTENT);
#endif
#ifdef LOG_AUTHPRIV
	/* AIX doesn't have LOG_AUTHPRIV */
	REGISTER_LONG_CONSTANT("LOG_AUTHPRIV", LOG_AUTHPRIV, CONST_CS | CONST_PERSISTENT);
#endif
#if !defined(PHP_WIN32)
	REGISTER_LONG_CONSTANT("LOG_LOCAL0", LOG_LOCAL0, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("LOG_LOCAL1", LOG_LOCAL1, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("LOG_LOCAL2", LOG_LOCAL2, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("LOG_LOCAL3", LOG_LOCAL3, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("LOG_LOCAL4", LOG_LOCAL4, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("LOG_LOCAL5", LOG_LOCAL5, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("LOG_LOCAL6", LOG_LOCAL6, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("LOG_LOCAL7", LOG_LOCAL7, CONST_CS | CONST_PERSISTENT);
#endif
	/* options */
	REGISTER_LONG_CONSTANT("LOG_PID", LOG_PID, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("LOG_CONS", LOG_CONS, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("LOG_ODELAY", LOG_ODELAY, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("LOG_NDELAY", LOG_NDELAY, CONST_CS | CONST_PERSISTENT);
#ifdef LOG_NOWAIT
	REGISTER_LONG_CONSTANT("LOG_NOWAIT", LOG_NOWAIT, CONST_CS | CONST_PERSISTENT);
#endif
#ifdef LOG_PERROR
	/* AIX doesn't have LOG_PERROR */
	REGISTER_LONG_CONSTANT("LOG_PERROR", LOG_PERROR, CONST_CS | CONST_PERSISTENT); /*log to stderr*/
#endif

	return SUCCESS;
}


PHP_RINIT_FUNCTION(syslog)
{
	BLS_FETCH();

	if (INI_INT("define_syslog_variables")) {
		start_syslog(BLS_C);
	} else {
		BG(syslog_started)=0;
	}
	BG(syslog_device)=NULL;
	return SUCCESS;
}


PHP_RSHUTDOWN_FUNCTION(syslog)
{
	BLS_FETCH();
	
	if (BG(syslog_device)) {
		efree(BG(syslog_device));
	}
	return SUCCESS;
}


static void start_syslog(BLS_D)
{
	ELS_FETCH();
	
	/* error levels */
	SET_VAR_LONG("LOG_EMERG", LOG_EMERG); /* system unusable */
	SET_VAR_LONG("LOG_ALERT", LOG_ALERT); /* immediate action required */
	SET_VAR_LONG("LOG_CRIT", LOG_CRIT); /* critical conditions */
	SET_VAR_LONG("LOG_ERR", LOG_ERR); 
	SET_VAR_LONG("LOG_WARNING", LOG_WARNING);
	SET_VAR_LONG("LOG_NOTICE", LOG_NOTICE);
	SET_VAR_LONG("LOG_INFO", LOG_INFO);
	SET_VAR_LONG("LOG_DEBUG", LOG_DEBUG);
	/* facility: type of program logging the message */
	SET_VAR_LONG("LOG_KERN", LOG_KERN);
	SET_VAR_LONG("LOG_USER", LOG_USER); /* generic user level */
	SET_VAR_LONG("LOG_MAIL", LOG_MAIL); /* log to email */
	SET_VAR_LONG("LOG_DAEMON", LOG_DAEMON); /* other system daemons */
	SET_VAR_LONG("LOG_AUTH", LOG_AUTH);
	SET_VAR_LONG("LOG_SYSLOG", LOG_SYSLOG);
	SET_VAR_LONG("LOG_LPR", LOG_LPR);
#ifdef LOG_NEWS
	/* No LOG_NEWS on HP-UX */
	SET_VAR_LONG("LOG_NEWS", LOG_NEWS); /* usenet new */
#endif
#ifdef LOG_UUCP
	/* No LOG_UUCP on HP-UX */
	SET_VAR_LONG("LOG_UUCP", LOG_UUCP);
#endif
#ifdef LOG_CRON
	/* apparently some systems don't have this one */
	SET_VAR_LONG("LOG_CRON", LOG_CRON);
#endif
#ifdef LOG_AUTHPRIV
	/* AIX doesn't have LOG_AUTHPRIV */
	SET_VAR_LONG("LOG_AUTHPRIV", LOG_AUTHPRIV);
#endif
#if !defined(PHP_WIN32)
	SET_VAR_LONG("LOG_LOCAL0", LOG_LOCAL0);
	SET_VAR_LONG("LOG_LOCAL1", LOG_LOCAL1);
	SET_VAR_LONG("LOG_LOCAL2", LOG_LOCAL2);
	SET_VAR_LONG("LOG_LOCAL3", LOG_LOCAL3);
	SET_VAR_LONG("LOG_LOCAL4", LOG_LOCAL4);
	SET_VAR_LONG("LOG_LOCAL5", LOG_LOCAL5);
	SET_VAR_LONG("LOG_LOCAL6", LOG_LOCAL6);
	SET_VAR_LONG("LOG_LOCAL7", LOG_LOCAL7);
#endif
	/* options */
	SET_VAR_LONG("LOG_PID", LOG_PID);
	SET_VAR_LONG("LOG_CONS", LOG_CONS);
	SET_VAR_LONG("LOG_ODELAY", LOG_ODELAY);
	SET_VAR_LONG("LOG_NDELAY", LOG_NDELAY);
#ifdef LOG_NOWAIT
	/* BeOS doesn't have LOG_NOWAIT */
	SET_VAR_LONG("LOG_NOWAIT", LOG_NOWAIT);
#endif
#ifdef LOG_PERROR
	/* AIX doesn't have LOG_PERROR */
	SET_VAR_LONG("LOG_PERROR", LOG_PERROR); /*log to stderr*/
#endif

	BG(syslog_started)=1;
}

/* {{{ proto void define_syslog_variables(void)
   Initializes all syslog-related variables */
PHP_FUNCTION(define_syslog_variables)
{
	BLS_FETCH();

	if (!BG(syslog_started)) {
		start_syslog(BLS_C);
	}
}

/* {{{ proto int openlog(string ident, int option, int facility)
   Open connection to system logger */
/*
   ** OpenLog("nettopp", $LOG_PID, $LOG_LOCAL1);
   ** Syslog($LOG_EMERG, "help me!")
   ** CloseLog();
 */
PHP_FUNCTION(openlog)
{
	pval **ident, **option, **facility;
	BLS_FETCH();

	if (ZEND_NUM_ARGS() != 3 || zend_get_parameters_ex(3, &ident, &option, &facility) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(ident);
	convert_to_long_ex(option);
	convert_to_long_ex(facility);
	if (BG(syslog_device)) {
		efree(BG(syslog_device));
	}
	BG(syslog_device) = estrndup((*ident)->value.str.val,(*ident)->value.str.len);
	openlog(BG(syslog_device), (*option)->value.lval, (*facility)->value.lval);
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto int closelog(void)
   Close connection to system logger */
PHP_FUNCTION(closelog)
{
	BLS_FETCH();

	closelog();
	if (BG(syslog_device)) {
		efree(BG(syslog_device));
		BG(syslog_device)=NULL;
	}
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto int syslog(int priority, string message)
   Generate a system log message */
PHP_FUNCTION(syslog)
{
	pval **priority, **message;

	if (ZEND_NUM_ARGS() != 2 || zend_get_parameters_ex(2, &priority, &message) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_long_ex(priority);
	convert_to_string_ex(message);

	/*
	 * CAVEAT: if the message contains patterns such as "%s",
	 * this will cause problems.
	 */

	php_syslog((*priority)->value.lval, "%.500s",(*message)->value.str.val);
	RETURN_TRUE;
}
/* }}} */

#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */
