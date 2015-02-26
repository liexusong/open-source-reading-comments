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
|          Mike Jackson <mhjack@tscnet.com>                            |
|          Steven Lawrance <slawrance@technologist.com>                |
+----------------------------------------------------------------------+
*/
/* $Id: snmp.c,v 1.39.4.1 2001/05/24 12:42:07 ssb Exp $ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "ext/standard/info.h"
#include "php_snmp.h"
#include <sys/types.h>
#ifdef PHP_WIN32
#include <winsock.h>
#include <errno.h>
#include <process.h>
#include "win32/time.h"
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#ifndef _OSD_POSIX
#include <sys/errno.h>
#else
#include <errno.h>  /* BS2000/OSD uses <errno.h>, not <sys/errno.h> */
#endif
#include <netdb.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_SNMP

#ifndef __P
#ifdef __GNUC__
#define __P(args) args
#else
#define __P(args) ()
#endif
#endif

#ifdef HAVE_DEFAULT_STORE_H
#include "default_store.h"
#endif
#include "asn1.h"
#include "snmp_api.h"
#include "snmp_client.h"
#include "snmp_impl.h"
#include "snmp.h"
#include "parse.h"
#include "mib.h"
#include "version.h"

/* ucd-snmp 3.3.1 changed the name of a few #defines... They've been changed back to the original ones in 3.5.3! */
#ifndef SNMP_MSG_GET
#define SNMP_MSG_GET GET_REQ_MSG
#define SNMP_MSG_GETNEXT GETNEXT_REQ_MSG
#endif

void php_snmp(INTERNAL_FUNCTION_PARAMETERS, int st);

/* constant - can be shared among threads */
static oid objid_mib[] = {1, 3, 6, 1, 2, 1};

function_entry snmp_functions[] = {
		PHP_FE(snmpget, NULL)
		PHP_FE(snmpwalk, NULL)
		PHP_FE(snmprealwalk, NULL)
		PHP_FALIAS(snmpwalkoid, snmprealwalk, NULL)
		PHP_FE(snmp_get_quick_print, NULL)
		PHP_FE(snmp_set_quick_print, NULL)
		PHP_FE(snmpset, NULL)
    {NULL,NULL,NULL}
};

zend_module_entry snmp_module_entry = {
	"snmp",
	snmp_functions,
	PHP_MINIT(snmp),
	NULL,
	NULL,
	NULL,
	PHP_MINFO(snmp),
	STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_SNMP
ZEND_GET_MODULE(snmp)
#endif

/* THREAD_LS snmp_module php_snmp_module; - may need one of these at some point */

PHP_MINIT_FUNCTION(snmp)
{
	init_mib();
	return SUCCESS;
}

PHP_MINFO_FUNCTION(snmp)
{
	php_info_print_table_start();
	php_info_print_table_row(2, "UCD-SNMP Support", "enabled");
	php_info_print_table_row(2, "UCD-SNMP Version", VersionInfo);
	php_info_print_table_end();
}


/*
* Generic SNMP object fetcher
*
* st=1   snmpget() - query an agent and return a single value.
* st=2   snmpwalk() - walk the mib and return a single dimensional array 
*          containing the values.
* st=3 snmprealwalk() and snmpwalkoid() - walk the mib and return an 
*          array of oid,value pairs.
* st=5-8 ** Reserved **
* st=11  snmpset() - query an agent and set a single value
*
*/
void php_snmp(INTERNAL_FUNCTION_PARAMETERS, int st) {
	zval **a1, **a2, **a3, **a4, **a5, **a6, **a7;
	struct snmp_session session, *ss;
	struct snmp_pdu *pdu=NULL, *response;
	struct variable_list *vars;
    char *objid;
    oid name[MAX_NAME_LEN];
    int name_length;
    int status, count,rootlen=0,gotroot=0;
	oid root[MAX_NAME_LEN];
	char buf[2048];
	char buf2[2048];
	int keepwalking=1;
	long timeout=SNMP_DEFAULT_TIMEOUT;
	long retries=SNMP_DEFAULT_RETRIES;
	int myargc = ZEND_NUM_ARGS();
    char type = (char) 0;
    char *value = (char *) 0;
	
	if (myargc < 3 || myargc > 7 ||
		zend_get_parameters_ex(myargc, &a1, &a2, &a3, &a4, &a5, &a6, &a7) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	convert_to_string_ex(a1);
	convert_to_string_ex(a2);
	convert_to_string_ex(a3);
	
	if (st == 11) {
		if (myargc < 5) {
			WRONG_PARAM_COUNT;
		}

		convert_to_string_ex(a4);
		convert_to_string_ex(a5);
	
		if(myargc > 5) {
			convert_to_long_ex(a6);
			timeout = (*a6)->value.lval;
		}

		if(myargc > 6) {
			convert_to_long_ex(a7);
			retries = (*a7)->value.lval;
		}

		type = (*a4)->value.str.val[0];
		value = (*a5)->value.str.val;
	} else {
		if(myargc > 3) {
			convert_to_long_ex(a4);
			timeout = (*a4)->value.lval;
		}

		if(myargc > 4) {
			convert_to_long_ex(a5);
			retries = (*a5)->value.lval;
		}
	}

	objid = (*a3)->value.str.val;
	
	if (st >= 2) { /* walk */
		rootlen = MAX_NAME_LEN;
		if ( strlen(objid) ) { /* on a walk, an empty string means top of tree - no error */
			if ( read_objid(objid, root, &rootlen) ) {
				gotroot = 1;
			} else {
				php_error(E_WARNING,"Invalid object identifier: %s\n", objid);
			}
		}
		if (gotroot == 0) {
			memmove((char *)root, (char *)objid_mib, sizeof(objid_mib));
			rootlen = sizeof(objid_mib) / sizeof(oid);
			gotroot = 1;
		}
	}
	
	memset(&session, 0, sizeof(struct snmp_session));

	session.peername = (*a1)->value.str.val;
	session.version = SNMP_VERSION_1;
	/*
	* FIXME: potential memory leak
	* This is a workaround for an "artifact" (Mike Slifcak)
	* in (at least) ucd-snmp 3.6.1 which frees
	* memory it did not allocate
	*/
#ifdef UCD_SNMP_HACK
	session.community = (u_char *)strdup((*a2)->value.str.val); /* memory freed by SNMP library, strdup NOT estrdup */
#else
	session.community = (u_char *)(*a2)->value.str.val;
#endif
	session.community_len = (*a2)->value.str.len;
	session.retries = retries;
	session.timeout = timeout;
	
	session.authenticator = NULL;
	snmp_synch_setup(&session);

	if ((ss = snmp_open(&session)) == NULL) {
		php_error(E_WARNING,"Could not open snmp\n");
		RETURN_FALSE;
	}

	if (st >= 2) {
		memmove((char *)name, (char *)root, rootlen * sizeof(oid));
		name_length = rootlen;
		if (array_init(return_value) == FAILURE) {
			php_error(E_WARNING, "Cannot prepare result array");
			RETURN_FALSE;
		}
	}

	while(keepwalking) {
		keepwalking=0;
		if (st == 1) {
			pdu = snmp_pdu_create(SNMP_MSG_GET);
			name_length = MAX_NAME_LEN;
			if ( !read_objid(objid, name, &name_length) ) {
				php_error(E_WARNING,"Invalid object identifier: %s\n", objid);
				RETURN_FALSE;
			}
			snmp_add_null_var(pdu, name, name_length);
		} else if (st == 11) {
			pdu = snmp_pdu_create(SNMP_MSG_SET);
			if (snmp_add_var(pdu, name, name_length, type, value)) {
				php_error(E_WARNING,"Could not add variable: %s\n", name);
				RETURN_FALSE;
			}
		} else if (st >= 2) {
			pdu = snmp_pdu_create(SNMP_MSG_GETNEXT);
			snmp_add_null_var(pdu, name, name_length);
		}
		
retry:
		status = snmp_synch_response(ss, pdu, &response);
		if (status == STAT_SUCCESS) {
			if (response->errstat == SNMP_ERR_NOERROR) {
				for (vars = response->variables; vars; vars = vars->next_variable) {
					if (st >= 2 && st != 11 && 
						(vars->name_length < rootlen || memcmp(root, vars->name, rootlen * sizeof(oid)))) {
						continue;       /* not part of this subtree */
					}

					if (st != 11) {
						sprint_value(buf,vars->name, vars->name_length, vars);
					}
#if 0
					Debug("snmp response is: %s\n",buf);
#endif
					if (st == 1) {
						RETVAL_STRING(buf,1);
					} else if (st == 2) {
						add_next_index_string(return_value,buf,1); /* Add to returned array */
					} else if (st == 3)  {
						sprint_objid(buf2, vars->name, vars->name_length);
						add_assoc_string(return_value,buf2,buf,1);
					}
					if (st >= 2 && st != 11) {
						if (vars->type != SNMP_ENDOFMIBVIEW && 
							vars->type != SNMP_NOSUCHOBJECT && vars->type != SNMP_NOSUCHINSTANCE) {
							memmove((char *)name, (char *)vars->name,vars->name_length * sizeof(oid));
							name_length = vars->name_length;
							keepwalking = 1;
						}
					}
				}	
			} else {
				if (st != 2 || response->errstat != SNMP_ERR_NOSUCHNAME) {
					php_error(E_WARNING,"Error in packet.\nReason: %s\n", snmp_errstring(response->errstat));
					if (response->errstat == SNMP_ERR_NOSUCHNAME) {
						for (count=1, vars = response->variables; vars && count != response->errindex;
						vars = vars->next_variable, count++);
						if (vars) {
							sprint_objid(buf,vars->name, vars->name_length);
						}
						php_error(E_WARNING,"This name does not exist: %s\n",buf);
					}
					if (st == 1) {
						if ((pdu = snmp_fix_pdu(response, SNMP_MSG_GET)) != NULL) {
							goto retry;
						}
					} else if (st == 11) {
						if ((pdu = snmp_fix_pdu(response, SNMP_MSG_SET)) != NULL) {
							goto retry;
						}
					} else if (st >= 2) {
						if ((pdu = snmp_fix_pdu(response, SNMP_MSG_GETNEXT)) != NULL) {
							goto retry;
						}
					}
					RETURN_FALSE;
				}
			}
		} else if (status == STAT_TIMEOUT) {
			php_error(E_WARNING,"No Response from %s\n", (*a1)->value.str.val);
			RETURN_FALSE;
		} else {    /* status == STAT_ERROR */
			php_error(E_WARNING,"An error occurred, Quitting...\n");
			RETURN_FALSE;
		}
		if (response) {
			snmp_free_pdu(response);
		}
	} /* keepwalking */
	snmp_close(ss);
}

/* {{{ proto string snmpget(string host, string community, string object_id [, int timeout [, int retries]]) 
   Fetch a SNMP object */
PHP_FUNCTION(snmpget) {
	php_snmp(INTERNAL_FUNCTION_PARAM_PASSTHRU,1);
}
/* }}} */

/* {{{ proto array snmpwalk(string host, string community, string object_id [, int timeout [, int retries]]) 
   Return all objects under the specified object id */
PHP_FUNCTION(snmpwalk) {
	php_snmp(INTERNAL_FUNCTION_PARAM_PASSTHRU,2);
}
/* }}} */

/* {{{ proto array snmprealwalk(string host, string community, string object_id [, int timeout [, int retries]])
   Return all objects including their respective object id withing the specified one */
PHP_FUNCTION(snmprealwalk) {
	php_snmp(INTERNAL_FUNCTION_PARAM_PASSTHRU,3);
}
/* }}} */

/* {{{ proto bool snmp_get_quick_print(void)
   Return the current status of quick_print */
PHP_FUNCTION(snmp_get_quick_print) {
	RETURN_LONG(snmp_get_quick_print() ? 1 : 0);
}
/* }}} */

/* {{{ proto void snmp_set_quick_print(int quick_print)
   Return all objects including their respective object id withing the specified one */
PHP_FUNCTION(snmp_set_quick_print) {
	zval **a1;
	if (ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &a1) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_long_ex(a1);
	snmp_set_quick_print((int)(*a1)->value.lval);
}
/* }}} */

/* {{{ proto int snmpset(string host, string community, string object_id, string type, mixed value [, int timeout [, int retries]]) 
   Set the value of a SNMP object */
PHP_FUNCTION(snmpset) {
	php_snmp(INTERNAL_FUNCTION_PARAM_PASSTHRU,11);
}
/* }}} */

#endif

/*
* Local variables:
* tab-width: 4
* c-basic-offset: 4
* End:
*/
