/* newsfv.c - creates a new checksum listing
   
   Copyright (C) 2000 Bryan Call <bc@fodder.org>

   Modified by Jim Faulkner for Newspost.

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

#include "../newspost.h"
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

extern void pnsfv_head(FILE *sfvfile);
extern void pfileinfo(struct filell *firstfile, FILE *sfvfile);
#ifdef OSF
extern void pcrc(char*, unsigned int, FILE *sfvfile);
extern int  crc32(int, unsigned int*, unsigned int*);
#else
extern void pcrc(char*, unsigned long, FILE *sfvfile);
extern int  crc32(int, unsigned long*, unsigned long*);
#endif

int newsfv(struct filell *firstfile, char *sfvname)
{
  int           fd, rval = 0;
  char          *fn;
#ifdef OSF
  unsigned int len, val;
#else
  unsigned long len, val;
#endif
  struct filell *loc = firstfile;
  FILE *sfvfile = fopen(sfvname,"w");
  if(sfvfile == NULL){
    printf("Unable to create %s: \n%s\n",sfvname,strerror(errno));
  } 
 
  pnsfv_head(sfvfile);
  pfileinfo(firstfile,sfvfile);
  
  while (loc != NULL) {
    fn = loc->filename;
    if ((fd = open(fn, O_RDONLY, 0)) < 0) {
      fprintf(stderr, "cksfv: %s: %s\n", fn, strerror(errno)); 
      rval = 1;
      loc = loc->next;
      continue;
    }
    
    if (crc32(fd, &val, &len)) {
      fprintf(stderr, "cksfv: %s: %s\n", fn, strerror(errno)); 
      rval = 1;
    } else
      pcrc(fn, val, sfvfile);
    close(fd);
    loc = loc->next;
  }

  if(sfvfile != NULL){
    fclose(sfvfile);
  }
  return(rval);
}
