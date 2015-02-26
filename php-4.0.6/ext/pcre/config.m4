dnl $Id: config.m4,v 1.23 2001/03/27 20:34:38 sniper Exp $
dnl config.m4 for extension pcre

dnl By default we'll compile and link against the bundled PCRE library
dnl if DIR is supplied, we'll use that for linking

PHP_ARG_WITH(pcre-regex,whether to include PCRE support,
[  --without-pcre-regex    Do not include Perl Compatible Regular Expressions 
                          support. Use --with-pcre-regex=DIR to specify DIR
                          where PCRE's include and library files are located,
                          if not using bundled library.],yes)

if test "$PHP_PCRE_REGEX" != "no"; then
  PHP_EXTENSION(pcre, $ext_shared)
  if test "$PHP_PCRE_REGEX" = "yes"; then
    PCRE_LIBADD=pcrelib/libpcre.la
    PCRE_SHARED_LIBADD=pcrelib/libpcre.la
    PCRE_SUBDIRS=pcrelib
    CPPFLAGS="$CPPFLAGS -DSUPPORT_UTF8"
    PHP_SUBST(PCRE_LIBADD)
    PHP_SUBST(PCRE_SUBDIRS)
    AC_DEFINE(HAVE_BUNDLED_PCRE, 1, [ ])
    PHP_FAST_OUTPUT($ext_builddir/pcrelib/Makefile)
    LIB_BUILD($ext_builddir/pcrelib,$ext_shared,yes)
  else
    test -f $PHP_PCRE_REGEX/pcre.h && PCRE_INCDIR=$PHP_PCRE_REGEX
    test -f $PHP_PCRE_REGEX/include/pcre.h && PCRE_INCDIR=$PHP_PCRE_REGEX/include
    
    if test -z "$PCRE_INCDIR"; then
      AC_MSG_RESULT(Could not find pcre.h in $PHP_PCRE_REGEX)
    fi

    changequote({,})
    pcre_major=`grep PCRE_MAJOR $PCRE_INCDIR/pcre.h | sed -e 's/[^0-9]//g'`
    pcre_minor=`grep PCRE_MINOR $PCRE_INCDIR/pcre.h | sed -e 's/[^0-9]//g'`
    changequote([,])
    pcre_minor_length=`echo "$pcre_minor" | wc -c | sed -e 's/[^0-9]//g'`
    if test "$pcre_minor_length" -eq 2 ; then
      pcre_minor="$pcre_minor"0
    fi
    pcre_version=$pcre_major$pcre_minor
    if test "$pcre_version" -lt 208; then
      AC_MSG_ERROR(The PCRE extension requires PCRE library version >= 2.08)
    fi

    test -f $PHP_PCRE_REGEX/libpcre.a && PCRE_LIBDIR="$PHP_PCRE_REGEX"
    test -f $PHP_PCRE_REGEX/lib/libpcre.a && PCRE_LIBDIR="$PHP_PCRE_REGEX/lib"

    if test -z "$PCRE_LIBDIR" ; then
      AC_MSG_ERROR(Could not find libpcre.a in $PHP_PCRE_REGEX)
    fi

    PHP_ADD_LIBRARY_WITH_PATH(pcre, $PCRE_LIBDIR, PCRE_SHARED_LIBADD)
    
    PHP_ADD_INCLUDE($PCRE_INCDIR)
    AC_DEFINE(HAVE_PCRE, 1, [ ])
  fi
fi
PHP_SUBST(PCRE_SHARED_LIBADD)


AC_CHECK_FUNC(memmove, [], [AC_DEFINE(USE_BCOPY, 1, [ ])])
