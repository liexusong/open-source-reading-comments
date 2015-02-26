dnl $Id: config.m4,v 1.5 2001/03/27 20:34:25 sniper Exp $

PHP_ARG_WITH(fbsql, for FrontBase SQL92 (fbsql) support,
[  --with-fbsql[=DIR]      Include FrontBase support. DIR is the FrontBase base
                          directory.])

if test "$PHP_FBSQL" != "no"; then

  AC_DEFINE(HAVE_FBSQL, 1, [Whether you have FrontBase])
  PHP_EXTENSION(fbsql,$ext_shared)

  FBSQL_INSTALLATION_DIR=""
  if test "$PHP_FBSQL" = "yes"; then

    for i in /Local/Library /usr /usr/local /opt /Library; do
      if test -f $i/FrontBase/include/FBCAccess/FBCAccess.h; then
        FBSQL_INSTALLATION_DIR="$i/FrontBase"
        break
      fi
    done

    if test -z "$FBSQL_INSTALLATION_DIR"; then
      AC_MSG_ERROR(Cannot find FrontBase in well know installation directories)
    fi

  elif test "$PHP_FBSQL" != "no"; then

    if test -f $PHP_FBSQL/include/FBCAccess/FBCAccess.h; then
      FBSQL_INSTALLATION_DIR=$PHP_FBSQL
    else
      AC_MSG_ERROR(Directory $PHP_FBSQL is not a FrontBase installation directory)
    fi
  fi  

  if test -z "$FBSQL_INSTALLATION_DIR/lib/libFBCAccess.a"; then
     AC_MSG_ERROR(Could not find $FBSQL_INSTALLATION_DIR/lib/libFBCAccess.a)
  fi

  PHP_ADD_LIBRARY_WITH_PATH(FBCAccess, $FBSQL_INSTALLATION_DIR/lib, $FBSQL_INSTALLATION_DIR/lib)
  PHP_ADD_INCLUDE($FBSQL_INSTALLATION_DIR/include)
fi
