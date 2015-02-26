dnl $Id: config.m4,v 1.2.2.1 2001/05/21 16:19:34 sniper Exp $
dnl config.m4 for extension iconv

PHP_ARG_WITH(iconv, for iconv support,
[  --with-iconv[=DIR]      Include iconv support])

if test "$PHP_ICONV" != "no"; then

  if test -r $PHP_ICONV/include/iconv.h; then
    ICONV_DIR=$PHP_ICONV
  else
    AC_MSG_CHECKING(for iconv in default path)
    for i in /usr/local /usr; do
      if test -r $i/include/iconv.h; then
        ICONV_DIR=$i
        AC_MSG_RESULT(found in $i)
      fi
    done
  fi

  if test -z "$ICONV_DIR"; then
    AC_MSG_RESULT(not found)
    AC_MSG_ERROR(Please reinstall the iconv library)
  fi
  
  PHP_ADD_INCLUDE($ICONV_DIR/include)

  PHP_SUBST(ICONV_SHARED_LIBADD)

  if test -f $ICONV_DIR/lib/libconv.a -o -f $ICONV_DIR/lib/libiconv.so ; then
    PHP_ADD_LIBRARY_WITH_PATH(iconv, $ICONV_DIR/lib, ICONV_SHARED_LIBADD)
    AC_CHECK_LIB(iconv, iconv_open, AC_DEFINE(HAVE_ICONV, 1, [ ]))
  else
    AC_CHECK_LIB(c, iconv_open, AC_DEFINE(HAVE_ICONV, 1, [ ]))
  fi

  PHP_EXTENSION(iconv, $ext_shared)
fi
