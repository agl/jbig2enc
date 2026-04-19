#!/bin/sh

if [ "$1" = "get-vcs" ]; then
  cat "$MESON_SOURCE_ROOT/VERSION" | tr -d '\n'
else
  exit 1
fi
