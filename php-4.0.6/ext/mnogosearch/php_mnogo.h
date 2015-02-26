/* $Source: /repository/php4/ext/mnogosearch/php_mnogo.h,v $ */
/* $Id: php_mnogo.h,v 1.9 2001/04/30 11:11:18 gluke Exp $ */

/* 
   +----------------------------------------------------------------------+
   | PHP version 4.0                                                      |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997, 1998, 1999, 2000, 2001 The PHP Group             |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.02 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available at through the world-wide-web at                           |
   | http://www.php.net/license/2_02.txt.                                 |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: 								  |
   |  Initial version     by  Alex Barkov <bar@izhcom.ru>                 |
   |                      and Ramil Kalimullin <ram@izhcom.ru>            |
   |  Further development by  Sergey Kartashoff <gluke@biosys.net>        |
   +----------------------------------------------------------------------+
 */

#ifndef _PHP_MNOGO_H
#define _PHP_MNOGO_H

#if HAVE_MNOGOSEARCH

#include <udm_config.h>
#include <udmsearch.h>

extern zend_module_entry mnogosearch_module_entry;
#define mnogosearch_module_ptr &mnogosearch_module_entry

#ifdef PHP_WIN32                            
#define PHP_MNOGO_API __declspec(dllexport)  
#else                                       
#define PHP_MNOGO_API                        
#endif                                      

#ifdef ZTS       
#include "TSRM.h"
#endif           

/* mnoGoSearch functions */
DLEXPORT PHP_MINIT_FUNCTION(mnogosearch);
DLEXPORT PHP_RINIT_FUNCTION(mnogosearch);
DLEXPORT PHP_MSHUTDOWN_FUNCTION(mnogosearch);
DLEXPORT PHP_MINFO_FUNCTION(mnogosearch);

DLEXPORT PHP_FUNCTION(udm_api_version);

DLEXPORT PHP_FUNCTION(udm_alloc_agent);
DLEXPORT PHP_FUNCTION(udm_set_agent_param);

DLEXPORT PHP_FUNCTION(udm_load_ispell_data);
DLEXPORT PHP_FUNCTION(udm_free_ispell_data);

DLEXPORT PHP_FUNCTION(udm_add_search_limit);
DLEXPORT PHP_FUNCTION(udm_clear_search_limits);

DLEXPORT PHP_FUNCTION(udm_error);
DLEXPORT PHP_FUNCTION(udm_errno);

DLEXPORT PHP_FUNCTION(udm_find);
DLEXPORT PHP_FUNCTION(udm_get_res_field);
DLEXPORT PHP_FUNCTION(udm_get_res_param);

DLEXPORT PHP_FUNCTION(udm_cat_list);
DLEXPORT PHP_FUNCTION(udm_cat_path);

DLEXPORT PHP_FUNCTION(udm_free_res);
DLEXPORT PHP_FUNCTION(udm_free_agent);

#if UDM_VERSION_ID > 30110
DLEXPORT PHP_FUNCTION(udm_get_doc_count);
#endif

#else

#define mnogosearch_module_ptr NULL

#endif

#define phpext_mnogosearch_ptr mnogosearch_module_ptr

#endif /* _PHP_MNOGO_H */
