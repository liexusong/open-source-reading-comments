dnl $Id: config.m4,v 1.1.2.1 2001/05/12 09:29:06 sas Exp $
dnl config.m4 for extension mbstring

PHP_ARG_ENABLE(mbstring, whether to enable multibyte string support,
[  --enable-mbstring       Enable multibyte string support])

if test "$PHP_MBSTRING" != "no"; then
  AC_DEFINE(HAVE_MBSTRING,1,[ ])
  PHP_EXTENSION(mbstring, $ext_shared)
fi

AC_MSG_CHECKING(whether to enable japanese encoding translation)
AC_ARG_ENABLE(mbstr_enc_trans,
[  --enable-mbstr-enc-trans   Enable japanese encoding translation],[
  if test "$enableval" = "yes" ; then
    AC_DEFINE(MBSTR_ENC_TRANS, 1, [ ])
    AC_MSG_RESULT(yes)
  else
    AC_MSG_RESULT(no)
  fi
],[
  AC_MSG_RESULT(no)
])
