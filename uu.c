/* Newspost 1.0

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
#include "uulib/uudeview.h"

void BusyCallBack (void *opaque,uuprogress *uuinfo);

int encode_and_post(struct postsocket *sock,const char *filename,struct headerinfo *header,int maxlines,struct filell *zerofile);

int numchunks = 1;

void BusyCallBack (void *opaque,uuprogress *uuinfo){
  numchunks = uuinfo->numparts;
  UUSetBusyCallback(&numchunks,NULL,0);
}

int encode_and_post(struct postsocket *sock,const char *filename,
                    struct headerinfo *header,
                    int maxlines,struct filell *zerofile){
  FILE *myfile;
  char tmpfilename[1024];
  char strippedfilename[512];
  FILE *tmpfile = NULL;
  int i=1;
  struct headerinfo tmpheader; 

  numchunks = 1;

  tmpheader.from = calloc(1024,sizeof(char));
  tmpheader.newsgroups = calloc(1024,sizeof(char));
  tmpheader.subject = calloc(1024,sizeof(char));
  tmpheader.organization = calloc(1024,sizeof(char));
  tmpheader.useragent = calloc(1024,sizeof(char));

  sprintf(tmpheader.from,header->from);
  sprintf(tmpheader.newsgroups,header->newsgroups);
  sprintf(tmpheader.subject,header->subject);
  sprintf(tmpheader.organization,header->organization);
  sprintf(tmpheader.useragent,header->useragent);

  strippath(filename,strippedfilename);

  /* debug1("Posting %s\n",filename); */

  myfile = fopen(filename,"r");
  if(myfile == NULL){
    printf("Error opening %s: \n%s\n",filename,strerror(errno));
    exit(0);
  }
 
  while((feof(myfile) == 0) && i <= numchunks){
    if(i==1){
      UUSetBusyCallback(&numchunks,BusyCallBack,0);
    }
    sprintf(tmpfilename,"/tmp/newspost.tmp.%s.%d",strippedfilename,i);
    tmpfile = fopen(tmpfilename,"w");
    if(tmpfile == NULL){
      printf("Error creating temporary file %s: \n%s\n",tmpfilename,strerror(errno));
      exit(0);
    }
    /* encode the chunk */
    UUEncodePartial(tmpfile,myfile,NULL,UU_ENCODED,
                    strippedfilename,NULL,NULL,i,maxlines);
    fclose(tmpfile);
    /* post text prefix if there is one */
    if( (zerofile != NULL)
        && (i == 1)){
      sprintf(tmpheader.subject,"%s - %s (0/%d)\r\n",header->subject,strippedfilename,numchunks);
      postfile(&tmpheader,zerofile->filename,sock,TRUE);
      printf("Posted %s as text\n",zerofile->filename);
    }
    sprintf(tmpheader.subject,"%s - %s (%d/%d)\r\n",header->subject,strippedfilename,i,numchunks);
    /* post it! */
    if(i==1) postfile(&tmpheader,tmpfilename,sock,FALSE);
    else postfile(&tmpheader,tmpfilename,sock,TRUE);
    printf("%1s","#");
    fflush(stdout);
    unlink(tmpfilename);
    i++;
  }
  printf("\n");
  fclose(myfile);
  free(tmpheader.from);
  free(tmpheader.newsgroups);
  free(tmpheader.subject);
  free(tmpheader.organization);
  free(tmpheader.useragent);
}

