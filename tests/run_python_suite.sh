#!/bin/sh
# Runs the jbig2enc Python unittest suite (tests/run.py) under `make check`.
#
# The environment is set by TESTS_ENVIRONMENT in src/Makefile.am:
#   JBIG2_EXE  path to the freshly built jbig2 binary
#   PYTHON     python interpreter
#   TESTS_DIR  path to the tests/ directory
#
# Exit 77 (skip) if the environment is incomplete; otherwise the exit code is
# whatever tests/run.py returns.
test -n "$JBIG2_EXE"  || { echo "run_python_suite: JBIG2_EXE not set"  >&2; exit 77; }
test -n "$TESTS_DIR"  || { echo "run_python_suite: TESTS_DIR not set"  >&2; exit 77; }
py="${PYTHON:-python3}"
exec "$py" "$TESTS_DIR/run.py"