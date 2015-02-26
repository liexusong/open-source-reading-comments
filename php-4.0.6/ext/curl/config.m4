dnl $Id: config.m4,v 1.5.2.7 2001/05/31 03:58:47 sterling Exp $
dnl config.m4 for extension CURL

PHP_ARG_WITH(curl, for CURL support,
[  --with-curl[=DIR]       Include CURL support])

if test "$PHP_CURL" != "no"; then
  if test -r $PHP_CURL/include/curl/easy.h; then
    CURL_DIR=$PHP_CURL
  else
    AC_MSG_CHECKING(for CURL in default path)
    for i in /usr/local /usr; do
      if test -r $i/include/curl/easy.h; then
        CURL_DIR=$i
        AC_MSG_RESULT(found in $i)
      fi
    done
  fi

  if test -z "$CURL_DIR"; then
    AC_MSG_RESULT(not found)
    AC_MSG_ERROR(Please reinstall the libcurl distribution -
    easy.h should be in <curl-dir>/include/curl/)
  fi

  CURL_CONFIG="curl-config"
  AC_MSG_CHECKING(for cURL greater than or equal to 7.8)

  if ${CURL_DIR}/bin/curl-config --libs print > /dev/null 2>&1; then
    CURL_CONFIG=${CURL_DIR}/bin/curl-config
  fi

  curl_version_full=`$CURL_CONFIG --version`
  curl_version=`$CURL_CONFIG --vernum 2>/dev/null`
  if test "$curl_version" -ge 70800; then
    AC_MSG_RESULT($curl_version_full)
    CURL_LIBS=`$CURL_CONFIG --libs`
  else
    AC_MSG_ERROR(cURL version 7.8 or later is required to compile php with cURL support)
  fi

  PHP_ADD_INCLUDE($CURL_DIR/include)
  PHP_EVAL_LIBLINE($CURL_LIBS, CURL_SHARED_LIBADD)
  PHP_ADD_LIBRARY_WITH_PATH(curl, $CURL_DIR/lib, CURL_SHARED_LIBADD)

  AC_CHECK_LIB(curl,curl_easy_perform, 
  [ 
    AC_DEFINE(HAVE_CURL,1,[ ])
  ],[
    AC_MSG_ERROR(There is something wrong. Please check config.log for more information.)
  ])

  PHP_EXTENSION(curl, $ext_shared)
  PHP_SUBST(CURL_SHARED_LIBADD)
fi
