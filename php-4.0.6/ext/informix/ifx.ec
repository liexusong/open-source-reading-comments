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
   | Authors: Danny Heijl  <Danny.Heijl@cevi.be> : initial cut (ODS 7.2x) |
   |                                               PHP4 port              |
   |          Christian Cartus <chc@idgruppe.de> : blobs, and IUS 9       |
   |          Jouni Ahto <jouni.ahto@exdec.fi>   : configuration stuff    |
   | Based on the MySQL code by:  Zeev Suraski <zeev@php.net>             |
   +----------------------------------------------------------------------+
*/

/* $Id: ifx.ec,v 1.53 2001/04/01 05:55:20 sniper Exp $ */

/* -------------------------------------------------------------------
 * if you want a function reference : "grep '^\*\*' ifx.ec" will give
 * you a very short one
 * -------------------------------------------------------------------
*/

/* TODO:
 *
 * ? Safe mode implementation
 */


/* prevent mod_ssl.h's header file from being included. */
#define AP_HOOK_H

#include "php.h"
#include "php_globals.h"
#include "ext/standard/php_standard.h"
#include "php_informix.h"
#include "php_globals.h"
#include "php_ini.h"

#ifdef PHP_WIN32
#include <winsock.h>
#else
#include "build-defs.h"

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#include <netdb.h>
#include <netinet/in.h>
#endif



#if HAVE_IFX

                            /* local function prototypes */
static void php3_ifx_set_default_link(int id);
static long php3_intifx_getType(long id, HashTable *list);
static long php3_intifx_create_blob(long type, long mode, char* param, long len, HashTable *list);
static long php3_intifx_free_blob(long id, HashTable *list);
static long php3_intifx2_free_blob(long id, HashTable *list);
static long php3_intifx_get_blob(long bid, HashTable *list, char** content);
static long php3_intifx_update_blob(long bid, char* param, long len, HashTable *list);
static loc_t *php3_intifx_get_blobloc(long bid, HashTable *list);
static char* php3_intifx_create_tmpfile(long bid);
static long php3_intifx_copy_blob(long bid, HashTable *list);
static char* php3_intifx_null();
static long php3_intifx_create_char(char* param, long len, HashTable *list);
static long php3_intifx_free_char(long id, HashTable *list);
static long php3_intifx_update_char(long bid, char* param, long len, HashTable *list);
static long php3_intifx_get_char(long bid, HashTable *list, char** content);
#if HAVE_IFX_IUS
static long php3_intifxus_create_slob(long create_mode, HashTable *list);
static long php3_intifxus_free_slob(long bid, HashTable *list);
static long php3_intifxus_close_slob(long bid, HashTable *list);
static long php3_intifxus_open_slob(long bid, long create_mode, HashTable *list);
static long php3_intifxus_new_slob(HashTable *list);
static ifx_lo_t *php3_intifxus_get_slobloc(long bid, HashTable *list);
#endif

#ifndef CLIENT_SQLI_VER	             /* 7.10 on (at least) AIX is missing this */
#define CLIENT_SQLI_VER IFX_VERSION
#endif

#define TYPE_BLBYTE 0
#define TYPE_BLTEXT 1
#define TYPE_SLOB 2
#define TYPE_CHAR 3


#define BLMODE_INMEM    0
#define BLMODE_INFILE   1


#define IFX_SCROLL  1
#define IFX_HOLD    2

EXEC SQL include locator;
EXEC SQL include sqltypes;
EXEC SQL include sqlstype;

#include <errno.h>

typedef char IFX[128];

#define SAFE_STRING(s) ((s)?(s):"")

function_entry ifx_functions[] = {
    PHP_FE(ifx_connect,            NULL)
    PHP_FE(ifx_pconnect,           NULL)
    PHP_FE(ifx_close,              NULL)
    PHP_FE(ifx_query,              NULL)
    PHP_FE(ifx_prepare,            NULL)
    PHP_FE(ifx_do,                 NULL)
    PHP_FE(ifx_error,              NULL)
    PHP_FE(ifx_errormsg,           NULL)
    PHP_FE(ifx_affected_rows,      NULL)
    PHP_FE(ifx_num_rows,           NULL)
    PHP_FE(ifx_num_fields,         NULL)
    PHP_FE(ifx_fetch_row,          NULL)
    PHP_FE(ifx_free_result,        NULL)
    PHP_FE(ifx_htmltbl_result,     NULL)
    PHP_FE(ifx_fieldtypes,         NULL)
    PHP_FE(ifx_fieldproperties,    NULL)
    PHP_FE(ifx_getsqlca,           NULL)

    PHP_FE(ifx_create_blob,        NULL)
    PHP_FE(ifx_free_blob,          NULL)
    PHP_FE(ifx_get_blob,           NULL)
    PHP_FE(ifx_update_blob,        NULL)
    PHP_FE(ifx_copy_blob,          NULL)
    PHP_FE(ifx_textasvarchar,      NULL)
    PHP_FE(ifx_byteasvarchar,      NULL)
    PHP_FE(ifx_nullformat,         NULL)
    PHP_FE(ifx_blobinfile_mode,    NULL)

    PHP_FE(ifx_create_char,        NULL)
    PHP_FE(ifx_free_char,          NULL)
    PHP_FE(ifx_get_char,           NULL)
    PHP_FE(ifx_update_char,        NULL)

$ifdef HAVE_IFX_IUS;
    PHP_FE(ifxus_create_slob,      NULL)
    PHP_FE(ifxus_close_slob,       NULL)
    PHP_FE(ifxus_open_slob,        NULL)
    PHP_FE(ifxus_free_slob,        NULL)
    PHP_FE(ifxus_read_slob,        NULL)
    PHP_FE(ifxus_write_slob,       NULL)
    PHP_FE(ifxus_seek_slob,        NULL)
    PHP_FE(ifxus_tell_slob,        NULL)
$endif;
    
    {NULL,  NULL,                  NULL}
};

zend_module_entry ifx_module_entry = {
    "Informix", 
    ifx_functions, 
    PHP_MINIT(ifx), 
    PHP_MSHUTDOWN(ifx), 
    PHP_RINIT(ifx), 
    NULL, 
    PHP_MINFO(ifx),
    STANDARD_MODULE_PROPERTIES
};

static php_ifx_listids ifx_listids;	/* these are globals, no thread safety needed says zeeev */


#ifdef COMPILE_DL_INFORMIX
ZEND_GET_MODULE(ifx)
#if 0
BOOL WINAPI DllMain(HANDLE hModule, 
                      DWORD  ul_reason_for_call, 
                      LPVOID lpReserved)
{
    return 1;
}
#endif
#endif

#ifdef ZTS 
int ifx_globals_id;
#else
PHP_IFX_API php_ifx_globals ifx_globals;
#endif


#define CHECK_LINK(link) { if (link==0) { php_error(E_WARNING, "Informix:  A link to the server could not be established"); RETURN_FALSE; }}

#define DUP    1

EXEC SQL DEFINE IFX_SUCCESS 0;
EXEC SQL DEFINE IFX_WARNING 1;
EXEC SQL DEFINE IFX_ERROR  -1;
EXEC SQL DEFINE IFX_NODATA 100;

static int ifx_check()
{
    int _ifx_check;

    _ifx_check = IFX_ERROR;
    
    if (SQLSTATE[0] == '0') {
        switch (SQLSTATE[1]) {
            case '0':
                _ifx_check = IFX_SUCCESS;
                break;
            case '1':
                _ifx_check = IFX_WARNING;
                break;
            case '2':
                _ifx_check = IFX_NODATA;
                break;
            default :
                _ifx_check = IFX_ERROR;
                break;
        }  
    }

    return _ifx_check;
}

static char *ifx_error(ifx)
    EXEC SQL BEGIN DECLARE SECTION;
        PARAMETER char *ifx;
    EXEC SQL END DECLARE SECTION;
{
   char *ifx_err_msg;
   char   c;
   int errorcode;

   IFXLS_FETCH();

   if (IFXG(sv_sqlcode) == 0)
       errorcode = SQLCODE;
   else
       errorcode = IFXG(sv_sqlcode);

   switch (ifx_check()) {
       case IFX_SUCCESS:
         c = ' ';
         break;       
       case IFX_WARNING:
         c = 'W';
         break;       
       case IFX_ERROR:
         c = 'E';
         break;       
       case IFX_NODATA:
         c = 'N';
         break;       
       default:
         c = '?';
         break;       
   }    
   ifx_err_msg = emalloc(64);
   sprintf(ifx_err_msg,"%c [SQLSTATE=%c%c %c%c%c  SQLCODE=%d]",
                        c,
                        SQLSTATE[0],
                        SQLSTATE[1],
                        SQLSTATE[2],
                        SQLSTATE[3],
                        SQLSTATE[4],
                        errorcode);
   return(ifx_err_msg);
}



static void _close_ifx_link(link)
    EXEC SQL BEGIN DECLARE SECTION;
        PARAMETER char *link;
    EXEC SQL END DECLARE SECTION;
{

    IFXLS_FETCH();

    EXEC SQL SET CONNECTION :link;
    if (ifx_check() >= 0) {
      EXEC SQL close database;
      EXEC SQL DISCONNECT CURRENT;
    }
    efree(link);
    IFXG(num_links)--;
}

static void _close_ifx_plink(link)
EXEC SQL BEGIN DECLARE SECTION;
  PARAMETER char *link;
EXEC SQL END DECLARE SECTION;
{

    IFXLS_FETCH();

    EXEC SQL SET CONNECTION :link;
    if (ifx_check() >= 0) {
      EXEC SQL close database;
      EXEC SQL DISCONNECT CURRENT;
    }
    free(link);
    IFXG(num_persistent)--;
    IFXG(num_links)--;
}

static void ifx_free_result(a_result_id)
char *a_result_id;
{
  return;
}


/* static PHP_INI_DISP(display_link_numbers)
{
  char *value;

  if (type==PHP_INI_DISPLAY_ORIG && ini_entry->modified) {
     value = ini_entry->orig_value;
  } else {
    if (ini_entry->value) {
      value = ini_entry->value;
    } else {
      value = NULL;
    }
  }	 
  if (value) {
    if (atoi(value)==-1) {
      PUTS("Unlimited");
    } else {
      php_printf("%s", value);
    }
  }
}
*/						
						


PHP_INI_BEGIN()
    STD_PHP_INI_BOOLEAN("ifx.allow_persistent", "1", PHP_INI_SYSTEM, 
                       OnUpdateInt, allow_persistent, php_ifx_globals, ifx_globals)
    STD_PHP_INI_ENTRY_EX("ifx.max_persistent", "-1", PHP_INI_SYSTEM, 
                       OnUpdateInt, max_persistent, php_ifx_globals, ifx_globals,
                       display_link_numbers)
    STD_PHP_INI_ENTRY_EX("ifx.max_links", "-1", PHP_INI_SYSTEM, 
                       OnUpdateInt, max_links, php_ifx_globals, ifx_globals,
		       display_link_numbers)
    STD_PHP_INI_ENTRY("ifx.default_host", NULL, PHP_INI_SYSTEM, 
                       OnUpdateString, default_host, php_ifx_globals, ifx_globals)
    STD_PHP_INI_ENTRY("ifx.default_user", NULL, PHP_INI_SYSTEM, 
                       OnUpdateString, default_user, php_ifx_globals, ifx_globals)
    STD_PHP_INI_ENTRY("ifx.default_password", NULL, PHP_INI_SYSTEM, 
                       OnUpdateString, default_password, php_ifx_globals, ifx_globals)
    STD_PHP_INI_ENTRY("ifx.blobinfile", "1", PHP_INI_ALL, 
                       OnUpdateInt, blobinfile, php_ifx_globals, ifx_globals)
    STD_PHP_INI_ENTRY("ifx.textasvarchar", "0", PHP_INI_ALL, 
                       OnUpdateInt, textasvarchar, php_ifx_globals, ifx_globals)
    STD_PHP_INI_ENTRY("ifx.byteasvarchar", "0", PHP_INI_ALL, 
                       OnUpdateInt, byteasvarchar, php_ifx_globals, ifx_globals)
    STD_PHP_INI_ENTRY("ifx.charasvarchar", "0", PHP_INI_ALL, 
                       OnUpdateInt, charasvarchar, php_ifx_globals, ifx_globals)
    STD_PHP_INI_ENTRY("ifx.nullformat", "0", PHP_INI_ALL, 
                       OnUpdateInt, nullformat, php_ifx_globals, ifx_globals)
PHP_INI_END()

#ifdef ZTS
static void php_ifx_init_globals(php_ifx_globals *ifx_globals)
{
    IFXG(num_persistent) = 0;
    IFXG(nullvalue) = malloc(1);
    IFXG(nullvalue)[0] = 0;
    IFXG(nullstring) = malloc(5);
    strcpy(IFXG(nullstring), "NULL");

    IFXG(num_persistent)=0;
    IFXG(sv_sqlcode)=0;

}
#endif

PHP_MINIT_FUNCTION(ifx)
{
   /*ELS_FETCH();*/
   
#ifdef ZTS
    ifx_globals_id = ts_allocate_id(sizeof(php_ifx_globals), php_ifx_init_globals, NULL);

#else
    IFXG(num_persistent)=0;
    IFXG(nullvalue) = malloc(1);
    IFXG(nullvalue)[0] = 0;
    IFXG(nullstring) = malloc(5);
    strcpy(IFXG(nullstring), "NULL");

    IFXG(num_persistent)=0;
    IFXG(sv_sqlcode)=0;

#endif

    REGISTER_INI_ENTRIES();

    IFXL(le_result)   = register_list_destructors(ifx_free_result,NULL);
    IFXL(le_idresult) = register_list_destructors(ifx_free_result,NULL);
    IFXL(le_link)     = register_list_destructors(_close_ifx_link,NULL);
    IFXL(le_plink)    = register_list_destructors(NULL,_close_ifx_plink);

    ifx_module_entry.type = type;
    
#if 0
    printf("Registered:  %d,%d,%d\n",
         IFXL(le_result),
         IFXL(le_link),
         IFXL(le_plink));
#endif

    REGISTER_LONG_CONSTANT("IFX_SCROLL", IFX_SCROLL, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("IFX_HOLD", IFX_HOLD, CONST_CS | CONST_PERSISTENT);
$ifdef HAVE_IFX_IUS;
    REGISTER_LONG_CONSTANT("IFX_LO_RDONLY", LO_RDONLY, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("IFX_LO_WRONLY", LO_WRONLY, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("IFX_LO_APPEND", LO_APPEND, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("IFX_LO_RDWR", LO_RDWR, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("IFX_LO_BUFFER", LO_BUFFER, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("IFX_LO_NOBUFFER", LO_NOBUFFER, CONST_CS | CONST_PERSISTENT);
$endif;

    return SUCCESS;
}


PHP_MSHUTDOWN_FUNCTION(ifx) 
{

    UNREGISTER_INI_ENTRIES();
    return SUCCESS;
    
}

PHP_RINIT_FUNCTION(ifx) 
{
    IFXLS_FETCH();

    IFXG(default_link)=-1;
    IFXG(num_links) = IFXG(num_persistent);
    return SUCCESS;
}

PHP_MINFO_FUNCTION(ifx)
{
    char buf[32];
    char maxp[16],maxl[16];
   
    IFXLS_FETCH();

    
    if (IFXG(max_persistent)==-1) {
        strcpy(maxp,"Unlimited");
    } else {
        snprintf(maxp,15,"%ld",IFXG(max_persistent));
        maxp[15]=0;
    }
    if (IFXG(max_links)==-1) {
        strcpy(maxl,"Unlimited");
    } else {
        snprintf(maxl,15,"%ld",IFXG(max_links));
        maxl[15]=0;
    }
   
    php_info_print_table_start();
    php_info_print_table_header(2, "Informix support", "enabled");
    sprintf(buf, "%ld", IFXG(num_persistent));
    php_info_print_table_row(2, "Persistent links", buf);
    sprintf(buf, "%ld", IFXG(num_links)); 
    php_info_print_table_row(2, "Total links", buf);
    sprintf(buf, "%02.2f", (double)(IFX_VERSION/100.0)); 
    php_info_print_table_row(2, "ESQL/C Version", buf);
    php_info_print_table_end();

    DISPLAY_INI_ENTRIES();
   
}

static void php3_ifx_set_default_link(int id) 
{

    IFXLS_FETCH();
    
    if (IFXG(default_link) != -1) {
        zend_list_delete(IFXG(default_link));
    }
    IFXG(default_link) = id;
    zend_list_addref(id);
    
}

/* ----------------------------------------------------------------------
** int ifx_(p)connect(string database, string userid, string password)
**
** connects to $database (db@server syntax) using $userid and $password
**
** returns a connection id on success or FALSE one error  
** ----------------------------------------------------------------------
*/

static void php3_ifx_do_connect(INTERNAL_FUNCTION_PARAMETERS,int persistent)
{

    EXEC SQL BEGIN DECLARE SECTION;
      char *user,*passwd,*host;
      char *ifx;
    EXEC SQL END DECLARE SECTION;    

    char *hashed_details;
    int hashed_details_length;
    IFXLS_FETCH();
    PLS_FETCH();
    
    if (PG(sql_safe_mode)) {
        if (ZEND_NUM_ARGS()>0) {
            php_error(E_NOTICE,
            "SQL safe mode in effect - ignoring host/user/password information");
        }
        host = passwd = NULL;
        user = php_get_current_user();
        hashed_details_length = strlen(user)+3+3;
        hashed_details = (char *) emalloc(hashed_details_length+1);
        sprintf(hashed_details,"ifx__%s_",user);
    } else {
        host = IFXG(default_host);
        user = IFXG(default_user);
        passwd = IFXG(default_password);
        
        switch(ZEND_NUM_ARGS()) {
            case 0: /* defaults */
                break;
            case 1: {
                    pval *yyhost;
                    
                    if (getParameters(ht, 1, &yyhost)==FAILURE) {
                        RETURN_FALSE;
                    }
                    convert_to_string(yyhost);
                    host = yyhost->value.str.val;
                }
                break;
            case 2: {
                    pval *yyhost,*yyuser;
                    
                    if (getParameters(ht, 2, &yyhost, &yyuser)==FAILURE) {
                        RETURN_FALSE;
                    }
                    convert_to_string(yyhost);
                    convert_to_string(yyuser);
                    host = yyhost->value.str.val;
                    user = yyuser->value.str.val;
                }
                break;
            case 3: {
                    pval *yyhost,*yyuser,*yypasswd;
                
                    if (getParameters(ht, 3, &yyhost, &yyuser, &yypasswd) == FAILURE) {
                        RETURN_FALSE;
                    }
                    convert_to_string(yyhost);
                    convert_to_string(yyuser);
                    convert_to_string(yypasswd);
                    host = yyhost->value.str.val;
                    user = yyuser->value.str.val;
                    passwd = yypasswd->value.str.val;
                }
                break;
            default:
                WRONG_PARAM_COUNT;
                break;
        }
        hashed_details_length = sizeof("ifx___")-1 + 
                          strlen(SAFE_STRING(host))+
                          strlen(SAFE_STRING(user))+
                          strlen(SAFE_STRING(passwd));
        hashed_details = (char *) emalloc(hashed_details_length+1);
        sprintf(hashed_details,"ifx_%s_%s_%s",
                               SAFE_STRING(host), 
                               SAFE_STRING(user), 
                               SAFE_STRING(passwd));
    }


    IFXG(sv_sqlcode) = 0;

    if (!IFXG(allow_persistent)) {
        persistent=0;
    }
    if (persistent) {
        list_entry *le;
        
        /* try to find if we already have this link in our persistent list */
        if (zend_hash_find(&EG(persistent_list), hashed_details, hashed_details_length+1, 
                            (void **) &le)==FAILURE) {  /* we don't */
            list_entry new_le;

            if (IFXG(max_links)!=-1 &&
                    IFXG(num_links) >=
                    IFXG(max_links)) {
                php_error(E_WARNING,
                           "Informix:  Too many open links (%d)",
                           IFXG(num_links));
                efree(hashed_details);
                RETURN_FALSE;
            }
            if (IFXG(max_persistent)!=-1 && 
                    IFXG(num_persistent) >=
                    IFXG(max_persistent)) {
                php_error(E_WARNING,
                           "Informix:  Too many open persistent links (%d)",
                           IFXG(num_persistent));
                efree(hashed_details);
                RETURN_FALSE;
            }
            /* create the link */
            ifx = (char *)malloc(sizeof(IFX));
            IFXG(connectionid)++;
            sprintf(ifx,"%s%x", 
                    user, 
                    IFXG(connectionid));

            EXEC SQL CONNECT TO :host AS :ifx 
                     USER :user USING :passwd 
                     WITH CONCURRENT TRANSACTION;  

            if (ifx_check() == IFX_ERROR) {
                IFXG(sv_sqlcode) = SQLCODE;
                php_error(E_WARNING,ifx_error(ifx));
                free(ifx);
                efree(hashed_details);
                RETURN_FALSE;
            }
            
            /* hash it up */
            new_le.type = IFXL(le_plink);
            new_le.ptr = ifx;
            if (zend_hash_update(&EG(persistent_list), hashed_details, 
                   hashed_details_length+1, 
                   (void *) &new_le, sizeof(list_entry), NULL)==FAILURE) {
                free(ifx);
                efree(hashed_details);
                RETURN_FALSE;
            }
            IFXG(num_persistent)++;
            IFXG(num_links)++;
        } else {  /* we do */
            if (le->type != IFXL(le_plink)) {
                RETURN_FALSE;
            }
            /* ensure that the link did not die */
            ifx = le->ptr;
            EXEC SQL SET CONNECTION :ifx;
            if (ifx_check() == IFX_ERROR) {
                          /* the link died      */
                ifx = le->ptr;        /* reconnect silently */
                EXEC SQL CONNECT TO :host AS :ifx 
                         USER :user USING :passwd 
                         WITH CONCURRENT TRANSACTION;  

                if (ifx_check() == IFX_ERROR) {
                    IFXG(sv_sqlcode) = SQLCODE;
                    php_error(E_WARNING,
                               "Informix:  Link to server lost, unable to reconnect (%s)",
                               ifx_error(ifx));
                    zend_hash_del(&EG(persistent_list), hashed_details, 
                                   hashed_details_length+1);
                    efree(hashed_details);
                    RETURN_FALSE;
                }
            }
            ifx = le->ptr;
        }
        ZEND_REGISTER_RESOURCE(return_value, ifx, IFXL(le_plink));
    } else { /* non persistent */
        list_entry *index_ptr,new_index_ptr;
        
        /* first we check the hash for the hashed_details key.  if it exists,
         * it should point us to the right offset where the actual ifx link sits.
         * if it doesn't, open a new ifx link, add it to the resource list,
         * and add a pointer to it with hashed_details as the key.
         */
        if (zend_hash_find(&EG(regular_list),hashed_details,hashed_details_length+1,
                           (void **) &index_ptr) == SUCCESS) {
            int type,link;
            void *ptr;

            if (index_ptr->type != le_index_ptr) {
                RETURN_FALSE;
            }
            link = (int) index_ptr->ptr;
            ptr = zend_list_find(link, &type);   /* check if the link is still there */
            if (ptr && (type==IFXL(le_link) || type==IFXL(le_plink))) {
        	/* ensure that the link is not closed */
        	ifx = ptr;
        	EXEC SQL SET CONNECTION :ifx;
        	if (ifx_check() == IFX_ERROR) {
                    /* the link is closed */
                    ifx = ptr;        /* reconnect silently */
                    EXEC SQL CONNECT TO :host AS :ifx 
                             USER :user USING :passwd 
                             WITH CONCURRENT TRANSACTION;  

                    if (ifx_check() == IFX_ERROR) {
                	IFXG(sv_sqlcode) = SQLCODE;
                	php_error(E_WARNING,"Informix:  unable to connect (%s)", ifx_error(ifx));
                	zend_hash_del(&EG(regular_list), hashed_details, hashed_details_length+1);
                	efree(hashed_details);
                	RETURN_FALSE;
                    }
        	}
            	zend_list_addref(link);
                return_value->value.lval = link;
                php3_ifx_set_default_link(link);
                return_value->type = IS_RESOURCE;
                efree(hashed_details);
                return;
            } else {
                zend_hash_del(&EG(regular_list),hashed_details,hashed_details_length+1);
            }
        }
        if (IFXG(max_links) != -1 && 
            IFXG(num_links) >= 
                             IFXG(max_links)) {
            php_error(E_WARNING,
                       "Informix:  Too many open links (%d)",
                       IFXG(num_links));
            efree(hashed_details);
            RETURN_FALSE;
        }
        ifx = (char *) emalloc(sizeof(IFX));
        IFXG(connectionid)++;        
        sprintf(ifx,"connec%x", 
                IFXG(connectionid));

        EXEC SQL CONNECT TO :host AS :ifx 
             USER :user USING :passwd 
             WITH CONCURRENT TRANSACTION;  
        if (ifx_check() == IFX_ERROR) {
            IFXG(sv_sqlcode) = SQLCODE;
            php_error(E_WARNING,"ifx_connect : %s", ifx_error(ifx));
            efree(hashed_details);
            efree(ifx);
            RETURN_FALSE;
        }

        /* add it to the list */
        ZEND_REGISTER_RESOURCE(return_value, ifx, IFXL(le_link));
        
        /* add it to the hash */
        new_index_ptr.ptr = (void *) return_value->value.lval;
        new_index_ptr.type = le_index_ptr;
        if (zend_hash_update(&EG(regular_list),
                              hashed_details,
                              hashed_details_length+1,
                              (void *) &new_index_ptr, 
                              sizeof(list_entry), NULL) == FAILURE) {
            efree(hashed_details);
            RETURN_FALSE;
        }
        IFXG(num_links)++;
    }
    efree(hashed_details);
    php3_ifx_set_default_link(return_value->value.lval);
}


/* {{{ proto int ifx_connect([string database [, string userid [, string password]]])
   Connects to database using userid/password, returns connection id */
PHP_FUNCTION(ifx_connect)
{
    php3_ifx_do_connect(INTERNAL_FUNCTION_PARAM_PASSTHRU,0);
}
/* }}} */

/* {{{ proto int ifx_pconnect([string database [, string userid [, string password]]])
   Connects to database using userid/password, returns connection id */
PHP_FUNCTION(ifx_pconnect)
{
    php3_ifx_do_connect(INTERNAL_FUNCTION_PARAM_PASSTHRU,1);
}
/* }}} */

static int php3_ifx_get_default_link(INTERNAL_FUNCTION_PARAMETERS)
{
    IFXLS_FETCH();

    if (IFXG(default_link)==-1) { /* no link opened yet, implicitly open one */
        ht = 0;
        php3_ifx_do_connect(INTERNAL_FUNCTION_PARAM_PASSTHRU,0);
    }
    return IFXG(default_link);
}

/* ----------------------------------------------------------------------
** int ifx_close(int connid)
**
** closes connection connid 
** always returns TRUE
** ----------------------------------------------------------------------
*/

/* {{{ proto int ifx_close(int connid)
   Close informix connection */
PHP_FUNCTION(ifx_close)
{
    pval **ifx_link = NULL;
    int id;

EXEC SQL BEGIN DECLARE SECTION;
    char *ifx;
EXEC SQL END DECLARE SECTION;

    IFXLS_FETCH();


    
    id = -1;

    switch (ZEND_NUM_ARGS()) {
        case 0:
            id = IFXG(default_link);
            break;
        case 1:
            if (zend_get_parameters_ex(1, &ifx_link)==FAILURE) {
                RETURN_FALSE;
            }
            break;
        default:
            WRONG_PARAM_COUNT;
            break;
    }
    
    IFXG(sv_sqlcode) = 0;

    ZEND_FETCH_RESOURCE2(ifx, char *, ifx_link, id, "IFX link", IFXL(le_link), IFXL(le_plink));
    
    EXEC SQL SET CONNECTION :ifx;
    EXEC SQL close database;    
    EXEC SQL DISCONNECT CURRENT;    

    zend_list_delete(id);
    RETURN_TRUE;
}
/* }}} */

/* ----------------------------------------------------------------------
** int ifx_query(string query, int connid 
**               [, int cursortype] [, array blobidarray])
** cursortype and blobidarray are optional
** 
** executes query query on connection connid
** for select queries a cursor is declared and opened
** non-select queries are "execute immediate"
** select queries accept an optional cursortype param: 
** IFX_SCROLL, IFX_HOLD (or'ed mask)
** non-select queries accept an optional "blobarryid" parameter
** blobsupport: mark the blob-column(s) with ? in the insert/update query 
**   and add a blob-id-array-functionparameter
** select queries return "blob-ids" for blob columns 
**   except if text/byteasvarchar is set
** example: ifx_query("insert into catalog (stock_num, manu_code, 
**           cat_descr,cat_picture) values(1,'HRO',?,?)",$cid,$bidarray);
** 
** returns a "result id" on success or FALSE on error
** also sets "affected_rows for retrieval by ifx_affected_rows()
** ----------------------------------------------------------------------
*/

/* {{{ proto int ifx_query(string query, int connid [, int cursortype] [, array idarray])
   Perform a query on a given connection */
PHP_FUNCTION(ifx_query)
{
    pval **query,**ifx_link, **cursortype, **dummy;
    int id;
    IFX_RES *Ifx_Result;
    
EXEC SQL BEGIN DECLARE SECTION;
    char *ifx;                /* connection ID     */
    char cursorid[32];        /* query cursor id   */
    char statemid[32];        /* statement id      */
    char descrpid[32];        /* descriptor id     */
    char *statement;          /* query text        */
    int  fieldcount;          /* field count       */
    int  i;                   /* field index       */
    short fieldtype;
    loc_t *locator;
    int loc_t_type=CLOCATORTYPE;  /* WORKAROUND:TYPE=CLOCATORTYPE doesn't work,  */
    int sqlchar_type=SQLCHAR;     /* don't ask me, why. */
    char *char_tmp;
    long len;
    int indicator;    
$ifdef HAVE_IFX_IUS;
    fixed binary 'blob' ifx_lo_t *slocator;
$endif;

EXEC SQL END DECLARE SECTION;

    char *blobfilename;
    int  locind;
    int  ctype;
    int  affected_rows;
    long sqlerrd[6];
    int  e;
    int  query_type;
    int  cursoryproc;
        
    IFXLS_FETCH();

    if(ZEND_NUM_ARGS()<2) {
     WRONG_PARAM_COUNT;
    }

    IFXG(sv_sqlcode) = 0;

    /* get the first 2 parameters, 
      php4 insists on the correct number of arguments */
    switch(ZEND_NUM_ARGS()) {
      case 2:
        if (zend_get_parameters_ex(2, &query, &ifx_link)==FAILURE)  
	  RETURN_FALSE;
        break;
      case 3:
        if (zend_get_parameters_ex(3, &query, &ifx_link, &dummy)==FAILURE)  
	  RETURN_FALSE;
        break;
      case 4:
        if (zend_get_parameters_ex(4, &query, &ifx_link, &dummy, &dummy)==FAILURE)  
	  RETURN_FALSE;
        break;
    }

    id = -1;
    ZEND_FETCH_RESOURCE2(ifx, char *, ifx_link, id, "IFX link", IFXL(le_link), IFXL(le_plink));
    
   
    affected_rows = -1;      /* invalid */

    convert_to_string_ex(query);

    statement = (*query)->value.str.val;
    IFXG(cursorid)++;    
    sprintf(statemid, "statem%x", IFXG(cursorid)); 
    sprintf(cursorid, "cursor%x", IFXG(cursorid)); 
    sprintf(descrpid, "descrp%x", IFXG(cursorid)); 
    
    EXEC SQL set connection :ifx;
    if (ifx_check() < 0) {
        IFXG(sv_sqlcode) = SQLCODE;
        php_error(E_WARNING,"Set connection %s fails (%s)",
                              ifx,
                              ifx_error(ifx));
        RETURN_FALSE;
    }
    EXEC SQL PREPARE :statemid FROM :statement;
    if (ifx_check() < 0) {
        IFXG(sv_sqlcode) = SQLCODE;
        php_error(E_WARNING,"Prepare fails (%s)",
                              ifx_error(ifx));
        RETURN_FALSE;
    }

    affected_rows = sqlca.sqlerrd[0];    /* save estimated affected rows */
    for (e = 0; e < 6; e++) sqlerrd[e] = sqlca.sqlerrd[e];
   
    EXEC SQL ALLOCATE DESCRIPTOR :descrpid WITH MAX 384;
    if (ifx_check() < 0) {
        IFXG(sv_sqlcode) = SQLCODE;
        php_error(E_WARNING,"Allocate desciptor <%s> fails (%s)",
                              descrpid,
                              ifx_error(ifx));
        EXEC SQL free :statemid;
        RETURN_FALSE;
    }
    EXEC SQL DESCRIBE :statemid USING SQL DESCRIPTOR :descrpid;
    if (ifx_check() < 0) {
        IFXG(sv_sqlcode) = SQLCODE;
        php_error(E_WARNING,"Describe fails (%s)",
                              ifx_error(ifx));
        EXEC SQL DEALLOCATE DESCRIPTOR :descrpid;
        EXEC SQL free :statemid;
        RETURN_FALSE;
    }

    query_type = sqlca.sqlcode;
    
    Ifx_Result = (IFX_RES *)emalloc(sizeof(IFX_RES));
    if (Ifx_Result == NULL) { 
        php_error(E_WARNING,"Out of memory allocating IFX_RES");
        EXEC SQL DEALLOCATE DESCRIPTOR :descrpid;
        EXEC SQL free :statemid;
        RETURN_FALSE;
    }

    /* initialize result data structure */

    Ifx_Result->rowid = 0;
    strcpy(Ifx_Result->connecid, ifx); 
    strcpy(Ifx_Result->descrpid, descrpid);
    for (i = 0; i < MAX_RESID; ++i)
        Ifx_Result->res_id[i] = -1;
    
    cursoryproc = 0;
    if (query_type == SQ_EXECPROC) {
        EXEC SQL GET DESCRIPTOR :descrpid :i = COUNT;
        if (i > 0) {
            cursoryproc = 1;
        } 
    }

    Ifx_Result->iscursory = -1; /* prevent ifx_do */
    Ifx_Result->paramquery=0;  
    
    if ((query_type != 0) && (!cursoryproc)) {  /* NO RESULT SET */
      /* ##
         ## NONSELECT-STATEMENT 
         ##
      */
      pval **pblobidarr, **tmp;

      Ifx_Result->iscursory = 0;
      
      strcpy(Ifx_Result->cursorid, "");
      strcpy(Ifx_Result->descrpid, descrpid);
      strcpy(Ifx_Result->statemid, statemid);

      if(ZEND_NUM_ARGS()>3) {
         WRONG_PARAM_COUNT;      
      }

      if(ZEND_NUM_ARGS()==3) {
          if (zend_get_parameters_ex(3, &dummy, &dummy, &pblobidarr) == FAILURE) {
              php_error(E_WARNING,"Can't get blob array param");
              EXEC SQL DEALLOCATE DESCRIPTOR :descrpid;
              EXEC SQL free :statemid;
              efree(Ifx_Result);
              RETURN_FALSE;
          } 
          if ((*pblobidarr)->type != IS_ARRAY) {
              php_error(E_WARNING,"blob-parameter not an array");
              EXEC SQL DEALLOCATE DESCRIPTOR :descrpid;
              EXEC SQL free :statemid;
              efree(Ifx_Result);
              RETURN_FALSE;
          }

          zend_hash_internal_pointer_reset((*pblobidarr)->value.ht);
          i=1;
          while (zend_hash_get_current_data((*pblobidarr)->value.ht, 
                                           (void **) &tmp) == SUCCESS) {
              convert_to_long(*tmp);
              if ((query_type == SQ_UPDATE) || (query_type == SQ_UPDALL)) {
                   EXEC SQL SET DESCRIPTOR :descrpid COUNT = :i;
               }
              /* TEXT/BYTE */
 	      if(php3_intifx_getType((int)(*tmp)->value.lval,&EG(regular_list))==TYPE_BLTEXT || php3_intifx_getType((int)(*tmp)->value.lval,&EG(regular_list))==TYPE_BLBYTE) {
               locator=php3_intifx_get_blobloc((int)((*tmp)->value.lval),&EG(regular_list));
               if(locator==NULL) {
                   php_error(E_WARNING,"%d is not a Informix blob-result index",
                              (int)((*tmp)->value.lval));
                   EXEC SQL DEALLOCATE DESCRIPTOR :descrpid;
                   EXEC SQL free :statemid;
                   efree(Ifx_Result);
                   RETURN_FALSE;
               }
               if(locator->loc_loctype==LOCFNAME) {       
                   locator->loc_oflags=LOC_RONLY;
               }
               EXEC SQL SET DESCRIPTOR :descrpid VALUE :i 
                                        DATA= :*locator, 
                                        TYPE= :loc_t_type;
              }
              
              /* CHAR */
              if(php3_intifx_getType((int)(*tmp)->value.lval,&EG(regular_list))==TYPE_CHAR) {
               len=php3_intifx_get_char((int)((*tmp)->value.lval),&EG(regular_list),&char_tmp);
               indicator=0;
               if(char_tmp==NULL || len<0)
                indicator=-1;
               len++;
               EXEC SQL SET DESCRIPTOR :descrpid VALUE :i 
                                        DATA= :char_tmp,
                                        LENGTH= :len, 
                                        INDICATOR= :indicator,
                                        TYPE= :sqlchar_type;
              }
              
              
              i++;
              zend_hash_move_forward((*pblobidarr)->value.ht);
          }
          Ifx_Result->paramquery=1;  
          EXEC SQL EXECUTE :statemid USING SQL DESCRIPTOR :descrpid;
      } else {
          EXEC SQL EXECUTE :statemid;
      }
      if (ifx_check() < 0) {
        IFXG(sv_sqlcode) = SQLCODE;
        php_error(E_WARNING,"Execute immediate fails : %s (%s)",
                                 statement,
                                 ifx_error(ifx));
        efree(Ifx_Result);
        EXEC SQL DEALLOCATE DESCRIPTOR :descrpid;
        EXEC SQL free :statemid;
        RETURN_FALSE;
       }
       Ifx_Result->affected_rows = sqlca.sqlerrd[2]; /* really affected */
       for (e = 0; e < 6; e++) Ifx_Result->sqlerrd[e] = sqlca.sqlerrd[e];
    } else {
      /* ##
         ** SELECT-STATEMENT 
         **
      */

      ctype = 0;   /* preset */

      switch(ZEND_NUM_ARGS()) {
       case 2:
         break;
       case 3:
         if (zend_get_parameters_ex(3, 
                              &dummy, 
                              &dummy,
                              &cursortype)==FAILURE) {
                RETURN_FALSE;
         }
         convert_to_long_ex(cursortype);
         ctype = (*cursortype)->value.lval;
         break;
       default:
         WRONG_PARAM_COUNT;
         break;
      }
            


        Ifx_Result->affected_rows = affected_rows;   /* saved estimated from prepare */
        for (e = 0; e < 6; e++) Ifx_Result->sqlerrd[e] = sqlerrd[e];
       
        EXEC SQL GET DESCRIPTOR :descrpid :fieldcount = COUNT;
        if (ifx_check() < 0) {
            IFXG(sv_sqlcode) = SQLCODE;
            php_error(E_WARNING,"Can not get descriptor %s (%s)",
                                  descrpid,
                                  ifx_error(ifx));
            EXEC SQL DEALLOCATE DESCRIPTOR :descrpid;
            EXEC SQL free :statemid;
            efree(Ifx_Result);
            RETURN_FALSE;
        }
        Ifx_Result->numcols = fieldcount;

        Ifx_Result->isscroll = Ifx_Result->ishold = 0;
        if (ctype & IFX_SCROLL)
            Ifx_Result->isscroll = 1;
        if (ctype & IFX_HOLD)
            Ifx_Result->ishold = 1;

        if (Ifx_Result->isscroll) 
            if (Ifx_Result->ishold) 
               EXEC SQL DECLARE :cursorid SCROLL CURSOR WITH HOLD FOR :statemid;
            else
               EXEC SQL DECLARE :cursorid SCROLL CURSOR FOR :statemid;
        else
            if (Ifx_Result->ishold)
               EXEC SQL DECLARE :cursorid CURSOR WITH HOLD FOR :statemid;
            else
               EXEC SQL DECLARE :cursorid CURSOR FOR :statemid;
                
        if (ifx_check() < 0) {
            IFXG(sv_sqlcode) = SQLCODE;
            php_error(E_WARNING,"Declare cursor fails (%s)", ifx_error(ifx));
            efree(Ifx_Result);
            EXEC SQL DEALLOCATE DESCRIPTOR :descrpid;
            EXEC SQL free :statemid;
            RETURN_FALSE;
        }
        EXEC SQL OPEN :cursorid;
        if (ifx_check() < 0) {
            IFXG(sv_sqlcode) = SQLCODE;
            php_error(E_WARNING,"Open cursor fails (%s)", ifx_error(ifx));
            efree(Ifx_Result);
            EXEC SQL DEALLOCATE DESCRIPTOR :descrpid;
            EXEC SQL free :statemid;
            EXEC SQL free :cursorid;
            RETURN_FALSE;
        }
        strcpy(Ifx_Result->cursorid, cursorid);
        strcpy(Ifx_Result->descrpid, descrpid);
        strcpy(Ifx_Result->statemid, statemid);
        
        /* check for text/blob columns */
        

        locind = 0;
        for (i = 1; i <= fieldcount; ++i) {
           EXEC SQL GET DESCRIPTOR :descrpid VALUE :i  :fieldtype = TYPE;
           if (
               (fieldtype == SQLTEXT) || (fieldtype == SQLBYTES)
$ifdef HAVE_IFX_IUS;
               || (fieldtype==SQLUDTFIXED)
$endif;
                ) {
            
               int bid = 0;
               if(fieldtype==SQLTEXT) {
                   bid=php3_intifx_create_blob(TYPE_BLTEXT,BLMODE_INMEM,"",-1,&EG(regular_list));
                   locator=php3_intifx_get_blobloc(bid,&EG(regular_list));
                   EXEC SQL SET DESCRIPTOR :descrpid VALUE :i DATA = :*locator;
               } 
               if(fieldtype==SQLBYTES) {
                   if(IFXG(blobinfile)==0) {
                       bid=php3_intifx_create_blob(TYPE_BLBYTE,BLMODE_INMEM,"",-1,&EG(regular_list));
                       locator=php3_intifx_get_blobloc(bid,&EG(regular_list));
                   } else {
                       blobfilename=php3_intifx_create_tmpfile(i);
                       bid=php3_intifx_create_blob(
                                      TYPE_BLBYTE,BLMODE_INFILE,
                                      blobfilename,strlen(blobfilename),&EG(regular_list));
                       locator=php3_intifx_get_blobloc(bid,&EG(regular_list));
                       locator->loc_oflags=LOC_WONLY;
                   }
                   EXEC SQL SET DESCRIPTOR :descrpid VALUE :i DATA = :*locator;
                } 
$ifdef HAVE_IFX_IUS;
               if(fieldtype==SQLUDTFIXED) {
                   bid=php3_intifxus_new_slob(&EG(regular_list));
                   slocator=php3_intifxus_get_slobloc(bid,&EG(regular_list));
                   EXEC SQL SET DESCRIPTOR :descrpid VALUE :i DATA = :*slocator;  
                } 
$endif;
                Ifx_Result->res_id[locind]=bid;
                ++locind;
            }
        }
                
    }

    ZEND_REGISTER_RESOURCE(return_value, Ifx_Result, IFXL(le_result));
}
/* }}} */


/* ----------------------------------------------------------------------
** int ifx_prepare(string query, int connid, 
**                 [, int cursortype] [, array blobidarry])
**
** $hold, $scroll are optional and valid only for select queries
** $blobidarray is optional, an array of blob id's
**
** prepares query $query on connection $connid
** select queries accept an optional cursortype param: IFX_SCROLL, IFX_HOLD (or'ed mask)
** blobsupport: mark the blob-column with ? and add a blob-id-functionparameter
** example: ifx_query("insert into catalog (stock_num, manu_code ,cat_descr,
**                    cat_picture) values(1,'HRO',?,?)",$cid,$bid1,$bid2);
** 
** returns a "result id" on success or FALSE on error
** also sets "affected_rows for retrieval by ifx_affected_rows
** ----------------------------------------------------------------------
*/

/* {{{ proto int ifx_prepare(string query, int connid [, int cursortype] [, array idarray])
   Prepare a query on a given connection */
PHP_FUNCTION(ifx_prepare)
{
   
    pval **query,**ifx_link, **cursortype, **dummy;
    int id;
    IFX_RES *Ifx_Result;
    
EXEC SQL BEGIN DECLARE SECTION;
    char *ifx;                /* connection ID     */
    char cursorid[32];        /* query cursor id   */
    char statemid[32];        /* statement id      */
    char descrpid[32];        /* descriptor id     */
    char *statement;          /* query text        */
    int  fieldcount;          /* field count       */
    int  i;                   /* field index       */
    loc_t *locator;
    int loc_t_type=CLOCATORTYPE; /* WORKAROUND: TYPE=CLOCATORTYPE doesn't work, */
    int sqlchar_type=SQLCHAR;     /* don't ask me, why. */
    char *char_tmp;
    long len;
    int indicator;    
    long sqlerrd[6];
    int e;


EXEC SQL END DECLARE SECTION;

    int  ctype;
    int  affected_rows;
    int  query_type;
    int  cursoryproc;
    
    IFXLS_FETCH();

    if(ZEND_NUM_ARGS()<2) {
     WRONG_PARAM_COUNT;
    }

    IFXG(sv_sqlcode) = 0;

    /* get the first 2 parameters, 
      php4 insists on the correct number of arguments */
    switch(ZEND_NUM_ARGS()) {
      case 2:
        if (zend_get_parameters_ex(2, &query, &ifx_link)==FAILURE)  
	  RETURN_FALSE;
        break;
      case 3:
        if (zend_get_parameters_ex(3, &query, &ifx_link, &dummy)==FAILURE)  
	  RETURN_FALSE;
        break;
      case 4:
        if (zend_get_parameters_ex(4, &query, &ifx_link, &dummy, &dummy)==FAILURE)  
	  RETURN_FALSE;
        break;
    }

    id = -1;
    ZEND_FETCH_RESOURCE2(ifx, char *, ifx_link, id, "IFX link", IFXL(le_link), IFXL(le_plink));

    affected_rows = -1;      /* invalid */


    convert_to_string_ex(query);

    statement = (*query)->value.str.val;
    IFXG(cursorid)++;    
    sprintf(statemid, "statem%x", IFXG(cursorid)); 
    sprintf(cursorid, "cursor%x", IFXG(cursorid)); 
    sprintf(descrpid, "descrp%x", IFXG(cursorid)); 
    
    EXEC SQL set connection :ifx;
    if (ifx_check() < 0) {
        IFXG(sv_sqlcode) = SQLCODE;
        php_error(E_WARNING,"Set connection %s fails (%s)",
                              ifx,
                              ifx_error(ifx));
        RETURN_FALSE;
    }
    EXEC SQL PREPARE :statemid FROM :statement;
    if (ifx_check() < 0) {
        IFXG(sv_sqlcode) = SQLCODE;
        php_error(E_WARNING,"Prepare fails (%s)",
                              ifx_error(ifx));
        RETURN_FALSE;
    }
    affected_rows = sqlca.sqlerrd[0];    /* save estimated affected rows */
    for (e = 0; e < 6; e++) sqlerrd[e] = sqlca.sqlerrd[e];
    EXEC SQL ALLOCATE DESCRIPTOR :descrpid WITH MAX 384;
    if (ifx_check() < 0) {
        IFXG(sv_sqlcode) = SQLCODE;
        php_error(E_WARNING,"Allocate desciptor <%s> fails (%s)",
                              descrpid,
                              ifx_error(ifx));
        EXEC SQL free :statemid;
        RETURN_FALSE;
    }
    EXEC SQL DESCRIBE :statemid USING SQL DESCRIPTOR :descrpid;
    if (ifx_check() < 0) {
        IFXG(sv_sqlcode) = SQLCODE;
        php_error(E_WARNING,"Describe fails (%s)",
                              ifx_error(ifx));
        EXEC SQL DEALLOCATE DESCRIPTOR :descrpid;
        EXEC SQL free :statemid;
        RETURN_FALSE;
    }

    query_type = sqlca.sqlcode;
    
    Ifx_Result = (IFX_RES *)emalloc(sizeof(IFX_RES));
    if (Ifx_Result == NULL) { 
        IFXG(sv_sqlcode) = SQLCODE;
        php_error(E_WARNING,"Out of memory allocating IFX_RES");
        EXEC SQL DEALLOCATE DESCRIPTOR :descrpid;
        EXEC SQL free :statemid;
        RETURN_FALSE;
    }

    /* initialize result data structure */

    Ifx_Result->rowid = 0;
    strcpy(Ifx_Result->connecid, ifx); 
    strcpy(Ifx_Result->descrpid, descrpid);
    strcpy(Ifx_Result->statemid, statemid);
    for (i = 0; i < MAX_RESID; ++i)
        Ifx_Result->res_id[i] = -1;
  
    cursoryproc = 0;
    if (query_type == SQ_EXECPROC) {
        EXEC SQL GET DESCRIPTOR :descrpid :i = COUNT;
        if (i > 0) {
            cursoryproc = 1;
        }
    }

    Ifx_Result->iscursory = -1; /* prevent ifx_do */
    Ifx_Result->paramquery=0;  

    if ((query_type != 0) && (!cursoryproc)) {  /* NO RESULT SET */
      /* ##
         ## NONSELECT-STATEMENT 
         ##
      */
      pval **pblobidarr, **tmp;

      Ifx_Result->iscursory = 0;

      strcpy(Ifx_Result->cursorid, cursorid);
      strcpy(Ifx_Result->cursorid, "");
      strcpy(Ifx_Result->descrpid, descrpid);


      if(ZEND_NUM_ARGS()>3) {
         WRONG_PARAM_COUNT;      
      }
      if(ZEND_NUM_ARGS()==3) {
          Ifx_Result->paramquery=1;
          if (zend_get_parameters_ex(3, &dummy, &dummy,&pblobidarr) == FAILURE) {
              php_error(E_WARNING,"Can't get blob array param");
              EXEC SQL DEALLOCATE DESCRIPTOR :descrpid;
              EXEC SQL free :statemid;
              efree(Ifx_Result);
              RETURN_FALSE;
          } 
          if((*pblobidarr)->type != IS_ARRAY) {
              php_error(E_WARNING,"blob-parameter not an array");
              EXEC SQL DEALLOCATE DESCRIPTOR :descrpid;
              EXEC SQL free :statemid;
              efree(Ifx_Result);
              RETURN_FALSE;
           } 
           zend_hash_internal_pointer_reset((*pblobidarr)->value.ht);
           i=1;
           while (zend_hash_get_current_data((*pblobidarr)->value.ht, 
                                              (void **) &tmp) == SUCCESS) {
              convert_to_long(*tmp);
              if ((query_type == SQ_UPDATE) || (query_type == SQ_UPDALL)) {
                  EXEC SQL SET DESCRIPTOR :descrpid COUNT = :i;
              }
              /* TEXT/BYTE */
 	      if(php3_intifx_getType((int)((*tmp)->value.lval),&EG(regular_list))==TYPE_BLTEXT || php3_intifx_getType((int)((*tmp)->value.lval),&EG(regular_list))==TYPE_BLBYTE) {
                locator=php3_intifx_get_blobloc((int)((*tmp)->value.lval),&EG(regular_list));
                if(locator==NULL) {
                    php_error(E_WARNING,"%d is not a Informix blob-result index",
                               (int)((*tmp)->value.lval));
                    EXEC SQL DEALLOCATE DESCRIPTOR :descrpid;
                    EXEC SQL free :statemid;
                    efree(Ifx_Result);
                    RETURN_FALSE;
                }
                if(locator->loc_loctype==LOCFNAME) {       
                    locator->loc_oflags=LOC_RONLY;
                }
                EXEC SQL SET DESCRIPTOR :descrpid VALUE :i 
                                        DATA= :*locator, 
                                        TYPE=:loc_t_type; 
              }
              /* CHAR */
              if(php3_intifx_getType((int)((*tmp)->value.lval),&EG(regular_list))==TYPE_CHAR) {
               len=php3_intifx_get_char((int)((*tmp)->value.lval),&EG(regular_list),&char_tmp);
               indicator=0;
               if(char_tmp==NULL || len<0)
                indicator=-1;
               len++;
               EXEC SQL SET DESCRIPTOR :descrpid VALUE :i 
                                        DATA= :char_tmp,
                                        LENGTH= :len, 
                                        INDICATOR= :indicator,
                                        TYPE= :sqlchar_type;
              }
               
              i++;
              zend_hash_move_forward((*pblobidarr)->value.ht);
            } /* while */
      } /* if paramquery */
      Ifx_Result->affected_rows = affected_rows;   /* saved estimated from prepare */
      for (e = 0; e < 6; e++) Ifx_Result->sqlerrd[e] = sqlerrd[e];
     } else {
      /* ##
         ** SELECT-STATEMENT 
         **
      */

      ctype = 0;;   /* preset */

      switch(ZEND_NUM_ARGS()) {
       case 2:
         break;
       case 3:
         if (zend_get_parameters_ex(3, &dummy, &dummy, &cursortype)==FAILURE) {
                RETURN_FALSE;
         }
         convert_to_long_ex(cursortype);
         ctype = (*cursortype)->value.lval;
         break;
       default:
         WRONG_PARAM_COUNT;
         break;
      } /* case */
        strcpy(Ifx_Result->cursorid, cursorid);

        Ifx_Result->iscursory = 1;
        Ifx_Result->affected_rows = affected_rows;   /* saved estimated from prepare */
        for (e = 0; e < 6; e++) Ifx_Result->sqlerrd[e] = sqlerrd[e];
        EXEC SQL GET DESCRIPTOR :descrpid :fieldcount = COUNT;
        if (ifx_check() < 0) {
            IFXG(sv_sqlcode) = SQLCODE;
            php_error(E_WARNING,"Can not get descriptor %s (%s)",
                                  descrpid,
                                  ifx_error(ifx));
            EXEC SQL DEALLOCATE DESCRIPTOR :descrpid;
            EXEC SQL free :statemid;
            efree(Ifx_Result);
            RETURN_FALSE;
        }
        Ifx_Result->numcols = fieldcount;

        Ifx_Result->isscroll = Ifx_Result->ishold = 0;
        if (ctype & IFX_SCROLL)
            Ifx_Result->isscroll = 1;
        if (ctype & IFX_HOLD)
            Ifx_Result->ishold = 1;

   } /* if select */

   ZEND_REGISTER_RESOURCE(return_value, Ifx_Result, IFXL(le_result));

}
/* }}} */

/* ----------------------------------------------------------------------
** int ifx_do(int resultid)
** 
** executes a previously prepared query or opens a cursor for it
**
** returns TRUE on success, false on error
** does NOT free $resultid on error !!!
** 
** also sets (real) affected_rows  for non-select statements
** for retrieval by ifx_affected_rows
** ----------------------------------------------------------------------
*/

/* {{{ proto int ifx_do(int resultid)
   Executes a previously prepared query or opens a cursor for it */
PHP_FUNCTION(ifx_do)
{
    pval **result;
    IFX_RES *Ifx_Result;

EXEC SQL BEGIN DECLARE SECTION;
    char *ifx;                /* connection ID     */
    char *cursorid;           /* query cursor id   */
    char *statemid;           /* statement id      */
    char *descrpid;           /* descriptor id     */
    int  fieldcount;          /* field count       */
    int  i;                   /* field index       */
    short fieldtype;
    loc_t *locator;
    
    int e;

$ifdef HAVE_IFX_IUS;
    fixed binary 'blob' ifx_lo_t *slocator;
$endif;

EXEC SQL END DECLARE SECTION;

    int  locind;
    char *blobfilename;
    
    IFXLS_FETCH();

    switch(ZEND_NUM_ARGS()) {
        case 0:
            WRONG_PARAM_COUNT;
            break;
        case 1:
            if (zend_get_parameters_ex(1, &result)==FAILURE) {
                RETURN_FALSE;
            }
            break;
        default:
            WRONG_PARAM_COUNT;
            break;
    }
    
    ZEND_FETCH_RESOURCE(Ifx_Result, IFX_RES *, result, -1, "Informix Result", IFXL(le_result));
    
    IFXG(sv_sqlcode) = 0;
    
    ifx        = Ifx_Result->connecid;
    cursorid   = Ifx_Result->cursorid;
    statemid   = Ifx_Result->statemid;
    descrpid   = Ifx_Result->descrpid;
    fieldcount = Ifx_Result->numcols;

    EXEC SQL set connection :ifx;
    if (ifx_check() < 0) {
        IFXG(sv_sqlcode) = SQLCODE;
        php_error(E_WARNING,"Set connection %s fails (%s)",
                              ifx,
                              ifx_error(ifx));
        RETURN_FALSE;
    }
    
    if (Ifx_Result->iscursory < 0) {
        php_error(E_WARNING, "Resultindex %d is not a prepared query",
                   (*result)->value.lval);
        RETURN_FALSE;
    }
    if (Ifx_Result->iscursory==0) {        /* execute immediate */
        if(Ifx_Result->paramquery!=0) {
           EXEC SQL EXECUTE :statemid USING SQL DESCRIPTOR :descrpid;
        } else {
           EXEC SQL EXECUTE :statemid;
        }
        if (ifx_check() < 0) {
            IFXG(sv_sqlcode) = SQLCODE;
            php_error(E_WARNING,"Execute immediate fails : %s ",
                                  ifx_error(ifx));
            RETURN_FALSE;
        }
        Ifx_Result->affected_rows = sqlca.sqlerrd[2]; /* really affected */
        for (e = 0; e < 6; e++) Ifx_Result->sqlerrd[e] = sqlca.sqlerrd[e];
    } else {                                /* open cursor */
        if (Ifx_Result->isscroll) 
            if (Ifx_Result->ishold) 
               EXEC SQL DECLARE :cursorid SCROLL CURSOR WITH HOLD FOR :statemid;
            else
               EXEC SQL DECLARE :cursorid SCROLL CURSOR FOR :statemid;
        else
            if (Ifx_Result->ishold)
               EXEC SQL DECLARE :cursorid CURSOR WITH HOLD FOR :statemid;
            else
               EXEC SQL DECLARE :cursorid CURSOR FOR :statemid;

        if (ifx_check() < 0) {
            IFXG(sv_sqlcode) = SQLCODE;
            php_error(E_WARNING,"Declare cursor fails (%s)", ifx_error(ifx));
            RETURN_FALSE;
        }
        EXEC SQL OPEN :cursorid;
        if (ifx_check() < 0) {
            IFXG(sv_sqlcode) = SQLCODE;
            php_error(E_WARNING,"Open cursor fails (%s)", ifx_error(ifx));
            RETURN_FALSE;
        }
        
        /* check for text/blob columns */


        locind = 0;
        for (i = 1; i <= fieldcount; ++i) {
           EXEC SQL GET DESCRIPTOR :descrpid VALUE :i  :fieldtype = TYPE;
           if (
               (fieldtype == SQLTEXT) || (fieldtype == SQLBYTES) 
$ifdef HAVE_IFX_IUS;
               || (fieldtype==SQLUDTFIXED)
$endif;
               ) {
            
               int bid = 0;
               if(fieldtype==SQLTEXT) {
                   bid=php3_intifx_create_blob(TYPE_BLTEXT,BLMODE_INMEM,"",-1,&EG(regular_list));
                   locator=php3_intifx_get_blobloc(bid,&EG(regular_list));
                   EXEC SQL SET DESCRIPTOR :descrpid VALUE :i DATA = :*locator;
               } 
               if(fieldtype==SQLBYTES) {
                   if(IFXG(blobinfile)==0) {
                       bid=php3_intifx_create_blob(TYPE_BLBYTE,BLMODE_INMEM,"",-1,&EG(regular_list));
                       locator=php3_intifx_get_blobloc(bid,&EG(regular_list));
                   } else {
                       blobfilename=php3_intifx_create_tmpfile(i);
                       bid=php3_intifx_create_blob(
                                TYPE_BLBYTE,BLMODE_INFILE,
                                blobfilename,strlen(blobfilename),&EG(regular_list));
                       locator=php3_intifx_get_blobloc(bid,&EG(regular_list));
                       locator->loc_oflags=LOC_WONLY;
                   }
                   EXEC SQL SET DESCRIPTOR :descrpid VALUE :i DATA = :*locator;
                } 
$ifdef HAVE_IFX_IUS;
               if(fieldtype==SQLUDTFIXED) {
                   bid=php3_intifxus_new_slob(&EG(regular_list));
                   slocator=php3_intifxus_get_slobloc(bid,&EG(regular_list));
                   EXEC SQL SET DESCRIPTOR :descrpid VALUE :i DATA = :*slocator;  
                } 
$endif;
                Ifx_Result->res_id[locind]=bid;
                ++locind;
            }
        }
                         
    } /* end open cursor */

    RETURN_TRUE;

}
/* }}} */




/* ----------------------------------------------------------------------
** string ifx_error([int connection_id]);
**
** returns the Informix error codes (SQLSTATE & SQLCODE)
**
** connection id is not checked, but remains for compatibility
** ----------------------------------------------------------------------
*/

/* {{{ proto string ifx_error([int connection_id])
   Returns the Informix error codes (SQLSTATE & SQLCODE) */
PHP_FUNCTION(ifx_error)
{
    pval **ifx_link;
    int id;
    IFXLS_FETCH();

    
    switch(ZEND_NUM_ARGS()) {
        case 0:
            id = IFXG(default_link);
            break;
        case 1:
            if (zend_get_parameters_ex(1, &ifx_link)==FAILURE) {
                RETURN_FALSE;
            }
            break;
        default:
            WRONG_PARAM_COUNT;
            break;
    }
    
    RETURN_STRING(ifx_error(0),0); /* ifx_error returns emalloced string */
}
/* }}} */

/* ----------------------------------------------------------------------
** string ifx_errormsg([int errorcode])
**
** returns the Informix errormessage associated with 
** the most recent Informix error if SQLCODE is nonzero, or,
** when the optional "errocode" param is present, the errormessage 
** corresponding to "errorcode".
** ----------------------------------------------------------------------
*/

/* {{{ proto string ifx_errormsg([int errorcode])
   Returns the Informix errormessage associated with  */
PHP_FUNCTION(ifx_errormsg)
{
    pval **errcode;

    int ifx_errorcode;
    int msglen, maxmsglen;
    char *ifx_errmsg;
    char * returnmsg;

    IFXLS_FETCH();
    
    switch(ZEND_NUM_ARGS()) {
        case 0:
            if (IFXG(sv_sqlcode) == 0)
                ifx_errorcode = SQLCODE;
            else
                ifx_errorcode = IFXG(sv_sqlcode);
            break;
        case 1:
            if (zend_get_parameters_ex(1, &errcode)==FAILURE) {
                RETURN_FALSE;
            }
            convert_to_long_ex(errcode);
            ifx_errorcode = (*errcode)->value.lval;
            break;
        default:
            WRONG_PARAM_COUNT;
            break;
    }

    maxmsglen = 255;
    ifx_errmsg = (char *)malloc(maxmsglen+1);
    if (ifx_errorcode != 0) {
         rgetlmsg(ifx_errorcode, ifx_errmsg, maxmsglen, &msglen);
         if (msglen > maxmsglen) {
             maxmsglen = msglen + 1;
             free(ifx_errmsg);
             ifx_errmsg = (char *)malloc(maxmsglen + 1);
             rgetlmsg(ifx_errorcode, ifx_errmsg, maxmsglen, &msglen);
         }
    } else {
         ifx_errmsg[0] = 0;
    }

    returnmsg = (char *) emalloc(strlen(ifx_errmsg) + 128);
    sprintf(returnmsg,ifx_errmsg, sqlca.sqlerrm);
    free(ifx_errmsg);
    RETURN_STRING(returnmsg,0); 
     
}
/* }}} */

/* --------------------------------------------------------------
** int ifx_affected_rows(int resultid)
**
** returns the number of rows affected by query $resultid
**
** for selects : estimated number of rows (sqlerrd[0])
** for insert/update/delete : real number (sqlerrd[2])
** ---------------------------------------------------------------
*/

/* {{{ proto int ifx_affected_rows(int resultid)
   Returns the number of rows affected by query identified by resultid */
PHP_FUNCTION(ifx_affected_rows)
{
    pval **result;
    IFX_RES *Ifx_Result;

    IFXLS_FETCH();

    switch(ZEND_NUM_ARGS()) {
        case 0:
            WRONG_PARAM_COUNT;
            break;
        case 1:
            if (zend_get_parameters_ex(1, &result)==FAILURE) {
                RETURN_FALSE;
            }
            break;
        default:
            WRONG_PARAM_COUNT;
            break;
    }
    
    IFXG(sv_sqlcode )= 0;

    ZEND_FETCH_RESOURCE(Ifx_Result, IFX_RES *, result, -1, "Informix Result", IFXL(le_result));
    
    return_value->value.lval = Ifx_Result->affected_rows;
    return_value->type = IS_LONG;
}
/* }}} */

/* ----------------------------------------------------------------------
** array ifx_fetch_row(int resultid [, mixed $position])
**
** fetches the next row, or if using a scroll cursor, and position
** is present, the row as given in position, into an associative
** array with the fieldnames as key
**
** returns FALSE on error
**
** position can be : "FIRST", "NEXT", "LAST", "PREVIOUS", "CURRENT" 
** or an absolute row number
** ----------------------------------------------------------------------
*/

/* {{{ proto array ifx_fetch_row(int resultid [, mixed position])
   Fetches the next row or <position> row if using a scroll cursor */
PHP_FUNCTION(ifx_fetch_row)
{
    pval **result, **position;
    IFX_RES *Ifx_Result;

EXEC SQL BEGIN DECLARE SECTION;
    char *ifx;                /* connection ID    */
    char *cursorid;           /* query cursor id  */
    char *statemid;           /* statement id     */
    char *descrpid;           /* descriptor id    */
    int  fieldcount;          /* field count      */
    int   i;                  /* an index         */ 
    char  fieldname[64];      /* fieldname        */
    short fieldtype;          /* field type       */
    int   fieldleng;          /* field length     */

$ifdef HAVE_IFX_IUS;
    ifx_int8_t  int8_var;
    lvarchar	*lvar_tmp;
$endif;
   
    short      indicator;
    int        int_data;
    char       *char_data;
    long       date_data;
    interval   intvl_data = {0};
    datetime   dt_data = {0};
    decimal    dec_data = {0};
    short      short_data;
    loc_t      *locator_b;

$ifdef HAVE_IFX_IUS;
    fixed binary 'blob' ifx_lo_t *slocator;
$endif;

    float      float_data;
    double     double_data;
    int        fetch_row;
EXEC SQL END DECLARE SECTION;

    int num_fields;
    int locind,bid,bid_b;
    
    char string_data[256];
    long long_data;
    char *p;
    char *blobfilename;            

    char *fetch_pos;

    char *nullstr;
    
    IFXLS_FETCH();

    switch(ZEND_NUM_ARGS()) {
        case 0:
            WRONG_PARAM_COUNT;
            break;
        case 1:
            if (zend_get_parameters_ex(1, &result)==FAILURE) {
                RETURN_FALSE;
            }
            fetch_pos = NULL;
            fetch_row = 0;
            break;
        case 2:
            if (zend_get_parameters_ex(2, &result, &position)==FAILURE) {
                RETURN_FALSE;
            }
            if ((*position)->type != IS_STRING) {
                fetch_pos = NULL;
                fetch_row = (*position)->value.lval;
            } else {
                fetch_pos = (*position)->value.str.val;
                fetch_row = 0;
            }
            break;
        default:
            WRONG_PARAM_COUNT;
            break;
    }
    
    ZEND_FETCH_RESOURCE(Ifx_Result, IFX_RES *, result, -1, "Informix Result", IFXL(le_result));
    
    nullstr=php3_intifx_null();

    IFXG(sv_sqlcode) = 0;
    
    if (strcmp(Ifx_Result->cursorid,"") == 0) {
        php_error(E_WARNING,"Not a select cursor !");
        RETURN_FALSE;
    }
    
    ifx        = Ifx_Result->connecid;
    cursorid   = Ifx_Result->cursorid;
    statemid   = Ifx_Result->statemid;
    descrpid   = Ifx_Result->descrpid;
    fieldcount = Ifx_Result->numcols;

    EXEC SQL set connection :ifx;
    if (ifx_check() < 0) {
        IFXG(sv_sqlcode) = SQLCODE;
        php_error(E_WARNING,"Set connection %s fails (%s)",
                              ifx,
                              ifx_error(ifx));
        RETURN_FALSE;
    }


    if (! Ifx_Result->isscroll) {    
        EXEC SQL FETCH :cursorid USING SQL DESCRIPTOR :descrpid;
    } else {
        if (fetch_pos == NULL) {
            if (fetch_row != 0) {
                EXEC SQL FETCH ABSOLUTE :fetch_row
                               :cursorid USING SQL DESCRIPTOR :descrpid;
            } else {
                EXEC SQL FETCH NEXT :cursorid USING SQL DESCRIPTOR :descrpid;
            }
        } else {
            if (strcasecmp(fetch_pos,"NEXT") == 0) {
               EXEC SQL FETCH NEXT :cursorid USING SQL DESCRIPTOR :descrpid;
            } else {
            if (strcasecmp(fetch_pos,"PREVIOUS") == 0) {
               EXEC SQL FETCH PREVIOUS :cursorid USING SQL DESCRIPTOR :descrpid;
            } else {
            if (strcasecmp(fetch_pos,"FIRST") == 0) {
               EXEC SQL FETCH FIRST :cursorid USING SQL DESCRIPTOR :descrpid;
            } else {
            if (strcasecmp(fetch_pos,"LAST") == 0) {
               EXEC SQL FETCH LAST :cursorid USING SQL DESCRIPTOR :descrpid;
            } else { 
            if (strcasecmp(fetch_pos,"CURRENT") == 0) {
               EXEC SQL FETCH CURRENT :cursorid USING SQL DESCRIPTOR :descrpid;
            } else {
               php_error(E_WARNING, "invalid positioning arg on fetch");
            }}}}}
        }
    }  
   if(SQLCODE!=-451) {    
    switch (ifx_check()) {
        case IFX_ERROR:
            IFXG(sv_sqlcode) = SQLCODE;
            php_error(E_WARNING,
                       "Can not fetch row on cursor %s (%s)",
                       ifx_error(ifx),
                       cursorid);
            RETURN_FALSE;
            break;
        case IFX_NODATA:
            RETURN_FALSE;
            break;
        default:
            break;
     }
    }
    Ifx_Result->rowid++;

    if (array_init(return_value)==FAILURE) {
        RETURN_FALSE;
    }
    num_fields = fieldcount;
    
    locind = 0;
    for (i = 1; i <= num_fields; i++) {
        EXEC SQL GET DESCRIPTOR :descrpid VALUE :i :fieldtype = TYPE,
                                                   :fieldname = NAME,
                                                   :fieldleng = LENGTH,
                                                   :indicator = INDICATOR;
        if (ifx_check() < 0) {
            IFXG(sv_sqlcode) = SQLCODE;
            php_error(E_WARNING,"Get descriptor (field # %d) fails (%s)",
                                 i,
                                 ifx_error(ifx));
            RETURN_FALSE;
        }
        
        p = fieldname;         /* rtrim fieldname */
        while ((*p != ' ') && (p < &fieldname[sizeof(fieldname) - 1])) ++p;
        *p = 0;
         
        if (strcmp("(expression)", fieldname) == 0)	/* stored proc */
            sprintf(fieldname, "[Expr_%d]", i);

        if (indicator == -1) {        /* NULL */
           if((IFXG(textasvarchar)==0 
                                                  && fieldtype==SQLTEXT) 
              || (IFXG(byteasvarchar)==0 
                                                  && fieldtype==SQLBYTES)) {
  
              bid_b=Ifx_Result->res_id[locind];
              bid=php3_intifx_copy_blob(bid_b, &EG(regular_list));
              php3_intifx_update_blob(bid,nullstr,strlen(nullstr),&EG(regular_list));
              add_assoc_long(return_value,fieldname,bid);
              ++locind;
              continue; 
            }
            if (
                (fieldtype==SQLTEXT) || (fieldtype==SQLBYTES)
$ifdef HAVE_IFX_IUS;
                 || (fieldtype==SQLUDTFIXED)
$endif;
                ) {
              ++locind;
            }
            add_assoc_string(return_value, fieldname, nullstr, DUP);
            continue;
        } /* NULL */
        switch (fieldtype) {
            case SQLSERIAL  : 
            case SQLINT     :
                EXEC SQL GET DESCRIPTOR :descrpid VALUE :i :int_data = DATA;
                long_data = int_data;
                sprintf(string_data,"%ld", long_data); 
                add_assoc_string(return_value, fieldname, string_data, DUP);
                break;
            case SQLSMINT   :
                EXEC SQL GET DESCRIPTOR :descrpid VALUE :i :short_data = DATA;
                long_data = short_data;
                sprintf(string_data,"%ld", long_data); 
                add_assoc_string(return_value, fieldname, string_data, DUP);
                break;
            case SQLDECIMAL :
            case SQLMONEY   :
                EXEC SQL GET DESCRIPTOR :descrpid VALUE :i :dec_data = DATA;
                memset(string_data, 0x20, 64);
                dectoasc(&dec_data, string_data, 63, -1);
                for (p =string_data; *p != ' '; ++p) ;
                *p = 0;                
                add_assoc_string(return_value, fieldname, string_data, DUP);
                break;
            case SQLSMFLOAT :
                EXEC SQL GET DESCRIPTOR :descrpid VALUE :i :float_data = DATA;
                double_data = float_data;
                sprintf(string_data,"%17.17g", double_data);
                add_assoc_string(return_value, fieldname, string_data, DUP);
                break;
            case SQLFLOAT   :
                EXEC SQL GET DESCRIPTOR :descrpid VALUE :i :double_data = DATA;
                sprintf(string_data,"%17.17g", double_data);
                add_assoc_string(return_value, fieldname, string_data, DUP);
                break;
            case SQLDATE    :
                EXEC SQL GET DESCRIPTOR :descrpid VALUE :i :date_data = DATA;
                rdatestr(date_data, string_data); 
                add_assoc_string(return_value, fieldname, string_data, DUP);
                break;
            case SQLDTIME   :
                EXEC SQL GET DESCRIPTOR :descrpid VALUE :i :dt_data = DATA;
                dttoasc(&dt_data, string_data); 
                add_assoc_string(return_value, fieldname, string_data, DUP);
                break;
            case SQLINTERVAL:
                EXEC SQL GET DESCRIPTOR :descrpid VALUE :i :intvl_data = DATA;
                intoasc(&intvl_data, string_data); 
                add_assoc_string(return_value, fieldname, string_data, DUP);
                break;

$ifdef HAVE_IFX_IUS;
            case SQLSERIAL8   :
            case SQLINT8   :
                EXEC SQL GET DESCRIPTOR :descrpid VALUE :i :int8_var = DATA;
	        memset(string_data, ' ', sizeof(string_data));
                ifx_int8toasc(&int8_var,string_data,200);
                p = string_data;         /* rtrim string_data */
                while ((*p != ' ') && (p < &string_data[sizeof(string_data) - 1])) ++p;
                *p = 0;		
                add_assoc_string(return_value, fieldname, string_data, DUP);
                break;
            case SQLLVARCHAR:
            	ifx_var_flag(&lvar_tmp,1);
                EXEC SQL GET DESCRIPTOR :descrpid VALUE :i :lvar_tmp = DATA;

        		fieldleng=ifx_var_getlen(&lvar_tmp);
 
                if (fieldleng > 2) fieldleng -= 2; /* fix by Alex Shepherd */

                if ((char_data = (char *)emalloc(fieldleng + 1)) == NULL) {
                    php_error(E_WARNING, "Out of memory");
                    RETURN_FALSE;
                }
                memcpy(char_data,ifx_var_getdata(&lvar_tmp),fieldleng);
		ifx_var_dealloc(&lvar_tmp);
                add_assoc_stringl(return_value, fieldname, char_data, fieldleng,0);
                break;
            case SQLBOOL:
$endif;
            case SQLVCHAR   :
            case SQLNVCHAR  :
            case SQLCHAR    :
            case SQLNCHAR   :
                if ((char_data = (char *)emalloc(fieldleng + 1)) == NULL) {
                    php_error(E_WARNING, "Out of memory");
                    RETURN_FALSE;
                }
                EXEC SQL GET DESCRIPTOR :descrpid VALUE :i :char_data = DATA;
                if (IFXG(charasvarchar) != 0
                     && (fieldtype == SQLCHAR || fieldtype == SQLNCHAR)) {
                    ldchar(char_data, fieldleng, char_data);
                }
                add_assoc_string(return_value, fieldname, char_data, DUP);
                efree(char_data);
                char_data = NULL;
                break;

$ifdef HAVE_IFX_IUS;
            case SQLUDTFIXED   :   
                bid_b=Ifx_Result->res_id[locind];
                add_assoc_long(return_value,fieldname,bid_b);

                bid=php3_intifxus_new_slob(&EG(regular_list));
                slocator=php3_intifxus_get_slobloc(bid,&EG(regular_list));
                EXEC SQL SET DESCRIPTOR :descrpid VALUE :i DATA = :*slocator;  
                Ifx_Result->res_id[locind]=bid;
                ++locind;
		break;
$endif;
		
            case SQLBYTES   :   
            case SQLTEXT    :     
                bid_b=Ifx_Result->res_id[locind];
                locator_b=php3_intifx_get_blobloc(bid_b,&EG(regular_list)); 
                ++locind;

                EXEC SQL GET DESCRIPTOR :descrpid VALUE :i :*locator_b = DATA;
                /* work around for ESQL/C bug with NULL values and BLOBS */                
                if ((locator_b->loc_status < 0) && (locator_b->loc_bufsize == 0)){
                  locator_b->loc_indicator = -1;
                }      
                /* normal check for NULL values */
                if (locator_b->loc_indicator == -1) { 
                  if((IFXG(textasvarchar)==0 && fieldtype==SQLTEXT) 
                      || (IFXG(byteasvarchar)==0 && fieldtype==SQLBYTES)) {
                    bid_b=Ifx_Result->res_id[locind];
                    bid=php3_intifx_copy_blob(bid_b, &EG(regular_list));
                    php3_intifx_update_blob(bid,nullstr,strlen(nullstr),&EG(regular_list));
                    add_assoc_long(return_value,fieldname,bid);
                    break; 
                  }
                  if (
                     (fieldtype==SQLTEXT) || (fieldtype==SQLBYTES)
$ifdef HAVE_IFX_IUS;
                                          || (fieldtype==SQLUDTFIXED)
$endif;
                      ) {
                    add_assoc_string(return_value, fieldname, nullstr, DUP);
		    break;
                  }
                }
		
                if (locator_b->loc_status < 0) {  /* blob too large */   
                       php_error(E_WARNING,"no memory (%d bytes) for blob",
                                  locator_b->loc_bufsize);
                       RETURN_FALSE;
                }
                                 /* copy blob */
                bid=php3_intifx_copy_blob(bid_b, &EG(regular_list));
                                 /* and generate new tempfile for next row */
                if(locator_b->loc_loctype==LOCFNAME) {
                   blobfilename=php3_intifx_create_tmpfile(bid_b);
                   php3_intifx_update_blob(bid_b,blobfilename,strlen(blobfilename),&EG(regular_list));
                   efree(blobfilename);
                   EXEC SQL SET DESCRIPTOR :descrpid VALUE :i  
                                           DATA= :*locator_b;
                } 
 
                                /* return blob as VARCHAR ?          */
                                /* note that in case of "blobinfile" */
                                /* you get the file name             */
                                /* a new one for every row !         */
                if((IFXG(textasvarchar)!=0 
                                            && fieldtype==SQLTEXT) 
                  || (IFXG(byteasvarchar)!=0 
                                            && fieldtype==SQLBYTES)) {
                   char *content;
                   long lg;
                   lg=php3_intifx_get_blob(bid, &EG(regular_list), &content);
                   if(content==NULL || lg<0) {
                       add_assoc_string(return_value,fieldname,nullstr,DUP);
                   } else {
                       add_assoc_stringl(return_value,fieldname,content,lg,DUP);
                   }
                   php3_intifx_free_blob(bid, &EG(regular_list));
                   break;
                } 
                                  /* no, return as blob id */
                add_assoc_long(return_value,fieldname,bid);
                break;
            default         :
                sprintf(string_data,"ESQL/C : %s : unsupported field type[%d]",
                        fieldname,
                        fieldleng);
                add_assoc_string(return_value, fieldname, string_data, DUP);
                break;
        }
            
        continue;
    }    
    
}
/* }}} */



/* ----------------------------------------------------------------------
** int ifx_htmltbl_result(int resultid [, string htmltableoptions])
**
** formats all rows of the resultid query into a html table
** the optional second argument is a string of <table> tag options
**
** returns the number of rows printed or FALSE on error
** ----------------------------------------------------------------------
*/

/* {{{ proto int ifx_htmltbl_result(int resultid [, string htmltableoptions])
   Formats all rows of the resultid query into a html table */
PHP_FUNCTION(ifx_htmltbl_result)
{
    pval **result, **arg2;
    IFX_RES *Ifx_Result;

EXEC SQL BEGIN DECLARE SECTION;
    char  *ifx;                 /* connection ID    */
    char  *cursorid;            /* query cursor id  */
    char  *statemid;            /* statement id     */
    char  *descrpid;            /* descriptor id    */
    int   fieldcount;           /* field count      */
    int   i;                    /* an index         */ 
    char  fieldname[64];        /* fieldname        */
    short fieldtype;            /* field type       */
    int   fieldleng;            /* field length     */

$ifdef HAVE_IFX_IUS;
    ifx_int8_t  int8_var;
    lvarchar	*lvar_tmp;
$endif;

    short     indicator;
    int       int_data;
    char      *char_data = NULL;
    long      date_data;
    interval  intvl_data = {0};
    datetime  dt_data = {0};
    decimal   dec_data = {0};
    short     short_data;
    float     float_data;
    double    double_data;
    loc_t     *locator_b;
EXEC SQL END DECLARE SECTION;


    char *content;
    char *copy_content;
    long lg;
    
    char *nullstr;
                  
    int num_fields;
    
    char string_data[256];
    long long_data;
    char *p;
    int  locind,bid_b;
    char *table_options;
    int  moredata;

    IFXLS_FETCH();

    switch (ZEND_NUM_ARGS()) {
        case 1:
            if (zend_get_parameters_ex(1, &result)==FAILURE) {
                RETURN_FALSE;
            }
            table_options = NULL;
            break;
        case 2:
            if (zend_get_parameters_ex(2, &result, &arg2)==FAILURE) {
                RETURN_FALSE;
            }
            table_options = (*arg2)->value.str.val;
            break;
        default:
            WRONG_PARAM_COUNT;
            break;
    }
    
    IFXG(sv_sqlcode) = 0;

    ZEND_FETCH_RESOURCE(Ifx_Result, IFX_RES *, result, -1, "Informix Result", IFXL(le_result));

    if (strcmp(Ifx_Result->cursorid,"") == 0) {
        php_error(E_WARNING,"Not a select cursor !");
        RETURN_FALSE;
    }

    ifx        = Ifx_Result->connecid;
    cursorid   = Ifx_Result->cursorid;
    statemid   = Ifx_Result->statemid;
    descrpid   = Ifx_Result->descrpid;
    fieldcount = Ifx_Result->numcols;

    EXEC SQL set connection :ifx;
    if (ifx_check() < 0) {
        IFXG(sv_sqlcode) = SQLCODE;
        php_error(E_WARNING,"Set connection %s fails (%s)",
                              ifx,
                              ifx_error(ifx));
        RETURN_FALSE;
    }


    /* try to fetch the first row */            
    EXEC SQL FETCH :cursorid USING SQL DESCRIPTOR :descrpid;
    switch (ifx_check()) {
        case IFX_ERROR:
            IFXG(sv_sqlcode) = SQLCODE;
            php_error(E_WARNING,
                       "Can not fetch next row on cursor %s (%s)",
                       ifx_error(ifx),
                       cursorid);
            RETURN_FALSE;
            break;
        case IFX_NODATA:
            moredata = 0;
            break;
        default:
            Ifx_Result->rowid = moredata = 1;
            break;
    }
    
    if(! moredata) {
        php_printf("<h2>No rows found</h2>\n");
        RETURN_LONG(0);
    }
    num_fields = fieldcount;
    nullstr = php3_intifx_null();

    /* start table tag */
    if (table_options == NULL)
        php_printf("<table><tr>");
    else
        php_printf("<table %s><tr>", table_options);

    /* table headings */
    for (i = 1; i <= num_fields; i++) {
        EXEC SQL GET DESCRIPTOR :descrpid VALUE :i :fieldname = NAME;
        if (ifx_check() < 0) {
            IFXG(sv_sqlcode) = SQLCODE;
            php_error(E_WARNING,"Get descriptor (field # %d) fails (%s)",
                                 i,
                                 ifx_error(ifx));
            RETURN_FALSE;
        }
            
        p = fieldname;         /* Capitalize and rtrim fieldname */
        *p = toupper(*p);
        while ((*p != ' ') && (p < &fieldname[sizeof(fieldname) - 1])) ++p;
        *p = 0;
        if (strcmp("(expression)", fieldname) == 0)	/* stored proc */
            sprintf(fieldname, "[Expr_%d]", i);
        
        php_printf("<th>%s</th>", fieldname);
    }    
    php_printf("</tr>\n");
    
    /* start spitting out rows untill none left */    
    while (moredata) { 
        php_printf("<tr>");
        locind = 0;
        for (i = 1; i <= num_fields; i++) {
            EXEC SQL GET DESCRIPTOR :descrpid VALUE :i :fieldtype = TYPE,
                                                       :fieldleng = LENGTH,
                                                       :indicator = INDICATOR;
            if (ifx_check() < 0) {
                IFXG(sv_sqlcode) = SQLCODE;
                php_error(E_WARNING,"Get descriptor (field # %d) fails (%s)",
                                     i,
                                     ifx_error(ifx));
                RETURN_FALSE;
            }
            
            if (indicator == -1) {        /* NULL */
                if(fieldtype==SQLTEXT || fieldtype==SQLBYTES) {
                    ++locind;
                }
                php_printf("<td>%s</td>", nullstr);
                continue;
            }
            switch (fieldtype) {
                case SQLSERIAL  : 
                case SQLINT     :
                    EXEC SQL GET DESCRIPTOR :descrpid VALUE :i :int_data = DATA;
                    long_data = int_data;
                    sprintf(string_data,"%ld", long_data); 
                    php_printf("<td>%s</td>", string_data);
                    break;
                case SQLSMINT   :
                    EXEC SQL GET DESCRIPTOR :descrpid VALUE :i :short_data = DATA;
                    long_data = short_data;
                    sprintf(string_data,"%ld", long_data); 
                    php_printf("<td>%s</td>", string_data);
                    break;
                case SQLDECIMAL :
                case SQLMONEY   :
                    EXEC SQL GET DESCRIPTOR :descrpid VALUE :i :dec_data = DATA;
                    memset(string_data, 0x20, 64);
                    dectoasc(&dec_data, string_data, 63, -1);
                    for (p =string_data; *p != ' '; ++p) ;
                    *p = 0;                
                    php_printf("<td>%s</td>", string_data);
                    break;
                case SQLSMFLOAT :
                    EXEC SQL GET DESCRIPTOR :descrpid VALUE :i :float_data = DATA;
                    double_data = float_data;
                    sprintf(string_data,"%17.17g", double_data);
                    php_printf("<td>%s</td>", string_data);
                    break;
                case SQLFLOAT   :
                    EXEC SQL GET DESCRIPTOR :descrpid VALUE :i :double_data = DATA;
                    sprintf(string_data,"%17.17g", double_data);
                    php_printf("<td>%s</td>", string_data);
                    break;
                case SQLDATE    :
                    EXEC SQL GET DESCRIPTOR :descrpid VALUE :i :date_data = DATA;
                    rdatestr(date_data, string_data); 
                    php_printf("<td>%s</td>", string_data);
                    break;
                case SQLDTIME   :
                    EXEC SQL GET DESCRIPTOR :descrpid VALUE :i :dt_data = DATA;
                    dttoasc(&dt_data, string_data); 
                    php_printf("<td>%s</td>", string_data);
                    break;
                case SQLINTERVAL:
                    EXEC SQL GET DESCRIPTOR :descrpid VALUE :i :intvl_data = DATA;
                    intoasc(&intvl_data, string_data); 
                    php_printf("<td>%s</td>", string_data);
                    break;
$ifdef HAVE_IFX_IUS;
                case SQLSERIAL8:
                case SQLINT8   :
                    EXEC SQL GET DESCRIPTOR :descrpid VALUE :i :int8_var = DATA;
                    memset(string_data, ' ', sizeof(string_data));
                    ifx_int8toasc(&int8_var,string_data,200);
                    p = string_data;         /* rtrim string_data */
                    while ((*p != ' ') && (p < &string_data[sizeof(string_data) - 1])) ++p;
                    *p = 0;		
                    php_printf("<td>%s</td>", string_data);
                    break;
                case SQLLVARCHAR:
	            	ifx_var_flag(&lvar_tmp,1);
        	        EXEC SQL GET DESCRIPTOR :descrpid VALUE :i :lvar_tmp = DATA;

			        fieldleng=ifx_var_getlen(&lvar_tmp);
 
                    if (fieldleng > 2) fieldleng -= 2; /* fix by Alex Shepherd */

        	        if ((char_data = (char *)emalloc(fieldleng + 1)) == NULL) {
                	    php_error(E_WARNING, "Out of memory");
            	        RETURN_FALSE;
             	   }
                	memcpy(char_data,ifx_var_getdata(&lvar_tmp),fieldleng);
			ifx_var_dealloc(&lvar_tmp);
                	add_assoc_stringl(return_value, fieldname, char_data, fieldleng,0);
                	break;

                case SQLBOOL    :
$endif;
                case SQLCHAR    :
                case SQLVCHAR   :
                case SQLNCHAR   :
                case SQLNVCHAR  :
                    if ((char_data = (char *)emalloc(fieldleng + 1)) == NULL) {
                        php_error(E_WARNING, "Out of memory");
                        RETURN_FALSE;
                    }
                    EXEC SQL GET DESCRIPTOR :descrpid VALUE :i :char_data = DATA;
                    if (IFXG(charasvarchar) != 0
                          && (fieldtype == SQLCHAR || fieldtype == SQLNCHAR)) {
                        ldchar(char_data, fieldleng, char_data);
                    }
                    php_printf("<td>%s</td>", char_data);
                    efree(char_data);
                    char_data = NULL;
                    break;
                case SQLTEXT    :
                                /* treated always as a long VARCHAR here     */
                                /* if blobinbfile, too bad                   */
                    bid_b=Ifx_Result->res_id[locind];
                    ++locind;

                    locator_b=php3_intifx_get_blobloc(bid_b,&EG(regular_list)); 
                  
                    EXEC SQL GET DESCRIPTOR :descrpid VALUE :i 
                                            :*locator_b = DATA;
                    /* work around for ESQL/C bug with NULL values and BLOBS */                
                    if ((locator_b->loc_status < 0) && (locator_b->loc_bufsize == 0)){
                      locator_b->loc_indicator = -1;
                    }      
                    /* normal check for NULL values */
                    if (locator_b->loc_indicator == -1) {
                      if (
                         (fieldtype==SQLTEXT) || (fieldtype==SQLBYTES)
$ifdef HAVE_IFX_IUS;
                                              || (fieldtype==SQLUDTFIXED)
$endif;
                          ) {
                        php_printf("<td>%s</td>", nullstr);
		        break;
                      }
		    }
                    if (locator_b->loc_status < 0) {  /* blob too large */   
                        php_error(E_WARNING,"no memory (%d bytes) for blob",
                                   locator_b->loc_bufsize);
                        RETURN_FALSE;
                    }
                    
                                       /* get blob contents */    
                    lg=php3_intifx_get_blob(bid_b, &EG(regular_list), &content);
                 
                    if(content==NULL || lg<0) {
                        php_printf("<td>%s</td>", nullstr);
                    } else {
                                /* need an extra byte for string terminator */
                        copy_content = malloc(lg + 1);
                        if (copy_content == NULL) {
                            php_error(E_WARNING,"no memory for TEXT column");
                            RETURN_FALSE;
                        }
                        memcpy(copy_content, content, lg);
                        copy_content[lg]=0;
                        php_printf("<td>%s</td>", copy_content);
                        free(copy_content);
                    }
                    break;

                case SQLBYTES   : 
                    ++locind;
                    php_printf("<td>(BYTE)</td>");
                    break;                
                default         :
                     sprintf(string_data,
                             "ESQL/C : %s : unsupported field type[%d]",
                             fieldname,
                             fieldleng);
                     php_printf("<td>%s</td>", string_data);
                     break;
            }
                
            continue;
        }
        php_printf("</tr>\n");    
        /* fetch next row */ 
        EXEC SQL FETCH :cursorid USING SQL DESCRIPTOR :descrpid;
        switch (ifx_check()) {
            case IFX_ERROR:
                IFXG(sv_sqlcode) = SQLCODE;
                php_error(E_WARNING,
                           "Can not fetch next row on cursor %s (%s)",
                           ifx_error(ifx),
                           cursorid);
                RETURN_FALSE;
                break;
            case IFX_NODATA:
                moredata = 0;
                break;
            default:
                break;
        }
        Ifx_Result->rowid++;
          
    } /* endwhile (moredata); */    
    php_printf("</table>\n");
    RETURN_LONG(Ifx_Result->rowid);

}
/* }}} */

/* ----------------------------------------------------------------------
** array ifx_fieldtypes(int resultid)
**
** returns an associative array with fieldnames as key
** and SQL fieldtypes as data for query $resultid
** 
** returns FALSE on error
** ----------------------------------------------------------------------
*/


/* {{{ proto array ifx_fieldtypes(int resultid)
   Returns an associative array with fieldnames as key for query <resultid> */
PHP_FUNCTION(ifx_fieldtypes)
{
    pval **result;
    IFX_RES *Ifx_Result;

EXEC SQL BEGIN DECLARE SECTION;
    char  *ifx;                 /* connection ID    */
    char  *cursorid;            /* query cursor id  */
    char  *statemid;            /* statement id     */
    char  *descrpid;            /* descriptor id    */
    int   fieldcount;           /* field count      */
    int   i;                    /* an index         */ 
    char  fieldname[64];        /* fieldname        */
    short fieldtype;            /* field type       */

    char      *char_data = NULL;

EXEC SQL END DECLARE SECTION;

    int num_fields;
    
    char *p;
            
    IFXLS_FETCH();

    switch (ZEND_NUM_ARGS()) {
        case 1:
            if (zend_get_parameters_ex(1, &result)==FAILURE) {
                RETURN_FALSE;
            }
            break;
        default:
            WRONG_PARAM_COUNT;
            break;
    }
    
    IFXG(sv_sqlcode) = 0;
    
    ZEND_FETCH_RESOURCE(Ifx_Result, IFX_RES *, result, -1, "Informix Result", IFXL(le_result));

    if (strcmp(Ifx_Result->cursorid,"") == 0) {
        php_error(E_WARNING,"Not a select cursor !");
        RETURN_FALSE;
    }

    ifx        = Ifx_Result->connecid;
    cursorid   = Ifx_Result->cursorid;
    statemid   = Ifx_Result->statemid;
    descrpid   = Ifx_Result->descrpid;
    fieldcount = Ifx_Result->numcols;

    EXEC SQL set connection :ifx;
    if (ifx_check() < 0) {
        IFXG(sv_sqlcode) = SQLCODE;
        php_error(E_WARNING,"Set connection %s fails (%s)",
                              ifx,
                              ifx_error(ifx));
        RETURN_FALSE;
    }

    if (array_init(return_value)==FAILURE) {
        RETURN_FALSE;
    }
    num_fields = fieldcount;
    for (i = 1; i <= num_fields; i++) {
        EXEC SQL GET DESCRIPTOR :descrpid VALUE :i :fieldname = NAME,
                                                   :fieldtype = TYPE;
        if (ifx_check() < 0) {
            IFXG(sv_sqlcode) = SQLCODE;
            php_error(E_WARNING,"Get descriptor (field # %d) fails (%s)",
                                 i,
                                 ifx_error(ifx));
            RETURN_FALSE;
        }
            
        p = fieldname;         /* rtrim fieldname */
        while ((*p != ' ') && (p < &fieldname[sizeof(fieldname) - 1])) ++p;
        *p = 0;
        if (strcmp("(expression)", fieldname) == 0)	/* stored proc */
            sprintf(fieldname, "[Expr_%d]", i);
        
        switch (fieldtype) {
            case SQLSERIAL  : 
                char_data = "SQLSERIAL";
                break;
            case SQLINT     :
                char_data = "SQLINT";
                break;
            case SQLSMINT   :
                char_data = "SQLSMINT";
                break;
            case SQLDECIMAL :
                char_data = "SQLDECIMAL";
                break;
            case SQLMONEY   :
                char_data = "SQLMONEY";
                break;
            case SQLSMFLOAT :
                char_data = "SQLSMFLOAT";
                break;
            case SQLFLOAT   :
                char_data = "SQLFLOAT";
                break;
            case SQLDATE    :
                char_data = "SQLDATE";
                break;
            case SQLDTIME   :
                char_data = "SQLDTIME";
                break;
            case SQLINTERVAL:
                char_data = "SQLINTERVAL";
                break;
            case SQLCHAR    :
                char_data = "SQLCHAR";
                break;
            case SQLVCHAR   :
                char_data = "SQLVCHAR";
                break;
            case  SQLNCHAR  :   
                char_data = "SQLNCHAR";
                break;
            case  SQLNVCHAR  :   
                char_data = "SQLNVCHAR";
                break;
            case SQLTEXT    :
                char_data = "SQLTEXT";
                break;
            case SQLBYTES   :   
                char_data = "SQLBYTES";
                break;
$ifdef HAVE_IFX_IUS;
            case  SQLUDTFIXED  :   
                char_data = "SQLUDTFIXED";
                break;
            case  SQLBOOL  :   
                char_data = "SQLBOOL";
                break;
            case  SQLINT8  :   
                char_data = "SQLINT8";
                break;
            case  SQLSERIAL8  :   
                char_data = "SQLSERIAL8";
                break;
            case  SQLLVARCHAR  :   
                char_data = "SQLLVARCHAR";
                break;
$endif;
            default         :
               char_data=emalloc(20);
               sprintf(char_data,"ESQL/C : %i",fieldtype);
               break;
        } /* switch (fieldtype) */
                
        add_assoc_string(return_value, fieldname, char_data, DUP);

    }   /* for() */ 
    
}
/* }}} */

/* ----------------------------------------------------------------------
** array ifx_fieldproperties(int resultid)
**
** returns an associative array with fieldnames as key
** and SQL fieldproperties as data for query $resultid
**
** properties are encoded as : "SQLTYPE;length;precision;scale;ISNULLABLE"
** where SQLTYPE = the Informix type like "SQLVCHAR"  etc...
**       ISNULLABLE = "Y" or "N"
** 
** returns FALSE on error
** ----------------------------------------------------------------------
*/

/* {{{ proto array ifx_fieldproperties(int resultid)
   Returns an associative for query <resultid> array with fieldnames as key */
PHP_FUNCTION(ifx_fieldproperties)
{
    pval **result;
    IFX_RES *Ifx_Result;

EXEC SQL BEGIN DECLARE SECTION;
    char  *ifx;                 /* connection ID    */
    char  *cursorid;            /* query cursor id  */
    char  *statemid;            /* statement id     */
    char  *descrpid;            /* descriptor id    */
    int   fieldcount;           /* field count      */
    int   i;                    /* an index         */ 
    char  fieldname[64];        /* fieldname        */
    short fieldtype;            /* field type       */
    char      *char_data = NULL;
    int       size;
    int       precision;
    int       scale;
    int       isnullable;
EXEC SQL END DECLARE SECTION;

    int num_fields;
    
    char string_data[256];
    char *p;
            
    IFXLS_FETCH();

    switch (ZEND_NUM_ARGS()) {
        case 1:
            if (zend_get_parameters_ex(1, &result)==FAILURE) {
                RETURN_FALSE;
            }
            break;
        default:
            WRONG_PARAM_COUNT;
            break;
    }
    
    IFXG(sv_sqlcode) = 0;
    
    ZEND_FETCH_RESOURCE(Ifx_Result, IFX_RES *, result, -1, "Informix Result", IFXL(le_result));

    if (strcmp(Ifx_Result->cursorid,"") == 0) {
        php_error(E_WARNING,"Not a select cursor !");
        RETURN_FALSE;
    }

    ifx        = Ifx_Result->connecid;
    cursorid   = Ifx_Result->cursorid;
    statemid   = Ifx_Result->statemid;
    descrpid   = Ifx_Result->descrpid;
    fieldcount = Ifx_Result->numcols;

    EXEC SQL set connection :ifx;
    if (ifx_check() < 0) {
        IFXG(sv_sqlcode) = SQLCODE;
        php_error(E_WARNING,"Set connection %s fails (%s)",
                              ifx,
                              ifx_error(ifx));
        RETURN_FALSE;
    }

   
    if (array_init(return_value)==FAILURE) {
        RETURN_FALSE;
    }
    num_fields = fieldcount;
    for (i = 1; i <= num_fields; i++) {
        EXEC SQL GET DESCRIPTOR :descrpid VALUE :i :fieldname = NAME,
                                                   :fieldtype = TYPE,
                                                   :size = LENGTH,
                                                   :precision = PRECISION,
                                                   :scale = SCALE,
                                                   :isnullable = NULLABLE;
        if (ifx_check() < 0) {
            IFXG(sv_sqlcode) = SQLCODE;
            php_error(E_WARNING,"Get descriptor (field # %d) fails (%s)",
                                 i,
                                 ifx_error(ifx));
            RETURN_FALSE;
        }
            
        p = fieldname;         /* rtrim fieldname */
        while ((*p != ' ') && (p < &fieldname[sizeof(fieldname) - 1])) ++p;
        *p = 0;
        if (strcmp("(expression)", fieldname) == 0)	/* stored proc */
            sprintf(fieldname, "[Expr_%d]", i);

        switch (fieldtype) {
            case SQLSERIAL  : 
                char_data = "SQLSERIAL";
                break;
            case SQLINT     :
                char_data = "SQLINT";
                break;
            case SQLSMINT   :
                char_data = "SQLSMINT";
                break;
            case SQLDECIMAL :
                char_data = "SQLDECIMAL";
                break;
            case SQLMONEY   :
                char_data = "SQLMONEY";
                break;
            case SQLSMFLOAT :
                char_data = "SQLSMFLOAT";
                break;
            case SQLFLOAT   :
                char_data = "SQLFLOAT";
                break;
            case SQLDATE    :
                char_data = "SQLDATE";
                break;
            case SQLDTIME   :
                char_data = "SQLDTIME";
                break;
            case SQLINTERVAL:
                char_data = "SQLINTERVAL";
                break;
            case SQLCHAR    :
                char_data = "SQLCHAR";
                break;
            case SQLVCHAR   :
                char_data = "SQLVCHAR";
                break;
            case SQLTEXT    :
                char_data = "SQLTEXT";
                break;
            case SQLBYTES   :   
                char_data = "SQLBYTES";
                break;
            case  SQLNCHAR  :   
                char_data = "SQLNCHAR";
                break;
            case  SQLNVCHAR  :   
                char_data = "SQLNVCHAR";
                break;
$ifdef HAVE_IFX_IUS;
            case  SQLUDTFIXED  :   
                char_data = "SQLUDTFIXED";
                break;
            case  SQLBOOL  :   
                char_data = "SQLBOOL";
                break;
            case  SQLINT8  :   
                char_data = "SQLINT8";
                break;
            case  SQLSERIAL8  :   
                char_data = "SQLSERIAL8";
                break;
            case  SQLLVARCHAR  :   
                char_data = "SQLLVARCHAR";
                break;
$endif;
            default         :
               char_data=emalloc(20);
               sprintf(char_data,"ESQL/C : %i",fieldtype);
                break;
        } /* switch (fieldtype) */

        sprintf(string_data,"%s;%d;%d;%d;%c",
                            char_data, 
                            size, 
                            precision, 
                            scale,
                            (isnullable?'Y':'N'));       
        add_assoc_string(return_value, fieldname, string_data, DUP);

    }   /* for() */ 
    
}
/* }}} */


/* --------------------------------------------------------------
** int ifx_num_rows(int resultid)
**
** returns the number of rows already fetched for query resultid
**
** ---------------------------------------------------------------
*/

/* {{{ proto int ifx_num_rows(int resultid)
   Returns the number of rows already fetched for query identified by resultid */
PHP_FUNCTION(ifx_num_rows)
{
    pval **result;
    IFX_RES *Ifx_Result;

    IFXLS_FETCH();

    if (ZEND_NUM_ARGS()!=1 || zend_get_parameters_ex(1, &result)==FAILURE) {
        WRONG_PARAM_COUNT;
    }
    
    IFXG(sv_sqlcode) = 0;
    
    ZEND_FETCH_RESOURCE(Ifx_Result, IFX_RES *, result, -1, "Informix Result", IFXL(le_result));

    return_value->value.lval = Ifx_Result->rowid;
    return_value->type = IS_LONG;
}
/* }}} */


/* --------------------------------------------------------------
** int ifx_getsqlca(int resultid)
**
** returns the sqlerrd[] fields of the sqlca struct for query resultid
** following the prepare (select) or execute immediate (insert/update/execute procedure)
**
** ---------------------------------------------------------------
*/

/* {{{ proto int ifx_getsqlca(int resultid)
   Returns the sqlerrd[] fields of the sqlca struct for query resultid */
PHP_FUNCTION(ifx_getsqlca)
{
    pval **result;
    IFX_RES *Ifx_Result;

    char fieldname[16];
    int e;
   
    IFXLS_FETCH();

    if (ZEND_NUM_ARGS()!=1 || zend_get_parameters_ex(1, &result)==FAILURE) {
        WRONG_PARAM_COUNT;
    }
    
    IFXG(sv_sqlcode) = 0;
    
    ZEND_FETCH_RESOURCE(Ifx_Result, IFX_RES *, result, -1, "Informix Result", IFXL(le_result));

    /* create pseudo-row array to return */
    if (array_init(return_value)==FAILURE) {
        RETURN_FALSE;
    }
    /* fill array with 6 fields sqlerrd0 .. sqlerrd5 */
    /* each ESQLC call saves these sqlca values      */
    for (e = 0; e < 6; e++) {
       sprintf(fieldname,"sqlerrd%d", e);
       add_assoc_long(return_value, fieldname, Ifx_Result->sqlerrd[e]);
    }

}
/* }}} */



/* ----------------------------------------------------------------------
** int ifx_num_fields(int resultid)
**
** returns the number of columns in query resultid
** or FALSE on error
** ----------------------------------------------------------------------
*/

/* {{{ proto int ifx_num_fields(int resultid)
   Returns the number of columns in query resultid */
PHP_FUNCTION(ifx_num_fields)
{
    pval **result;
    IFX_RES *Ifx_Result;

    IFXLS_FETCH();

    if (ZEND_NUM_ARGS()!=1 || zend_get_parameters_ex(1, &result)==FAILURE) {
        WRONG_PARAM_COUNT;
    }
    
    IFXG(sv_sqlcode) = 0;
    
    ZEND_FETCH_RESOURCE(Ifx_Result, IFX_RES *, result, -1, "Informix Result", IFXL(le_result));

    return_value->value.lval = Ifx_Result->numcols;
    return_value->type = IS_LONG;
}
/* }}} */




/* ----------------------------------------------------------------------
** int ifx_free_result(int resultid)
**
** releases resources for query associated with resultid
**
** returns FALSE on error
** ----------------------------------------------------------------------
*/

/* {{{ proto int ifx_free_result(int resultid)
   Releases resources for query associated with resultid */
PHP_FUNCTION(ifx_free_result)
{
    pval **result;
    IFX_RES *Ifx_Result;
    
EXEC SQL BEGIN DECLARE SECTION;
    char *ifx;                 /* connection ID    */
    char *cursorid;            /* query cursor id  */
    char *statemid;            /* statement id     */
    char *descrpid;            /* descriptor id    */
EXEC SQL END DECLARE SECTION;

    int i;    

    IFXLS_FETCH();

    if (ZEND_NUM_ARGS()!=1 || zend_get_parameters_ex(1, &result)==FAILURE) {
        WRONG_PARAM_COUNT;
    }
    
    ZEND_FETCH_RESOURCE(Ifx_Result, IFX_RES *, result, -1, "Informix Result", IFXL(le_result));

    IFXG(sv_sqlcode = 0);

    for (i = 0; i < MAX_RESID; ++i) {
        if (Ifx_Result->res_id[i]>0) {
         php3_intifx2_free_blob(Ifx_Result->res_id[i],&EG(regular_list));
         Ifx_Result->res_id[i]=-1;
        }
    }

    ifx        = Ifx_Result->connecid;
    cursorid   = Ifx_Result->cursorid;
    statemid   = Ifx_Result->statemid;
    descrpid   = Ifx_Result->descrpid;
    
    EXEC SQL set connection :ifx;
    if (ifx_check() < 0) {
        IFXG(sv_sqlcode) = SQLCODE;
        php_error(E_WARNING,"Set connection %s fails (%s)",
                              ifx,
                              ifx_error(ifx));
        RETURN_FALSE;
    }

   
    EXEC SQL free :statemid;
    if (strlen(cursorid) != 0) {
        EXEC SQL CLOSE :cursorid;
        EXEC SQL FREE :cursorid;
    }

        
    EXEC SQL DEALLOCATE DESCRIPTOR :descrpid;
        
    efree(Ifx_Result);             /* this can be safely done now */
  
    zend_list_delete((*result)->value.lval);
    RETURN_TRUE;
}
/* }}} */





/* ----------------------------------------------------------------------
 * internal function
 * long php3_intifx_getType(long id, HashTable *list)
 *
 * returns the type of an id-object
 *  bid: Id of object
 *  list: internal hashlist of php3
 * 
 * return -1 on error otherwise the type: TYPE_BLTEXT, TYPE_BLBYTE, TYPE_SLOB
 * ----------------------------------------------------------------------
*/
static long php3_intifx_getType(long id, HashTable *list) {
 IFX_IDRES *Ifx_res;
 int type;

 IFXLS_FETCH();

 Ifx_res = (IFX_IDRES *) zend_list_find(id,&type);
 if (type!=IFXL(le_idresult)) {
  php_error(E_WARNING,"%d is not a Informix id-result index",
            id);
  return -1;
 }
 return Ifx_res->type;
 }




/* ----------------------------------------------------------------------
** int ifx_create_blob(int type, int mode, string param)
**
** creates a blob-object
**  type: 1=TEXT, 0=BYTE
**  mode: blob-object holds 0=the content in memory, 1=content in file
**  param: if mode=0: pointer to the content
**            mode=1: pointer to the filestring
** return false on error otherwise the new Blob-Object-id
** ----------------------------------------------------------------------
*/

/* {{{ proto int ifx_create_blob(int type, int mode, string param)
   Creates a blob-object */
PHP_FUNCTION(ifx_create_blob) {
 pval *pmode, *pparam,*ptype;
 long id;
 long mode,type;
  
 if (ZEND_NUM_ARGS()!=3 || getParameters(ht, 3, &ptype,&pmode,&pparam)==FAILURE) {
   WRONG_PARAM_COUNT;
 }
 convert_to_long(pmode);
 convert_to_string(pparam);
 convert_to_long(ptype);

 type=ptype->value.lval;
 if(type!=0) 
  type=TYPE_BLTEXT;
 mode=pmode->value.lval;
 if(mode!=0) 
  mode=BLMODE_INFILE;
  
 id=php3_intifx_create_blob(type,mode,pparam->value.str.val,pparam->value.str.len,&EG(regular_list)); 
 if(id<0) {
    RETURN_FALSE;
 } 
 RETURN_LONG(id);
}
/* }}} */

/* ----------------------------------------------------------------------
 * internal function
 * long php3_intifx_create_blob(long type, long mode, char* param, long len, HashTable *list)
 *
 * creates an blob-object
 *  type: 1=TEXT, 0=BYTE
 *  mode: blob-object holds 0=the content in momory, 1=content in file
 *  param: if mode=0: pointer to the content
 *            mode=1: pointer to the filestring
 *  len: length of param
 *  list: internal hashlist of php3
 * return -1 on error otherwise the new Blob-Object-id
 * ----------------------------------------------------------------------
*/
static long php3_intifx_create_blob(long type, long mode, char* param, long len, HashTable *list) {
 IFX_IDRES *Ifx_blob;

 IFXLS_FETCH();

 Ifx_blob=emalloc(sizeof(IFX_IDRES));
 if(Ifx_blob==NULL) {
  php_error(E_WARNING,"can't create blob-resource");
  return -1;
 }
 
 memset(Ifx_blob, 0, sizeof(IFX_IDRES));      
 
 if(type==0 ) {
  Ifx_blob->type=TYPE_BLBYTE;
 } else {
  Ifx_blob->type=TYPE_BLTEXT;
 }
 Ifx_blob->BLOB.mode=(int)mode;

 if(mode==BLMODE_INMEM) {
  if(len>=0) {
   char *content=emalloc(len);
   if(content==NULL) {
    php_error(E_WARNING,"can't create blob-resource");
    return -1;
   }
   memcpy(content,param,len);
   Ifx_blob->BLOB.blob_data.loc_loctype=LOCMEMORY;
   Ifx_blob->BLOB.blob_data.loc_buffer=content;
   Ifx_blob->BLOB.blob_data.loc_bufsize=len;
   Ifx_blob->BLOB.blob_data.loc_size=len;
   Ifx_blob->BLOB.blob_data.loc_mflags=0;
   Ifx_blob->BLOB.blob_data.loc_oflags=0;
  } else {
   Ifx_blob->BLOB.blob_data.loc_loctype=LOCMEMORY;
   Ifx_blob->BLOB.blob_data.loc_buffer=NULL;
   Ifx_blob->BLOB.blob_data.loc_bufsize=-1;
   Ifx_blob->BLOB.blob_data.loc_size=-1;
   Ifx_blob->BLOB.blob_data.loc_mflags=0;
   Ifx_blob->BLOB.blob_data.loc_oflags=0;
  }
 } else {                             /* mode = BLMODE_INFILE */
  char *filename=emalloc(len+1);
  if(filename==NULL)  {
   php_error(E_WARNING,"can't create blob-resource");
   return -1;
  }
  memcpy(filename,param,len);
  filename[len]=0;
  Ifx_blob->BLOB.blob_data.loc_loctype=LOCFNAME;
  Ifx_blob->BLOB.blob_data.loc_fname=filename;
  Ifx_blob->BLOB.blob_data.loc_oflags=LOC_WONLY;
  Ifx_blob->BLOB.blob_data.loc_size=-1;
 }
 return zend_list_insert(Ifx_blob,IFXL(le_idresult));
}




/* ----------------------------------------------------------------------
** int ifx_copy_blob(int bid)
**
** duplicates the given blob-object
**  bid: Id of Blobobject
** 
** return false on error otherwise the new Blob-Object-id
** ----------------------------------------------------------------------
*/

/* {{{ proto int ifx_copy_blob(int bid)
   Duplicates the given blob-object */
PHP_FUNCTION(ifx_copy_blob) {
 pval *pbid;
 long newid;
  
 if (ZEND_NUM_ARGS()!=1 || getParameters(ht, 1, &pbid)==FAILURE) {
   WRONG_PARAM_COUNT;
 }
 convert_to_long(pbid);

 newid=php3_intifx_copy_blob(pbid->value.lval,&EG(regular_list)); 
 if(newid<0) {
  RETURN_FALSE;
 } 
 
 RETURN_LONG(newid);
}
/* }}} */



/* ----------------------------------------------------------------------
 * internal function
 * long php3_intifx_copy_blob(long bid, HashTable *list)
 *
 * duplicates the given blob-object
 *  bid: Id of Blobobject
 *  list: internal hashlist of php3
 * 
 * return -1 on error otherwise the new Blob-Object-id
 * ----------------------------------------------------------------------
*/
static long php3_intifx_copy_blob(long bid, HashTable *list) {
 IFX_IDRES *Ifx_blob, *Ifx_blob_orig;
 loc_t *locator, *locator_orig;
 int type;
 
 IFXLS_FETCH();

 Ifx_blob_orig = (IFX_IDRES *) zend_list_find(bid,&type);
 if (type!=IFXL(le_idresult) || !(Ifx_blob_orig->type==TYPE_BLBYTE || Ifx_blob_orig->type==TYPE_BLTEXT)) {
  php_error(E_WARNING,"%d is not a Informix blob-result index",
            bid);
  return -1;
 }
 Ifx_blob=emalloc(sizeof(IFX_IDRES));
 if(Ifx_blob==NULL) {
  php_error(E_WARNING,"can't create blob-resource");
  return -1;
 }
 
 memset(Ifx_blob, 0, sizeof(IFX_IDRES));
  
 Ifx_blob->type=Ifx_blob_orig->type;
 Ifx_blob->BLOB.mode=Ifx_blob_orig->BLOB.mode;
 
 locator=&(Ifx_blob->BLOB.blob_data);
 locator_orig=&(Ifx_blob_orig->BLOB.blob_data);

 if(Ifx_blob->BLOB.mode==BLMODE_INMEM) {
  char *content;
  if(locator_orig->loc_size>=0 && locator_orig->loc_buffer!=NULL) {
   if((content=emalloc(locator_orig->loc_size))==NULL) {
    php_error(E_WARNING,"can't create blob-resource");
    return -1;
   }
   memcpy(content,locator_orig->loc_buffer,  locator_orig->loc_size);
   locator->loc_buffer=content;
   locator->loc_bufsize=locator_orig->loc_size;
   locator->loc_size=locator_orig->loc_size;
  } else {
   locator->loc_buffer=NULL;
   locator->loc_bufsize=-1;
   locator->loc_size=-1;
  }
  locator->loc_loctype=LOCMEMORY;
  locator->loc_mflags=0;
  locator->loc_oflags=0;
 } else {  /* BLMODE_INFILE */
  char *filename;

  if((filename=emalloc(strlen(locator_orig->loc_fname)+1))==NULL)  {
   php_error(E_WARNING,"can't create blob-resource");
   return -1;
  }
  strcpy(filename,locator_orig->loc_fname);
  locator->loc_loctype=LOCFNAME;
  locator->loc_fname=filename;
  locator->loc_size=-1;
  locator->loc_oflags=locator_orig->loc_oflags;
 }
 
 return zend_list_insert(Ifx_blob,IFXL(le_idresult));
}



/* ----------------------------------------------------------------------
** int ifx_free_blob(int bid)
**
** deletes the blob-object
**  bid: Id of Blobobject
** return false on error otherwise true
** ----------------------------------------------------------------------
*/

/* {{{ proto int ifx_free_blob(int bid)
   Deletes the blob-object */
PHP_FUNCTION(ifx_free_blob) {
 pval *pid;
 long ret;
  
 if (ZEND_NUM_ARGS()!=1 || getParameters(ht, 1, &pid)==FAILURE) {
   WRONG_PARAM_COUNT;
 }
 convert_to_long(pid);

 ret=php3_intifx_free_blob(pid->value.lval,&EG(regular_list)); 
 if(ret<0) {
  RETURN_FALSE;
 } 
 RETURN_TRUE;
}
/* }}} */

/* ----------------------------------------------------------------------
 * internal function
 * long php3_intifx_free_blob(long bid, HashTable *list)
 *
 * deletes the blob-object
 *  bid: Id of Blobobject
 *  list: internal hashlist of php3
 * 
 * return -1 on error otherwise 0
 * FREES BYTE-MEMORY WITH EFREE()
 * ----------------------------------------------------------------------
*/
static long php3_intifx_free_blob(long bid, HashTable *list) {
 IFX_IDRES *Ifx_blob;
 int type;
 
 IFXLS_FETCH();

 Ifx_blob = (IFX_IDRES *) zend_list_find(bid,&type);
 if (type!=IFXL(le_idresult) && !(Ifx_blob->type==TYPE_BLTEXT || Ifx_blob->type==TYPE_BLBYTE)) {
  php_error(E_WARNING,"%d is not a Informix blob-result index",
            bid);
  return -1;
 }
 if(Ifx_blob->BLOB.mode==BLMODE_INMEM) {
    if(Ifx_blob->BLOB.blob_data.loc_buffer==NULL 
       || Ifx_blob->BLOB.blob_data.loc_size<=0) {;} else {
       efree(Ifx_blob->BLOB.blob_data.loc_buffer);  
    }
 } else {   /* BLMODE_INFILE */
    if(Ifx_blob->BLOB.blob_data.loc_fname!=NULL) {
       efree(Ifx_blob->BLOB.blob_data.loc_fname); 
    }
 }

 
 zend_list_delete(bid);
 efree(Ifx_blob); 
 return 0;
}



/* ----------------------------------------------------------------------
 * internal function
 * long php3_intifx2_free_blob(long bid, HashTable *list)
 *
 * deletes the blob-object
 *  bid: Id of Blobobject
 *  list: internal hashlist of php3
 * 
 * return -1 on error otherwise 0
 * FREES BYTE-MEMORY WITH FREE(), for blob memory allocated by ESQL/C  
 * use this for freeing blob-source after select (in ifx_free_result)
 * ----------------------------------------------------------------------
*/
static long php3_intifx2_free_blob(long bid, HashTable *list) {
 IFX_IDRES *Ifx_blob;
 int type;
 
 IFXLS_FETCH();

 Ifx_blob = (IFX_IDRES *) zend_list_find(bid,&type);
 if (type!=IFXL(le_idresult) && !(Ifx_blob->type==TYPE_BLTEXT || Ifx_blob->type==TYPE_BLBYTE)) {
  php_error(E_WARNING,"%d is not a Informix blob-result index",
            bid);
  return -1;
 }
#if IFX_VERSION < 724                                       /* this Informix  memory leak is fixed from 7.24 on     */
                                                            /* according to the current PERL DBD::Informix          */
                                                            /* and otherwise I get segmenation violations with 7.30 */

 if(Ifx_blob->BLOB.mode==BLMODE_INMEM) {
    if(Ifx_blob->BLOB.blob_data.loc_buffer==NULL ||
       Ifx_blob->BLOB.blob_data.loc_size<=0) {;} else {
       free(Ifx_blob->BLOB.blob_data.loc_buffer);  
    }
 } else {
    if(Ifx_blob->BLOB.blob_data.loc_fname!=NULL) {
       efree(Ifx_blob->BLOB.blob_data.loc_fname); 
    }
 }

#endif
  
 zend_list_delete(bid);
 efree(Ifx_blob); 
 return 0;
}

/* ----------------------------------------------------------------------
** string ifx_get_blob(int bid)
**
** returns the content of the blob-object
**  bid: Id of Blobobject
** return the content
** ----------------------------------------------------------------------
*/

/* {{{ proto string ifx_get_blob(int bid)
   Returns the content of the blob-object */
PHP_FUNCTION(ifx_get_blob) {
 pval *pbid;
 char *content;
 long len;
 
  
 if (ZEND_NUM_ARGS()!=1 || getParameters(ht, 1, &pbid)==FAILURE) {
   WRONG_PARAM_COUNT;
 }
 convert_to_long(pbid);

 len=php3_intifx_get_blob(pbid->value.lval,&EG(regular_list),&content); 
 if(content==NULL || len<0) {
   RETURN_STRING(php3_intifx_null(),1);
 }
 RETURN_STRINGL(content,len,1);
}
/* }}} */


/* ----------------------------------------------------------------------
 * internal function
 * long php3_intifx_get_blob(long bid, HashTable *list, char** content)
 *
 * returns the content of the blob-object
 *  bid: Id of Blobobject
 *  list: internal hashlist of php3
 * 
 * return -1 on error
 * returns the pointer to the content in char** content and the amount of content in bytes
 * ----------------------------------------------------------------------
*/
static long php3_intifx_get_blob(long bid, HashTable *list, char** content) {
 IFX_IDRES *Ifx_blob;
 int type;
 
 IFXLS_FETCH();

 Ifx_blob = (IFX_IDRES *) zend_list_find(bid,&type);
 if (type!=IFXL(le_idresult) && !(Ifx_blob->type==TYPE_BLTEXT || Ifx_blob->type==TYPE_BLBYTE)) {
  php_error(E_WARNING,"%d is not a Informix blob-result index",
            bid);
  return -1;
 }

 if(Ifx_blob->BLOB.mode==BLMODE_INMEM) {
  *content=Ifx_blob->BLOB.blob_data.loc_buffer;
  return Ifx_blob->BLOB.blob_data.loc_size;
 }
 *content=Ifx_blob->BLOB.blob_data.loc_fname;
 return strlen(Ifx_blob->BLOB.blob_data.loc_fname);
}


/* ----------------------------------------------------------------------
 * internal function
 * loc_t *php3_intifx_get_blobloc(long bid, HashTable *list)
 *
 * returns the blob-locator-structur
 *  bid: Id of Blobobject
 *  list: internal hashlist of php3
 * return NULL on error or the pointer to the locator-structur
 * ----------------------------------------------------------------------
*/
static loc_t *php3_intifx_get_blobloc(long bid, HashTable *list) {
 IFX_IDRES *Ifx_blob;
 int type;
 
 IFXLS_FETCH();

 Ifx_blob = (IFX_IDRES *) zend_list_find(bid,&type);
 if (type!=IFXL(le_idresult) && !(Ifx_blob->type==TYPE_BLTEXT || Ifx_blob->type==TYPE_BLBYTE)) {
  php_error(E_WARNING,"%d is not a Informix blob-result index",
            bid);
  return NULL;
 }

 return &(Ifx_blob->BLOB.blob_data);
}



/* ----------------------------------------------------------------------
** int update_blob(int bid, string content)
**
** updates the content of the blob-object
**  bid: Id of Blobobject
**  content: string of new data
** return false on error otherwise true
** ----------------------------------------------------------------------
*/

/* {{{ proto int ifx_update_blob(int bid, string content)
   Updates the content of the blob-object */
PHP_FUNCTION(ifx_update_blob) {
 pval *pbid,*pparam;
 long ret;
 
  
 if (ZEND_NUM_ARGS()!=2 || getParameters(ht, 2, &pbid,&pparam)==FAILURE) {
   WRONG_PARAM_COUNT;
 }
 convert_to_long(pbid);
 convert_to_string(pparam);
 
 ret=php3_intifx_update_blob(pbid->value.lval,
                             pparam->value.str.val,
                             pparam->value.str.len,
                             &EG(regular_list)); 
 if(ret<0) {
  RETURN_FALSE;
 } 
 RETURN_TRUE;
}
/* }}} */


/* ----------------------------------------------------------------------
 * internal function
 * long php3_intifx_update_blob(long bid, char* param, long len, HashTable *list)
 *
 * updates the content of the blob-object
 *  bid: Id of Blobobject
 *  param: string of new data
 *  len: length of string
 *  list: internal hashlist of php3
 * return nothing
 * ----------------------------------------------------------------------
*/
static long php3_intifx_update_blob(long bid, char* param, long len, HashTable *list) {
 IFX_IDRES *Ifx_blob;
 int type;
 
 IFXLS_FETCH();

 Ifx_blob = (IFX_IDRES *) zend_list_find(bid,&type);
 if (type!=IFXL(le_idresult) && !(Ifx_blob->type==TYPE_BLTEXT || Ifx_blob->type==TYPE_BLBYTE)) {
  php_error(E_WARNING,"%d is not a Informix blob-result index",
            bid);
  return -1;
 }

 if(Ifx_blob->BLOB.mode==BLMODE_INMEM) {
  char *content;

  if(Ifx_blob->BLOB.blob_data.loc_buffer!=NULL)
   efree(Ifx_blob->BLOB.blob_data.loc_buffer);
  if(len>=0) {
   if((content=emalloc(len))==NULL) {
    php_error(E_WARNING,"can't create blob-resource");
    return -1;
   }
   memcpy(content,param, len);
   Ifx_blob->BLOB.blob_data.loc_buffer=content;
   Ifx_blob->BLOB.blob_data.loc_bufsize=len;
   Ifx_blob->BLOB.blob_data.loc_size=len;
  } else {
   Ifx_blob->BLOB.blob_data.loc_buffer=NULL;
   Ifx_blob->BLOB.blob_data.loc_bufsize=-1;
   Ifx_blob->BLOB.blob_data.loc_size=-1;
  }
  Ifx_blob->BLOB.blob_data.loc_mflags=0;
  Ifx_blob->BLOB.blob_data.loc_oflags=0;
 } else {
  char *filename;

  if(Ifx_blob->BLOB.blob_data.loc_fname!=NULL)
   efree(Ifx_blob->BLOB.blob_data.loc_fname);
  if((filename=emalloc(len+1))==NULL)  {
   php_error(E_WARNING,"can't create blob-resource");
   return -1;
  }
  memcpy(filename,param, len);
  filename[len]=0;
  Ifx_blob->BLOB.blob_data.loc_fname=filename;
  Ifx_blob->BLOB.blob_data.loc_size=-1;
 }
 return 0;
}



/*-------------------------------------------------
 * internal function
 *
 * php3_intifx_create_tmpfile(long bid)
 * creates a temporary file to store a blob in 
 *-------------------------------------------------
*/


static char* php3_intifx_create_tmpfile(long bid) {
 char filename[10];
 char *blobdir;
 char *blobfile;
 char *retval;
  
 if ((blobdir = getenv("php3_blobdir")) == NULL)
   blobdir=".";
 
 sprintf(filename,"blb%d",(int)bid);
 blobfile=tempnam(blobdir,filename);
 /* free(blobdir); */
 
 if (blobfile == NULL) 
   return NULL;
 
 retval=emalloc(strlen(blobfile)+1);
 if(retval==NULL) 
  return NULL;
 strcpy(retval,blobfile);
 free(blobfile);
 return retval;
}




/* ----------------------------------------------------------------------
** void ifx_blobinfile_mode(int mode)
**
** sets the default blob-mode for all select-queries 
**  mode=0: save Byte-Blobs in momory
**  mode=1: save Byte-Blobs in a file
** return nothing
** ----------------------------------------------------------------------
*/

/* {{{ proto void ifx_blobinfile_mode(int mode)
   Sets the default blob-mode for all select-queries  */
PHP_FUNCTION(ifx_blobinfile_mode) {
 pval *pmode;

 
 IFXLS_FETCH();

 if (ZEND_NUM_ARGS()!=1 || getParameters(ht, 1, &pmode)==FAILURE) {
   WRONG_PARAM_COUNT;
 }
 convert_to_long(pmode);

  IFXG(blobinfile)=pmode->value.lval;
  RETURN_TRUE;
}
/* }}} */



/* ----------------------------------------------------------------------
** void ifx_textasvarchar(int mode)
**
** sets the default text-mode for all select-queries 
**  mode=0: select returns a blob-id
**  mode=1: select returns a varchar with text-content
** return nothing
** ----------------------------------------------------------------------
*/

/* {{{ proto void ifx_textasvarchar(int mode)
   Sets the default text-mode for all select-queries */
PHP_FUNCTION(ifx_textasvarchar) {
 pval *pmode;
 
 IFXLS_FETCH();

 if (ZEND_NUM_ARGS()!=1 || getParameters(ht, 1, &pmode)==FAILURE) {
   WRONG_PARAM_COUNT;
 }
 convert_to_long(pmode);

  IFXG(textasvarchar)=pmode->value.lval;
  RETURN_TRUE;
}
/* }}} */



/* ----------------------------------------------------------------------
** void ifx_byteasvarchar(int mode)
**
** sets the default byte-mode for all select-queries 
**  mode=0: select returns a blob-id
**  mode=1: select returns a varchar with byte-content
** return nothing
** ----------------------------------------------------------------------
*/

/* {{{ proto void ifx_byteasvarchar(int mode)
   Sets the default byte-mode for all select-queries  */
PHP_FUNCTION(ifx_byteasvarchar) {
 pval *pmode;
  
 
 IFXLS_FETCH();

 if (ZEND_NUM_ARGS()!=1 || getParameters(ht, 1, &pmode)==FAILURE) {
   WRONG_PARAM_COUNT;
 }
 convert_to_long(pmode);

  IFXG(byteasvarchar)=pmode->value.lval;
  RETURN_TRUE;
}



/* ----------------------------------------------------------------------
** void ifx_nullformat(int mode)
**
** sets the default return value of a NULL-value on a fetch-row 
**  mode=0: return ""
**  mode=1: return "NULL"
** return nothing
** ----------------------------------------------------------------------
*/

/* {{{ proto void ifx_nullformat(int mode)
   Sets the default return value of a NULL-value on a fetch-row  */
PHP_FUNCTION(ifx_nullformat) {
 pval *pmode;

 
 IFXLS_FETCH();

  
 if (ZEND_NUM_ARGS()!=1 || getParameters(ht, 1, &pmode)==FAILURE) {
   WRONG_PARAM_COUNT;
 }
 convert_to_long(pmode);

  IFXG(nullformat)=pmode->value.lval;
  RETURN_TRUE;
}
/* }}} */


/* ----------------------------------------------------------------------
 * void php3_intifx_null()
 *
 * return the NULL-string depending on .nullformat
 * return "" or "NULL"
 * ----------------------------------------------------------------------
*/
static char* php3_intifx_null() {
  char* tmp;

 
 IFXLS_FETCH();

  if(IFXG(nullformat)==0) {
   tmp=IFXG(nullvalue);
  } else {
   tmp=IFXG(nullstring);
  }
  return tmp;
}



/* ----------------------------------------------------------------------
** int ifx_create_char(string param)
**
** creates an char-object
**  param: content
** return false on error otherwise the new char-Object-id
** ----------------------------------------------------------------------
*/

/* {{{ proto int ifx_create_char(string param)
   Creates a char-object */
PHP_FUNCTION(ifx_create_char) {
 pval *pparam;
 long id;
  
 if (ZEND_NUM_ARGS()!=1 || getParameters(ht, 1, &pparam)==FAILURE) {
   WRONG_PARAM_COUNT;
 }
 convert_to_string(pparam);

 id=php3_intifx_create_char(pparam->value.str.val,pparam->value.str.len,&EG(regular_list)); 
 if(id<0) {
    RETURN_FALSE;
 } 
 RETURN_LONG(id);
}
/* }}} */

/* ----------------------------------------------------------------------
 * internal function
 * long php3_intifx_create_char(char* param, long len, HashTable *list)
 *
 * creates an char-object
 *  param: content
 *  len: length of param
 *  list: internal hashlist of php3
 * return -1 on error otherwise the new char-Object-id
 * ----------------------------------------------------------------------
*/
static long php3_intifx_create_char(char* param, long len, HashTable *list) {
 IFX_IDRES *Ifx_char;

 
 IFXLS_FETCH();

 Ifx_char=emalloc(sizeof(IFX_IDRES));
 if(Ifx_char==NULL) {
  php_error(E_WARNING,"can't create char-resource");
  return -1;
 }

 Ifx_char->type=TYPE_CHAR;

 if(param==NULL || len<0) {
  Ifx_char->CHAR.char_data=NULL;
  Ifx_char->CHAR.len=0;
 } else {
  Ifx_char->CHAR.char_data=emalloc(len+1);
  if(Ifx_char->CHAR.char_data==NULL) {
   efree(Ifx_char);
   php_error(E_WARNING,"can't create char-resource");
   return -1;
  } 
  memcpy(Ifx_char->CHAR.char_data,param,len);
  Ifx_char->CHAR.char_data[len]=0;
  Ifx_char->CHAR.len=len;
 }
 return zend_list_insert(Ifx_char,IFXL(le_idresult));
}

/* ----------------------------------------------------------------------
** string ifx_get_char(int bid)
**
** returns the content of the char-object
**  bid: Id of charobject
** return the content
** ----------------------------------------------------------------------
*/

/* {{{ proto string ifx_get_char(int bid)
   Returns the content of the char-object */
PHP_FUNCTION(ifx_get_char) {
 pval *pbid;
 char *content;
 long len;
 
  
 if (ZEND_NUM_ARGS()!=1 || getParameters(ht, 1, &pbid)==FAILURE) {
   WRONG_PARAM_COUNT;
 }
 convert_to_long(pbid);

 len=php3_intifx_get_char(pbid->value.lval,&EG(regular_list),&content); 
 if(content==NULL || len<0) {
   RETURN_STRING("",1);
 }
 RETURN_STRINGL(content,len,1);
}
/* }}} */


/* ----------------------------------------------------------------------
 * internal function
 * long php3_intifx_get_char(long bid, HashTable *list, char** content)
 *
 * returns the content of the char-object
 *  bid: Id of charobject
 *  list: internal hashlist of php3
 * 
 * return -1 on error
 * returns the pointer to the content in char** content and the amount of content in bytes
 * ----------------------------------------------------------------------
*/
static long php3_intifx_get_char(long bid, HashTable *list, char** content) {
 IFX_IDRES *Ifx_char;
 int type;
  
 IFXLS_FETCH();

 Ifx_char = (IFX_IDRES *) zend_list_find(bid,&type);
 if (type!=IFXL(le_idresult) && !(Ifx_char->type==TYPE_CHAR)) {
  php_error(E_WARNING,"%d is not a Informix char-result index",
            bid);
  return -1;
 }

 *content=Ifx_char->CHAR.char_data;
 return Ifx_char->CHAR.len;
}

/* ----------------------------------------------------------------------
** int ifx_free_char(int bid)
**
** deletes the char-object
**  bid: Id of charobject
** return false on error otherwise true
** ----------------------------------------------------------------------
*/

/* {{{ proto int ifx_free_char(int bid)
   Deletes the char-object */
PHP_FUNCTION(ifx_free_char) {
 pval *pid;
 long ret;
  
 if (ZEND_NUM_ARGS()!=1 || getParameters(ht, 1, &pid)==FAILURE) {
   WRONG_PARAM_COUNT;
 }
 convert_to_long(pid);

 ret=php3_intifx_free_char(pid->value.lval,&EG(regular_list)); 
 if(ret<0) {
  RETURN_FALSE;
 } 
 RETURN_TRUE;
}
/* }}} */

/* ----------------------------------------------------------------------
 * internal function
 * long php3_intifx_free_char(long bid, HashTable *list)
 *
 * deletes the char-object
 *  bid: Id of Charobject
 *  list: internal hashlist of php3
 * 
 * return -1 on error otherwise 0
 * ----------------------------------------------------------------------
*/
static long php3_intifx_free_char(long bid, HashTable *list) {
 IFX_IDRES *Ifx_char;
 int type;
 
 
 IFXLS_FETCH();

 Ifx_char = (IFX_IDRES *) zend_list_find(bid,&type);
 if (type!=IFXL(le_idresult) && !(Ifx_char->type==TYPE_CHAR)) {
  php_error(E_WARNING,"%d is not a Informix char-result index",
            bid);
  return -1;
 }

 if(Ifx_char->CHAR.char_data!=NULL) {
  efree(Ifx_char->CHAR.char_data);
 }
 
 zend_list_delete(bid);
 efree(Ifx_char); 
 return 0;
}

/* ----------------------------------------------------------------------
** int ifx_update_char(int bid, string content)
**
** updates the content of the char-object
**  bid: Id of charobject
**  content: string of new data
** return false on error otherwise true
** ----------------------------------------------------------------------
*/

/* {{{ proto int ifx_update_char(int bid, string content)
   Updates the content of the char-object */
PHP_FUNCTION(ifx_update_char) {
 pval *pbid,*pparam;
 long ret;
 
  
 if (ZEND_NUM_ARGS()!=2 || getParameters(ht, 2, &pbid,&pparam)==FAILURE) {
   WRONG_PARAM_COUNT;
 }
 convert_to_long(pbid);
 convert_to_string(pparam);
 
 ret=php3_intifx_update_char(pbid->value.lval,
                             pparam->value.str.val,
                             pparam->value.str.len,
                             &EG(regular_list)); 
 if(ret<0) {
  RETURN_FALSE;
 } 
 RETURN_TRUE;
}
/* }}} */


/* ----------------------------------------------------------------------
 * internal function
 * long php3_intifx_update_char(long bid, char* param, long len, HashTable *list)
 *
 * updates the content of the char-object
 *  bid: Id of charobject
 *  param: string of new data
 *  len: length of string
 *  list: internal hashlist of php3
 * return nothing
 * ----------------------------------------------------------------------
*/
static long php3_intifx_update_char(long bid, char* param, long len, HashTable *list) {
 IFX_IDRES *Ifx_char;
 int type;
 
 IFXLS_FETCH();

 Ifx_char = (IFX_IDRES *) zend_list_find(bid,&type);
 if (type!=IFXL(le_idresult) && !(Ifx_char->type==TYPE_CHAR)) {
  php_error(E_WARNING,"%d is not a Informix char-result index",
            bid);
  return -1;
 }

 if(Ifx_char->CHAR.char_data!=NULL) {
  efree(Ifx_char->CHAR.char_data);
 }
 Ifx_char->CHAR.char_data=NULL;
 Ifx_char->CHAR.len=0;
 
 if(param==NULL || len<0) {
  Ifx_char->CHAR.char_data=NULL;
  Ifx_char->CHAR.len=0;
 } else {
  Ifx_char->CHAR.char_data=emalloc(len+1);
  if(Ifx_char->CHAR.char_data==NULL) {
   php_error(E_WARNING,"can't create char-resource");
   return -1;
  } 
  memcpy(Ifx_char->CHAR.char_data,param,len);
  Ifx_char->CHAR.char_data[len]=0;
  Ifx_char->CHAR.len=len;
 }
 return 0;
}


$ifdef HAVE_IFX_IUS;


/* ----------------------------------------------------------------------
** int ifxus_create_slob(int mode)
**
** creates an slob-object and opens it
**  mode: 1=LO_RDONLY, 2=LO_WRONLY, 4=LO_APPEND, 8=LO_RDWR, 16=LO_BUFFER, 32=LO_NOBUFFER -> or-mask
** return false on error otherwise the new Slob-Object-id
** ----------------------------------------------------------------------
*/

/* {{{ proto int ifxus_create_slob(int mode)
   Creates a slob-object and opens it */
PHP_FUNCTION(ifxus_create_slob) {
 pval *pmode;
 long id;
 long mode,create_mode;
  
 if (ZEND_NUM_ARGS()!=1 || getParameters(ht, 1, &pmode)==FAILURE) {
   WRONG_PARAM_COUNT;
 }
 convert_to_long(pmode);

 mode=pmode->value.lval;
 
 create_mode=0;
 if((mode&1) !=0)   
  create_mode|=LO_RDONLY;
 if((mode&2) !=0)   
  create_mode|=LO_WRONLY;
 if((mode&4) !=0)   
  create_mode|=LO_APPEND;
 if((mode&8) !=0)   
  create_mode|=LO_RDWR;
 if((mode&16) !=0)   
  create_mode|=LO_BUFFER;
 if((mode&32) !=0)   
  create_mode|=LO_NOBUFFER;

  
 id=php3_intifxus_create_slob(create_mode,&EG(regular_list)); 
 if(id<0) {
  RETURN_FALSE;
 } 
 RETURN_LONG(id);
}
/* }}} */

/* ----------------------------------------------------------------------
 * internal function
 * long php3_intifx_create_slob(long create_mode, HashTable *list)
 *
 * creates an slob-object and opens it
 *  mode: 1=LO_RDONLY, 2=LO_WRONLY, 4=LO_APPEND, 8=LO_RDWR, 16=LO_BUFFER, 32=LO_NOBUFFER -> or-mask
 *  list: internal hashlist of php3
 * return -1 on error otherwise the new Blob-Object-id
 * ----------------------------------------------------------------------
*/
static long php3_intifxus_create_slob(long create_mode, HashTable *list) {
 IFX_IDRES *Ifx_slob;
 int errcode;
 

 Ifx_slob=emalloc(sizeof(IFX_IDRES));
 if(Ifx_slob==NULL) {
  php_error(E_WARNING,"can't create slob-resource");
  return -1;
 }

 errcode=ifx_lo_def_create_spec(&(Ifx_slob->SLOB.createspec));     
 if(errcode<0) {
  php_error(E_WARNING,"can't create slob-resource");
  return -1;
 }

 Ifx_slob->type=TYPE_SLOB;
 Ifx_slob->SLOB.lofd=ifx_lo_create(Ifx_slob->SLOB.createspec,create_mode,&(Ifx_slob->SLOB.slob_data),&errcode);     
 if(errcode<0 || Ifx_slob->SLOB.lofd<0) {
  php_error(E_WARNING,"can't create slob-resource");
  return -1;
 }

 return zend_list_insert(Ifx_slob,IFXL(le_idresult));
}



/* ----------------------------------------------------------------------
** int ifxus_free_slob(int bid)
**
** deletes the slob-object
**  bid: Id of Slobobject
** return false on error otherwise true
** ----------------------------------------------------------------------
*/

/* {{{ proto int ifxus_free_slob(int bid)
   Deletes the slob-object */
PHP_FUNCTION(ifxus_free_slob) {
 pval *pid;
 long ret;
  
 if (ZEND_NUM_ARGS()!=1 || getParameters(ht, 1, &pid)==FAILURE) {
   WRONG_PARAM_COUNT;
 }
 convert_to_long(pid);

 ret=php3_intifxus_close_slob(pid->value.lval,&EG(regular_list)); 
 if(ret<0) {
  RETURN_FALSE;
 } 
 RETURN_TRUE;
}
/* }}} */

/* ----------------------------------------------------------------------
 * internal function
 * long php3_intifxus_free_slob(long bid, HashTable *list)
 *
 * deletes the slob-object
 *  bid: Id of Slobobject
 *  list: internal hashlist of php3
 * 
 * return -1 on error otherwise 0
 * ----------------------------------------------------------------------
*/
static long php3_intifxus_free_slob(long bid, HashTable *list) {
 IFX_IDRES *Ifx_slob;
 int type;
 
 IFXLS_FETCH();

 Ifx_slob = (IFX_IDRES *) zend_list_find(bid,&type);
 if (type!=IFXL(le_idresult) || Ifx_slob->type!=TYPE_SLOB) {
  php_error(E_WARNING,"%d is not a Informix slob-result index",
            bid);
  return -1;
 }


 if(php3_intifxus_close_slob(bid, &EG(regular_list))<0) {
  return -1;
 }
 if(Ifx_slob->SLOB.createspec!=NULL) {
  ifx_lo_spec_free(Ifx_slob->SLOB.createspec);
  Ifx_slob->SLOB.createspec=NULL;
 }
 efree(Ifx_slob);
 zend_list_delete(bid);
 return 0;
}




/* ----------------------------------------------------------------------
** int ifxus_close_slob(int bid)
**
** deletes the slob-object
**  bid: Id of Slobobject
** return false on error otherwise true
** ----------------------------------------------------------------------
*/

/* {{{ proto int ifxus_close_slob(int bid)
   Deletes the slob-object */
PHP_FUNCTION(ifxus_close_slob) {
 pval *pid;
 long ret;
  
 if (ZEND_NUM_ARGS()!=1 || getParameters(ht, 1, &pid)==FAILURE) {
   WRONG_PARAM_COUNT;
 }
 convert_to_long(pid);

 ret=php3_intifxus_close_slob(pid->value.lval,&EG(regular_list)); 
 if(ret<0) {
  RETURN_FALSE;
 } 
 RETURN_TRUE;
}
/* }}} */

/* ----------------------------------------------------------------------
 * internal function
 * long php3_intifxus_close_slob(long bid, HashTable *list)
 *
 * deletes the slob-object
 *  bid: Id of Slobobject
 *  list: internal hashlist of php3
 * 
 * return -1 on error otherwise 0
 * ----------------------------------------------------------------------
*/
static long php3_intifxus_close_slob(long bid, HashTable *list) {
 IFX_IDRES *Ifx_slob;
 int type;
 
 IFXLS_FETCH();

 Ifx_slob = (IFX_IDRES *) zend_list_find(bid,&type);
 if (type!=IFXL(le_idresult) || Ifx_slob->type!=TYPE_SLOB) {
  php_error(E_WARNING,"%d is not a Informix slob-result index",
            bid);
  return -1;
 }


 if(Ifx_slob->SLOB.lofd<0) {
  php_error(E_WARNING,"slob-resource already closed");
  return -1;
 }

 if(ifx_lo_close(Ifx_slob->SLOB.lofd)<0) {
  php_error(E_WARNING,"can't close slob-resource");
  return -1;
 }

 Ifx_slob->SLOB.lofd=-1;
 return 0;
}





/* ----------------------------------------------------------------------
** int ifxus_open_slob(long bid, int mode)
**
** opens an slob-object
**  bid: existing slob-id
**  mode: 1=LO_RDONLY, 2=LO_WRONLY, 4=LO_APPEND, 8=LO_RDWR, 16=LO_BUFFER, 32=LO_NOBUFFER -> or-mask
** return false on error otherwise the new Slob-Object-id
** ----------------------------------------------------------------------
*/

/* {{{ proto int ifxus_open_slob(long bid, int mode)
   Opens an slob-object */
PHP_FUNCTION(ifxus_open_slob) {
 pval *pbid,*pmode;
 long mode,create_mode;
  
 if (ZEND_NUM_ARGS()!=2 || getParameters(ht, 1, &pbid,&pmode)==FAILURE) {
   WRONG_PARAM_COUNT;
 }
 convert_to_long(pmode);
 convert_to_long(pbid);
 mode=pmode->value.lval;
 
 create_mode=0;
 if((mode&1) !=0)   
  create_mode|=LO_RDONLY;
 if((mode&2) !=0)   
  create_mode|=LO_WRONLY;
 if((mode&4) !=0)   
  create_mode|=LO_APPEND;
 if((mode&8) !=0)   
  create_mode|=LO_RDWR;
 if((mode&16) !=0)   
  create_mode|=LO_BUFFER;
 if((mode&32) !=0)   
  create_mode|=LO_NOBUFFER;

 RETURN_LONG(php3_intifxus_open_slob(pbid->value.lval,create_mode,&EG(regular_list)));
}
/* }}} */


/* ----------------------------------------------------------------------
 * internal function
 * long php3_intifx_open_slob(long bid,long create_mode, HashTable *list)
 *
 * opens an slob-object
 *  mode: 1=LO_RDONLY, 2=LO_WRONLY, 4=LO_APPEND, 8=LO_RDWR, 16=LO_BUFFER, 32=LO_NOBUFFER -> or-mask
 *  list: internal hashlist of php3
 * return -1 on error otherwise the new Blob-Object-id
 * ----------------------------------------------------------------------
*/
static long php3_intifxus_open_slob(long bid, long create_mode, HashTable *list) {
 IFX_IDRES *Ifx_slob;
 int errcode;
 int type;
 
 IFXLS_FETCH();

 Ifx_slob = (IFX_IDRES *) zend_list_find(bid,&type);
 if (type!=IFXL(le_idresult)  || Ifx_slob->type!=TYPE_SLOB) {
  php_error(E_WARNING,"%d is not a Informix slob-result index",
            bid);
  return -1;
 }

 if(Ifx_slob->SLOB.lofd>0) {
  php_error(E_WARNING,"slob-resource already open");
  return -1;
 }

 Ifx_slob->SLOB.lofd=ifx_lo_open(&(Ifx_slob->SLOB.slob_data),create_mode,&errcode);     
 if(errcode<0 || Ifx_slob->SLOB.lofd<0) {
  php_error(E_WARNING,"can't open slob-resource");
  return -1;
 }

 return 0;
}


/* ----------------------------------------------------------------------
 * internal function
 * long php3_intifxus_new_slob(HashTable *list)
 *
 * creates an slob-object but don't open it
 *  list: internal hashlist of php3
 * return -1 on error otherwise the new slob-Object-id
 * ----------------------------------------------------------------------
*/
static long php3_intifxus_new_slob(HashTable *list) {
 IFX_IDRES *Ifx_slob;
 
 IFXLS_FETCH();

 Ifx_slob=emalloc(sizeof(IFX_IDRES));
 if(Ifx_slob==NULL) {
  php_error(E_WARNING,"can't create slob-resource");
  return -1;
 }

 Ifx_slob->type=TYPE_SLOB;
 Ifx_slob->SLOB.lofd=-1;
 Ifx_slob->SLOB.createspec=NULL;
 return zend_list_insert(Ifx_slob,IFXL(le_idresult));
}



/* ----------------------------------------------------------------------
 * internal function
 * ifx_lo_t *php3_intifxus_get_slobloc(long bid, HashTable *list)
 *
 * retuens the ifx_lo_t-structure of a slob-object
 *  list: internal hashlist of php3
 * return -1 on error otherwise the new Blob-Object-id
 * ----------------------------------------------------------------------
*/
static ifx_lo_t *php3_intifxus_get_slobloc(long bid, HashTable *list) {
 IFX_IDRES *Ifx_slob;
 int type;
 
 IFXLS_FETCH();

 Ifx_slob = (IFX_IDRES *) zend_list_find(bid,&type);
 if (type!=IFXL(le_idresult)  || Ifx_slob->type!=TYPE_SLOB) {
  php_error(E_WARNING,"%d is not a Informix slob-result index",
            bid);
  return NULL;
 }
 
 return &(Ifx_slob->SLOB.slob_data);
}



/* ----------------------------------------------------------------------
** int ifxus_tell_slob(long bid)
**
** returns the current file or seek position of an open slob-object
**  bid: existing slob-id
** return false on error otherwise the seek-position
** ----------------------------------------------------------------------
*/

/* {{{ proto int ifxus_tell_slob(long bid)
   Returns the current file or seek position of an open slob-object */
PHP_FUNCTION(ifxus_tell_slob) {
 pval *pbid;
 long bid;
 IFX_IDRES *Ifx_slob;
 ifx_int8_t akt_seek_pos;
 int type;
 long lakt_seek_pos;
 
 IFXLS_FETCH();

 
 if (ZEND_NUM_ARGS()!=1 || getParameters(ht, 1, &pbid)==FAILURE) {
   WRONG_PARAM_COUNT;
 }
 convert_to_long(pbid);
 bid=pbid->value.lval;
 Ifx_slob = (IFX_IDRES *) zend_list_find(bid,&type);
 if (type!=IFXL(le_idresult) || Ifx_slob->type!=TYPE_SLOB) {
  php_error(E_WARNING,"%d is not a Informix slob-result index",
            bid);
  RETURN_FALSE;
 }
 
 if(ifx_lo_tell(Ifx_slob->SLOB.lofd,&akt_seek_pos)<0) {
  php_error(E_WARNING,"can't perform tell-operation");
  RETURN_FALSE;
 }

 if(ifx_int8tolong(&akt_seek_pos,&lakt_seek_pos)<0) {
  php_error(E_WARNING,"seek-position to large for long");
  RETURN_FALSE;
 }
 RETURN_LONG(lakt_seek_pos);
}
/* }}} */



/* ----------------------------------------------------------------------
** int ifxus_seek_slob(long bid, int mode, long offset)
**
** sets the current file or seek position of an open slob-object
**  bid: existing slob-id
**  mode: 0=LO_SEEK_SET, 1=LO_SEEK_CUR, 2=LO_SEEK_END
**  offset: byte-offset
** return false on error otherwise the seek-position
** ----------------------------------------------------------------------
*/

/* {{{ proto int ifxus_seek_slob(long bid, int mode, long offset)
   Sets the current file or seek position of an open slob-object */
PHP_FUNCTION(ifxus_seek_slob) {
 pval *pbid, *pmode, *poffset;
 long bid,lakt_seek_pos;
 IFX_IDRES *Ifx_slob;
 ifx_int8_t akt_seek_pos,offset;
 int type,mode;
 
 
 IFXLS_FETCH();

 if (ZEND_NUM_ARGS()!=3 || getParameters(ht, 3, &pbid, &pmode, &poffset)==FAILURE) {
   WRONG_PARAM_COUNT;
 }
 convert_to_long(pbid);
 convert_to_long(pmode); 
 convert_to_long(poffset);
 
 bid=pbid->value.lval;
 Ifx_slob = (IFX_IDRES *) zend_list_find(bid,&type);
 if (type!=IFXL(le_idresult) || Ifx_slob->type!=TYPE_SLOB) {
  php_error(E_WARNING,"%d is not a Informix slob-result index",
            bid);
  RETURN_FALSE;
 }

 mode=LO_SEEK_SET;
 if(pmode->value.lval==1) {
  mode=LO_SEEK_CUR;
 }
 if(pmode->value.lval==2) {
  mode=LO_SEEK_END;
 }
 
 ifx_int8cvlong(poffset->value.lval,&offset);
 if(ifx_lo_seek(Ifx_slob->SLOB.lofd,&offset, mode,&akt_seek_pos)<0) {
  php_error(E_WARNING,"can't perform seek-operation");
  RETURN_FALSE;
 }

 if(ifx_int8tolong(&akt_seek_pos,&lakt_seek_pos)<0) {
  php_error(E_WARNING,"seek-position to large for long");
  RETURN_FALSE;
 }
 RETURN_LONG(lakt_seek_pos);
}
/* }}} */




/* ----------------------------------------------------------------------
** int ifxus_read_slob(long bid, long nbytes)
**
** reads nbytes of the slob-object
**  bid: existing slob-id
**  nbytes: bytes zu read
** return false on error otherwise the string
** ----------------------------------------------------------------------
*/

/* {{{ proto int ifxus_read_slob(long bid, long nbytes)
   Reads nbytes of the slob-object */
PHP_FUNCTION(ifxus_read_slob) {
 pval *pbid, *pnbytes;
 long bid, nbytes;
 IFX_IDRES *Ifx_slob;
 int errcode,type;
 char *buffer;
 
 
 IFXLS_FETCH();


 if (ZEND_NUM_ARGS()!=2 || getParameters(ht, 2, &pbid, &pnbytes)==FAILURE) {
   WRONG_PARAM_COUNT;
 }
 convert_to_long(pbid);
 convert_to_long(pnbytes); 
 
 bid=pbid->value.lval;
 Ifx_slob = (IFX_IDRES *) zend_list_find(bid,&type);
 if (type!=IFXL(le_idresult) || Ifx_slob->type!=TYPE_SLOB) {
  php_error(E_WARNING,"%d is not a Informix slob-result index",
            bid);
  RETURN_FALSE;
 }

 nbytes=pnbytes->value.lval;
 buffer=emalloc (nbytes);
 if(buffer==NULL) {
  php_error(E_WARNING,"cannot allocate memory");
  RETURN_FALSE;
 }
 if(ifx_lo_read(Ifx_slob->SLOB.lofd,buffer,nbytes,&errcode)<0) {
  efree(buffer);
  php_error(E_WARNING,"error during reading slob");
  RETURN_FALSE;
 }
 
 RETURN_STRINGL(buffer,nbytes,0); 
}
/* }}} */


/* ----------------------------------------------------------------------
** int ifxus_write_slob(long bid, string content)
**
** writes a string into the slob-object
**  bid: existing slob-id
**  content: content to write
** return false on error otherwise bytes written
** ----------------------------------------------------------------------
*/

/* {{{ proto int ifxus_write_slob(long bid, string content)
   Writes a string into the slob-object */
PHP_FUNCTION(ifxus_write_slob) {
 pval *pbid, *pcontent;
 long bid, nbytes;
 IFX_IDRES *Ifx_slob;
 int errcode,type;
 char *buffer;
 
 
 IFXLS_FETCH();


 if (ZEND_NUM_ARGS()!=2 || getParameters(ht, 2, &pbid, &pcontent)==FAILURE) {
   WRONG_PARAM_COUNT;
 }
 convert_to_long(pbid);
 convert_to_string(pcontent); 
 
 bid=pbid->value.lval;
 Ifx_slob = (IFX_IDRES *) zend_list_find(bid,&type);
 if (type!=IFXL(le_idresult) || Ifx_slob->type!=TYPE_SLOB) {
  php_error(E_WARNING,"%d is not a Informix slob-result index",
            bid);
  RETURN_FALSE;
 }

 buffer=pcontent->value.str.val;
 nbytes=pcontent->value.str.len;
 if(nbytes<=0) {
  php_error(E_WARNING,"string has no content");
  RETURN_FALSE;
 }
 if((nbytes=ifx_lo_write(Ifx_slob->SLOB.lofd,buffer,nbytes,&errcode))<0) {
  php_error(E_WARNING,"error during writing slob");
  RETURN_FALSE;
 }
 
 RETURN_LONG(nbytes); 
}
/* }}} */

$endif;


#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */
