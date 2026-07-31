AC_PREREQ([2.71])
m4_define([jbig2enc_version], m4_esyscmd([printf %s "$(cat VERSION)"]))
AC_INIT([jbig2enc], jbig2enc_version, [agl@imperialviolet.org], [jbig2enc-]jbig2enc_version, [https://github.com/agl/jbig2enc])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([-Wall -Werror foreign no-dependencies])

# this should fix automake 1.12 build and compatible with automake 1.11
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
LT_INIT

AC_PROG_CXX
AC_LANG_PUSH([C++])
AC_MSG_CHECKING([for C++17 compiler flag])
saved_CXXFLAGS=$CXXFLAGS
JBIG2ENC_CXX17_FLAG=
for flag in -std=c++17 -std=c++1z; do
	CXXFLAGS="$saved_CXXFLAGS $flag"
	AC_COMPILE_IFELSE(
		[AC_LANG_PROGRAM([[#if __cplusplus < 201703L
#error C++17 support is required
#endif
#include <array>
#include <optional>
		]], [[std::array<int, 1> a{{1}}; std::optional<int> v = a[0];]])],
		[JBIG2ENC_CXX17_FLAG=$flag; break],
		[])
done
CXXFLAGS=$saved_CXXFLAGS
if test "x$JBIG2ENC_CXX17_FLAG" = x; then
	AC_MSG_RESULT([not found])
	AC_MSG_ERROR([A compiler with C++17 support is required])
fi
AC_MSG_RESULT([$JBIG2ENC_CXX17_FLAG])
AC_SUBST([JBIG2ENC_CXX17_FLAG])
AC_LANG_POP([C++])
LT_INIT

# Release versioning
GENERIC_MAJOR_VERSION=m4_esyscmd([printf %s "$(cut -d. -f1 VERSION)"])
GENERIC_MINOR_VERSION=m4_esyscmd([printf %s "$(cut -d. -f2 VERSION)"])
GENERIC_MICRO_VERSION=m4_esyscmd([printf %s "$(cut -d. -f3 VERSION)"])
if test -z "$GENERIC_MICRO_VERSION"; then
  GENERIC_MICRO_VERSION=0
fi

# API version (often = GENERIC_MAJOR_VERSION.GENERIC_MINOR_VERSION)
GENERIC_API_VERSION=$GENERIC_MAJOR_VERSION.$GENERIC_MINOR_VERSION
GENERIC_LIBRARY_VERSION=$GENERIC_MAJOR_VERSION:$GENERIC_MINOR_VERSION
AC_SUBST(GENERIC_API_VERSION)
AC_SUBST(GENERIC_MAJOR_VERSION)

AC_SUBST(GENERIC_LIBRARY_VERSION)
PACKAGE=$GENERIC_LIBRARY_NAME
AC_SUBST(GENERIC_LIBRARY_NAME)

GENERIC_VERSION=$GENERIC_MAJOR_VERSION.$GENERIC_MINOR_VERSION.$GENERIC_MICRO_VERSION
GENERIC_RELEASE=$GENERIC_MAJOR_VERSION.$GENERIC_MINOR_VERSION
AC_SUBST(GENERIC_RELEASE)
AC_SUBST(GENERIC_VERSION)

# default conditional
AM_CONDITIONAL(MINGW, false)

#############################
#
# Platform specific setup
#
#############################
AC_CANONICAL_HOST
case "${host_os}" in
	mingw*)
		AC_DEFINE_UNQUOTED([MINGW], 1, [This is a MinGW system])
		dnl Try to detect winsock2 on mingw32/64 systems.
		AC_CHECK_LIB(ws2_32, [_head_libws2_32_a])
		AC_CHECK_LIB(ws2_32, [_head_lib32_libws2_32_a])
		AC_CHECK_LIB(ws2_32, [_head_lib64_libws2_32_a])
		;;
	*)
		# default
		;;
esac

AC_CHECK_LIB([leptonica], [findFileFormatStream], [], [
			echo "Error! Leptonica not detected."
			exit -1
			])
# Check if PKG_CHECK_MODULES macro exists (provided by pkg-config)
m4_ifdef([PKG_CHECK_MODULES], [
	# Macro exists: Use pkg-config to check version and get CFLAGS
	PKG_CHECK_MODULES([LEPTONICA], [lept >= 1.74], [have_lept=true], [have_lept=false])
	if $have_lept; then
		CPPFLAGS="$CPPFLAGS $LEPTONICA_CFLAGS"
	else
		AC_MSG_ERROR([Leptonica 1.74 or higher is required. Try to install libleptonica-dev package.])
	fi
], [
	AC_MSG_ERROR([pkg-config macro not found. Please install the 'pkg-config' package.])
])

AC_CHECK_LIB([m], [sqrt], [], [
			echo "Error! libm not detected."
			exit -1
			])

# Python interpreter for the unittest suite (optional; tests skip if absent).
AC_CHECK_PROGS([PYTHON], [python3 python])
AM_CONDITIONAL([HAVE_PYTHON], [test "x$PYTHON" != "x"])

AC_CONFIG_FILES([
	Makefile
	src/Makefile
	doc/Makefile
	tests/Makefile
])
AC_OUTPUT
