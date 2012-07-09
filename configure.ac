AC_PREREQ(2.50)
AC_INIT([jbig2enc], [0.28], [agl@imperialviolet.org], [jbig2enc-0.28],
		[https://github.com/agl/jbig2enc])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([-Wall -Werror foreign no-dependencies])
AC_PROG_CXX
AC_PROG_LIBTOOL

# Release versioning
GENERIC_MAJOR_VERSION=0
GENERIC_MINOR_VERSION=28
GENERIC_MICRO_VERSION=0

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

# this should fix automake 1.12 build and compatible with automake 1.11
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])

# default conditional
AM_CONDITIONAL(MINGW, false)

#############################
#
# Platform specific setup
#
#############################
AC_CANONICAL_HOST
case $host_os in
	mingw*)
		AC_DEFINE_UNQUOTED(MINGW,1,[This is a MinGW system])
		AC_CHECK_LIB(ws2_32, _head_libws2_32_a,,
			echo "Error! zlib not detected."
			exit -1)
		LIBS="$LIBS -lws2_32"
		;;
	 *)
		# default
		;;
esac 

AC_CHECK_LIB([lept], [findFileFormatStream], [], [
			echo "Error! Leptonica not detected."
			exit -1
			])
# test for function - it should detect leptonica dependecies

# Check for possible dependancies of leptonica.
# Because at the moment there is no way how to identify used libraries
# all presented libraries will be used...
AC_CHECK_LIB([tiff], TIFFOpen )
AC_CHECK_LIB([png], png_read_png )
AC_CHECK_LIB([jpeg], jpeg_read_scanlines )
AC_CHECK_LIB([gif], DGifOpenFileHandle )
AC_CHECK_LIB([webp], WebPGetInfo )
AC_CHECK_LIB([z], [zlibVersion], [], [
			echo "Error! zlib not detected."
			exit -1
			])
AC_CHECK_LIB([m], [sqrt], [], [
			echo "Error! libm not detected."
			exit -1
			])

AC_CONFIG_FILES([
	Makefile
	src/Makefile
	doc/Makefile
])
AC_OUTPUT
