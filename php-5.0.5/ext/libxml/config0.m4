dnl
dnl $Id: config0.m4,v 1.1 2003/11/12 23:42:38 sniper Exp $
dnl

PHP_ARG_ENABLE(libxml, whether to enable LIBXML support,
[  --disable-libxml        Disable new LIBXML support.], yes)

if test -z "$PHP_LIBXML_DIR"; then
  PHP_ARG_WITH(libxml-dir, libxml2 install dir,
  [  --with-libxml-dir[=DIR]   libxml2 install prefix.], no, no)
fi

if test "$PHP_LIBXML" != "no"; then

  dnl This extension can not be build as shared
  ext_shared=no

  PHP_SETUP_LIBXML(LIBXML_SHARED_LIBADD, [
    AC_DEFINE(HAVE_LIBXML,1,[ ])
    PHP_NEW_EXTENSION(libxml, [libxml.c], $ext_shared)
  ], [
    AC_MSG_ERROR([xml2-config not found. Please check your libxml2 installation.])
  ])
fi
