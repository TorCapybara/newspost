/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Author: Jim Faulkner <newspost@unixcab.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */

/* Modified by William McBrine <wmcbrine@users.sf.net> */

#include "newspost.h"
#include "../ui/ui.h"
#include "socket.h"
#include "nntp.h"
#include "encode.h"
#include "../cksfv/sfv.h"
#include "../parchive/parintrf.h"

/**
*** Private Declarations
**/

static int encode_and_post(newspost_data *data, SList *file_list,
			    SList *parfiles);
static SList *preprocess(newspost_data *data, SList *file_list);
static long read_text_file(const char *filename);

static int post_file(newspost_data *data, file_entry *file_data,
	int filenumber, int number_of_files, const char *filestring,
	char *data_buffer);

static void make_subject(char *subject, newspost_data *data,
	int filenumber, int number_of_files, const char *filename,
	int partnumber, int number_of_parts, const char *filestring);

/**
*** Public Routines
**/

int newspost(newspost_data *data, SList *file_list) {
	int retval;
	SList *parfiles = NULL;

	/* preprocess */
	parfiles = preprocess(data, file_list);

	/* and post! */
	ui_post_start(data, file_list, parfiles);
	retval = encode_and_post(data, file_list, parfiles);

	return retval;
}

/* Find last occurrence of '/' or '\\'; return rest of string */
const char *basename(const char *path) {
	const char *current;

	for (current = path; *current; current++)
		if (('/' == *current) || ('\\' == *current))
			path = current + 1;

	return path;
}

SList *slist_next(SList *slist) {
	return slist->next;
}

SList *slist_remove(SList *slist, void *data) {
	SList *si = slist;
	SList *prev = NULL;

	while (si != NULL) {
		if (si->data == data) {
			if (prev == NULL) {
				si = si->next;
				free(slist);
				return si;
			}
			prev->next = si->next;
			free(si);
			return slist;
		}
		prev = si;
		si = si->next;
	}
	return slist;
}

SList *slist_append(SList *slist, void *data) {
	SList *si;
	if (slist == NULL) {
		slist = (SList *) malloc(sizeof(SList));
		slist->data = data;
		slist->next = NULL;
		return slist;
	}
	else {
		si = slist;
		while (si->next != NULL)
			si = si->next;

		si->next = (SList *) malloc(sizeof(SList));
		si = si->next;
		si->data = data;
		si->next = NULL;
		return slist;
	}
}

SList *slist_prepend(SList *slist, void *data) {
	SList *si;
	if (slist == NULL) {
		slist = (SList *) malloc(sizeof(SList));
		slist->data = data;
		slist->next = NULL;
		return slist;		
	}
	else {
		si = (SList *) malloc(sizeof(SList));
		si->data = data;
		si->next = slist;
		return si;
	}
}

void slist_free(SList *slist) {
	SList *si;
	while (slist != NULL) {
		si = slist;
		slist = slist->next;
		free(si);
	}
}

int slist_length(SList *slist) {
	int i = 0;
	while (slist != NULL) {
		i++;
		slist = slist->next;
	}
	return i;
}

/**
*** Private Routines
**/

static char *text_buffer;

static int encode_and_post(newspost_data *data, SList *file_list,
			    SList *parfiles) {
	char subject[STRING_BUFSIZE];
	int number_of_parts;
	long number_of_bytes;
	int number_of_files;
	int i;
	file_entry *file_data;
	int retval = NORMAL;
	char *data_buffer = 
		(char *) malloc(get_buffer_size_per_encoded_part(data));

	/* create the socket */
	ui_socket_connect_start(data->address);
	retval = socket_create(data->address, data->port);
	if (retval < 0)
		return retval;

	ui_socket_connect_done();

	/* log on to the server */
	ui_nntp_logon_start(data->address);
	if (nntp_logon(data) == FALSE) {
		socket_close();
		return LOGON_FAILED;
	}
	ui_nntp_logon_done();

	if (data->text == TRUE) {
		file_data = file_list->data;
		/* post */
		number_of_bytes = read_text_file(file_data->filename);
		retval = nntp_post(data->subject, data, text_buffer,
				   number_of_bytes, TRUE);
		/* clean up */
		free(text_buffer);
	}
	else {
		/* post any sfv files... */
		if (data->sfv[0] != '\0') {
			file_data = (file_entry *) malloc(sizeof(file_entry));
			file_data->parts = NULL;
			strcpy(file_data->filename, data->sfv);
			if (stat(data->sfv, &file_data->fileinfo) == -1)
				ui_sfv_gen_error(data->sfv, errno);
			else {
				retval = post_file(data, file_data, 1, 1,
						   "SFV File", data_buffer);
				if (retval < 0)
					return retval;

				unlink(data->sfv);
			}
			free(file_data);
		}

		number_of_files = slist_length(file_list);

		/* if there's a prefix, post that */
		if (data->prefix[0] != '\0') {
			ui_posting_prefix_start(data->prefix);

			file_data = (file_entry *) file_list->data;
			number_of_parts = 
				get_number_of_encoded_parts(data, file_data);
			make_subject(subject, data, 1 , number_of_files,
				     file_data->filename, 0 , number_of_parts,
				     "File");

			number_of_bytes = read_text_file(data->prefix);
			if (number_of_bytes > 0) {
				retval = nntp_post(subject, data, text_buffer, 
						   number_of_bytes, TRUE);
				if (retval == POSTING_NOT_ALLOWED)
					return retval;
				else if (retval == POSTING_FAILED) {
					/* dont bother retrying...
					   who knows what's in that file */
					ui_posting_prefix_failed();
					retval = NORMAL;
				}
				else if (retval == NORMAL)
					ui_posting_prefix_done();
			}
			else
				ui_posting_prefix_failed();

			free(text_buffer);
		}
	
		/* post the files */
		i = 1;
		while (file_list != NULL) {

			file_data = (file_entry *) file_list->data;

			retval = post_file(data, file_data, i, number_of_files,
					   "File", data_buffer);
			if (retval < 0)
				return retval;

			i++;
			file_list = slist_next(file_list);
		}

		/* post any par files */
		i = 1;
		file_list = parfiles;
		number_of_files = slist_length(parfiles);
		while (file_list != NULL) {

			file_data = (file_entry *) file_list->data;

			retval = post_file(data, file_data, i, number_of_files,
					   "PAR File", data_buffer);
			if (retval < 0)
				return retval;

			unlink(file_data->filename);
			free(file_data);
			i++;
			file_list = slist_next(file_list);
		}
		slist_free(parfiles);
	}

	nntp_logoff();
	socket_close();
	
	free(data_buffer);

	return retval;
}

static int post_file(newspost_data *data, file_entry *file_data, 
	  int filenumber, int number_of_files,
	  const char *filestring, char *data_buffer) {
	char subject[STRING_BUFSIZE];
	long number_of_bytes;
	int j, retval;
	int number_of_tries = 0;
	int parts_posted = 0;
	int number_of_parts = 
		get_number_of_encoded_parts(data, file_data);
	static int total_failures = 0;
	boolean posting_started = FALSE;

	for (j = 1; j <= number_of_parts; j++) {

		if ((file_data->parts != NULL) &&
		    (file_data->parts[j] == FALSE))
			continue;

		make_subject(subject, data, filenumber, number_of_files,
			     file_data->filename, j, number_of_parts,
			     filestring);
		
		number_of_bytes = get_encoded_part(data, file_data, j,
						   data_buffer);
		if (posting_started == FALSE) {
			ui_posting_file_start(data, file_data, 
				      number_of_parts, number_of_bytes);
			posting_started = TRUE;
		}
	
		ui_posting_part_start(file_data, j, number_of_parts,
				      number_of_bytes);

		retval = nntp_post(subject, data, data_buffer,
				   number_of_bytes, FALSE);

		if (retval == NORMAL) {
			ui_posting_part_done(file_data, j, number_of_parts,
					     number_of_bytes);
			parts_posted++;
		}
		else if (retval == POSTING_NOT_ALLOWED)
			return retval;
		else {
			if (number_of_tries < 5) {
				ui_nntp_posting_retry();
				sleep(5);
				number_of_tries++;
				continue;
			}
			else {
				total_failures++;
				if (total_failures == 5) {
					nntp_logoff();
					socket_close();
					ui_too_many_failures();
				}
			}
		}
		number_of_tries = 0;
	}
	ui_posting_file_done(file_data->filename, parts_posted);
	return NORMAL;
}

static void make_subject(char *subject, newspost_data *data, int filenumber,
			 int number_of_files, const char *filename,
			 int partnumber, int number_of_parts,
			 const char *filestring) {
	char numchunkbuf[32];
	int numchunksize;

	sprintf(numchunkbuf, "%i", number_of_parts);
	numchunksize = strlen(numchunkbuf);

	if (data->subject[0] != '\0')
		subject += sprintf(subject, "%s - ", data->subject);
	if (data->filenumber == TRUE)
		subject += sprintf(subject, "%s %i of %i: ", filestring,
			filenumber, number_of_files);
	sprintf(subject, (data->yenc == TRUE) ? "\"%s\" yEnc (%0*i/%i)" :
		"%s (%0*i/%i)", basename(filename), numchunksize,
		partnumber, number_of_parts);
}

static SList *preprocess(newspost_data *data, SList *file_list) {
	char tmpstring[STRING_BUFSIZE];
	SList *parfiles = NULL;

	/* make the from line */
	if (data->name[0] != '\0') {
		strcpy(tmpstring, data->from);
		snprintf(data->from, STRING_BUFSIZE, "%s <%s>",
			 data->name, tmpstring);
	}

	if (data->text == FALSE) {
		/* calculate CRCs if needed; generate any sfv files */
		if ((data->yenc == TRUE) || (data->sfv[0] != '\0')) {
			calculate_crcs(file_list);
			
			if (data->sfv[0] != '\0')
				newsfv(file_list, data);
		}
		
		/* generate any par files */
		if (data->par[0] != '\0') {
			parfiles = par_newspost_interface(data, file_list);
			if (data->yenc == TRUE)
				calculate_crcs(parfiles);
		}
	}

	return parfiles;
}

/* returns number of bytes read */
static long read_text_file(const char *filename) {
	FILE *file;
	char line[STRING_BUFSIZE];
	long i;
	int j;
	long buffersize = 10240;

	text_buffer = malloc(buffersize);

	file = fopen(filename, "r");
	if (file != NULL) {
		i = 0;
		while (fgets(line, STRING_BUFSIZE, file) != NULL) {
			if ((i + 1024) >= buffersize) {
				buffersize += 10240;
				text_buffer = realloc(text_buffer, buffersize);
			}
			/* translate for posting */
			if (line[0] == '.')
				text_buffer[i++] = '.';

			j = 0;
			while (line[j] != '\0') {
				if ('\n' == line[j])
					if ((0 == j) || ('\r' != line[j - 1]))
						text_buffer[i++] = '\r';

				text_buffer[i++] = line[j++];
			}
		}
		fclose(file);
		i--;
		return i;
	}
	return 0;
}
