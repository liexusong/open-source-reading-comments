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
   | Author: Edin Kadribasic                                                             |
   +----------------------------------------------------------------------+
 */

/* $Id: md5crypt.h,v 1.4 2004/01/08 17:33:28 sniper Exp $ */
#ifndef _MD5CRYPT_H_
#define _MD5CRYPT_H_

#ifdef __cplusplus
extern "C"
{
#endif

#define MD5_MAGIC	"$1$"
#define MD5_MAGIC_LEN	3

char	*md5_crypt(const char *pw, const char *salt);

#ifdef __cplusplus
}
#endif

#endif /* _MD5CRYPT_H_ */