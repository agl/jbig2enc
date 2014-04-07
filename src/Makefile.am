AM_CXXFLAGS = -Wall
AM_LDFLAGS = -Wl,-E
if ENABLE_RPATH
AM_LDFLAGS += -Wl,-rpath -Wl,$(libdir)
endif

lib_LTLIBRARIES = libjbig2enc.la
libjbig2enc_la_SOURCES = jbig2enc.cc jbig2arith.cc jbig2sym.cc jbig2comparator.cc
libjbig2enc_la_LDFLAGS = -no-undefined -version-info $(GENERIC_LIBRARY_VERSION)
include_HEADERS = jbig2arith.h jbig2sym.h jbig2structs.h jbig2segments.h jbig2comparator.h

bin_PROGRAMS = jbig2
jbig2_SOURCES = jbig2.cc
jbig2_LDADD = libjbig2enc.la 
jbig2_LDFLAGS = -static

if MINGW
jbig2_LDADD += -lws2_32
endif 
