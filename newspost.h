/* Newspost - a binary autoposter for unix

   Copyright (C) 2000 Jim Faulkner <jfaulkne@ccs.neu.edu>

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
#ifdef OSF
#include <sys/time.h>
#endif
#ifndef AIX
#include <sys/timex.h>
#endif

/* these are the only things you should change */

#define ORGANIZATION "My Organization"
#define READS_UNTIL_FAIL 10

/* number of seconds it waits before posting the files */
#define SLEEPTIME 10

/* that's it, please leave the rest alone */

#define bool int
#define TRUE 1
#define FALSE 0

#define BPL 45 /* bytes per line */

#define BUFSIZE 8192

#define debug1 //
#define debug2 //

#ifdef DEBUG1
#undef debug1
#define debug1 printf
#endif

#ifdef DEBUG2
#undef debug1
#undef debug2 
#define debug1 printf
#define debug2 printf
#endif

#define ISFIRST 1
#define ISLAST 2
#define ISBOTH 3

#define NVERSION "1.13"

struct postsocket{
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

struct headerinfo{
  char *from;
  char *newsgroups;
  char *subject;
  char *organization;
  char *useragent;
};

struct uuprogress{
  int action;
  int curfile;
  int partno;
  int numparts;
  int percent;
  int fsize;
};

struct filell{
  struct stat fileinfo;
  struct filell *next;
  char filename[1024];
};

