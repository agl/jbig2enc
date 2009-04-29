CC=g++
LEPTONICA=../leptonlib-1.58
# For example, a fink MacOSX install:
# EXTRA=-I/sw/include/ -I/sw/include/libpng -I/sw/include/libjpeg -L/sw/lib
CFLAGS=-I${LEPTONICA}/src -Wall -I/usr/include -L/usr/lib -O3 ${EXTRA}

jbig2: libjbig2enc.a jbig2.cc
	$(CC) -o jbig2 jbig2.cc -L. -ljbig2enc ${LEPTONICA}/src/liblept.a $(CFLAGS) -lpng -ljpeg -ltiff -lm

libjbig2enc.a: jbig2enc.o jbig2arith.o jbig2sym.o
	ar -rcv libjbig2enc.a jbig2enc.o jbig2arith.o jbig2sym.o

jbig2enc.o: jbig2enc.cc jbig2arith.h jbig2sym.h jbig2structs.h jbig2segments.h
	$(CC) -c jbig2enc.cc $(CFLAGS)
jbig2arith.o: jbig2arith.cc jbig2arith.h
	$(CC) -c jbig2arith.cc $(CFLAGS)
jbig2sym.o: jbig2sym.cc jbig2arith.h
	$(CC) -c jbig2sym.cc -DUSE_EXT $(CFLAGS)

delta: delta.c
	$(CC) -o delta delta.c $(CFLAGS) ${LEPTONICA}/src/liblept.a -lpng -ljpeg -ltiff -lm

clean:
	rm -f *.o jbig2 libjbig2enc.a
