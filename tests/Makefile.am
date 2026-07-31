AM_CXXFLAGS = $(JBIG2ENC_CXX17_FLAG) -Wall

# byte_regression is a compiled byte-level regression test for the JBIG2
# struct serializers.  Built and run by `make check`.
check_PROGRAMS = byte_regression
byte_regression_SOURCES = byte_regression.cc
byte_regression_LDADD = $(top_builddir)/src/libjbig2enc.la

if MINGW
byte_regression_LDADD += -lws2_32
endif

TESTS = byte_regression

if HAVE_PYTHON
# Run the Python unittest suite (tests/run.py) as part of `make check`.
# TESTS_ENVIRONMENT points the suite at the freshly built jbig2 binary.
TESTS += run_python_suite.sh
TESTS_ENVIRONMENT = JBIG2_EXE=$(top_builddir)/src/jbig2$(EXEEXT) PYTHON=$(PYTHON) TESTS_DIR=$(top_srcdir)/tests
endif

EXTRA_DIST = run_python_suite.sh