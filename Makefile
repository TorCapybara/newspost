# Newspost 1.20

# makefile for newspost

PREFIX = /usr/local
BINDIR = $(PREFIX)/bin
CFLAGS = -O -Wall
CC = cc
INSTALL = install
SRC_FILES = uuencode.c newspost.c uu.c nntp.c socket.c utils.c sfv.c
HDR_FILES = newspost.h

all: newspost

newspost: $(SRC_FILES) $(HDR_FILES)
	$(CC) $(CFLAGS) -o $@ $(SRC_FILES) 

install: newspost
	$(INSTALL) -c -m 755 newspost $(BINDIR)

clean:
	-rm -f *~ *.o newspost
