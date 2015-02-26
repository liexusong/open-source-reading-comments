dnl $Id: config.m4,v 1.6 2001/03/27 20:34:27 sniper Exp $
dnl config.m4 for extension gettext
dnl don't forget to call PHP_EXTENSION(gettext)

PHP_ARG_WITH(gettext,whether to include GNU gettext support,
[  --with-gettext[=DIR]    Include GNU gettext support.  DIR is the gettext
                          install directory, defaults to /usr/local])

if test "$PHP_GETTEXT" != "no"; then
  for i in /usr /usr/local $PHP_GETTEXT; do
    if test -r $i/include/libintl.h; then
	  GETTEXT_DIR=$i
    fi
  done

  if test -z "$GETTEXT_DIR"; then
    AC_MSG_ERROR(Cannot locate header file libintl.h)
  fi

  GETTEXT_LIBDIR=$GETTEXT_DIR/lib
  GETTEXT_INCDIR=$GETTEXT_DIR/include
  
  O_LDFLAGS="$LDFLAGS"
  LDFLAGS="$LDFLAGS -L$GETTEXT_LIBDIR"
  AC_CHECK_LIB(intl, bindtextdomain, GETTEXT_LIBS="intl",[
      AC_CHECK_LIB(c, bindtextdomain, GETTEXT_LIBS= ,[
          AC_MSG_ERROR(Unable to find required gettext library)
      ])
  ])
  LDFLAGS="$O_LDFLAGS"

  AC_DEFINE(HAVE_LIBINTL,1,[ ])
  PHP_EXTENSION(gettext, $ext_shared)
  PHP_SUBST(GETTEXT_SHARED_LIBADD)

  if test -n "$GETTEXT_LIBS"; then
    PHP_ADD_LIBRARY_WITH_PATH($GETTEXT_LIBS, $GETTEXT_LIBDIR, GETTEXT_SHARED_LIBADD)
  fi

  PHP_ADD_INCLUDE($GETTEXT_INCDIR)
fi
