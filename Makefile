CC=g++
LEPTONICA=../leptonlib-1.33/
CFLAGS=-I${LEPTONICA}/src -Wall -I/usr/include -L/usr/lib -ggdb

jbig2: libjbig2enc.a jbig2.cc
	$(CC) -o jbig2 jbig2.cc -L. -ljbig2enc -limage -L${LEPTONICA}/lib/nodebug $(CFLAGS) -lpng -ljpeg -ltiff -lm

libjbig2enc.a: jbig2enc.o jbig2arith.o jbig2sym.o
	ar -rcv libjbig2enc.a jbig2enc.o jbig2arith.o jbig2sym.o

jbig2enc.o: jbig2enc.cc jbig2arith.h
	$(CC) -c jbig2enc.cc $(CFLAGS)
jbig2arith.o: jbig2arith.cc jbig2arith.h
	$(CC) -c jbig2arith.cc $(CFLAGS)
jbig2sym.o: jbig2sym.cc jbig2arith.h
	$(CC) -c jbig2sym.cc -DUSE_EXT $(CFLAGS)

clean:
	rm -f *.o jbig2 libjbig2enc.a
