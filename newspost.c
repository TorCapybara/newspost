
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

struct filell *parseargs(int argc, char **argv, 
               struct headerinfo *header, 
               char *address, int *port, 
               char *username, char *password,
               bool *istext, bool *setasdefault,
	       int *maxlines, struct filell **zerofile,
               char *sfvname);

void printhelp(struct headerinfo *header,
               char *address, int *port,
               char *username, char *password,
               int *maxlines);

void needarg(const char *myarg);

bool writedefaults(struct headerinfo *header,
                   char *address,int *port,
                   char *username,char *password,
                   int *maxlines);

void getdefaults(struct headerinfo *header,
                 char *address,int *port,
                 char *username,char *password,
                 int *maxlines);

extern struct filell *checkfiles(struct filell *firstfile);

extern struct filell *addfile(const char *myfile, struct filell *first);

extern struct filell *freefilell(struct filell *first,struct filell *tofree);

int main(int argc, char **argv){
  struct postsocket sock;
  int port;
  int maxlines;
  struct headerinfo header;
  char unstripped[1024];
  bool istext = FALSE;
  bool setasdefault = FALSE;
  float totalsize = 0;
  int numfiles = 0;
  int begintime = 0;
  float totaltime = 0;
  struct filell *firstfile = NULL;
  struct filell *loc = firstfile;
  struct filell *zerofile = NULL;
  struct filell *oldfirstfile = NULL;
  char *sfvname = calloc(1024,sizeof(char)); 
  char *address = calloc(1024,sizeof(char));
  char *username = calloc(1024,sizeof(char));
  char *password = calloc(1024,sizeof(char));
  header.from = calloc(1024,sizeof(char));
  header.newsgroups = calloc(1024,sizeof(char));
  header.subject = calloc(1024,sizeof(char));
  header.organization = calloc(1024,sizeof(char));
  header.useragent = calloc(1024,sizeof(char));

  getdefaults(&header,address,&port,username,password,&maxlines);

  firstfile = parseargs( argc, argv, &header, 
                         address, &port, 
                         username, password, 
                         &istext, &setasdefault, 
                         &maxlines, &zerofile,
                         sfvname );

  if(setasdefault == TRUE){ 
    if(writedefaults(&header,
                  address,&port,
                  username,password,&maxlines)==FALSE){
      printf("Error writing ~/.newspost\n");
    }
    else{
      printf("Defaults written to ~/.newspost\n");
    }
  }

  firstfile = checkfiles(firstfile);

  zerofile = checkfiles(zerofile);

  if(firstfile == NULL){
    printf("No files to post!\n");
    exit(0);
  }
  if( (istext == TRUE) 
      && (firstfile->next != NULL) ){
    printf("Only one text file may be posted at a time.\n");
    exit(0);
  }
  if( (istext == TRUE)
      && (zerofile != NULL) ){
    printf("Only one text file may be posted at a time.\n");
    exit(0);
  }
  if( (header.subject == NULL)
      || (strncmp(header.subject,"Subject: ",9)!=0)){
    printf("There is no subject line.\n");
    exit(0);
  }
  if( (header.organization == NULL)
      || (strncmp(header.organization,"Organization: ",14)!=0)){
    printf("There is no organization line.\n");
    exit(0);
  }
  if( (header.from == NULL)
      || (strncmp(header.from,"From: ",6)!=0)){
    printf("There is no from line.\n");
    exit(0);     
  }
  if( (header.newsgroups == NULL)
      || (strncmp(header.newsgroups,"Newsgroups: ",12)!=0)){
    printf("There is no newsgroups line.\n");
    exit(0);
  }
  if( (address == NULL)
      || (strcmp(address,"")==0)
      || (strcmp(address,"\n")==0)
      || (strcmp(address,"\r\n")==0)){
    printf("You must provide a server name or set the NNTPSERVER environment variable.\n");
    exit(0);
  }

  loc = firstfile;
  while(loc != NULL){
    totalsize += loc->fileinfo.st_size;
    numfiles++;
    loc = loc->next;
  }

  printf("\n");
  printf("%s",header.from);
  printf("%s",header.newsgroups);
  printf("%s\n",header.subject);
  printf("%s",header.organization);
  printf("\n");
  if(istext != TRUE){
    if(totalsize < (1024*1024))
      printf("Posting %.0f bytes in %d files.\n",totalsize,numfiles);
    else
      printf("Posting %.1f megabytes in %d files.\n",((totalsize/1024)/1024),numfiles);
  }
  else
    printf("Posting %s as body of message.\n",firstfile->filename);
  printf("Posting will begin in %d seconds.\n",SLEEPTIME);
  sleep(SLEEPTIME);
  printf("\n");

  socket_create(&sock,address,port);
  socket_getline(&sock);
  /* debug1("%s",sock.line_buffer); */

  if( ((username != NULL) 
      && (strcmp(username,"")!=0)
      && (strcmp(username,"\n")!=0)
      && (strcmp(username,"\r\n")!=0))
     ||
      ((password != NULL)
      && (strcmp(password,"")!=0)
      && (strcmp(password,"\n")!=0)
      && (strcmp(password,"\r\n")!=0))){
    if(nntpauth(&sock,username,password)==FALSE){
      printf("Incorrect username or password.\n");
      exit(0);
    }
  }

  if(istext == TRUE){
    printf("Posting %s as text.\n",firstfile->filename);
    postfile(&header,firstfile->filename,&sock,TRUE);
    printf("\n");
    printf("Posted %s\n",firstfile->filename);
  }
  else{
    totalsize = 0;
    UUInitialize();
    oldfirstfile = firstfile;
    while(firstfile != NULL){
      begintime = time(0);
      encode_and_post(&sock,firstfile->filename,&header,maxlines,zerofile);
      zerofile = NULL;
      begintime = (time(0) - begintime);
      totaltime += begintime;
      totalsize += firstfile->fileinfo.st_size;
      if(begintime != 0)
	printf("Posted %s (%d bytes) in %d seconds (%d KB/second)\n",firstfile->filename,firstfile->fileinfo.st_size,begintime, ((firstfile->fileinfo.st_size/begintime)/1024) );
      else
	printf("Posted %s (%d bytes) in 0 seconds (very quickly)\n",firstfile->filename,firstfile->fileinfo.st_size);
      firstfile = firstfile->next;
    }

    if(sfvname != NULL
       && (strcmp(sfvname,"")!=0)
       && (strcmp(sfvname,"\n")!=0)
       && (strcmp(sfvname,"\r\n")!=0)){
      sprintf(unstripped,"/tmp/%s",sfvname);
      newsfv(oldfirstfile,unstripped);
      encode_and_post(&sock,unstripped,&header,maxlines,zerofile);
      unlink(unstripped); 
      printf("Posted %s\n",sfvname); 
    }

    printf("\n");
    if(totaltime != 0)
      printf("Posted %d files.\nTotal %.0f bytes in %.0f minutes (%.0f KB/second)\n",numfiles,totalsize,(totaltime/60),((totalsize/totaltime)/1024));
    else
      printf("Posted %d files.\nTotal %.0f bytes in 0 minutes (really quickly)\n",numfiles,totalsize);
    UUCleanUp();
  }
  /* This stuff has yet to be free()'d */
  /* but hey, we're exiting anyway */
  /*
  while(oldfirstfile != NULL){
    oldfirstfile = freefilell(oldfirstfile,oldfirstfile);
  }
  free(password);
  free(address);
  free(username);
  free(header.subject);
  free(header.newsgroups);
  free(header.from);
  free(header.organization);
  free(header.useragent);
  */
  return 0;
}

struct filell *parseargs(int argc, char **argv, 
               struct headerinfo *header, 
               char *address, int *port, 
               char *username, char *password,
               bool *istext, bool *setasdefault,
               int *maxlines, struct filell **zerofile,
               char *sfvname){
  struct filell *firstfile = NULL;
  int i = 1;
  if(argc==1) printhelp(header,address,port,
                        username,password,maxlines);
  while(i<argc){
    if(strcmp(argv[i],"-s")==0){
      /* get the subject line */
      if((i+1)==argc) needarg("-s");
      i++;
      sprintf(header->subject,"Subject: %s",argv[i]);
    }
    else if(strcmp(argv[i],"-n")==0){
      /* get the newsgroups line */
      if((i+1)==argc) needarg("-n");
      i++;
      sprintf(header->newsgroups,"Newsgroups: %s\r\n",argv[i]);
    }
    else if( (strcmp(argv[i],"-h")==0)
            || (strcmp(argv[i],"-?")==0))
	   printhelp(header,address,port,
                     username,password,maxlines);
    else if(strcmp(argv[i],"-f")==0){
      /* get the from line */
      if((i+1)==argc) needarg("-f");
      i++;
      sprintf(header->from,"From: %s\r\n",argv[i]);
    }
    else if(strcmp(argv[i],"-o")==0){
      /* get the organization line */
      if((i+1)==argc) needarg("-o");
      i++;
      sprintf(header->organization,"Organization: %s\r\n",argv[i]);
    }
    else if(strcmp(argv[i],"-d")==0){
      /* set as the default */
      *setasdefault = TRUE;
    }
    else if(strcmp(argv[i],"-t")==0){
      /* post a text message */
      *istext = TRUE; 
    }
    else if(strcmp(argv[i],"-u")==0){
      /* get the username */
      if((i+1)==argc) needarg("-u");
      i++;
      strcpy(username,argv[i]);
    }
    else if(strcmp(argv[i],"-p")==0){
      /* get the password */
      if((i+1)==argc) needarg("-p");
      i++;
      strcpy(password,argv[i]);
    }
    else if(strcmp(argv[i],"-i")==0){
      /* get the server name or ip address */
      if((i+1)==argc) needarg("-i");
      i++;
      strcpy(address,argv[i]);
    }
    else if(strcmp(argv[i],"-z")==0){
      /* get the port number */
      if((i+1)==argc) needarg("-z");
      i++;
      *port = atol(argv[i]);
    }
    else if(strcmp(argv[i],"-l")==0){
      /* get the maximum number of lines */
      if((i+1)==argc) needarg("-l");
      i++;
      *maxlines = atol(argv[i]);
    }
    else if(strcmp(argv[i],"-0")==0){
      /* get the zero file */
      if((i+1)==argc) needarg("-0");
      i++;
      *zerofile = addfile(argv[i],*zerofile);
    }
    else if(strcmp(argv[i],"-v")==0){
      /* get the sfv file */
      if((i+1)==argc) needarg("-v");
      i++;
      strcpy(sfvname,argv[i]);
    }
    else{
      firstfile = addfile(argv[i], firstfile); 
    }
    i++;
  }
  return firstfile; 
}

void printhelp(struct headerinfo *header,
               char *address, int *port,
               char *username, char *password,
               int *maxlines){
  printf("Usage: newspost [OPTIONS [ARGUMENTS]] file1 file2 file3...\n");
  printf("Options:\n");
  printf("  -h, -?  - prints help.\n");
  printf("  -s SUBJ - subject for all of your posts.\n");
  if( (header->newsgroups != NULL)
      && (strcmp(header->newsgroups,"")!=0)
      && (strcmp(header->newsgroups,"\n")!=0)
      && (strcmp(header->newsgroups,"\r\n")!=0)){
    printf("  -n NWSG - newsgroups to post to, seperated by commas. Default %s",header->newsgroups);
  }
  else printf("  -n NWSG - newsgroups to post to, seperated by commas.\n");

  if( (header->from != NULL)
      && (strcmp(header->from,"")!=0)
      && (strcmp(header->from,"\n")!=0)
      && (strcmp(header->from,"\r\n")!=0)){
    printf("  -f FROM - your e-mail address. Default %s",header->from); 
  }
  else printf("  -f FROM - your e-mail address.\n");
  if( (header->organization != NULL)
      && (strcmp(header->organization,"")!=0)
      && (strcmp(header->organization,"\n")!=0)
      && (strcmp(header->organization,"\r\n")!=0)){
    printf("  -o ORGN - your organization. Default %s",header->organization);
  }
  else printf("  -o ORGN - your organization.\n");
  if( (address != NULL)
      && (strcmp(address,"")!=0)
      && (strcmp(address,"\n")!=0)
      && (strcmp(address,"\r\n")!=0)){
    printf("  -i ADDR - hostname or ip address of your news server. Default Server: %s\n",address);
  }
  else printf("  -i ADDR - hostname or ip address of your news server.\n");
  printf("  -z PORT - port number of the NNTP server if nonstandard. Default Port: %d\n",*port);
 
  if( (username != NULL)
      && (strcmp(username,"")!=0)
      && (strcmp(username,"\n")!=0)
      && (strcmp(username,"\r\n")!=0)){
    printf("  -u USER - username, only used if authorization needed. Default Username: %s\n",username);
  }
  else printf("  -u USER - username, only used if authorization needed.\n");
  
  if( (password != NULL)
      && (strcmp(password,"")!=0)
      && (strcmp(password,"\n")!=0)
      && (strcmp(password,"\r\n")!=0)){
    printf("  -p PASS - password, only used if authorization needed. Default Password: %s\n",password); 
  }
  else printf("  -p PASS - password, only used if authorization needed.\n");
  printf("  -l LNES - maximum number of lines per post. Default Maximum Lines: %d\n",*maxlines);
  printf("  -0 FILE - FILE is posted as text as a prefix (0/xx) to the first file posted.\n");
  printf("  -v NAME - create and post as a sfv checksum file.\n");
  printf("  -d      - set current options as your default.\n");
  printf("  -t      - post as text (no uuencoding). only one file may be posted.\n");
  printf("Examples:\n");
  printf("  $ newspost -f you@yourbox.yourdomain -o \"Your Organization\" -i news.yourisp.com -n alt.binaries.sounds.mp3 -d\n");
  printf("  $ newspost -s \"This is me singing\" -0 musicinfo.txt *.mp3 -v mysongs.sfv\n");
  printf("  $ newspost -s \"Some pics of my kitty\" -n alt.binaries.pets.cats kitty1.jpg kitty2.jpg kitty3.jpg\n");
  exit(0);
}

void needarg(const char *myarg){
  printf("The %s option requires an argument.\n",myarg);
  exit(0);
}

bool writedefaults(struct headerinfo *header,
                   char *address,int *port,
                   char *username,char *password,
                   int *maxlines){
  char defaultsfile[1024];
  char tmp[1024];
  FILE *myfile;
  char *homedir;

  homedir = getenv("HOME");
  if(homedir != NULL) sprintf(defaultsfile,"%s/.newspost",homedir);
  else sprintf(defaultsfile,"~/.newspost");
  myfile = fopen(defaultsfile,"w");
  if (myfile == NULL) return FALSE;
  sprintf(tmp,"%s",(header->from+6));
  if(header->from[1] != 0) fprintf(myfile,"%s",tmp);
  else fprintf(myfile,"\r\n");
  sprintf(tmp,"%s",(header->organization+14));
  if(header->organization[1] != 0) fprintf(myfile,"%s",tmp);
  else fprintf(myfile,"\r\n");
  sprintf(tmp,"%s",(header->newsgroups+12));
  if(header->newsgroups[1] != 0) fprintf(myfile,"%s",tmp);
  else fprintf(myfile,"\r\n");
  fprintf(myfile,"%s\r\n",address);
  fprintf(myfile,"%d\r\n",*port);
  fprintf(myfile,"%s\r\n",username);
  fprintf(myfile,"%s\r\n",password);
  fprintf(myfile,"%d\r\n",*maxlines);
  fclose(myfile);
  return TRUE;
}

void getdefaults(struct headerinfo *header,
                 char *address,int *port,
                 char *username,char *password,
                 int *maxlines){
  char *homedir;
  char defaultsfile[1024];
  FILE *myfile;
  char thisline[1024];
  int i = 0;
  int j = 0;
  char *loc;
  char *envvar = NULL;
  char *tmp1 = NULL;
  char *tmp2 = NULL;

  sprintf(header->useragent,"User-Agent: newspost v%s\r\n",NVERSION);
  sprintf(header->organization,"Organization: %s\r\n",ORGANIZATION);
  sprintf(header->subject,"");
  sprintf(header->from,"");
  sprintf(header->newsgroups,"");
  envvar = getenv("NNTPSERVER");
  if(envvar == NULL) sprintf(address,"");
  else strcpy(address,envvar);
  tmp1 = getenv("USER");
  tmp2 = getenv("USERNAME");
  envvar = getenv("HOSTNAME");
  if( (tmp1 != NULL)
      && (envvar != NULL)){
    if(tmp2 != NULL)
      sprintf(header->from,"From: %s <%s@%s>\r\n",tmp2,tmp1,envvar);
    else
      sprintf(header->from,"From: %s@%s\r\n",tmp1,envvar);
  }
  *port = 119;
  *maxlines = 7500;

  homedir = getenv("HOME");
  if(homedir != NULL) sprintf(defaultsfile,"%s/.newspost",homedir);
  else sprintf(defaultsfile,"~/.newspost");
  myfile = fopen(defaultsfile,"r");
 
  if(myfile != NULL){
    while(i<8){
      fgets(thisline,1024,myfile);
      if( (strcmp(thisline,"")!=0)
         && (strcmp(thisline,"\n")!=0)
         && (strcmp(thisline,"\r\n")!=0)){

        if(i==0) sprintf(header->from,"From: %s",thisline);
        else if(i==1) sprintf(header->organization,"Organization: %s",thisline);
        else if(i==2) sprintf(header->newsgroups,"Newsgroups: %s",thisline);
        else if(i==3){
          loc = thisline;
          while( (*loc != '\r') 
                && (*loc != '\n')){
            loc++;
            j++;
          }
          strncpy(address,thisline,j);
        }
        else if(i==4) *port = atol(thisline);
        else if(i==5){
          j = 0;
          loc = thisline;
          while( (*loc != '\r')
                && (*loc != '\n')){
            loc++;
            j++;
          }
          strncpy(username,thisline,j);
        } 
        else if(i==6){
          j = 0;
          loc = thisline;
          while( (*loc != '\r')
                && (*loc != '\n')){
            loc++;
            j++;
          }
          strncpy(password,thisline,j);
        }
        else if(i==7) *maxlines = atol(thisline);
      }
      i++;
    }
    fclose(myfile);
  }
}

