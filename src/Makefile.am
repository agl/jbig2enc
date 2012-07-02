AM_CXXFLAGS = -Wall

lib_LTLIBRARIES = libjbig2enc.la
libjbig2enc_la_SOURCES = jbig2enc.cc jbig2arith.cc jbig2sym.cc
include_HEADERS = jbig2arith.h jbig2sym.h jbig2structs.h jbig2segments.h

bin_PROGRAMS = jbig2
jbig2_SOURCES = jbig2.cc
jbig2_LDADD = libjbig2enc.la 
jbig2_LDFLAGS = -all-static

if MINGW
jbig2_LDADD += -lws2_32
endif 
