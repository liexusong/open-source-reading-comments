dnl $Id: config.m4,v 1.7.2.1 2005/04/27 13:13:38 sniper Exp $
dnl config.m4 for extension simplexml

PHP_ARG_ENABLE(simplexml, whether to enable SimpleXML support,
[  --disable-simplexml     Disable SimpleXML support], yes)

if test -z "$PHP_LIBXML_DIR"; then
  PHP_ARG_WITH(libxml-dir, libxml2 install dir,
  [  --with-libxml-dir=DIR     SimpleXML: libxml2 install prefix], no, no)
fi

if test "$PHP_SIMPLEXML" != "no"; then

  if test "$PHP_LIBXML" = "no"; then   
    AC_MSG_ERROR([SimpleXML extension requires LIBXML extension, add --enable-libxml])                
  fi

  PHP_SETUP_LIBXML(SIMPLEXML_SHARED_LIBADD, [
    AC_DEFINE(HAVE_SIMPLEXML,1,[ ])
    PHP_NEW_EXTENSION(simplexml, simplexml.c, $ext_shared)
    PHP_SUBST(SIMPLEXML_SHARED_LIBADD)
  ], [
    AC_MSG_ERROR([xml2-config not found. Please check your libxml2 installation.])
  ])
fi
