/* Newspost 1.20

   Copyright (C) 2000 Jim Faulkner <newspost@unixcab.org>

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

#include "newspost.h"

int badreads = 0;

int socket_create(struct postsocket *sock, const char *address, int port) {
  const char *error_str; /* used for some error checking */
  struct sigaction act, oact; /* used to prevent a crash in case of a
				 dead socket */
  /* use these to make the socket to the host */
  struct sockaddr_in serv_addr;
  struct in_addr inaddr;
  int on;
  struct hostent *hp;
  int sockfd = -1;

#if (defined(sun) || defined(__sun)) && (defined(__svr4) || \
defined(__SVR4) || defined(__svr4__))
  unsigned long tinaddr;
#endif

  sock->sockfd = -1;
  sock->bytes_read = 0;
  sock->bytes_written = 0;
  sock->line_buffer = malloc(BUFSIZE);
  sock->inbuf = malloc(BUFSIZE);
  sock->endbuf = sock->inbuf;
  sock->inptr = sock->inbuf;
  
  memset ( &serv_addr, 0, sizeof(serv_addr) );
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);

  /* Try to convert host name as dotted decimal */
#if (defined(sun) || defined(__sun)) && (defined(__svr4) || \
defined(__SVR4) || defined(__svr4__))
  tinaddr = inet_addr(address);
  if ( tinaddr != -1 )
#else
  if ( inet_aton(address, &inaddr) != 0 ) 
#endif
  {
    memcpy(&serv_addr.sin_addr, &inaddr, sizeof(inaddr));
  }
  else /* If that failed, then look up the host name */
  {
    while (NULL == (hp = gethostbyname(address))) {
      error_str = NULL;

      if (errno)
      	error_str = strerror(errno);

      printf ("Can't resolve %s -- Socket not created\n", address
       		/*	(error_str ? error_str : "") */ );
      exit(0);
    }
    memcpy(&serv_addr.sin_addr, hp->h_addr, hp->h_length);
  }
  sockfd = socket(AF_INET, SOCK_STREAM, 0);

  if (sockfd < 0) {
    printf ("Can't create socket: %s", strerror(errno));
    exit(0);
  }

  if (connect (sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
    close (sockfd);
    printf ("socket connect to %s, port %d failed: %s",
      address, port, strerror(errno));
    exit(0);
  }

  on = 1;
  if (setsockopt (sockfd, SOL_SOCKET, SO_KEEPALIVE,
    (char*)&on, sizeof(on)) < 0) {
      close(sockfd);
      printf ("socket keepalive option failed: %s", strerror(errno));
      exit(0);
  }

  sock->sockfd = sockfd;

  /* If we write() to a dead socket, then let write return EPIPE rather
     than crashing the program. */
  act.sa_handler = SIG_IGN;
  sigemptyset (&act.sa_mask);
  act.sa_flags = 0;
  sigaction (SIGPIPE, &act, &oact);

#ifdef DEBUG1
  printf("socket created: %p for %s, port %d, sockfd %d\n", 
    &sock, address, port, sockfd);
#endif
  return 0;
}

/*
 * get_server - read from a socket until \n
 */
int socket_getline (struct postsocket *sock) {

  char *src, *dest;
  bool done = FALSE;

  sock->last_action_time = time(0);

  while (!done) {
    /* make sure we have text to read */
    if (sock->inptr == sock->endbuf) {

      errno = 0;

      if (read_from_socket(sock) < 0) {
	if (errno)
	  printf ("socket [%d] failed its read:\n%s\n",
		  sock->sockfd, strerror(errno));
	else
	  printf ("socket [%d] failed its read\n",
		  sock->sockfd);

	exit(0);
      }
	else badreads = 0;
    }
	  
    /* copy the data */
    for (src = sock->inptr, dest = sock->line_buffer;
      src != sock->endbuf; ++src) {
	*dest++ = *src;
        if (*src == '\n') {
	  done = TRUE;
	  ++src; /* keep the \n in the bytes_read counter */
	  break;
        }
    }
    *dest = '\0';

    /* update stats */
    sock->bytes_read += src - sock->inptr;
    sock->inptr = src;
  }

#ifdef DEBUG2
  printf("socket [%p] received [%s]", sock, (char*)sock->line_buffer);
#endif
  return 0;
}

/* appends "\r\n" to what is to be posted */
/* returns -1 if can't put the line, 0 otherwise */
int socket_putstring (struct postsocket *sock, const char *line) {

  char newline[1024];

  sprintf(newline, "%s\r\n", line);
  return socket_putline(sock, newline);
}

/* returns -1 if can't put the line, 0 otherwise */
int socket_putline (struct postsocket *sock, const char *line) {

  int nleft = strlen(line);

  sock->last_action_time = time(0);

#ifdef DEBUG2
  printf("[%ld][socket %p putline][%s]", time(0), (void*)sock, line);
#endif
  while (nleft > 0) {
    /* write */
    const int nwritten = write(sock->sockfd, line, nleft);
    if (nwritten == -1)
	return -1;

    sock->bytes_written += nwritten;
    nleft -= nwritten;
    line += nwritten;
  }

  return nleft;
}

/* simple read buffering to hopefully speed things up. */
int read_from_socket (struct postsocket* ps) {

  int read_count = 0;
  int retval = 0;
  struct timeval tv;
  fd_set rfds;
  FD_ZERO (&rfds);
  FD_SET (ps->sockfd, &rfds);
    
  /* We will timeout after 1 minute. */
  tv.tv_sec = 60;
  tv.tv_usec = 0;
  retval = select (ps->sockfd + 1, &rfds, NULL, NULL, &tv);

  if (retval <= 0) {
    badreads++;
    if (errno)
      printf ("select() failed: %s", strerror(errno));
    /* else
      printf ("Timed out waiting for select()\n"); */
    if (badreads >= READS_UNTIL_FAIL) return -1;
    else {
      /* printf("Retrying..."); */
      sleep(5);
      return read_from_socket(ps);
    }
  }
  read_count = read (ps->sockfd, ps->inbuf, BUFSIZE);
  ps->endbuf = ps->inbuf + read_count;
  ps->inptr = ps->inbuf;

  if (read_count <= 0)
    return read_count;

  return 0;
}
