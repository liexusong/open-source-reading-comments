dnl $Id: config.m4,v 1.5.2.1 2001/05/12 09:29:06 sas Exp $
dnl config.m4 for extension libming

PHP_ARG_WITH(ming, whether to include ming support,
[  --with-ming[=DIR]       Include ming support])

if test "$PHP_MING" != "no"; then
  for i in $PHP_MING /usr/local /usr; do
    if test -r $i/lib/libming.so; then
      MING_DIR=$i
    fi
  done

  if test -z "$MING_DIR"; then
    AC_MSG_ERROR(Please reinstall libming.so - I cannot find libming.so)
  fi

  PHP_ADD_INCLUDE($MING_DIR/include)

  PHP_SUBST(MING_SHARED_LIBADD)
  PHP_ADD_LIBRARY_WITH_PATH(ming, $MING_DIR/lib, MING_SHARED_LIBADD)

  AC_CHECK_LIB(ming, Ming_setScale, [
    AC_DEFINE(HAVE_MING,1,[ ])
  ],[
    AC_MSG_ERROR(Ming library 0.1.0 or greater required.)
  ])

  PHP_EXTENSION(ming, $ext_shared)
fi
