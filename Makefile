# Newspost 1.0

# makefile for newspost

PREFIX = /usr/local
BINDIR = $(PREFIX)/bin
CFLAGS = -O
CC = cc
INSTALL = install
SRC_FILES = newspost.c uu.c nntp.c socket.c utils.c cksfv/crc32.c cksfv/newsfv.c cksfv/print.c

all: uulib/libuu.a newspost

uulib/libuu.a: uulib/configure
	cd uulib ; ./configure ; make ; cd ..

newspost: $(SRC_FILES)
	$(CC) $(CFLAGS) -o $@ $(SRC_FILES) -L./uulib/ -luu 

install: uulib/libuu.a newspost
	$(INSTALL) -c -m 755 newspost $(BINDIR)

clean:
	cd uulib ; make clean ; cd .. ;
	-cd cksfv ; rm -f *~ *.o ; cd .. ; 
	-rm -f *~ *.o newspost

