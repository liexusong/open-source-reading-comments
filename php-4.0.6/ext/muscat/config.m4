dnl $Id: config.m4,v 1.3 2001/03/27 20:34:33 sniper Exp $

PHP_ARG_WITH(muscat, for Muscat support,
[  --with-muscat[=DIR]     Include muscat support.])

if test "$PHP_MUSCAT" != "no"; then
  PHP_EXTENSION(muscat, $ext_shared)
  for i in $PHP_MUSCAT /usr/local /usr /usr/local/empower /usr/local/empower/developer; do
    if test -f $i/include/Muscat.h; then
      MUSCAT_INCDIR=$i/include
      MUSCAT_DIR=$i
    fi
    if test -f $i/libs/libhdmuscat3.6.a; then
      MUSCAT_LIBDIR=$i/libs
      MUSCAT_DIR=$i
    elif test -f $i/lib/libhdmuscat3.6.a; then
      MUSCAT_LIBDIR=$i/lib
      MUSCAT_DIR=$i
    fi
  done

  if test -z "$MUSCAT_INCDIR"; then
    AC_MSG_ERROR(Cannot find Muscat include dir)
  fi

  if test -z "$MUSCAT_LIBDIR"; then
    AC_MSG_ERROR(Cannot find Muscat lib dir)
  fi

  AC_DEFINE(HAVE_MUSCAT, 1, [ ])
  PHP_SUBST(MUSCAT_SHARED_LIBADD)
  AC_DEFINE_UNQUOTED(PHP_MUSCAT_DIR, "$MUSCAT_DIR", [ ])
  PHP_ADD_LIBRARY_WITH_PATH(hdmuscat3.6, $MUSCAT_LIBDIR, MUSCAT_SHARED_LIBADD)
  PHP_ADD_INCLUDE($MUSCAT_INCDIR)
fi
