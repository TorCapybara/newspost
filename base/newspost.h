/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#ifndef __NEWSPOST_H__
#define __NEWSPOST_H__

/*********************************/
/* USER-CONFIGURABLE DEFINITIONS */
/*********************************/

#define SLEEP_TIME 10 /* time to pause before posting in seconds */

/* #define ALLOW_NO_SUBJECT */ /* makes the subject line optional */

/* #define WINSFV32_COMPATIBILITY_MODE */

/* #define REPORT_ONLY_FULLPARTS */ /* limit update of KBps display */

/* ONLY CHANGE THESE IF YOU GET AN ERROR DURING COMPILATION */
typedef unsigned char           n_uint8; /* 1 byte unsigned integer */
typedef unsigned short int      n_uint16; /* 2 byte unsigned integer */
typedef unsigned int       	n_uint32; /* 4 byte unsigned integer */
typedef long long               n_int64; /* 8 byte signed integer */

/***********************************/
/* END OF CONFIGURABLE DEFINITIONS */
/***********************************/

#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>

typedef n_uint8                 boolean;

#define FALSE 0
#define TRUE 1

/* remember to change VERSION and PAR_CLIENT for new version numbers */
#define VERSION "2.0"
#define PAR_CLIENT 0xFE020000

#define NEWSPOSTURL "http://newspost.unixcab.org/"
#define NEWSPOSTNAME "Newspost"
#define USER_AGENT NEWSPOSTNAME "/" VERSION " (" NEWSPOSTURL ")"

#define STRING_BUFSIZE 1024

#define NORMAL 0
#define FAILED_TO_CREATE_TMPFILES -1
#define FAILED_TO_RESOLVE_HOST -2
#define FAILED_TO_CREATE_SOCKET -3
#define LOGON_FAILED -4
#define POSTING_NOT_ALLOWED -5
#define POSTING_FAILED -6

typedef struct Newspost_SList {
	void *data;
	struct Newspost_SList *next;
}
SList;

typedef struct {
	char subject[STRING_BUFSIZE];
	char newsgroup[STRING_BUFSIZE];
	char from[STRING_BUFSIZE];
	char organization[STRING_BUFSIZE];
	char address[STRING_BUFSIZE];	/* server address and port */
	int port;
	char user[STRING_BUFSIZE];
	char password[STRING_BUFSIZE];
	int lines;			/* lines per message */
	char prefix[STRING_BUFSIZE];	/* filename of text prefix file */
	boolean yenc;
	char sfv[STRING_BUFSIZE];	/* filename for generated sfv file */
	char par[STRING_BUFSIZE];	/* prefix filename for par file(s) */
	int parnum;			/* number of pars */
	int filesperpar;		/* or number of files per par */
	char reference[STRING_BUFSIZE]; /* message-id to reference */
	boolean filenumber;		/* include "File X of Y" in subject? */
	char tmpdir[STRING_BUFSIZE];
	boolean noarchive;		/* include X-No-Archive: yes header? */
	char followupto[STRING_BUFSIZE];
	char replyto[STRING_BUFSIZE];
	char name[STRING_BUFSIZE];
	boolean text;
	SList * extra_headers;
}
newspost_data;

typedef struct {
	struct stat fileinfo;
	char filename[STRING_BUFSIZE];
	n_uint32 crc;
	boolean *parts;
}
file_entry;

int newspost(newspost_data *data, SList *file_list);

SList *slist_next(SList *slist);
SList *slist_append(SList *slist, void *data);
SList *slist_prepend(SList *slist, void *data);
SList *slist_remove(SList *slist, void *data);
void slist_free(SList *slist);
int slist_length(SList *slist);

const char *basename(const char *path);

#endif /* __NEWSPOST_H__ */
