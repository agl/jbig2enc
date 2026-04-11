#!/bin/sh
set -e

version_clean="$1"
version_major="$2"
version_minor="$3"
version_micro="$4"

pat_cmake_version='set\(Version "[^"]+"\)'
pat_ac_init='(AC_INIT\(\[[^]]+\],\[)[0-9\.]+(\],\[[^]]*\],\[jbig2enc-)[0-9\.]*'
pat_ac_major='(GENERIC_MAJOR_VERSION=)[0-9]*'
pat_ac_minor='(GENERIC_MINOR_VERSION=)[0-9]*'
pat_ac_micro='(GENERIC_MICRO_VERSION=)[0-9]*'

cd "$MESON_PROJECT_DIST_ROOT"
sed -i -E "s/$pat_cmake_version/set(Version \"$version_clean\")/" CMakeLists.txt
sed -i -E "s/$pat_ac_init/\\1$version_clean\\2$version_clean/" configure.ac
sed -i -E "s/$pat_ac_major/\\1$version_major/;s/$pat_ac_minor/\\1$version_minor/;s/$pat_ac_micro/\\1$version_micro/" configure.ac

./autogen.sh
rm -rf autom4te.cache
