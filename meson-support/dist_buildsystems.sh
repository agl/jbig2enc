#!/bin/sh
set -e


cd "$MESON_PROJECT_DIST_ROOT"

./autogen.sh
rm -rf autom4te.cache
