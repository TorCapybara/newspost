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

int encode_and_post(struct postsocket *sock,const char *filename,struct headerinfo *header,int maxlines,struct filell *zerofile);

int encode_and_post(struct postsocket *sock,const char *filename,
                    struct headerinfo *header,
                    int maxlines,struct filell *zerofile){
  FILE *myfile;
  char tmpfilename[1024];
  char strippedfilename[512];
  FILE *tmpfile = NULL;
  int i=1;
  struct headerinfo tmpheader; 
  struct stat fileinfo;
  int numchunks = 1;
  char numchunkbuf[21]; /* big enough for 2^64 */
  int numchunksize;

  stat(filename,&fileinfo);

  if(fileinfo.st_size > (BPL * maxlines)){
    if((fileinfo.st_size % (BPL * maxlines)) == 0)
      numchunks = (fileinfo.st_size/(BPL * maxlines));
    else
      numchunks = ((fileinfo.st_size/(BPL * maxlines)) + 1);
  }
  sprintf(numchunkbuf, "%d", numchunks);
  numchunksize=strlen(numchunkbuf);

  tmpheader.from = calloc(1024,sizeof(char));
  tmpheader.newsgroups = calloc(1024,sizeof(char));
  tmpheader.subject = calloc(1024,sizeof(char));
  tmpheader.organization = calloc(1024,sizeof(char));
  tmpheader.useragent = calloc(1024,sizeof(char));

  sprintf(tmpheader.from,"%s",header->from);
  sprintf(tmpheader.newsgroups,"%s",header->newsgroups);
  sprintf(tmpheader.subject,"%s",header->subject);
  sprintf(tmpheader.organization,"%s",header->organization);
  sprintf(tmpheader.useragent,"%s",header->useragent);

  strippath(filename,strippedfilename);

  /* debug1("Posting %s\n",filename); */

  myfile = fopen(filename,"r");
  if(myfile == NULL){
    printf("Error opening %s: \n%s\n",filename,strerror(errno));
    exit(0);
  }
 
  while((feof(myfile) == 0) && i <= numchunks){
    sprintf(tmpfilename,"/tmp/newspost.tmp.%s.%d",strippedfilename,i);
  
    tmpfile = fopen(tmpfilename,"w");
    if(tmpfile == NULL){
      printf("Error creating temporary file %s: \n%s\n",tmpfilename,strerror(errno));
      exit(0);
    }
    /* encode the chunk */
    if (i == numchunks) encode(myfile,tmpfile,maxlines,TRUE);
    else encode(myfile,tmpfile,maxlines,FALSE);

    fclose(tmpfile);
    /* post text prefix if there is one */
    if( (zerofile != NULL)
        && (i == 1)){
      sprintf(tmpheader.subject,"%s - %s (%0*d/%d)\r\n",header->subject,strippedfilename,numchunksize,0,numchunks);
      postfile(&tmpheader,zerofile->filename,sock,NULL,NULL);
      printf("Posted %s as text\n",zerofile->filename);
    }
    sprintf(tmpheader.subject,"%s - %s (%0*d/%d)\r\n",header->subject,strippedfilename,numchunksize,i,numchunks);
    /* post it! */
    if((i==1) && (i != numchunks)) postfile(&tmpheader,tmpfilename,sock,ISFIRST,strippedfilename);
    else if (i==1) postfile(&tmpheader,tmpfilename,sock,ISBOTH,strippedfilename);
    else if (i==numchunks) postfile(&tmpheader,tmpfilename,sock,ISLAST,NULL);
    else postfile(&tmpheader,tmpfilename,sock,NULL,NULL);
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

