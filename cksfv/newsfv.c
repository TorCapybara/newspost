/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* newsfv.c - creates a new checksum listing
   
   Copyright (C) 2000 Bryan Call <bc@fodder.org>

   Modified for Newspost by Jim Faulkner <newspost@unixcab.org>
                        and William McBrine <wmcbrine@users.sf.net>

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

#include "../base/newspost.h"
#include "../ui/ui.h"

#include <time.h>
#include <stdio.h>

/**
*** Public Routines
**/

void newsfv(SList *file_list, newspost_data *np_data)
{
	SList         *file_list_orig;
	char          *fn;
	time_t        clock;
	struct tm     *timeinfo;
	file_entry     *data;
	FILE           *sfvfile;
	Buff          *tmpstring = NULL;

	ui_sfv_gen_start();

	/* check if the name really ends in .sfv */
	if(strlen(np_data->sfv->data) > 4){
		fn = np_data->sfv->data;
		fn += (strlen(np_data->sfv->data) - 4);
		if (strncasecmp(fn, ".sfv", 4) != 0) {
			np_data->sfv = buff_add(np_data->sfv,".sfv");
		}
	}
	else{
		np_data->sfv = buff_add(np_data->sfv,".sfv");
	}

	tmpstring = buff_create(tmpstring, "%s/%s", np_data->tmpdir->data, np_data->sfv->data);
	np_data->sfv = buff_create(np_data->sfv, "%s", tmpstring->data);
	buff_free(tmpstring);

	sfvfile = fopen(np_data->sfv->data, "wb");
	if (sfvfile == 0) {
		ui_sfv_gen_error(np_data->sfv->data, 0);
		return;
	}

	clock = time(NULL);
	timeinfo = localtime(&clock);

	/* Header (comments) */

#ifdef WINSFV32_COMPATIBILITY_MODE
	fprintf(sfvfile,
		"; Generated by WIN-SFV32 v1.1a NOT! compatability line.\r\n");
#endif
	fprintf(sfvfile, "; Generated by " NEWSPOSTNAME "/" VERSION " on "
		"%02d-%02d-%02d at %02d:%02d.%02d\r\n;\r\n",
		timeinfo->tm_year+1900, (timeinfo->tm_mon + 1), timeinfo->tm_mday,
		timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

	/* Comments - file names, sizes and dates */

	file_list_orig = file_list;
	while (file_list != NULL) {
		data = (file_entry *) file_list->data; 
		fn = data->filename->data;
		timeinfo = localtime(&(data->fileinfo.st_mtime));
		fprintf(sfvfile,
			";%13ld  %02d:%02d.%02d %02d-%02d-%02d %s\r\n",
			(long) data->fileinfo.st_size, timeinfo->tm_hour,
			timeinfo->tm_min, timeinfo->tm_sec,
			timeinfo->tm_year + 1900, timeinfo->tm_mon,
			timeinfo->tm_mday, n_basename(fn));
		file_list = slist_next(file_list);
	}

	/* The meat - file names and CRCs */

	file_list = file_list_orig;
	while (file_list != NULL) {
		data = (file_entry *) file_list->data;
		fn = data->filename->data;
		
		fprintf(sfvfile, "%s %08X\r\n", n_basename(fn), data->crc);
		
		file_list = slist_next(file_list);
	}

	fclose(sfvfile);
	chmod(np_data->sfv->data, S_IRUSR | S_IWUSR);

	ui_sfv_gen_done(np_data->sfv->data);
}
