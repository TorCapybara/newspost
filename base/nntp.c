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

#include "nntp.h"
#include "../ui/ui.h"
#include "socket.h"

/**
*** Public Routines
**/

boolean nntp_logon(newspost_data *data) {
	char buffer[STRING_BUFSIZE];

	if (nntp_get_response(buffer) < 0)
		return FALSE;

	if (data->user[0] != '\0') {
		sprintf(buffer, "AUTHINFO USER %s", data->user);
		if (nntp_issue_command(buffer) < 0)
			return FALSE;
		if (nntp_get_response(buffer) < 0)
			return FALSE;
		/* 381: More Authentication required */
		if (strncmp(buffer,
		    NNTP_MORE_AUTHENTICATION_REQUIRED, 3) == 0) {
			sprintf(buffer, "AUTHINFO PASS %s", data->password);
			if (nntp_issue_command(buffer) < 0)
				return FALSE;
			if (nntp_get_response(buffer) < 0)
				return FALSE;
			if (strncmp(buffer,
			    NTTP_AUTHENTICATION_UNSUCCESSFUL, 3) == 0) {
				ui_nntp_authentication_failed(buffer);
				return FALSE;
			}
		}
		/* 281: Authentication successful */
		else if (strncmp(buffer,
			 NNTP_AUTHENTICATION_SUCCESSFUL, 3) == 0) {
			return TRUE;
		}
		/* 500: server doesn't support authinfo */
		else if (strncmp(buffer, NNTP_UNKNOWN_COMMAND, 3) == 0) {
			/* ignore it */
		}
		/* unknown response */
		else
			ui_nntp_unknown_response(buffer);
	}

	return TRUE;
}

void nntp_logoff() {
	char tmpbuffer[STRING_BUFSIZE];
	nntp_issue_command("QUIT");
	nntp_get_response(tmpbuffer);
}

int nntp_post(const char *subject, newspost_data *data, 
	      const char *buffer, long length,
	      boolean no_ui_updates) {
	char response[STRING_BUFSIZE];
	const char *pi;
	long i, chunksize;
	SList * listptr;

	nntp_issue_command("POST");

	nntp_get_response(response);

	if (strncmp(response, NNTP_POSTING_NOT_ALLOWED, 3) == 0)
		return POSTING_NOT_ALLOWED;

	if (strncmp(response, NNTP_PROCEED_WITH_POST, 3) != 0) {
		/* this shouldn't really happen */
		ui_nntp_unknown_response(response);
		return POSTING_FAILED;
	}

	socket_write("From: ", 6);
	socket_write(data->from, (strlen(data->from)));
	socket_write("\r\n", 2);

	socket_write("Newsgroups: ", 12);
	socket_write(data->newsgroup, (strlen(data->newsgroup)));
	socket_write("\r\n", 2);

	socket_write("Subject: ", 9);
	socket_write(subject, (strlen(subject)));
	socket_write("\r\n", 2);

	socket_write("User-Agent: ", 12);
	socket_write(USER_AGENT, (strlen(USER_AGENT)));
	socket_write("\r\n", 2);

	if (data->replyto[0] != '\0') {
		socket_write("Reply-To: ", 10);
		socket_write(data->replyto, (strlen(data->replyto)));
		socket_write("\r\n", 2);
	}

	if (data->followupto[0] != '\0') {
		socket_write("Followup-To: ", 13);
		socket_write(data->followupto, (strlen(data->followupto)));
		socket_write("\r\n", 2);
	}

	if (data->organization[0] != '\0') {
		socket_write("Organization: ", 14);
		socket_write(data->organization, (strlen(data->organization)));
		socket_write("\r\n", 2);
	}

	if (data->reference[0] != '\0') {
		socket_write("Reference: ", 11);
		socket_write(data->reference, (strlen(data->reference)));
		socket_write("\r\n", 2);
	}

	if (data->noarchive == TRUE)
		socket_write("X-No-Archive: yes\r\n", 19);

	listptr = data->extra_headers;
	while (listptr != NULL) {
		socket_write(listptr->data, (strlen(listptr->data)));
		socket_write("\r\n", 2);
		listptr = slist_next(listptr);
	}

	socket_write("\r\n", 2);

	if (!no_ui_updates)
		ui_chunk_posted(0, 0);

	pi = buffer;
	i = 0;
	chunksize = 32768;
	while ((length - i) > chunksize) {
		socket_write(pi, chunksize);
		i += chunksize;
		pi += chunksize;
#ifndef REPORT_ONLY_FULLPARTS
		if (!no_ui_updates)
			chunksize = ui_chunk_posted(chunksize, i);
#endif
	}
	socket_write(pi, (length - i));
	i += (length - i);

	socket_write("\r\n.\r\n", 5);

	nntp_get_response(response);

	if (!no_ui_updates)
		ui_chunk_posted((length - i), i);

	if (strncmp(response, NNTP_POSTING_FAILED, 3) == 0) {
		ui_nntp_posting_failed(response);
		return POSTING_FAILED;
	}
	else if (strncmp(response, NNTP_ARTICLE_POSTED_OK, 3) != 0) {
		/* shouldn't really happen */
		ui_nntp_unknown_response(response);
		return POSTING_FAILED;
	}
	return NORMAL;
}

/* returns number of bytes written */
int nntp_issue_command(const char *command) {
	int bytes_written;
	bytes_written = socket_write(command, strlen(command));
	if (bytes_written > 0) {
		bytes_written += socket_write("\r\n", 2);
		ui_nntp_command_issued(command);
	}
	return bytes_written;
}

/* returns number of bytes read */
int nntp_get_response(char *response) {
	int bytes_read;
	bytes_read = socket_getline(response);
	if (bytes_read > 0)
		ui_nntp_server_response(response);

	return bytes_read;
}
