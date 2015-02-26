# $Source: /repository/php4/ext/xml/config.m4,v $
# $Id: config.m4,v 1.25.2.2 2001/06/01 21:09:52 sas Exp $

dnl Fallback for --with-xml[=DIR]
AC_ARG_WITH(xml,[],enable_xml=$withval)

AC_C_BIGENDIAN

if test "$ac_cv_c_bigendian" = "yes"; then
  order=21
else
  order=12
fi

PHP_ARG_ENABLE(xml,for XML support,
[  --disable-xml           Disable XML support using bundled expat lib], yes)

PHP_ARG_WITH(expat-dir, external libexpat install dir,
[  --with-expat-dir=DIR    XML: external libexpat install dir])

if test "$PHP_XML" = "yes"; then
if test "$PHP_EXPAT_DIR" = "no"; then

  PHP_EXTENSION(xml, $ext_shared)
  PHP_SUBST(EXPAT_SHARED_LIBADD)
  AC_DEFINE(HAVE_LIBEXPAT, 1, [ ])

  CPPFLAGS="$CPPFLAGS -DXML_BYTE_ORDER=$order"
  EXPAT_INTERNAL_LIBADD="expat/libexpat.la"	    
  PHP_SUBST(EXPAT_INTERNAL_LIBADD)
  EXPAT_SUBDIRS="expat"	    
  PHP_SUBST(EXPAT_SUBDIRS)
  LIB_BUILD($ext_builddir/expat,$ext_shared,yes)
  LIB_BUILD($ext_builddir/expat/xmlparse,$ext_shared,yes)
  LIB_BUILD($ext_builddir/expat/xmltok,$ext_shared,yes)
  PHP_ADD_INCLUDE($ext_srcdir/expat/xmltok)
  PHP_ADD_INCLUDE($ext_srcdir/expat/xmlparse)
  PHP_FAST_OUTPUT($ext_builddir/expat/Makefile $ext_builddir/expat/xmlparse/Makefile $ext_builddir/expat/xmltok/Makefile)

else
  
  PHP_EXTENSION(xml, $ext_shared)
  PHP_SUBST(EXPAT_SHARED_LIBADD)
  AC_DEFINE(HAVE_LIBEXPAT,  1, [ ])
  AC_DEFINE(HAVE_LIBEXPAT2, 1, [Whether external expat libs are used])

  for i in $PHP_XML $PHP_EXPAT_DIR; do
    if test -f $i/lib/libexpat.a -o -f $i/lib/libexpat.s? ; then
      EXPAT_DIR=$i
    fi
  done

  if test -z "$EXPAT_DIR"; then
    AC_MSG_ERROR(not found. Please reinstall the expat distribution.)
  fi

  PHP_ADD_INCLUDE($EXPAT_DIR/include)
  PHP_ADD_LIBRARY_WITH_PATH(expat, $EXPAT_DIR/lib, EXPAT_SHARED_LIBADD)
fi
fi
