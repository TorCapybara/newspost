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

const char *strippath(const char *filename) {
  bool check = FALSE;
  const char *ptr = filename;
  const char *lastslash = filename;

  while ('\0' != *ptr) {
    if ('/' == *ptr) { 
      lastslash = ptr;
      check = TRUE;
    }
    ptr++;
  }
  if (check == TRUE) lastslash++;

  return lastslash;
}

struct filell *checkfiles(struct filell *firstfile) {
  FILE *thisfile;
  struct filell *loc = firstfile;
  struct filell *oldloc = NULL;

  while (loc != NULL) {
    if (stat(loc->filename, &loc->fileinfo) == -1) {
      printf("Could not stat %s: %s\n", loc->filename, strerror(errno));
      oldloc = loc->next;
      firstfile = freefilell(firstfile, loc);
      loc = oldloc;
    }
    else {
      if (S_ISDIR(loc->fileinfo.st_mode)) {
        printf("%s is a directory.\n", loc->filename);
        oldloc = loc->next;
        firstfile = freefilell(firstfile, loc);
        loc = oldloc;
      }
      else {
        thisfile = fopen(loc->filename, "rb");
        if (thisfile == NULL) {
          printf("Could not open %s: %s\n", loc->filename, strerror(errno));
          oldloc = loc->next;
          firstfile = freefilell(firstfile, loc);
          loc = oldloc;
        }
        else { 
          fclose(thisfile);
          loc = loc->next;
        }
      }
    }
  }

  return firstfile;
}

/* returns a pointer to the first in the list */
struct filell *addfile(const char *myfile, struct filell *first) {
  struct filell *wherami;

  if (first == NULL) {
    first = malloc(sizeof(struct filell));
    wherami = first;
  }
  else {
    wherami = first;
    while (wherami->next != NULL)
      wherami = wherami->next;

    wherami->next = malloc(sizeof(struct filell));
    wherami = wherami->next;
  }
  strcpy(wherami->filename, myfile);
  wherami->crc = 0xffffffffL;
  wherami->next = NULL;

  return first;
}

struct filell *freefilell(struct filell *first, struct filell *tofree) {
  struct filell *loc = first;
  struct filell *oldloc = loc;

  if (first == tofree) {
    first = loc->next;
    free(oldloc);
  }
  else {
    while (loc != tofree) {
      oldloc = loc;
      loc = loc->next;
    }
    oldloc->next = loc->next;
    free(loc);
  }

  return first;
}
