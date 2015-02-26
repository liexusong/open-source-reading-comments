dnl $Id: config.m4,v 1.10 2001/03/27 20:34:42 sniper Exp $

PHP_ARG_WITH(mm,for mm support,
[  --with-mm[=DIR]         Include mm support for session storage])

PHP_ARG_ENABLE(trans-sid,whether to enable transparent session id propagation,
[  --enable-trans-sid      Enable transparent session id propagation])

PHP_ARG_ENABLE(session, whether to enable session support,
[  --disable-session       Disable session support], yes)

if test "$PHP_MM" != "no"; then
  for i in /usr/local /usr $PHP_MM; do
    if test -f "$i/include/mm.h"; then
      MM_DIR="$i"
    fi
  done

  if test -z "$MM_DIR" ; then
    AC_MSG_ERROR(cannot find mm library)
  fi
  
  PHP_ADD_LIBRARY_WITH_PATH(mm, $MM_DIR/lib, SESSION_SHARED_LIBADD)
  PHP_ADD_INCLUDE($MM_DIR/include)
  AC_DEFINE(HAVE_LIBMM, 1, [Whether you have libmm])
  PHP_MODULE_PTR(phpext_ps_mm_ptr)
fi

if test "$PHP_TRANS_SID" = "yes"; then
  AC_DEFINE(TRANS_SID, 1, [Whether you want transparent session id propagation])
fi

if test "$PHP_SESSION" != "no"; then
  PHP_EXTENSION(session,$ext_shared)
  PHP_SUBST(SESSION_SHARED_LIBADD)
fi
