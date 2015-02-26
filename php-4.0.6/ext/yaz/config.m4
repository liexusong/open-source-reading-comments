dnl $Id: config.m4,v 1.3 2000/06/17 10:51:20 sas Exp $

PHP_ARG_WITH(yaz,for YAZ support,
[  --with-yaz[=DIR]        Include YAZ support (ANSI/NISO Z39.50). DIR is
                          the YAZ bin install directory])


if test "$PHP_YAZ" != "no"; then
  yazconfig=NONE
  if test "$PHP_YAZ" = "yes"; then
    AC_PATH_PROG(yazconfig, yaz-config, NONE)
  else
    if test -r ${PHP_YAZ}/yaz-config; then
      yazconfig=${PHP_YAZ}/yaz-config
    else
      yazconfig=${PHP_YAZ}/bin/yaz-config
    fi
  fi

  if test -f $yazconfig; then
    AC_DEFINE(HAVE_YAZ,1,[Whether you have YAZ])
    . $yazconfig
    PHP_EVAL_LIBLINE($YAZLIB, YAZ_SHARED_LIBADD)
    PHP_EVAL_INCLINE($YAZINC)
    PHP_SUBST(YAZ_SHARED_LIBADD)
    PHP_EXTENSION(yaz, $ext_shared)
  fi
fi
