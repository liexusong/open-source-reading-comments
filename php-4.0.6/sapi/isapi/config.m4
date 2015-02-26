dnl ## $Id: config.m4,v 1.11.2.1 2001/05/12 09:29:09 sas Exp $ -*- sh -*-

RESULT=no
AC_MSG_CHECKING(for Zeus ISAPI support)
AC_ARG_WITH(isapi,
[  --with-isapi=DIR        Build PHP as an ISAPI module for use with Zeus.],
[
	if test "$withval" = "yes"; then
		ZEUSPATH=/usr/local/zeus # the default
	else
		ZEUSPATH=$withval
	fi
	test -f "$ZEUSPATH/web/include/httpext.h" || AC_MSG_ERROR(Unable to find httpext.h in $ZEUSPATH/web/include)
	PHP_BUILD_THREAD_SAFE
	AC_DEFINE(WITH_ZEUS,1,[ ])
	PHP_ADD_INCLUDE($ZEUSPATH/web/include)
	PHP_SAPI=isapi
	PHP_BUILD_SHARED
	INSTALL_IT="\$(SHELL) \$(srcdir)/install-sh -m 0755 $SAPI_SHARED \$(INSTALL_ROOT)$ZEUSPATH/web/bin/"
	RESULT=yes
])
AC_MSG_RESULT($RESULT)

dnl ## Local Variables:
dnl ## tab-width: 4
dnl ## End:
