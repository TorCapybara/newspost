/* Newspost 1.20

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

#include "newspost.h"

void postheader(struct headerinfo *header, struct postsocket *sock) {
  char thisline[1024];

  socket_putstring(sock, "POST");
  socket_getline(sock);

#ifdef DEBUG1
  printf("%s", sock->line_buffer);
#endif

  if (strncmp(sock->line_buffer, "340", 3) !=0 )
    printf("%s", sock->line_buffer);

  sprintf(thisline, "From: %s\r\n", header->from);
  socket_putline(sock, thisline);
  sprintf(thisline, "Newsgroups: %s\r\n", header->newsgroups);
  socket_putline(sock, thisline);
  sprintf(thisline, "Subject: %s\r\n", header->subject);
  socket_putline(sock, thisline);
  if (notblank(header->organization)) {
    sprintf(thisline, "Organization: %s\r\n", header->organization);
    socket_putline(sock, thisline);
  }
  sprintf(thisline, "User-Agent: " USERAGENT "\r\n");
  socket_putline(sock, thisline);
  if (notblank(header->reference)) {
    sprintf(thisline, "References: %s\r\n", header->reference);
    socket_putline(sock, thisline);
  }

  socket_putline(sock, "\r\n");
}

void postfinish(struct postsocket *sock) {
  socket_putline(sock, "\r\n.\r\n");
  socket_getline(sock);

#ifdef DEBUG1
  printf("%s",sock->line_buffer);
#endif

if (strncmp(sock->line_buffer, "240", 3) != 0)
    printf("%s", sock->line_buffer);
}

void posttext(const char *filename, struct headerinfo *header,
	      struct postsocket *sock) {
  char thisline[1024];
  FILE *tobeposted;

  tobeposted = fopen(filename, "r");
  if (tobeposted == NULL) {
    printf("Error opening %s: \n%s\n", filename, strerror(errno));
    exit(0);
  }

  clearerr(tobeposted);

  postheader(header, sock);

  thisline[0] = '.';
  while (fgets(thisline + 1, 1023, tobeposted) != NULL) {
    char *end = thisline + strlen(thisline) - 1;
    while (('\n' == *end) || ('\r' == *end))
      *end-- = '\0';
    *++end = '\r';
    *++end = '\n';
    *++end = '\0';
    socket_putline(sock, thisline + ('.' != thisline[1]));
  }

  postfinish(sock);

  fclose(tobeposted);
}

void postfile(struct filell *loc, FILE *myfile, int maxlines,
	      struct headerinfo *header, struct postsocket *sock, int check,
	      const char *stripped, bool makesfv) {
  char thisline[1024];

  postheader(header, sock);

  if ((check == ISFIRST) || (check == ISBOTH)) {
    sprintf(thisline, "begin 644 %s\r\n", stripped);
    socket_putline(sock, thisline);
  }

  encode(loc, myfile, sock, maxlines,
	((check == ISLAST) || (check == ISBOTH)), makesfv);

  if ((check == ISLAST) || (check == ISBOTH))
    socket_putline(sock, "end\r\n");

  postfinish(sock);
}

/* unused in this version 
void printnntphelp(struct postsocket *sock) {
  socket_putstring(sock,"HELP");
  socket_getline(sock);
  printf("%s", sock->line_buffer);
  while (strcmp(sock->line_buffer, ".\r\n") != 0) {
    socket_getline(sock);
    printf("%s", sock->line_buffer);
  }
}
*/

bool nntpauth(struct postsocket *sock, const char *username,
	      const char *password) {
  bool retval;
  char usercmd[256];
  char passcmd[256];

  sprintf(usercmd, "AUTHINFO USER %s", username);
  sprintf(passcmd, "AUTHINFO PASS %s", password);
  retval = TRUE;

  socket_putstring(sock, usercmd);
  socket_getline(sock);

#ifdef DEBUG1
  printf(sock->line_buffer);
#endif

  if (strncmp(sock->line_buffer, "381", 3) != 0) {
    if (strncmp(sock->line_buffer, "281", 3) != 0) {
      if (strncmp(sock->line_buffer, "500", 3) != 0) retval = FALSE;
    }
  }
  socket_putstring(sock, passcmd);
  socket_getline(sock);

#ifdef DEBUG1
  printf(sock->line_buffer);
#endif

  if (strncmp(sock->line_buffer, "281", 3) !=0 ) {
    if (strncmp(sock->line_buffer, "381", 3) !=0 ) {
      if (strncmp(sock->line_buffer, "500", 3) !=0 ) retval = FALSE;
    }
  }
  return retval;
}

