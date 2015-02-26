dnl
dnl $Id: config.m4,v 1.32.2.1 2001/05/22 00:38:46 sniper Exp $
dnl

AC_DEFUN(IMAP_INC_CHK,[if test -r "$i$1/c-client.h"; then
    AC_DEFINE(HAVE_IMAP2000, 1, [ ])
    IMAP_DIR=$i
    IMAP_INC_DIR=$i$1
  elif test -r "$i$1/rfc822.h"; then 
    IMAP_DIR=$i; 
    IMAP_INC_DIR=$i$1
])

AC_DEFUN(IMAP_LIB_CHK,[
  str="$IMAP_DIR/$1/lib$lib.*"
  for i in `echo $str`; do
    if test -r $i; then
      IMAP_LIBDIR=$IMAP_DIR/$1
      break 2
    fi
  done
])

AC_DEFUN(PHP_IMAP_KRB_CHK, [
  PHP_ARG_WITH(kerberos,for Kerberos support in IMAP,
  [  --with-kerberos[=DIR]     IMAP: Include Kerberos support.])

  if test "$PHP_KERBEROS" = "yes"; then
    test -d /usr/kerberos && PHP_KERBEROS=/usr/kerberos
  fi

  if test "$PHP_KERBEROS" != "no"; then
    AC_DEFINE(HAVE_IMAP_KRB,1,[ ])
    PHP_ADD_LIBPATH($PHP_KERBEROS/lib, IMAP_SHARED_LIBADD)
    PHP_ADD_LIBRARY(gssapi_krb5, 1, IMAP_SHARED_LIBADD)
    PHP_ADD_LIBRARY(krb5, 1, IMAP_SHARED_LIBADD)
    PHP_ADD_LIBRARY(k5crypto, 1, IMAP_SHARED_LIBADD)
    PHP_ADD_LIBRARY(com_err,  1, IMAP_SHARED_LIBADD)
  fi
])

AC_DEFUN(PHP_IMAP_SSL_CHK, [
  PHP_ARG_WITH(imap-ssl,for SSL support in IMAP,
  [  --with-imap-ssl[=DIR]     IMAP: Include SSL support.])

  if test "$PHP_IMAP_SSL" = "yes"; then
    PHP_IMAP_SSL=/usr
  fi

  if test "$PHP_IMAP_SSL" != "no"; then
    PHP_ADD_LIBPATH($PHP_IMAP_SSL/lib, IMAP_SHARED_LIBADD)
    PHP_ADD_LIBRARY_DEFER(crypto)
    PHP_ADD_LIBRARY_DEFER(ssl)

    old_LIBS=$LIBS
    LIBS="$LIBS -lc-client"
    if test $PHP_KERBEROS != "no"; then
      LIBS="$LIBS -lgssapi_krb5 -lkrb5 -lk5crypto -lcom_err"
    fi
    AC_TRY_RUN([
      void mm_log(void){}
      void mm_dlog(void){}
      void mm_flags(void){}
      void mm_fatal(void){}
      void mm_critical(void){}
      void mm_nocritical(void){}
      void mm_notify(void){}
      void mm_login(void){}
      void mm_diskerror(void){}
      void mm_status(void){}
      void mm_lsub(void){}
      void mm_list(void){}
      void mm_exists(void){}
      void mm_searched(void){}
      void mm_expunged(void){}
      char ssl_onceonlyinit();
      int main() {
        ssl_onceonlyinit();
        return 0;
      }
    ],[
      AC_DEFINE(HAVE_IMAP_SSL,1,[ ])
    ], [
      AC_MSG_ERROR(This c-client library does not support SSL. Recompile or remove --with-imap-ssl from configure line.)
    ])
    LIBS=$old_LIBS
  fi
])


PHP_ARG_WITH(imap,for IMAP support,
[  --with-imap[=DIR]       Include IMAP support.  DIR is the IMAP include
                          and c-client.a directory.])

if test "$PHP_IMAP" != "no"; then  

    PHP_SUBST(IMAP_SHARED_LIBADD)
    PHP_EXTENSION(imap, $ext_shared)
    AC_DEFINE(HAVE_IMAP,1,[ ])

    for i in /usr/local /usr $PHP_IMAP; do
      IMAP_INC_CHK()
      el[]IMAP_INC_CHK(/include/c-client)
      el[]IMAP_INC_CHK(/include/imap)
      el[]IMAP_INC_CHK(/include)
      el[]IMAP_INC_CHK(/imap)
      el[]IMAP_INC_CHK(/c-client)
      fi
    done

    AC_CHECK_LIB(pam, pam_start) 
    
    PHP_EXPAND_PATH($IMAP_DIR, IMAP_DIR)

    if test -z "$IMAP_DIR"; then
      AC_MSG_ERROR(Cannot find rfc822.h. Please check your IMAP installation.)
    fi

    if test -r "$IMAP_DIR/c-client/c-client.a"; then
      ln -s "$IMAP_DIR/c-client/c-client.a" "$IMAP_DIR/c-client/libc-client.a" >/dev/null 2>&1
    elif test -r "$IMAP_DIR/lib/c-client.a"; then
      ln -s "$IMAP_DIR/lib/c-client.a" "$IMAP_DIR/lib/libc-client.a" >/dev/null 2>&1
    fi

    for lib in imap c-client4 c-client; do
      IMAP_LIB=$lib
      IMAP_LIB_CHK(lib)
      IMAP_LIB_CHK(c-client)
    done

    if test -z "$IMAP_LIBDIR"; then
      AC_MSG_ERROR(Cannot find imap library. Please check your IMAP installation.)
    fi

    PHP_ADD_INCLUDE($IMAP_INC_DIR)
    PHP_ADD_LIBPATH($IMAP_LIBDIR, IMAP_SHARED_LIBADD)
    PHP_ADD_LIBRARY_DEFER($IMAP_LIB)
    PHP_IMAP_KRB_CHK
    PHP_IMAP_SSL_CHK
fi
