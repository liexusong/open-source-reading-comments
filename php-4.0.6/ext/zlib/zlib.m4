dnl
dnl $Id: zlib.m4,v 1.1.2.2 2001/05/20 23:17:49 sniper Exp $
dnl

PHP_ARG_WITH(zlib-dir,if the location of ZLIB install directory is defined,
[  --with-zlib-dir=<DIR>   Define the location of zlib install directory])

PHP_ARG_WITH(zlib,whether to include ZLIB support,
[  --with-zlib[=DIR]       Include zlib support (requires zlib >= 1.0.9).
                          DIR is the zlib install directory.])

if test "$PHP_ZLIB" != "no" -o "$PHP_ZLIB_DIR" != "no"; then
  PHP_EXTENSION(zlib, $ext_shared)
  PHP_SUBST(ZLIB_SHARED_LIBADD)
  
  if test "$PHP_ZLIB" != "yes" -a "$PHP_ZLIB" != "no"; then 
    if test -f $PHP_ZLIB/include/zlib/zlib.h; then
      ZLIB_DIR=$PHP_ZLIB
      ZLIB_INCDIR=$ZLIB_DIR/include/zlib
    elif test -f $PHP_ZLIB/include/zlib.h; then
      ZLIB_DIR=$PHP_ZLIB
      ZLIB_INCDIR=$ZLIB_DIR/include
    fi
  else 
    for i in /usr/local /usr $PHP_ZLIB_DIR; do
      if test -f $i/include/zlib/zlib.h; then
        ZLIB_DIR=$i
        ZLIB_INCDIR=$i/include/zlib
      elif test -f $i/include/zlib.h; then
        ZLIB_DIR=$i
        ZLIB_INCDIR=$i/include
      fi
    done
  fi
  
  if test -z "$ZLIB_DIR"; then
    AC_MSG_ERROR(Cannot find libz)
  fi

  PHP_ADD_LIBPATH($ZLIB_DIR/lib, ZLIB_SHARED_LIBADD)

  AC_CHECK_LIB(z, gzgets, [
    AC_DEFINE(HAVE_ZLIB,1,[ ]) 
  ],[
    AC_MSG_ERROR(ZLIB extension requires zlib >= 1.0.9)
  ])

  PHP_ZLIB_DIR=$ZLIB_DIR
  PHP_ADD_LIBRARY(z,, ZLIB_SHARED_LIBADD)
  PHP_ADD_INCLUDE($ZLIB_INCDIR)

  PHP_FOPENCOOKIE
fi
