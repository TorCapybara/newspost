/* Newspost - a binary autoposter for unix

   Copyright (C) 2000 Jim Faulkner <newspost@unixcab.org>
   Modified by William McBrine <wmcbrine@users.sf.net>, May 2002

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. */

#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <signal.h>
#include <sys/time.h>

#define READS_UNTIL_FAIL 10
/* number of seconds it waits before posting the files */
#define SLEEPTIME 10

#define SFV_COMPATIBILITY_MODE
#define FORCESUBJECT

typedef unsigned long crc_t;

#define bool int
#define TRUE 1
#define FALSE 0

#define BPL 45 /* bytes per line */

#define BUFSIZE 8192

#define ISFIRST 1
#define ISLAST 2
#define ISBOTH 3

#define NVERSION "1.20"
#define NEWSPOSTURL "http://newspost.unixcab.org/"
#define USERAGENT "newspost/" NVERSION " (" NEWSPOSTURL ")"

struct postsocket {
  time_t last_action_time;
  /* bool error; */

  int sockfd;
  char *inbuf;
  char *endbuf;
  char *inptr;
  char *line_buffer;

  unsigned int bytes_read;
  unsigned int bytes_written;
};

struct headerinfo {
  char *from;
  char *newsgroups;
  char *subject;
  char *organization;
  char *reference;
};

struct filell {
  struct stat fileinfo;
  struct filell *next;
  crc_t crc;
  char filename[1024];
};

/* Defined in nntp.c: */
void posttext(const char *filename, struct headerinfo *header,
              struct postsocket *sock);
void postfile(struct filell *loc, FILE *myfile, int maxlines,
	      struct headerinfo *header, struct postsocket *sock, int check,
	      const char *stripped, bool makesfv);
bool nntpauth(struct postsocket *sock, const char *username,
	      const char *password);

/* Defined in socket.c: */
int socket_create(struct postsocket *sock, const char *address, int port);
int socket_getline(struct postsocket *sock);
int socket_putstring(struct postsocket *sock, const char *line);
int socket_putline(struct postsocket *sock, const char *line);
int read_from_socket (struct postsocket *ps);

/* Defined in utils.c: */
const char *strippath(const char *filename);
struct filell *checkfiles(struct filell *firstfile);
struct filell *addfile(const char *myfile, struct filell *first);
struct filell *freefilell(struct filell *first, struct filell *tofree);

/* Defined in uu.c: */
void encode_and_post(struct postsocket *sock, struct filell *loc,
		     struct headerinfo *header, int maxlines,
		     struct filell *zerofile, bool makesfv);

/* Defined in uuencode.c: */
void encode(struct filell *loc, FILE *infile, struct postsocket *sock,
	    int maxlines, bool lastfile, bool makesfv);

/* Defined in sfv.c: */
void newsfv(struct filell *firstfile, const char *sfvname);
void crc32(struct filell *loc, const char *buf, int len);

/* Defined in newspost.c: */
int notblank(const char *source);
