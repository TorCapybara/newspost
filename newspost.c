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

struct filell *parseargs(int argc, char **argv, 
               struct headerinfo *header, 
               char *address, int *port, 
               char *username, char *password,
               bool *istext, bool *setasdefault,
	       int *maxlines, struct filell **zerofile,
               char *sfvname);

void printhelp(struct headerinfo *header,
               const char *address, int port,
               const char *username, const char *password,
               int maxlines);

bool writedefaults(struct headerinfo *header,
                   const char *address, int port,
                   const char *username, const char *password,
                   int maxlines);

void getdefaults(struct headerinfo *header,
                 char *address, int *port,
                 char *username, char *password,
                 int *maxlines);

int notblank(const char *source) {
  return ( ((source != NULL)
    && (*source != '\0')
    && (strcmp(source, "\n") != 0)
    && (strcmp(source, "\r\n") != 0)));
}			 

int main(int argc, char **argv) {
  struct postsocket sock;
  int port;
  int maxlines;
  struct headerinfo header;
  char unstripped[1024];

  bool istext = FALSE;
  bool setasdefault = FALSE;
  long totalsize = 0;
  int numfiles = 0;
  long begintime = 0;
  long totaltime = 0;

  struct filell *firstfile = NULL;
  struct filell *loc = firstfile;
  struct filell *zerofile = NULL;
  struct filell *oldfirstfile = NULL;

  char *sfvname = calloc(1024, sizeof(char)); 
  char *address = calloc(1024, sizeof(char));
  char *username = calloc(1024, sizeof(char));
  char *password = calloc(1024, sizeof(char));

  header.from = calloc(1024, sizeof(char));
  header.newsgroups = calloc(1024, sizeof(char));
  header.subject = calloc(1024, sizeof(char));
  header.organization = calloc(1024, sizeof(char));
  header.reference = calloc(1024, sizeof(char));
  
  getdefaults(&header, address, &port, username, password, &maxlines);

  firstfile = parseargs( argc, argv, &header, 
                         address, &port, 
                         username, password, 
                         &istext, &setasdefault, 
                         &maxlines, &zerofile,
                         sfvname );

  if (setasdefault == TRUE) {
    if (writedefaults(&header, address, port,
	username, password, maxlines) == FALSE)
      printf("Error writing ~/.newspost\n");
    else
      printf("Defaults written to ~/.newspost\n");
  }

  firstfile = checkfiles(firstfile);
  zerofile = checkfiles(zerofile);

  if (firstfile == NULL) {
    printf("No files to post!\n");
    exit(0);
  }
  if ( (istext == TRUE)
      && ((firstfile->next != NULL) || (zerofile != NULL)) ) {
    printf("Only one text file may be posted at a time.\n");
    exit(0);
  }
  if (
#ifndef FORCESUBJECT
    (istext == TRUE) &&
#endif
     !notblank(header.subject) ) {
    printf("There is no subject line.\n");
    exit(0);
  }
  if (!notblank(header.from)) {
    printf("There is no from line.\n");
    exit(0);     
  }
  if (!notblank(header.newsgroups)) {
    printf("There is no newsgroups line.\n");
    exit(0);
  }
  if (!notblank(address)) {
    printf(
 "You must provide a server name or set the NNTPSERVER environment variable.\n"
	);
    exit(0);
  }

  loc = firstfile;
  while (loc != NULL) {
    totalsize += loc->fileinfo.st_size;
    numfiles++;
    loc = loc->next;
  }

  printf("\n");
  printf("From: %s\n", header.from);
  printf("Newsgroups: %s\n", header.newsgroups);
  if (notblank(header.subject))
    printf("Subject: %s\n", header.subject);
  if (notblank(header.organization))
    printf("Organization: %s\n", header.organization);
  if (notblank(header.reference))
    printf("References: %s\n", header.reference);
  printf("\n");

  if (istext != TRUE) {
    if (totalsize < (1024 * 1024))
      printf("Posting %ld bytes in %d files.\n", totalsize, numfiles);
    else
      printf("Posting %.1f megabytes in %d files.\n",
	( ((float) totalsize / 1024) / 1024), numfiles);
  }
  else
    printf("Posting %s as body of message.\n", firstfile->filename);

  printf("Posting will begin in %d seconds.\n", SLEEPTIME);
  sleep(SLEEPTIME);
  printf("\n");

  socket_create(&sock, address, port);
  socket_getline(&sock);

#ifdef DEBUG1
  printf("%s", sock.line_buffer);
#endif
  
  if (notblank(username) && notblank(password)) {
    if (nntpauth(&sock, username, password) == FALSE) {
      printf("Incorrect username or password.\n");
      exit(0);
    }
  }

  if (istext == TRUE) {
    printf("Posting %s as text.\n", firstfile->filename);
    posttext(firstfile->filename, &header, &sock);
    printf("\nPosted %s\n", firstfile->filename);
  }
  else {
    totalsize = 0;
    oldfirstfile = firstfile;
    while (firstfile != NULL) {
      begintime = time(0);
      encode_and_post(&sock, firstfile, &header, maxlines, zerofile,
		      notblank(sfvname));
      zerofile = NULL;
      begintime = (time(0) - begintime);
      totaltime += begintime;
      totalsize += firstfile->fileinfo.st_size;
      if (begintime != 0)
	printf("Posted %s (%ld bytes) in %ld seconds (%.2f KB/second)\n",
	  firstfile->filename, (long) firstfile->fileinfo.st_size, begintime, 
	  (((float) firstfile->fileinfo.st_size / begintime) / 1024) );
      else
	printf("Posted %s (%ld bytes) in 0 seconds (very quickly)\n",
	  firstfile->filename, (long) firstfile->fileinfo.st_size);
      firstfile = firstfile->next;
    }

    if (notblank(sfvname)) {
      struct filell *sfv;

      sprintf(unstripped, "/tmp/%s", sfvname);
      newsfv(oldfirstfile, unstripped);

      sfv = addfile(unstripped, NULL);
      checkfiles(sfv);

      encode_and_post(&sock, sfv, &header, maxlines, zerofile, FALSE);

      remove(unstripped);
      free(sfv);

      printf("Posted %s         \n", sfvname);
    }

    printf("\n");
    if(totaltime != 0)
      printf(
       "Posted %d files.\nTotal %ld bytes in %ld minutes (%.2f KB/second)\n",
	numfiles, totalsize, (totaltime / 60),
	(((float) totalsize / totaltime) / 1024));
    else
      printf(
	"Posted %d files.\nTotal %ld bytes in 0 minutes (really quickly)\n",
	numfiles, totalsize);
  }
  /* This stuff has yet to be free()'d */
  /* but hey, we're exiting anyway */
  /*
  while (oldfirstfile != NULL)
    oldfirstfile = freefilell(oldfirstfile, oldfirstfile);

  free(password);
  free(address);
  free(username);
  free(header.subject);
  free(header.newsgroups);
  free(header.from);
  free(header.organization);
  free(header.reference);
  */
  return 0;
}

struct filell *parseargs(int argc, char **argv, 
               struct headerinfo *header, 
               char *address, int *port, 
               char *username, char *password,
               bool *istext, bool *setasdefault,
               int *maxlines, struct filell **zerofile,
               char *sfvname) {
  const char *needy = "snfouprizl0v";	/* Options that take an argument */
  struct filell *firstfile = NULL;
  int i = 1;
  char c;

  if (argc == 1) printhelp(header, address, *port,
                        username, password, *maxlines);
  while (i < argc) {
    if (('-' == argv[i][0]) && (2 == strlen(argv[i]))) {
	c = argv[i][1];

	if (strchr(needy, c) != NULL) {		/* Argument needed? */
	    i++;
	    if (i == argc) {
		printf("The -%c option requires an argument.\n", c);
		exit(0);
	    }
	}

	switch (c) {
	    case 's':	/* get the subject line */
		strcpy(header->subject, argv[i]);
		break;
	    case 'n':	/* get the newsgroups line */
		strcpy(header->newsgroups, argv[i]);
		break;
	    case 'h':
	    case '?':
	  	printhelp(header, address, *port,
			username, password, *maxlines);
		break;
	    case 'f':	/* get the from line */
		strcpy(header->from, argv[i]);
 		break;
	    case 'o':	/* get the organization line */
		strcpy(header->organization, argv[i]);
		break;
	    case 'd':	/* set as the default */
      		*setasdefault = TRUE;
		break;
	    case 't':	/* post a text message */
		*istext = TRUE;
		break;
	    case 'u':	/* get the username */
		strcpy(username, argv[i]);
		break;
	    case 'p':	/* get the password */
		strcpy(password, argv[i]);
		break;
	    case 'r':	/* get the referenced message id */
		strcpy(header->reference, argv[i]);
		break;
	    case 'i':	/* get the server name or ip address */
		strcpy(address, argv[i]);
		break;
	    case 'z':	/* get the port number */
		*port = atol(argv[i]);
		break;
	    case 'l':	/* get the maximum number of lines */
		*maxlines = atol(argv[i]);
		break;
	    case '0':	/* get the zero file */
		*zerofile = addfile(argv[i], *zerofile);
		break;
	    case 'v':	/* get the sfv file */
		strcpy(sfvname, argv[i]);
		break;
	    case 'V':	/* display the version information and exit */
		printf("newspost v" NVERSION "\n");
		exit(0);
		break;
	    default:
		printf("Invalid option: -%c\n", c);
		exit(0);
	}
    }
    else
      firstfile = addfile(argv[i], firstfile); 

    i++;
  }
  return firstfile; 
}

void printhelp(struct headerinfo *header,
               const char *address, int port,
               const char *username, const char *password,
               int maxlines) {
  printf("newspost v%s\n", NVERSION);
  printf("Usage: newspost [OPTIONS [ARGUMENTS]] file1 file2 file3...\n"
         "Options:\n"
         "  -h, -?  - prints help.\n"
         "  -s SUBJ - subject for all of your posts.\n"
         "  -n NWSG - newsgroups to post to, separated by commas.");
  if (notblank(header->newsgroups))
    printf(" Default Newsgroups: %s\n", header->newsgroups);
  else
    printf("\n");

  printf("  -f FROM - your e-mail address.");
  if (notblank(header->from))
    printf(" Default From: %s\n", header->from);
  else
    printf("\n");

  printf("  -o ORGN - your organization.");
  if (notblank(header->organization))
    printf(" Default Organization: %s\n", header->organization);
  else
    printf("\n");

  printf("  -i ADDR - hostname or ip address of your news server.");
  if (notblank(address))
    printf(" Default Server: %s\n", address);
  else
    printf("\n");

  printf("  -z PORT - port number of the NNTP server if nonstandard."
    " Default Port: %d\n", port);

  printf("  -u USER - username, only used if authorization needed.");
  if (notblank(username))
    printf(" Default Username: %s\n", username);
  else
    printf("\n");

  printf("  -p PASS - password, only used if authorization needed.");
  printf(notblank(password) ? "Default Password: ********\n" : "\n");

  printf("  -l LNES - maximum number of lines per post."
	 " Default Maximum Lines: %d\n", maxlines);
  printf("  -0 FILE - FILE is posted as text as a prefix (0/xx)"
	 " to the first file posted.\n"
         "  -v NAME - create and post as a sfv checksum file.\n"
         "  -d      - set current options as your default.\n"
         "  -t      - post as text (no uuencoding). only one file may"
	 " be posted.\n"
         "  -r MGID - reference MESSAGE_ID\n"
         "  -V      - display version information and exit\n");
  printf("Examples:\n"
         "  $ newspost -f you@yourbox.yourdomain -o \"Your Organization\" "
	 "-i news.yourisp.com -n alt.binaries.sounds.mp3 -d\n"
         "  $ newspost -s \"This is me singing\" -0 musicinfo.txt *.mp3 "
	 "-v mysongs.sfv\n"
	 "  $ newspost -s \"Some pics of my kitty\" -n "
	 "alt.binaries.pets.cats kitty1.jpg kitty2.jpg kitty3.jpg\n");
  exit(0);
}

bool writedefaults(struct headerinfo *header,
                   const char *address, int port,
                   const char *username, const char *password,
                   int maxlines) {
  char defaultsfile[1024];
  FILE *myfile;
  char *homedir;

  homedir = getenv("HOME");
  if (homedir != NULL) sprintf(defaultsfile, "%s/.newspost", homedir);
  else strcpy(defaultsfile, "~/.newspost");

  myfile = fopen(defaultsfile, "w");
  if (myfile == NULL) return FALSE;

  fprintf(myfile, "%s\n%s\n%s\n", header->from, header->organization,
	header->newsgroups);
  fprintf(myfile, "%s\n%d\n%s\n%s\n%d\n", address, port, username,
	password, maxlines);
  fclose(myfile);

  chmod(defaultsfile, 00600);	/* Protect the password */
  return TRUE;
}

void setfield(char *field, const char *src) {
  const char *loc;

  loc = src;
  while((*loc != '\0') && (*loc != '\r') && (*loc != '\n'))
    *field++ = *loc++;

  *field = '\0';
}				    

void getdefaults(struct headerinfo *header,
                 char *address, int *port,
                 char *username, char *password,
                 int *maxlines) {
  FILE *myfile;
  char defaultsfile[1024];
  char thisline[1024];
  char *homedir, *envvar, *tmp1, *tmp2;
  int i;

  header->organization[0] = '\0';
  header->subject[0] = '\0';
  header->from[0] = '\0';
  header->newsgroups[0] = '\0';
  header->reference[0] = '\0';
  
  envvar = getenv("NNTPSERVER");
  if (envvar == NULL) address[0] = '\0';
  else strcpy(address, envvar);

  tmp1 = getenv("USER");
  tmp2 = getenv("USERNAME");
  envvar = getenv("HOSTNAME");

  if ( (tmp1 != NULL) && (envvar != NULL)) {
    if (tmp2 != NULL)
      sprintf(header->from, "%s <%s@%s>", tmp2, tmp1, envvar);
    else
      sprintf(header->from, "%s@%s", tmp1, envvar);
  }
  *port = 119;
  *maxlines = 7500;

  homedir = getenv("HOME");
  if (homedir != NULL) sprintf(defaultsfile, "%s/.newspost", homedir);
  else sprintf(defaultsfile, "~/.newspost");
  myfile = fopen(defaultsfile, "r");
 
  if (myfile != NULL) {
    for (i = 0; i < 8; i++) {
      fgets(thisline, 1024, myfile);
      if (notblank(thisline)) {

        if (i == 0) setfield(header->from, thisline);
        else if (i == 1) setfield(header->organization, thisline);
        else if (i == 2) setfield(header->newsgroups, thisline);
        else if (i == 3) setfield(address, thisline);
        else if (i == 4) *port = atol(thisline);
        else if (i == 5) setfield(username, thisline);
        else if (i == 6) setfield(password, thisline);
        else if (i == 7) *maxlines = atol(thisline);
      }
    }
    fclose(myfile);
  }
}

