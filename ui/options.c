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

#include "../base/encode.h"
#include "options.h"
#include "ui.h"

/* Command-line option keys */

#define help_option 'h'
#define subject_option 's'
#define newsgroup_option 'n'
#define from_option 'f'
#define name_option 'F'
#define organization_option 'o'
#define address_option 'i'
#define port_option 'z'
#define user_option 'u'
#define password_option 'p'
#define lines_option 'l'
#define prefix_option 'e'
#define alternate_prefix_option '0'
#define edit_prefix_option 'E'
#define yenc_option 'y'
#define sfv_option 'c'
#define par_option 'a'
#define parnum_option 'A'
#define filesperpar_option 'B'
#define reference_option 'r'
#define default_option 'd'
#define version_option 'V'
#define filenumber_option 'q'
#define verbose_option 'v'
#define delay_option 'T'
#define noarchive_option 'x'
#define followupto_option 'w'
#define replyto_option 'm'
#define tmpdir_option 'k'
#define disable_option 'D'
#define extraheader_option 'X'
#define text_option 't'

/* Option table for getopt() -- options which take parameters
   are followed by colons */

static const char valid_flags[] = {
	help_option,
	subject_option, ':',
	newsgroup_option, ':',
	from_option, ':',
	organization_option, ':',
	address_option, ':',
	port_option, ':',
	user_option, ':',
	password_option, ':',
	lines_option, ':',
	prefix_option, ':',
	alternate_prefix_option, ':',
	yenc_option,
	sfv_option, ':',
	par_option, ':',
	parnum_option, ':',
	reference_option, ':',
	default_option,
	version_option,
	filenumber_option,
	delay_option, ':',
	verbose_option,
	noarchive_option,
	followupto_option, ':',
	replyto_option, ':',
	tmpdir_option, ':',
	disable_option, ':',
	edit_prefix_option,
	filesperpar_option, ':',
	name_option, ':',
	extraheader_option,':',
	text_option,
	'\0'
};

/* Symbolic labels for .newspostrc keywords */

enum {
	newsgroup = 0,
	from,
	organization,
	address,
	port,
	user,
	password,
	lines,
	yenc,
	filenumber,
	noarchive,
	followupto,
	replyto,
	tmpdir,
	filesperpar,
	name,
	extraheader,
	number_of_defaults	/* Leave this at the end! */
};

/* The .newspostrc keywords themselves -- must be in the same order */

static const char *rc_keyword[number_of_defaults] = {
	"newsgroup",
	"from",
	"organization",
	"address",
	"port",
	"user",
	"password",
	"lines",
	"yenc",
	"filenumber",
	"noarchive",
	"followupto",
	"replyto",
	"tmpdir",
	"filesperpar",
	"name",
	"extraheader"
};

/* Comment lines for the .newspostrc -- must be in the same order */

static const char *rc_comment[number_of_defaults] = {
	"newsgroup(s) to post to",
	"your e-mail address",
	"your organization",
	"address of news server",
	"port of news server",
	"username on news server",
	"password on news server",
	"lines per message",
	"0 to uuencode, 1 to yencode",
	"0 doesn't include filenumber in subject line, 1 does",
	"0 doesn't include X-No-Archive header, 1 does",
	"the Followup-To header",
	"the Reply-To header",
	"directory for storing temporary files",
	"create a par volume for every x files",
	"your full name",
	"any extra headers"
};

static void setfield(char *field, const char *src);
static void version_info();
static boolean parse_file_parts(newspost_data *data,
			file_entry *this_file_entry, const char *arg, int i);
static void parse_delay_option(const char *option);


/**
*** Public Routines
**/

SList *parse_input_files(int argc, char **argv, int optind,
		newspost_data *data) {
	FILE *testopen;
	file_entry *this_file_entry;
	SList *file_list = NULL;
	SList *tmplist, *tmplist2;
	file_entry *data1, *data2;
	int i;
	boolean thisfilepart = FALSE;

	while (optind < argc) {
		this_file_entry = (file_entry *) malloc(sizeof(file_entry));
		this_file_entry->parts = NULL;

		i = strlen(argv[optind]);
		thisfilepart = FALSE;
		while ((i > 0) && (argv[optind][i] != ':'))
			i--;

		if (i < 2)
			setfield(this_file_entry->filename, argv[optind]);
		else {
			strncpy(this_file_entry->filename, argv[optind], i);
			this_file_entry->filename[i] = '\0';
			thisfilepart = TRUE;
		}

		if ( (stat(this_file_entry->filename,
			&this_file_entry->fileinfo) == 0) &&
		   (testopen = fopen(this_file_entry->filename, "rb")) ) {

			fclose(testopen);

			if (S_ISDIR(this_file_entry->fileinfo.st_mode)) {
				/* it's a directory */
				fprintf(stderr,
					"\nWARNING: %s is a directory"
					" - SKIPPING",
					this_file_entry->filename);
				if (data->text == TRUE) {
					printf("\nNothing to post!\n");
					exit(EXIT_NO_FILES);
				}
				/* only free() if we skip it */
				free(this_file_entry);
			}
			else {
				/* it's a good file */
				boolean postany = TRUE;

				/* for file parts */
				if (thisfilepart == TRUE)
					postany = parse_file_parts(data, 
						this_file_entry,
						argv[optind], i);
				else
					this_file_entry->parts = NULL;

				if (postany == TRUE)
					/* add it to the list */
					file_list = slist_append(file_list,
							this_file_entry);
			}
		}
		else {
			fprintf(stderr,
				"\nWARNING: %s %s - SKIPPING",
				strerror(errno), this_file_entry->filename);
			if (data->text == TRUE) {
				printf("\nNothing to post!\n");
				exit(EXIT_NO_FILES);
			}
			/* only free() if we skip it */
			free(this_file_entry);
		}
		optind++;
	}

	/* make sure there are no duplicate entries */
	tmplist = file_list;
	while (tmplist != NULL) {
		tmplist2 = slist_next(tmplist);
		while (tmplist2 != NULL) {
			data1 = (file_entry *) tmplist->data;
			data2 = (file_entry *) tmplist2->data;
			if (strcmp(data1->filename, data2->filename) == 0) {
				fprintf(stderr,
					"\nWARNING: duplicate filename"
					" %s - IGNORING",
					data2->filename);
				tmplist2 = slist_next(tmplist2);
				/* only free() if we skip it */
				free(data2);
				file_list = slist_remove(file_list, data2);
			}
			else
				tmplist2 = slist_next(tmplist2);
		}
		tmplist = slist_next(tmplist);
	}

	return file_list;
}

void parse_environment(newspost_data *data) {
	const char *envopt, *envopt2;

	envopt = getenv("NNTPSERVER");
	if (envopt != NULL)
		setfield(data->address, envopt);

	envopt = getenv("USER");
	if (envopt != NULL) {
		envopt2 = getenv("HOSTNAME");
		if (envopt2 != NULL) {
			setfield(data->from, envopt);
			strcat(data->from, "@");
			strcat(data->from, envopt2);
		}
	}

	envopt = getenv("TMPDIR");
	if (envopt != NULL)
		setfield(data->tmpdir, envopt);
	else {
		envopt = getenv("TMP");
		if (envopt != NULL)
			setfield(data->tmpdir, envopt);
	}

	envopt = getenv("EDITOR");
	if (envopt != NULL)
		EDITOR = envopt;
}

void parse_defaults(newspost_data *data) {
	const char *envopt;
	char filename[STRING_BUFSIZE];
	char line[STRING_BUFSIZE];
	char *setting;
	char *header;
	FILE *file;
	int i, linenum = 0;

	envopt = getenv("HOME");
	if (envopt != NULL) {
		sprintf(filename, "%s/.newspostrc", envopt);
		file = fopen(filename, "r");
		if (file != NULL) {
			while (fgets(line, STRING_BUFSIZE, file) != NULL) {
				linenum++;

				/* ignore comment lines */
				for (i = 0; line[i] != ' ' ; i++);
				if (line[i] == '#')
					continue;

				setting = strchr(line, '=');
				if (setting != NULL) {
				    *setting++ = '\0';

				    for (i = 0; i < number_of_defaults; i++)
					if (strcmp(line, rc_keyword[i]) == 0)
					    break;

				    switch (i) {
				    case newsgroup:
					setfield(data->newsgroup, setting);
					break;
				    case from:
					setfield(data->from, setting);
					break;
				    case organization:
					setfield(data->organization, setting);
					break;
				    case address:
					setfield(data->address, setting);
					break;
				    case port:
					data->port = atoi(setting);
					break;
				    case user:
					setfield(data->user, setting);
					break;
				    case password:
					setfield(data->password, setting);
					break;
				    case lines:
					data->lines = atoi(setting);
					break;
				    case yenc:
					data->yenc = atoi(setting);
					break;
				    case filenumber:
					data->filenumber = atoi(setting);
					break;
				    case noarchive:
					data->noarchive = atoi(setting);
					break;
				    case filesperpar:
					data->filesperpar = atoi(setting);
					break;
				    case followupto:
					setfield(data->followupto, setting);
					break;
				    case replyto:
					setfield(data->replyto, setting);
					break;
				    case tmpdir:
					setfield(data->tmpdir, setting);
					break;
				    case name:
					setfield(data->name, setting);
					break;
				    case extraheader:
					header = malloc(STRING_BUFSIZE);
					setfield(header, setting);
					data->extra_headers =
					   slist_append(data->extra_headers,
							header);
					break;
				    default:
					fprintf(stderr,
					    "\nWARNING: invalid option in"
					    " %s: line %i",
					    filename, linenum);
				    }
				}
			}
			fclose(file);
		}
		else {
			if (errno != ENOENT)
				fprintf(stderr,
					"\nWARNING: %s %s", strerror(errno),
					filename);

			/* try to read the old .newspost */
			sprintf(filename, "%s/.newspost", envopt);
			file = fopen(filename, "r");
			if (file != NULL) {
				for (linenum = 1; linenum < 9; linenum++) {
					fgets(line, STRING_BUFSIZE, file);

					switch (linenum) {
					case 1:
						setfield(data->from, line);
						break;
					case 2:
						setfield(data->organization,
							 line);
						break;
					case 3:
						setfield(data->newsgroup,
							 line);
						break;
					case 4:
						setfield(data->address, line);
						break;
					case 5:
						data->port = atoi(line);
						break;
					case 6:
						setfield(data->user, line);
						break;
					case 7:
						setfield(data->password,
							 line);
						break;
					case 8:
						data->lines = atoi(line);
					}
				}
			}
		}
	}
	else
		fprintf(stderr,
			"\nWARNING: Unable to determine your home directory"
			"\nPlease set your HOME environment variable");
}

boolean set_defaults(newspost_data *data) {
	const char *envopt;
	char filename[STRING_BUFSIZE];
	FILE *file;
	SList *tmplist;

	envopt = getenv("HOME");
	if (envopt != NULL) {
		sprintf(filename, "%s/.newspostrc", envopt);
		file = fopen(filename, "w");
		if (file != NULL) {
			fprintf(file, "# %s\n%s=%s\n\n# %s\n%s=%s\n\n"
				"# %s\n%s=%s\n\n# %s\n%s=%s\n\n",
				rc_comment[newsgroup],
				rc_keyword[newsgroup], data->newsgroup,
				rc_comment[from],
				rc_keyword[from], data->from,
				rc_comment[organization],
				rc_keyword[organization], data->organization,
				rc_comment[address],
				rc_keyword[address], data->address);
			fprintf(file, "# %s\n%s=%i\n\n",
				rc_comment[port],
				rc_keyword[port], data->port);
			fprintf(file, "# %s\n%s=%s\n\n# %s\n%s=%s\n\n",
				rc_comment[user],
				rc_keyword[user], data->user,
				rc_comment[password],
				rc_keyword[password], data->password);
			fprintf(file, "# %s\n%s=%i\n\n# %s\n%s=%i\n\n"
				"# %s\n%s=%i\n\n# %s\n%s=%i\n\n",
				rc_comment[lines],
				rc_keyword[lines], data->lines,
				rc_comment[yenc],
				rc_keyword[yenc], data->yenc,
				rc_comment[filenumber],
				rc_keyword[filenumber], data->filenumber,
				rc_comment[noarchive],
				rc_keyword[noarchive], data->noarchive);
			fprintf(file, "# %s\n%s=%s\n\n# %s\n%s=%s\n\n#"
				" %s\n%s=%s\n\n",
				rc_comment[followupto],
				rc_keyword[followupto], data->followupto,
				rc_comment[replyto],
				rc_keyword[replyto], data->replyto,
				rc_comment[tmpdir],
				rc_keyword[tmpdir], data->tmpdir);
			fprintf(file, "# %s\n%s=%i\n\n",
				rc_comment[filesperpar],
				rc_keyword[filesperpar], data->filesperpar);
			fprintf(file, "# %s\n%s=%s\n\n",
				rc_comment[name],
				rc_keyword[name], data->name);
			fprintf(file, "# %s\n",
				rc_comment[extraheader]);

			tmplist = data->extra_headers;
			while (tmplist != NULL) {
				fprintf(file, "%s=%s\n",
					rc_keyword[extraheader],
					(const char *) tmplist->data);
				tmplist = slist_next(tmplist);
			}
			fclose(file);
			chmod(filename, S_IRUSR | S_IWUSR);
			return TRUE;
		}
		else {
			fprintf(stderr,
				"\nWARNING: %s %s", strerror(errno), filename);
			return FALSE;
		}
	}
	else {
		fprintf(stderr,
			"\nWARNING: Unable to determine your home directory"
			"\nPlease set your HOME environment variable");
		return FALSE;
	}
}

/* returns index of first non-option argument */
int parse_options(int argc, char **argv, newspost_data *data) {
	int flag, i;
	char *header;
	boolean isflag = FALSE;
	SList *listptr;

	opterr = 0;

	while (TRUE) {
		flag = getopt(argc, argv, valid_flags);
		if (flag == -1)
			break;
		else {
			switch (flag) {
				
			case help_option:
				print_help();
				break;

			case subject_option:
				setfield(data->subject, optarg);
				break;

			case newsgroup_option:
				setfield(data->newsgroup, optarg);
				break;

			case from_option:
				setfield(data->from, optarg);
				break;

			case organization_option:
				setfield(data->organization, optarg);
				break;

			case address_option:
				setfield(data->address, optarg);
				break;

			case port_option:
				data->port = atoi(optarg);
				break;

			case user_option:
				setfield(data->user, optarg);
				break;

			case password_option:
				setfield(data->password, optarg);
				break;

			case lines_option:
				data->lines = atoi(optarg);
				break;

			case alternate_prefix_option:
			case prefix_option:
				setfield(data->prefix, optarg);
				break;

			case yenc_option:
				data->yenc = TRUE;
				break;

			case sfv_option:
				setfield(data->sfv, optarg);
				break;

			case par_option:
				setfield(data->par, optarg);
				break;

			case parnum_option:
				data->parnum = atoi(optarg);
				break;

			case filesperpar_option:
				data->parnum = 0;
				data->filesperpar = atoi(optarg);
				break;

			case reference_option:
				setfield(data->reference, optarg);
				break;

			case default_option:
				writedefaults = TRUE;
				break;

			case version_option:
				version_info();
				printf("\n");
				exit(EXIT_NORMAL);

			case filenumber_option:
				data->filenumber = TRUE;
				break;

			case delay_option:
				parse_delay_option(optarg);
				break;

			case verbose_option:
				verbosity = TRUE;
				break;

			case noarchive_option:
				data->noarchive = FALSE;
				break;

			case followupto_option:
				setfield(data->followupto, optarg);
				break;

			case replyto_option:
				setfield(data->replyto, optarg);
				break;

			case tmpdir_option:
				setfield(data->tmpdir, optarg);
				break;

			case edit_prefix_option:
				temporary_prefix = TRUE;
				break;

			case name_option:
				setfield(data->name, optarg);
				break;

			case extraheader_option:
				header = malloc(STRING_BUFSIZE);
				setfield(header,optarg);
				data->extra_headers =
					slist_append(data->extra_headers,
						     header);
				break;

			case text_option:
				data->text = TRUE;
				break;

			case disable_option:
				switch (optarg[0]) {

				case user_option:
					setfield(data->user, "");
					break;

				case password_option:
					setfield(data->password, "");
					break;

				case name_option:
					setfield(data->name, "");
					break;

				case organization_option:
					setfield(data->organization, "");
					break;

				case followupto_option:
					setfield(data->followupto, "");
					break;

				case replyto_option:
					setfield(data->replyto, "");
					break;

				case noarchive_option:
					data->noarchive = TRUE;
					break;

				case extraheader_option:
					listptr = data->extra_headers;
					while (listptr != NULL) {
						free(listptr->data);
						listptr = slist_next(listptr);
					}
					slist_free(data->extra_headers);
					data->extra_headers = NULL;
					break;

				case yenc_option:
					data->yenc = FALSE;
					break;

				case filenumber_option:
					data->filenumber = FALSE;
					break;

				default:
					fprintf(stderr,
						"\nUnknown argument to"
						" -%c option: %c",
						disable_option,optarg[0]);
					exit(EXIT_MISSING_ARGUMENT);
				}
				break;
	
			case '?':
				/* a bit of nastiness... my linux box
				   sends me to this case though the man
				   page says it should send me to the
				   next */
				for (i = 0 ; valid_flags[i] != '\0' ; i++) {
					while (valid_flags[i] == ':') {
						i++;
					}
					if (valid_flags[i] == '\0')
						break;
					if (optopt == valid_flags[i])
						isflag = TRUE;
				}
				if (!isflag) {
		     
					fprintf(stderr,
						"\nUnknown argument: -%c",
						optopt);
					print_help();
				}
				/* else continue to the next case */
			
			case ':':
				fprintf(stderr,
					"\nThe -%c option requires"
					" an argument.\n", optopt);
				exit(EXIT_MISSING_ARGUMENT);
				
			default:
				fprintf(stderr,
					"\nUnknown argument: -%c", optopt);
				print_help();	
				break;
			}
		}
	}
	return optind;
}

void check_options(newspost_data *data) {
	FILE *testfile;
	boolean goterror = FALSE;
	
	if (data->newsgroup[0] == '\0') {
		fprintf(stderr,
			"\nThe newsgroup to post to is required.\n");
		goterror = TRUE;
	}
	else {
		const char *pi;
		int comma_count = 0;

		for (pi = data->newsgroup; *pi != '\0'; pi++)
			if (*pi == ',')
				comma_count++;

		if (comma_count > 4) {
			fprintf(stderr,
				"\nCrossposts are limited to 5 newsgroups.\n");
			goterror = TRUE;
		}
	}

	if (data->from[0] == '\0') {
		fprintf(stderr,
			"\nYour e-mail address is required.\n");
		goterror = TRUE;
	}
	if (data->address[0] == '\0') {
		fprintf(stderr,
			"\nThe news server's IP address or hostname"
			" is required.\n");
		goterror = TRUE;
	}
#ifndef ALLOW_NO_SUBJECT
	if (data->subject[0] == '\0') {
		fprintf(stderr,
			"\nThe subject line is required.\n");
		goterror = TRUE;
	}
#else
	if ((data->subject[0] == '\0') && (data->text == TRUE)) {
		fprintf(stderr,
			"\nThe subject line is always required"
			" when posting text.\n");
		goterror = TRUE;
	}
#endif
	if (data->lines > 20000) {
		if (data->lines > 50000) {
			fprintf(stderr,
				"\nYour messages have too many lines!"
				"\nSet the maximum number of lines to 10000\n");
			goterror = TRUE;
		}
		else {
			fprintf(stderr,
				"\nWARNING: Most uuencoded messages are"
				" 5000 to 10000 lines");
		}
	}
	else if (data->lines < 3000) {
		if (data->lines < 500) {
			fprintf(stderr,
				"\nYour messages have too few lines!"
				"\nSet the maximum number of lines to 5000\n");
			goterror = TRUE;
		}
		else {
			fprintf(stderr,
				"\nWARNING: Most uuencoded messages are"
				" 5000 to 10000 lines");
		}
	}
	if (data->prefix[0] != '\0') {
		testfile = fopen(data->prefix, "rb");
		if (testfile == NULL) {
			fprintf(stderr,
				"\nWARNING: %s %s -  NO PREFIX WILL BE POSTED",
				strerror(errno), data->prefix);
			data->prefix[0] = '\0';
		}
		else
			fclose(testfile);
	}
	if (goterror == TRUE)
		exit(EXIT_BAD_HEADER_LINE);
}

void print_help() {
	version_info();
	printf("\n\nUsage: newspost [OPTIONS [ARGUMENTS]]"
		" file1 file2 file3...");
	printf("\nOptions:");
	printf("\n  -%c ARG - hostname or IP of the news server",
		address_option);
	printf("\n  -%c ARG - port number on the news server", port_option);
	printf("\n  -%c ARG - username on the news server", user_option);
	printf("\n  -%c ARG - password on the news server", password_option);
	printf("\n  -%c ARG - your e-mail address", from_option);
	printf("\n  -%c ARG - your full name",name_option);
	printf("\n  -%c ARG - your organization", organization_option);
	printf("\n  -%c ARG - newsgroups to post to", newsgroup_option);
	printf("\n  -%c ARG - subject", subject_option);
	printf("\n  -%c ARG - newsgroup to put in the Followup-To header",
		followupto_option);
	printf("\n  -%c ARG - e-mail address to put in the Reply-To header",
		replyto_option);
	printf("\n  -%c ARG - reference these message IDs", reference_option);
	printf("\n  -%c     - do NOT include \"X-No-Archive: yes\" header",
		noarchive_option);
	printf("\n  -%c ARG - a complete header line", extraheader_option);
	printf("\n  -%c     - include \"File x of y\" in subject line",
		filenumber_option);
	printf("\n  -%c     - yencode instead of uuencode", yenc_option);
	printf("\n  -%c ARG - text prefix", prefix_option);
	printf("\n  -%c     - write text prefix in text editor set by $EDITOR",
		edit_prefix_option);
	printf("\n  -%c ARG - generate SFV file", sfv_option);
	printf("\n  -%c ARG - generate PAR files",par_option);
	printf("\n  -%c ARG - number of PAR volumes to create", parnum_option);
	printf("\n  -%c ARG - number of files per PAR volume",
		filesperpar_option);
	printf("\n  -%c ARG - number of lines per message", lines_option);
	printf("\n  -%c     - post one file as plain text", text_option);
	printf("\n  -%c ARG - time to wait before posting", delay_option);
	printf("\n  -%c ARG - use this directory for storing temporary files",
		tmpdir_option);
	printf("\n  -%c     - set current options as default", default_option);
	printf("\n  -%c ARG - disable or clear another option", disable_option);
	printf("\n  -%c     - be verbose", verbose_option);
	printf("\n  -%c     - print version info and exit", version_option);
	printf("\n  -%c     - display this help screen and exit", help_option);
	printf("\nPlease see the newspost manpage for more"
		" information and examples.");
	printf("\n");
	exit(EXIT_NORMAL);
}

/**
*** Private Routines
**/

static boolean parse_file_parts(newspost_data *data,
		file_entry *this_file_entry, const char *arg, int i) {

	int numparts, thisint, prevint;
	boolean postany, postall;

	numparts = get_number_of_encoded_parts(data, this_file_entry);
	this_file_entry->parts = (boolean *) calloc((numparts + 1),
		sizeof(boolean));
	/* i is on the colon */
	i++;
	while (i < strlen(arg)) {
		if ((arg[i] < '0') || (arg[i] > '9')) {
			i++;
			continue;
		}
		thisint = atoi(arg + i);
		if (thisint > numparts) {
			fprintf(stderr,
				"\nWARNING: %s only has %i parts",
				this_file_entry->filename, numparts);
			while ((arg[i] != ',') && (arg[i] != '\0'))
				i++;
			i++;
			continue;
		}
		else if (thisint < 1) {
			fprintf(stderr,
				"\nWARNING: invalid part number %i",
				thisint);
			while ((arg[i] != ',') && (arg[i] != '\0'))
				i++;
			i++;
			continue;
		}
		while ((arg[i] >= '0') && (arg[i] <= '9'))
			i++;

		switch (arg[i]) {
		case '+':
			while (thisint <= numparts) {
				this_file_entry->parts[thisint] = TRUE;
				thisint++;
			}
			i += 2; /* i++ should be a comma or nothing */
			continue;

		case '-':
			prevint = thisint;
			i++;
			thisint = atoi(arg + i);

			if (prevint > thisint) {	/* backwards! */
				int swap = prevint;
				prevint = thisint;
				thisint = swap;
			}
			if (thisint > numparts) {
				fprintf(stderr,
					"\nWARNING: %s only has %i parts",
					this_file_entry->filename, numparts);

				thisint = numparts;
			}

			while (prevint <= thisint)
				this_file_entry->parts[prevint++] = TRUE;

			/* make sure i is in the right place */
			while ((arg[i] >= '0') && (arg[i] <= '9'))
				i++;

			continue;

		case '\0':
		case ',':
			this_file_entry->parts[thisint] = TRUE;
			i++;
			continue;

		default:
			/* encountered an unexpected character */
			fprintf(stderr,
				"\nWARNING: bad character %c in %s - IGNORING",
				arg[i], arg);
			i++;
		}
	}

	/* are we posting any parts? */
	postany = FALSE;
	postall = TRUE;

	for (i = 1; i <= numparts; i++)
		if (this_file_entry->parts[i] == TRUE)
			postany = TRUE;
		else
			postall = FALSE;

	/* not posting any parts */
	if (postany == FALSE) {
		free(this_file_entry->parts);
		fprintf(stderr,
			"\nWARNING: Not posting %s: valid"
			" parts are 1 through %i",
			this_file_entry->filename, numparts);
		free(this_file_entry);
	}

	/* posting all parts despite colon */
	if (postall == TRUE) {
		free(this_file_entry->parts);
		this_file_entry->parts = NULL;

		fprintf(stderr,
			"\nWARNING: Posting ALL of %s (parts 1 - %i):"
			" is that what you meant to do?",
			this_file_entry->filename, numparts);
	}

	return postany;
}

/* Copy max of STRING_BUFSIZE chars from src to field, stopping at '\r',
   '\n', or end of string, and terminate copied string. */
static void setfield(char *field, const char *src) {
	const char *loc;

	loc = src;
	while ((*loc != '\0') && (*loc != '\r') && (*loc != '\n') &&
		((loc - src) < STRING_BUFSIZE))
			*field++ = *loc++;

	*field = '\0';
}

static void parse_delay_option(const char *option) {
	time_t hours = 0, minutes = 0, seconds = 0;

	seconds = atoi(option);
	while ((*option >= '0') && (*option <= '9'))
		option++;

	switch (*option) {

	case 'h':
	case 'H':
		hours = seconds;
		seconds = 0;
		break;

	case 'm':
	case 'M':
		minutes = seconds;
		seconds = 0;
		break;

	case ':':
		minutes = seconds;
		option++;
		seconds = atoi(option);
		while ((*option >= '0') && (*option <= '9'))
			option++;

		if (*option == ':') {
			hours = minutes;
			minutes = seconds;
			option++;
			seconds = atoi(option);
		}
	}

	post_delay = hours * 3600 + minutes * 60 + seconds;

	if (post_delay < 3)
		post_delay = 3;
}

static void version_info() {
	printf("\n" NEWSPOSTNAME " version " VERSION
		"\nCopyright (C) 2001 - 2002 Jim Faulkner"
		"\nThis is free software; see the source for"
		" copying conditions.  There is NO"
		"\nwarranty; not even for MERCHANTABILITY or"
		" FITNESS FOR A PARTICULAR PURPOSE.");
}
