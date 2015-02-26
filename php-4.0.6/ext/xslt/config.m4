dnl config.m4 for extension xslt
dnl +------------------------------------------------------------------------------+
dnl |  This is where the magic of the extension reallly is.  Depending on what     |
dnl |  backend the user chooses, this script performs the magic                    |
dnl +------------------------------------------------------------------------------+
dnl   $Id: config.m4,v 1.4.2.1 2001/06/01 06:43:09 sniper Exp $

PHP_ARG_ENABLE(xslt, whether to enable xslt support,
[  --enable-xslt           Enable xslt support])

PHP_ARG_WITH(xslt-sablot, whether to enable the XSLT Sablotron backend,
[  --with-xslt-sablot      XSLT: Enable the sablotron backend])

PHP_ARG_WITH(expat-dir, libexpat dir for Sablotron XSL support,
[  --with-expat-dir=DIR    Sablotron: libexpat dir for Sablotron 0.50])

if test "$PHP_XSLT" != "no"; then

  PHP_EXTENSION(xslt, $ext_shared)
  PHP_SUBST(XSLT_SHARED_LIBADD)

  if test "$PHP_XSLT_SABLOT" != "no"; then
    XSLT_CHECK_DIR=$PHP_XSLT_SABLOT
    XSLT_TEST_FILE=/include/sablot.h
    XSLT_BACKEND_NAME=Sablotron
    XSLT_LIBNAME=sablot
  fi

  condition="$XSLT_CHECK_DIR$XSLT_TEST_FILE"

  if test -r $condition; then
    XSLT_DIR=$XSLT_CHECK_DIR
  else
    AC_MSG_CHECKING(for $XSLT_BACKEND_NAME libraries in the default path)
    for i in /usr /usr/local; do
      condition="$i$XSLT_TEST_FILE"
      if test -r $condition; then
        XSLT_DIR=$i
        AC_MSG_RESULT(found $XSLT_BACKEND_NAME in $i)
      fi
    done
  fi

  if test -z "$XSLT_DIR"; then
    AC_MSG_ERROR(not found. Please re-install the $XSLT_BACKEND_NAME distribution)
  fi
					
  PHP_ADD_INCLUDE($XSLT_DIR/include)
  PHP_ADD_LIBRARY_WITH_PATH($XSLT_LIBNAME, $XSLT_DIR/lib, XSLT_SHARED_LIBADD)

  if test "$PHP_XSLT_SABLOT" != "no"; then
    found_expat=no
    for i in $PHP_EXPAT_DIR $XSLT_DIR; do
      if test -f $i/lib/libexpat.a -o -f $i/lib/libexpat.so; then
        AC_DEFINE(HAVE_LIBEXPAT2, 1, [ ])
        PHP_ADD_INCLUDE($i/include)
        PHP_ADD_LIBRARY_WITH_PATH(expat, $i/lib)
        found_expat=yes
      fi
    done

    if test "$found_expat" = "no"; then
      PHP_ADD_LIBRARY(xmlparse)
      PHP_ADD_LIBRARY(xmltok)
    fi

    found_iconv=no
    AC_CHECK_LIB(c, iconv_open, found_iconv=yes)
    if test "$found_iconv" = "no"; then
      if test "$PHP_ICONV" = "no"; then
        for i in /usr /usr/local; do
          if test -f $i/lib/libconv.a -o -f $i/lib/libiconv.so; then
            PHP_ADD_LIBRARY_WITH_PATH(iconv, $i/lib)
            found_iconv=yes
          fi
        done
      fi
    fi

    if test "$found_iconv" = "no"; then
      AC_MSG_ERROR(iconv not found, in order to build sablotron you need the iconv library)
    fi
 
    AC_DEFINE(HAVE_SABLOT_BACKEND, 1, [ ])
    AC_CHECK_LIB(sablot, SablotSetEncoding, AC_DEFINE(HAVE_SABLOT_SET_ENCODING, 1, [ ]))
  fi

  AC_DEFINE(HAVE_XSLT, 1, [ ])
fi
