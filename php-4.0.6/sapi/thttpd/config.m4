AC_ARG_WITH(thttpd,
[  --with-thttpd=SRCDIR    Build PHP as thttpd module],[
  test -d $withval || AC_MSG_RESULT(thttpd directory does not exist ($withval))
  PHP_EXPAND_PATH($withval, THTTPD)
  INSTALL_IT="\
    echo 'PHP_LIBS = -L. -lphp4 \$(PHP_LIBS) \$(EXTRA_LIBS)' > $THTTPD/php_makefile; \
    echo 'PHP_LDFLAGS = \$(NATIVE_RPATHS) \$(PHP_LDFLAGS)' >> $THTTPD/php_makefile; \
    echo 'PHP_CFLAGS = \$(COMMON_FLAGS) \$(CFLAGS) \$(CPPFLAGS) \$(EXTRA_CFLAGS)' >> $THTTPD/php_makefile; \
    \$(LN_S) $abs_srcdir/sapi/thttpd/thttpd.c $THTTPD/php_thttpd.c; \
    cp $abs_srcdir/sapi/thttpd/php_thttpd.h $SAPI_STATIC $THTTPD;\
    test -f $THTTPD/php_patched || \
    (cd $THTTPD && patch < $abs_srcdir/sapi/thttpd/thttpd_patch && touch php_patched)"
  PHP_THTTPD="yes, using $THTTPD"
  PHP_ADD_INCLUDE($THTTPD)
  PHP_BUILD_STATIC
  PHP_SAPI=thttpd
],[
  PHP_THTTPD="no"
])

AC_MSG_CHECKING(for thttpd)
AC_MSG_RESULT($PHP_THTTPD)
