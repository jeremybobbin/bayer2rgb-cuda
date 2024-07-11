#!/usr/bin/make -f
.POSIX:
.SUFFIXES: .c .o .cu .fatbin 
.DEFAULT: bayer2rgb

PREFIX="/usr/local"
CC=cc
CFLAGS=-O2 -std=c11
LDLIBS=-lcuda
NVCCFLAGS=-O2

.cu.fatbin:
	nvcc $(NVCCFLAGS) --fatbin -o $@ $^

.fatbin.o:
	$(LD) -r -b binary -o $@ $^

.o:
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

bayer2rgb: bayer2rgb.o debayer.o

install:
	cp -v bayer2rgb $(PREFIX)/bin

clean:
	rm -f bayer2rgb *.o *.fatbin

