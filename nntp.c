/* Newspost 1.12

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

#include "newspost.h"

void postfile(struct headerinfo *header,const char *filename,
	      struct postsocket *sock, int check, char *stripped);

bool nntpauth(struct postsocket *sock, char *username,char *password);

extern char *reference;

void postfile(struct headerinfo *header,const char *filename,
	      struct postsocket *sock, int check, char *stripped){
  char thisline[1024];
  FILE *tobeposted;

  socket_putstring(sock,"POST");
  socket_getline(sock);
  /* debug1("%s",sock->line_buffer); */
  if(strncmp(sock->line_buffer,"340",3)!=0){
    printf("%s",sock->line_buffer);
  }

  socket_putline(sock,header->from);
  socket_putline(sock,header->newsgroups);
  socket_putline(sock,header->subject);
  socket_putline(sock,header->organization);
  socket_putline(sock,header->useragent);
  if(reference != NULL){
    socket_putline(sock,reference);
  }

  socket_putline(sock,"\r\n");

  if((check == ISFIRST)
     || (check == ISBOTH)) {
    sprintf(thisline,"begin 644 %s\n",stripped);
    socket_putline(sock,thisline);
  }

  tobeposted = fopen(filename,"r");
  if(tobeposted == NULL){
    printf("Error opening %s: \n%s\n",filename,strerror(errno));
    exit(0);
  }

  clearerr(tobeposted);

  while(fgets(thisline,1024,tobeposted) != NULL){
    sprintf(thisline,"%s",thisline);
    socket_putline(sock,thisline);
  }

  if((check == ISLAST)
     || (check == ISBOTH)) {
    sprintf(thisline,"end\n");
    socket_putline(sock,thisline);
  }

  socket_putline(sock,"\r\n.\r\n");
  socket_getline(sock);
  /* debug1("%s",sock->line_buffer); */
  if(strncmp(sock->line_buffer,"240",3)!=0){
    printf("%s",sock->line_buffer);
  }
  fclose(tobeposted);
}

/* unused in this version 
void printnntphelp(struct postsocket *sock){
  socket_putstring(sock,"HELP");
  socket_getline(sock);
  printf("%s",sock->line_buffer);
  while(strcmp(sock->line_buffer,".\r\n")!=0){
    socket_getline(sock);
    printf("%s",sock->line_buffer);
  }
}
*/

bool nntpauth(struct postsocket *sock, char *username,char *password){
  bool retval;
  char usercmd[256];
  char passcmd[256];

  sprintf(usercmd,"AUTHINFO USER %s",username);
  sprintf(passcmd,"AUTHINFO PASS %s",password);
  retval = TRUE;

  socket_putstring(sock,usercmd);
  socket_getline(sock);
  /* debug1 (sock->line_buffer); */
  if(strncmp(sock->line_buffer,"381",3)!=0){
    if(strncmp(sock->line_buffer,"281",3)!=0){
      if(strncmp(sock->line_buffer,"500",3)!=0) retval = FALSE;
    }
  }
  socket_putstring(sock,passcmd);
  socket_getline(sock);
  /* debug1 (sock->line_buffer); */
  if(strncmp(sock->line_buffer,"281",3)!=0){
    if(strncmp(sock->line_buffer,"381",3)!=0){
      if(strncmp(sock->line_buffer,"500",3)!=0) retval = FALSE;
    }
  }
  return retval;
}

