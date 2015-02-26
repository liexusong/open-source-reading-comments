dnl
dnl $Id: config.m4,v 1.60.2.6 2001/06/04 01:36:47 sniper Exp $
dnl

AC_DEFUN(PHP_GD_JPEG,[
  PHP_ARG_WITH(jpeg-dir, for the location of libjpeg,
  [  --with-jpeg-dir=DIR       GD: Set the path to libjpeg install prefix.])

  if test "$PHP_JPEG_DIR" != "no"; then

    if test "$PHP_JPEG_DIR" = "yes"; then
      PHP_JPEG_DIR=/usr/local
    fi

    AC_CHECK_LIB(jpeg,jpeg_read_header,
    [
      PHP_ADD_LIBRARY_WITH_PATH(jpeg, $PHP_JPEG_DIR/lib, GD_SHARED_LIBADD)
    ],[
      AC_MSG_ERROR(libjpeg not found!)
    ],[
      -L$PHP_JPEG_DIR/lib
    ])
  else 
    AC_MSG_RESULT(not set. If configure fails try --with-jpeg-dir=<DIR>)
  fi
])

AC_DEFUN(PHP_GD_PNG,[
  PHP_ARG_WITH(png-dir, for the location of libpng,
  [  --with-png-dir=DIR        GD: Set the path to libpng install prefix.])

  if test "$PHP_PNG_DIR" != "no"; then

    if test "$PHP_PNG_DIR" = "yes"; then
      PHP_PNG_DIR=/usr/local
    fi

    if test "$PHP_ZLIB_DIR" = "no"; then
      AC_MSG_ERROR(PNG support requires ZLIB. Use --with-zlib-dir=<DIR>)
    fi
    
    AC_CHECK_LIB(png,png_info_init,
    [
      PHP_ADD_LIBRARY_WITH_PATH(z, $PHP_ZLIB_DIR/lib, GD_SHARED_LIBADD)
      PHP_ADD_LIBRARY_WITH_PATH(png, $PHP_PNG_DIR/lib, GD_SHARED_LIBADD)
    ],[
      AC_MSG_ERROR(libpng not found!)
    ],[
      -L$PHP_ZLIB_DIR/lib -lz -L$PHP_PNG_DIR/lib
    ])
  else 
    AC_MSG_RESULT(If configure fails try --with-png-dir=<DIR> and --with-zlib-dir=<DIR>)
  fi
])

AC_DEFUN(PHP_GD_XPM,[
  PHP_ARG_WITH(xpm-dir, for the location of libXpm,
  [  --with-xpm-dir=DIR        GD: Set the path to libXpm install prefix.])

  if test "$PHP_XPM_DIR" != "no"; then
    if test "$PHP_XPM_DIR" = "yes"; then
      PHP_XPM_DIR=/usr/local
    fi

    AC_CHECK_LIB(Xpm,XpmFreeXpmImage, 
    [
      PHP_ADD_LIBRARY_WITH_PATH(Xpm, $PHP_XPM_DIR/lib, GD_SHARED_LIBADD)
      PHP_ADD_LIBRARY_WITH_PATH(X11, $PHP_XPM_DIR/lib, GD_SHARED_LIBADD)
    ],[
      AC_MSG_ERROR(libXpm.(a|so) or libX11.(a|so) not found!)
    ],[
      -L$PHP_XPM_DIR/lib -lX11
    ])
  else 
    AC_MSG_RESULT(If configure fails try --with-xpm-dir=<DIR>)
  fi
])

AC_DEFUN(PHP_GD_FREETYPE1,[
  PHP_ARG_WITH(ttf,whether to include include FreeType 1.x support,
  [  --with-ttf[=DIR]          GD: Include FreeType 1.x support])
  
  if test "$PHP_TTF" != "no"; then
    if test "$PHP_FREETYPE_DIR" = "no" -o "$PHP_FREETYPE_DIR" = ""; then
      if test -n "$PHP_TTF" ; then
        for i in /usr /usr/local "$PHP_TTF" ; do
          if test -f "$i/include/freetype.h" ; then
            TTF_DIR="$i"
            unset TTF_INC_DIR
          fi
          if test -f "$i/include/freetype/freetype.h"; then
            TTF_DIR="$i"
            TTF_INC_DIR="$i/include/freetype"
          fi
        done
      fi
      if test -n "$TTF_DIR" ; then
        AC_DEFINE(HAVE_LIBTTF,1,[ ])
        PHP_ADD_LIBRARY_WITH_PATH(ttf, $TTF_DIR/lib, GD_SHARED_LIBADD)
      fi
      if test -z "$TTF_INC_DIR"; then
        TTF_INC_DIR="$TTF_DIR/include"
      fi
      PHP_ADD_INCLUDE($TTF_INC_DIR)
    else
      AC_MSG_RESULT(no - FreeType 2.x is to be used instead)
    fi
  fi
])

AC_DEFUN(PHP_GD_FREETYPE2,[
  PHP_ARG_WITH(freetype-dir, for freetype(2),
  [  --with-freetype-dir=DIR   GD: Set the path to freetype2 install prefix.])

  if test "$PHP_FREETYPE_DIR" != "no"; then
    for i in /usr /usr/local "$PHP_FREETYPE_DIR" ; do
      if test -f "$i/include/freetype2/freetype/freetype.h"; then
        FREETYPE2_DIR="$i"
        FREETYPE2_INC_DIR="$i/include/freetype2/freetype"
      fi
    done
    
    if test -n "$FREETYPE2_DIR" ; then
      PHP_ADD_LIBRARY_WITH_PATH(freetype, $FREETYPE2_DIR/lib, GD_SHARED_LIBADD)
      PHP_ADD_INCLUDE($FREETYPE2_INC_DIR)
      AC_DEFINE(USE_GD_IMGSTRTTF, 1, [ ])
      AC_DEFINE(HAVE_LIBFREETYPE,1,[ ])
    else
      AC_MSG_ERROR(freetype2 not found!)
    fi
  else 
    AC_MSG_RESULT(If configure fails try --with-freetype-dir=<DIR>)
  fi
])

AC_DEFUN(PHP_GD_T1LIB,[
  PHP_ARG_WITH(t1lib, whether to include T1lib support,
  [  --with-t1lib[=DIR]        GD: Include T1lib support.])

  if test "$PHP_T1LIB" != "no"; then
    for i in /usr /usr/local $PHP_T1LIB; do
      if test -f "$i/include/t1lib.h"; then
        T1_DIR=$i
      fi
    done

    if test -n "$T1_DIR"; then
      AC_CHECK_LIB(t1, T1_GetExtend, 
      [
        AC_DEFINE(HAVE_LIBT1,1,[ ])
        PHP_ADD_INCLUDE("$T1_DIR/include")
        PHP_ADD_LIBRARY_WITH_PATH(t1, "$T1_DIR/lib", GD_SHARED_LIBADD)
      ],[
        AC_MSG_ERROR(Problem with libt1.(a|so). Please check config.log for more information.) 
      ],[
        -L$T1_DIR/lib
      ])
    else
      AC_MSG_ERROR(Your T1lib distribution is not installed correctly. Please reinstall it.) 
    fi
  fi
])

AC_DEFUN(PHP_GD_TTSTR,[
  PHP_ARG_ENABLE(gd-native-tt, whether to enable truetype string function in gd,
  [  --enable-gd-native-ttf    GD: Enable TrueType string function in gd])
  
  if test "$PHP_GD_NATIVE_TT" = "yes"; then
    AC_DEFINE(USE_GD_IMGSTRTTF, 1, [ ])
  fi
])

AC_DEFUN(PHP_GD_CHECK_VERSION,[
  AC_CHECK_LIB(gd, gdImageString16,        [AC_DEFINE(HAVE_LIBGD13, 1, [ ])])
  AC_CHECK_LIB(gd, gdImagePaletteCopy,     [AC_DEFINE(HAVE_LIBGD15, 1, [ ])])
  AC_CHECK_LIB(gd, gdImageCreateFromPng,   [AC_DEFINE(HAVE_GD_PNG,  1, [ ])])
  AC_CHECK_LIB(gd, gdImageCreateFromGif,   [AC_DEFINE(HAVE_GD_GIF,  1, [ ])])
  AC_CHECK_LIB(gd, gdImageWBMP,            [AC_DEFINE(HAVE_GD_WBMP, 1, [ ])])
  AC_CHECK_LIB(gd, gdImageCreateFromJpeg,  [AC_DEFINE(HAVE_GD_JPG,  1, [ ])])
  AC_CHECK_LIB(gd, gdImageCreateFromXpm,   [AC_DEFINE(HAVE_GD_XPM,  1, [ ])])
  AC_CHECK_LIB(gd, gdImageCreateTrueColor, [AC_DEFINE(HAVE_LIBGD20, 1, [ ])])
  AC_CHECK_LIB(gd, gdImageSetTile,         [AC_DEFINE(HAVE_GD_IMAGESETTILE,  1, [ ])])
  AC_CHECK_LIB(gd, gdImageSetBrush,        [AC_DEFINE(HAVE_GD_IMAGESETBRUSH, 1, [ ])])
  AC_CHECK_LIB(gd, gdImageStringFTEx,      [AC_DEFINE(HAVE_GD_STRINGFTEX,    1, [ ])])
  AC_CHECK_LIB(gd, gdImageColorClosestHWB, [AC_DEFINE(HAVE_COLORCLOSESTHWB,     1, [ ])])
  AC_CHECK_LIB(gd, gdImageColorResolve,    [AC_DEFINE(HAVE_GDIMAGECOLORRESOLVE, 1, [ ])])
  AC_CHECK_LIB(gd, gdImageGifCtx,          [AC_DEFINE(HAVE_GD_GIF_CTX,  1, [ ])])
])


PHP_ARG_WITH(gd, whether to include GD support,
[  --with-gd[=DIR]         Include GD support (DIR is GD's install dir).
                          Set DIR to "shared" to build as a dl, or 
                          "shared,DIR" to build as a dl and still specify DIR.])

if test "$PHP_GD" != "no"; then

  PHP_EXTENSION(gd, $ext_shared)
  PHP_SUBST(GD_SHARED_LIBADD)

dnl Various checks for GD features
  PHP_GD_TTSTR
  PHP_GD_JPEG
  PHP_GD_PNG
  PHP_GD_XPM
  PHP_GD_FREETYPE2
  PHP_GD_FREETYPE1
  PHP_GD_T1LIB

  case "$PHP_GD" in
    yes)
      PHP_ADD_LIBRARY(gd,, GD_SHARED_LIBADD)
      PHP_GD_CHECK_VERSION
      AC_DEFINE(HAVE_LIBGD,1,[ ])
	;;
    *)
dnl A whole whack of possible places where these might be
      for i in include/gd1.3 include/gd include gd1.3 gd ""; do
        test -f $PHP_GD/$i/gd.h && GD_INCLUDE="$PHP_GD/$i"
      done

      for i in lib/gd1.3 lib/gd lib gd1.3 gd ""; do
        test -f $PHP_GD/$i/libgd.s? -o -f $PHP_GD/$i/libgd.a && GD_LIB="$PHP_GD/$i"
      done

      if test -n "$GD_INCLUDE" && test -n "$GD_LIB" ; then
        PHP_ADD_LIBRARY_WITH_PATH(gd, $GD_LIB, GD_SHARED_LIBADD)
        AC_DEFINE(HAVE_LIBGD,1,[ ])
        PHP_GD_CHECK_VERSION
      else
        AC_MSG_ERROR([Unable to find libgd.(a|so) anywhere under $withval])
      fi 
    ;;
  esac

dnl NetBSD package structure
  if test -f /usr/pkg/include/gd/gd.h -a -z "$GD_INCLUDE" ; then
    GD_INCLUDE="/usr/pkg/include/gd"
  fi

dnl SuSE 6.x package structure
  if test -f /usr/include/gd/gd.h -a -z "$GD_INCLUDE" ; then
    GD_INCLUDE="/usr/include/gd"
  fi

  PHP_EXPAND_PATH($GD_INCLUDE, GD_INCLUDE)
  PHP_ADD_INCLUDE($GD_INCLUDE)

fi
