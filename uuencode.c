/* uuencode utility.
   Copyright (C) 1994, 1995 Free Software Foundation, Inc.

   This product is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This product is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this product; see the file COPYING.  If not, write to
   the Free Software Foundation, 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA.  */

/* Copyright (c) 1983 Regents of the University of California.
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:
   1. Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
   2. Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
   3. All advertising materials mentioning features or use of this software
      must display the following acknowledgement:
	 This product includes software developed by the University of
	 California, Berkeley and its contributors.
   4. Neither the name of the University nor the names of its contributors
      may be used to endorse or promote products derived from this software
      without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
   ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
   FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
   DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
   OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
   HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
   LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
   OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
   SUCH DAMAGE.  */

#include "newspost.h"

/* Reworked to GNU style by Ian Lance Taylor, ian@airs.com, August 93.  */
/* Modified for Newspost by Jim Faulkner newspost@unixcab.org */
/* Modified by William McBrine <wmcbrine@users.sf.net>, May 2002 */

/*=======================================================\
| uuencode [INPUT] OUTPUT				 |
| 							 |
| Encode a file so it can be mailed to a remote system.	 |
\=======================================================*/

/* The standard uuencoding translation table. */
const char uu_std[64] =
{
  '`', '!', '"', '#', '$', '%', '&', '\'',
  '(', ')', '*', '+', ',', '-', '.', '/',
  '0', '1', '2', '3', '4', '5', '6', '7',
  '8', '9', ':', ';', '<', '=', '>', '?',
  '@', 'A', 'B', 'C', 'D', 'E', 'F', 'G',
  'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
  'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W',
  'X', 'Y', 'Z', '[', '\\', ']', '^', '_'
};

/* ENC is the basic 1 character encoding function to make a char printing.  */
#define ENC(Char) (uu_std[(Char)])

/*------------------------------------------------.
| Copy from IN to OUT, encoding as you go along.  |
`------------------------------------------------*/

void
encode(struct filell *loc, FILE *infile, struct postsocket *sock,
	int maxlines, bool lastfile, bool makesfv)
{
  int counter = 0;
  register int n;
  register char *p, *ch;
  char inbuf[80], outbuf[80];

  outbuf[0] = '.';

  while (counter < maxlines)
    {
      counter++;
      n = fread(inbuf, 1, 45, infile);

      if (n == 0)
	break;

      if (makesfv)
	crc32(loc, inbuf, n);

      outbuf[1] = ENC(n);

      for (p = inbuf, ch = outbuf + 2; n > 0; n -= 3, p += 3)
	{
	  if (n < 3) {
	    p[2] = '\0';
	    if (n < 2)
	      p[1] = '\0';
	  }
	  *ch++ = ENC( (*p >> 2) & 077 );
	  *ch++ = ENC( ((*p << 4) & 060) | ((p[1] >> 4) & 017) );
	  *ch++ = ENC( ((p[1] << 2) & 074) | ((p[2] >> 6) & 03) );
	  *ch++ = ENC( p[2] & 077 );
	}
      *ch++ = '\r';
      *ch++ = '\n';
      *ch = '\0';

      socket_putline(sock, outbuf + ('.' != outbuf[1]));
    }

  if (ferror(infile))
    {
      printf("Read error\n");
      exit(0);
    }

  if (lastfile == TRUE)
    {
      outbuf[0] = ENC('\0');
      outbuf[1] = '\0';
      socket_putstring(sock, outbuf);
    }
}
