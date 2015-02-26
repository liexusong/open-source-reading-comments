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
   | Author: David Eriksson <david@2good.com>                            |
   +----------------------------------------------------------------------+
 */

/*
 * $Id: corba.h,v 1.3 2001/02/26 06:07:15 andi Exp $
 * vim: syntax=c tabstop=2 shiftwidth=2
 */

/*
 * Control access to CORBA_ORB and CORBA_Environment objects
 */
#ifndef __corba_h__
#define __corba_h__

#include <orb/orbit.h>

/* corba handling */
CORBA_boolean orbit_corba_init();
CORBA_boolean orbit_corba_shutdown();
CORBA_ORB orbit_get_orb();
CORBA_Environment * orbit_get_environment();

/* exception handling */
CORBA_boolean orbit_caught_exception();
#define orbit_error_test(info)	orbit_caught_exception()

#endif /* __corba_h__ */

