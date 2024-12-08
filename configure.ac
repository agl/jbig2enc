AC_PREREQ(2.50)
AC_INIT([jbig2enc], [0.29], [agl@imperialviolet.org], [jbig2enc-0.29],
		[https://github.com/agl/jbig2enc])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([-Wall -Werror foreign no-dependencies])

# this should fix automake 1.12 build and compatible with automake 1.11
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
LT_INIT

AC_PROG_CXX
AC_PROG_LIBTOOL

# Release versioning
GENERIC_MAJOR_VERSION=0
GENERIC_MINOR_VERSION=29
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

# Check if rpath is disabled
AC_MSG_CHECKING(whether to use rpath)
AC_ARG_ENABLE(rpath,
	[AC_HELP_STRING([--disable-rpath],
			[Patches libtool to not use rpath in the libraries produced.])],
	[enable_rpath="$enableval"], [enable_rpath="yes"])
AC_MSG_RESULT($enable_rpath)

AM_CONDITIONAL([ENABLE_RPATH], [test "x$enable_rpath" = "xyes"])

AC_CONFIG_COMMANDS([libtool-rpath-patch],
	[if test "$libtool_patch_use_rpath" = "no"; then
		sed < libtool > libtool-2 's/^hardcode_libdir_flag_spec.*$'/'hardcode_libdir_flag_spec=""/'
		mv libtool-2 libtool
		sed < libtool > libtool-2 's/^runpath_var=LD_RUN_PATH$'/'runpath_var=DIE_RPATH_DIE/'
		mv libtool-2 libtool
		chmod 755 libtool
	fi],
[libtool_patch_use_rpath=$enable_rpath])

AC_CHECK_LIB([leptonica], [findFileFormatStream], [], [
			echo "Error! Leptonica not detected."
			exit -1
			])
AC_CHECK_FUNCS(expandBinaryPower2Low,,)
# test for function - it should detect leptonica dependencies

# Check for possible dependencies of leptonica.
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
