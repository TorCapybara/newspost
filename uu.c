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

void encode_and_post(struct postsocket *sock, struct filell *loc,
		     struct headerinfo *header, int maxlines,
		     struct filell *zerofile, bool makesfv) {
  FILE *myfile;
  const char *strippedfilename;
  char numchunkbuf[21]; /* big enough for 2^64 */
  struct headerinfo tmpheader; 
  int i, numchunks = 1;
  int numchunksize;

  if (loc->fileinfo.st_size > (BPL * maxlines))
    numchunks = (loc->fileinfo.st_size / (BPL * maxlines)) +
	((loc->fileinfo.st_size % (BPL * maxlines)) != 0);

  sprintf(numchunkbuf, "%d", numchunks);
  numchunksize = strlen(numchunkbuf);

  tmpheader.from = header->from;
  tmpheader.newsgroups = header->newsgroups;
  tmpheader.subject = calloc(1024, sizeof(char));
  tmpheader.organization = header->organization;
  tmpheader.reference = header->reference;

  strippedfilename = strippath(loc->filename);

  /* post text prefix if there is one */
  if ( (zerofile != NULL) ) {
    if (notblank(header->subject))
      sprintf(tmpheader.subject, "%s - %s (%0*d/%d)", header->subject,
	strippedfilename, numchunksize, 0, numchunks);
    else
      sprintf(tmpheader.subject, "%s (%0*d/%d)", strippedfilename,
	numchunksize, 0, numchunks);
    posttext(zerofile->filename, &tmpheader, sock);
    printf("Posted %s as text\n", zerofile->filename);
  }

  myfile = fopen(loc->filename, "rb");
  if (myfile == NULL) {
    printf("Error opening %s: \n%s\n", loc->filename, strerror(errno));
    exit(0);
  }

  for (i = 1; (feof(myfile) == 0) && (i <= numchunks); i++) {

    if (notblank(header->subject))
      sprintf(tmpheader.subject, "%s - %s (%0*d/%d)", header->subject,
	strippedfilename, numchunksize, i, numchunks);
    else
      sprintf(tmpheader.subject, "%s (%0*d/%d)", strippedfilename,
	numchunksize, i, numchunks);

    /* post it! */
    printf("\r%s: Posting %d of %d", strippedfilename, i, numchunks);
    fflush(stdout);

    postfile(loc, myfile, maxlines, &tmpheader, sock,
	(1 == i) ? ((numchunks == i) ? ISBOTH : ISFIRST) :
	(numchunks == i) ? ISLAST : 0, strippedfilename, makesfv);
  }

  printf("\r");
  fclose(myfile); 
  free(tmpheader.subject);
}
