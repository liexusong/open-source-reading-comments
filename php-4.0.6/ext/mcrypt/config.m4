dnl
dnl $Id: config.m4,v 1.17.2.1 2001/05/12 09:29:06 sas Exp $
dnl 

PHP_ARG_WITH(mcrypt, for mcrypt support,
[  --with-mcrypt[=DIR]     Include mcrypt support. DIR is the mcrypt install 
                          directory.])

if test "$PHP_MCRYPT" != "no"; then
  for i in /usr/local /usr $PHP_MCRYPT; do
    if test -f $i/include/mcrypt.h; then
      MCRYPT_DIR=$i
    fi
  done

  if test -z "$MCRYPT_DIR"; then
    AC_MSG_ERROR(mcrypt.h not found. Please reinstall libmcrypt.)
  fi

  AC_CHECK_LIB(mcrypt, mcrypt_module_open, 
  [
    PHP_ADD_LIBRARY(ltdl,, MCRYPT_SHARED_LIBADD)
    AC_DEFINE(HAVE_LIBMCRYPT24,1,[ ])
  ],[
    AC_CHECK_LIB(mcrypt, init_mcrypt, 
    [
      AC_DEFINE(HAVE_LIBMCRYPT22,1,[ ])
    ],[
      AC_MSG_ERROR(Sorry, I was not able to diagnose which libmcrypt version you have installed.)
    ],[
      -L$MCRYPT_DIR/lib
    ])
  ],[
    -L$MCRYPT_DIR/lib -lltdl
  ])

  PHP_ADD_LIBRARY_WITH_PATH(mcrypt, $MCRYPT_DIR/lib, MCRYPT_SHARED_LIBADD)
  PHP_ADD_INCLUDE($MCRYPT_DIR/include)
  AC_DEFINE(HAVE_LIBMCRYPT,1,[ ])

  PHP_SUBST(MCRYPT_SHARED_LIBADD)
  PHP_EXTENSION(mcrypt, $ext_shared)
fi
