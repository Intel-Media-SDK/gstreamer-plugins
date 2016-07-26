# Version notes:

m4_define([mediasdk_gst_major_version], [1])
m4_define([mediasdk_gst_minor_version], [0])
m4_define([mediasdk_gst_micro_version], [0])

m4_define([mediasdk_gst_version],
          [mediasdk_gst_major_version.mediasdk_gst_minor_version.mediasdk_gst_micro_version])

# Libraries versions are set in the following way:
#  mediasdk_major_version = 1 (Media SDK API major version number)
#  mediasdk_minor_version = <number=8,9,10,...> (Media SDK API minor version number)
#  mediasdk_micro_version = <release number>
m4_define([mediasdk_gst_lt_cur],
          [mediasdk_gst_major_version])
m4_define([mediasdk_gst_lt_rev],
          [mediasdk_gst_minor_version])
m4_define([mediasdk_lt_age],
          [mediasdk_gst_micro_version])

AC_PREREQ(2.63)
AC_INIT([mediasdk-gstreamer], [mediasdk_gst_version], [dmitry.v.rogozhkin@intel.com], mediasdk)
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([Makefile.am])

AC_CANONICAL_TARGET

AM_INIT_AUTOMAKE([dist-bzip2 subdir-objects])
AM_CONFIG_HEADER([config.h])

#########################################
# Options for the ./configure we support.
#########################################
AC_ARG_ENABLE(debug,
  [AC_HELP_STRING([--enable-debug],
                  [turn on debugging])],
  [], [debug="no"])

AC_ARG_VAR(MFX_HOME,
  [specify path to Media SDK installation])
#########################################

AC_DISABLE_STATIC
AC_PROG_LIBTOOL
AC_PROG_CC
AC_PROG_CXX
AC_PROG_INSTALL

AC_LIBTOOL_DLOPEN

AC_HEADER_STDC
AC_SYS_LARGEFILE

AC_TYPE_UINT32_T

PKG_CHECK_MODULES([LIBVA], [libva])
PKG_CHECK_MODULES([LIBVA_DRM], [libva-drm])
PKG_CHECK_MODULES([GST], [gstreamer-1.0])
PKG_CHECK_MODULES([GST_VIDEO], [gstreamer-video-1.0])
PKG_CHECK_MODULES([GST_ALLOCATORS], [gstreamer-allocators-1.0])

###################################
# Passing user flags to Makefile.am
###################################
DEBUG="no"
if test "$debug" = "yes"; then
  DEBUG="yes"
fi
AM_CONDITIONAL(DEBUG, test "$DEBUG" = "yes")
##################################

MEDIASDK_GST_MAJOR_VERSION=mediasdk_gst_major_version
MEDIASDK_GST_MINOR_VERSION=mediasdk_gst_minor_version
MEDIASDK_GST_MICRO_VERSION=mediasdk_gst_micro_version

MEDIASDK_GST_LT_CUR=mediasdk_gst_lt_cur
MEDIASDK_GST_LT_REV=mediasdk_gst_lt_rev
MEDIASDK_GST_LT_AGE=mediasdk_gst_lt_age

MEDIASDK_GST_VERSION=$MEDIASDK_GST_MAJOR_VERSION.$MEDIASDK_GST_MINOR_VERSION.$MEDIASDK_GST_MICRO_VERSION

AC_SUBST([MEDIASDK_GST_VERSION], [${MEDIASDK_GST_VERSION}])
AC_SUBST([MEDIASDK_GST_LT_LDFLAGS], ["-module -avoid-version"])

BUILD_32="no"
BUILD_64="no"
MFX_LIBDIR=""

case "$target_os" in
  linux*)
    case "$target_cpu" in
      i[[3456]]86)
        BUILD_32="yes"
        MFX_LIBDIR="ia32"
        AC_SUBST([MFX_ARCH_CFLAGS], ['-DLINUX32'])
        ;;
      x86_64)
        BUILD_64="yes"
        MFX_LIBDIR="x64"
        AC_SUBST([MFX_ARCH_CFLAGS], ['-DLINUX32 -DLINUX64'])
        ;;
    esac
    ;;
esac

AM_CONDITIONAL(BUILD_32, test "$BUILD_32" = "yes")
AM_CONDITIONAL(BUILD_64, test "$BUILD_64" = "yes")

NO_WARNINGS="" #"-Wno-unknown-pragmas -Wno-unused"

AC_SUBST([MFX_CFLAGS], ["-std=c++11"])
if test "$DEBUG" = "yes"; then
  AC_SUBST([MFX_CFLAGS], ["${MFX_CFLAGS} -D_DEBUG -Wall ${NO_WARNINGS}"])
else
  AC_SUBST([MFX_CFLAGS], ["${MFX_CFLAGS} -DNDEBUG -fPIC -D_FORTIFY_SOURCE=2 -fstack-protector -Wformat -Wformat-security -Wall ${NO_WARNINGS}"])
fi

AC_SUBST([MFX_CFLAGS], ["-I${MFX_HOME}/include ${MFX_ARCH_CFLAGS} ${MFX_CFLAGS}"])
AC_SUBST([MFX_CFLAGS], ["-DMFX_GST_VERSION=\\\"${MEDIASDK_GST_VERSION}\\\" ${MFX_CFLAGS}"])

MFX_LDFLAGS="-L${MFX_HOME}/lib/lin_${MFX_LIBDIR} -lmfx"

AC_SUBST([MFX_CFLAGS], ["-DLIBVA_SUPPORT ${MFX_CFLAGS}"])

##########################################################
# Usage: MFX_CHECK_LDFLAGS([<flags>, <action on failure>])
# Notes:
#  - <flags> will be appended to the MFX_LDFLAGS variable
##########################################################
AC_DEFUN([MFX_CHECK_LDFLAGS], [
  saved_ldflags="$LDFLAGS"
  LDFLAGS="$1"
  AC_MSG_CHECKING(if linker supports $LDFLAGS)
  AC_LINK_IFELSE(
    [AC_LANG_PROGRAM(
      [[]],
      [[int main(){ return 0; }]])],
      [AC_MSG_RESULT(yes)
        AC_SUBST([MFX_LDFLAGS], ["$1 ${MFX_LDFLAGS}"])],
      [AC_MSG_RESULT(no)]
        $2)
  LDFLAGS="$saved_ldflags"
])
##########################################################

##############################
# Checking for the link flags:
# 1. mandatory flags - actually that's just to pay atention that somethings is not supported
# 2. flags which may be not supported
##############################
MFX_CHECK_LDFLAGS(
  [-Wl,--no-undefined,-z,relro,-z,now,-z,noexecstack -fstack-protector],
  [AC_MSG_FAILURE(build is not possible due to unsupported build flags)])
MFX_CHECK_LDFLAGS(
  [-Wl,--default-symver],
  [])
##############################

AC_SUBST([MFX_LDFLAGS], ["${MFX_LDFLAGS}"])

AC_CONFIG_FILES([
  Makefile
  plugins/Makefile
  src/Makefile
  src/common/Makefile
  src/executors/Makefile
  src/wrappers/Makefile
  utils/Makefile
  ])

AC_OUTPUT
