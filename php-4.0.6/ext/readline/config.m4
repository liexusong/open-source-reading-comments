dnl $Id: config.m4,v 1.10 2001/03/27 20:34:41 sniper Exp $
dnl config.m4 for extension readline
dnl don't forget to call PHP_EXTENSION(readline)

PHP_ARG_WITH(libedit,for libedit readline replacement, 
[  --with-libedit[=DIR]    Include libedit readline replacement.])

PHP_ARG_WITH(readline,for readline support,
[  --with-readline[=DIR]   Include readline support.  DIR is the readline
                          install directory.])

if test "$PHP_READLINE" != "no"; then
  for i in /usr/local /usr $PHP_READLINE; do
    if test -f $i/include/readline/readline.h; then
      READLINE_DIR=$i
    fi
  done

  if test -z "$READLINE_DIR"; then
    AC_MSG_ERROR(Please reinstall readline - I cannot find readline.h)
  fi
  PHP_ADD_INCLUDE($READLINE_DIR/include)

  AC_CHECK_LIB(ncurses, tgetent, [
    PHP_ADD_LIBRARY_WITH_PATH(ncurses,,READLINE_SHARED_LIBADD)],[
    AC_CHECK_LIB(termcap, tgetent, [
      PHP_ADD_LIBRARY_WITH_PATH(termcap,,READLINE_SHARED_LIBADD)])
  ])

  PHP_ADD_LIBRARY_WITH_PATH(history, $READLINE_DIR/lib, READLINE_SHARED_LIBADD)
  PHP_ADD_LIBRARY_WITH_PATH(readline, $READLINE_DIR/lib, READLINE_SHARED_LIBADD)
  PHP_SUBST(READLINE_SHARED_LIBADD)

  AC_DEFINE(HAVE_LIBREADLINE, 1, [ ])
  PHP_EXTENSION(readline, $ext_shared)
fi

if test "$PHP_LIBEDIT" != "no"; then
  for i in /usr/local /usr $PHP_LIBEDIT; do
    if test -f $i/include/readline/readline.h; then
      LIBEDIT_DIR=$i
    fi
  done

  if test -z "$LIBEDIT_DIR"; then
    AC_MSG_ERROR(Please reinstall libedit - I cannot find readline.h)
  fi
  PHP_ADD_INCLUDE($LIBEDIT_DIR/include)

  AC_CHECK_LIB(ncurses, tgetent, [
    PHP_ADD_LIBRARY_WITH_PATH(ncurses,,READLINE__SHARED_LIBADD)],[
    AC_CHECK_LIB(termcap, tgetent, [
      PHP_ADD_LIBRARY_WITH_PATH(termcap,,READLINE_SHARED_LIBADD)])
  ])

  PHP_ADD_LIBRARY_WITH_PATH(edit, $LIBEDIT_DIR/lib, READLINE_SHARED_LIBADD)  
  PHP_SUBST(READLINE_SHARED_LIBADD)

  AC_DEFINE(HAVE_LIBEDIT, 1, [ ])
  PHP_EXTENSION(readline, $ext_shared)
fi