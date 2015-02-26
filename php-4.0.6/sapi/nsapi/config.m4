dnl ## $Id: config.m4,v 1.9 2001/04/03 20:59:49 wsanchez Exp $ -*- sh -*-

AC_MSG_CHECKING(for NSAPI support)
AC_ARG_WITH(nsapi,
[  --with-nsapi=DIR        Specify path to the installed Netscape],[
  PHP_NSAPI=$withval
],[
  PHP_NSAPI=no
])
AC_MSG_RESULT($PHP_NSAPI)

if test "$PHP_NSAPI" != "no"; then
  if test ! -d $PHP_NSAPI/bin ; then
    AC_MSG_ERROR(Please specify the path to the root of your Netscape server using --with-nsapi=DIR)
  fi
  AC_MSG_CHECKING(for NSAPI include files)
  if test -d $PHP_NSAPI/include ; then
    NSAPI_INCLUDE=$PHP_NSAPI/include
    AC_MSG_RESULT(Netscape-Enterprise/3.x style)
  elif test -d $PHP_NSAPI/plugins/include ; then
    NSAPI_INCLUDE=$PHP_NSAPI/plugins/include
    AC_MSG_RESULT(iPlanet/4.x style)
  else
    AC_MSG_ERROR(Please check you have nsapi.h in either DIR/include or DIR/plugins/include)
  fi
  PHP_ADD_INCLUDE($NSAPI_INCLUDE)
  PHP_BUILD_THREAD_SAFE
  AC_DEFINE(HAVE_NSAPI,1,[Whether you have a Netscape Server])
  PHP_SAPI=nsapi
  PHP_BUILD_SHARED
  INSTALL_IT="\$(INSTALL) -m 0755 $SAPI_SHARED \$(INSTALL_ROOT)$PHP_NSAPI/bin/"
fi


dnl ## Local Variables:
dnl ## tab-width: 4
dnl ## End:
