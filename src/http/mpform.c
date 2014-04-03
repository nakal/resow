
#include <string.h>
#include <unistd.h>

#include <mpform.h>

#include "../debug/debug.h"

#define OUTBUF_SIZE 4096

typedef struct {
	int expect_header;
	int writing_file;
	gchar *outbuf;
	ssize_t outbufpos;
} mpform_parse_t;

static int mpform_parse_data(mpform_t *f, const request_cgi_env_t *env,
	const gchar *boundary);
static int mpform_parse_content(mpform_parse_t *parse);

mpform_t * mpform_read(const request_cgi_env_t *env) {

	gchar **tok;
	gchar *boundary;
	mpform_t *f;

	if (env->content_type==NULL) return NULL;

	/* figure out MIME part boundary */
	tok = g_strsplit(env->content_type, ";", 2);
	g_strstrip(tok[0]); g_strstrip(tok[1]);

	if (strcmp(tok[0], "multipart/form-data")!=0) return NULL;
	if(!g_str_has_prefix(tok[1], "boundary=")) return NULL;
	boundary = g_strconcat("\n--", tok[1]+9, NULL);
	g_strfreev(tok);

	f = g_malloc0(sizeof(mpform_t));
	if (f==NULL) return NULL;

	if (mpform_parse_data(f, env, boundary)!=0) {
		/* XXX erase phantom files? */
		mpform_close(f);
		g_free(boundary);
		return NULL;
	}

	g_free(boundary);

	return f;
}

void mpform_close(mpform_t *mpform) {

	if (mpform) {
		/* TODO free pointers with content data */
		g_slist_free(mpform->file_content_types);
		g_slist_free(mpform->file_field_names);
		g_slist_free(mpform->file_names);
		g_free(mpform);
	}
}

static int mpform_parse_data(mpform_t *f, const request_cgi_env_t *env,
	const gchar *boundary) {

	gchar *buf, *p;
	ssize_t bufsize, bufpos;
	const char *bpos=NULL;
	mpform_parse_t parse;
	int retval=0;

	memset(&parse, 0, sizeof(mpform_parse_t));

	buf = g_malloc(MPFORM_RECVBUF_SIZE);
	if (buf==NULL) return -1;
	/* add length of boundary to do boundary detection easier */
	parse.outbuf = g_malloc(OUTBUF_SIZE+strlen(boundary)+2);
	if (parse.outbuf==NULL) {
		g_free(buf);
		return -1;
	}

	/* first look for boundary */
	bpos=boundary+1; parse.outbufpos=0;
	while ((bufsize=read(0, buf, MPFORM_RECVBUF_SIZE))>0) {
		bufpos=0; p=buf;

		while (bufpos<bufsize) {
			if (bpos!=NULL) {
				/* doing boundary scanning */
				/* printf("in boundary scan:\n"); */
				if (*bpos==0) {
					/* found, check if form end */
					if (*p=='-') goto parse_exit;
					else {
						if (*p=='\n') {
							if (mpform_parse_content(&parse)!=0) { retval=-1; goto parse_exit; }
							/*printf("--------------------boundary found\n");fflush(stdout);*/
							parse.expect_header=1;
							bpos=NULL;
						}
					}
					p++; bufpos++;
				} else {
					if (*bpos==*p) {
						bpos++;
						p++; bufpos++;
					} else {
						/*
							is not a boundary,
							copy prefix into
							outbuf
						*/

						size_t len=0;

						while (bpos!=boundary) {
							bpos--; len++;
						}
						/* parse.outbuf[parse.outbufpos++]='\n'; */
						memcpy(parse.outbuf+parse.outbufpos,
							boundary, len);
						parse.outbufpos+=len;
						bpos=NULL;

						if (parse.outbufpos>=
							OUTBUF_SIZE) {
							if (mpform_parse_content(&parse)!=0) { retval=-1; goto parse_exit; }
						}
					}
				}
			} else {
				/* scan for boundary start (\n) */
				while (bufpos<bufsize && *p!='\n') {

					parse.outbuf[parse.outbufpos]=*p++;
					bufpos++; parse.outbufpos++;
					if (parse.outbufpos>=OUTBUF_SIZE) {
						if (mpform_parse_content(&parse)!=0) { retval=-1; goto parse_exit; }
					}
				}

				/*
					if '\n' found, goto boundary detection
					next time
				*/
				if (bufpos<bufsize) bpos=boundary;

				if (parse.expect_header) {
					if (mpform_parse_content(&parse)!=0) { retval=-1; goto parse_exit; }
				}
			}
		}

	}

parse_exit:
	/* while (bufsize=read()>0) */
	mpform_parse_content(&parse);
	g_free(buf); g_free(parse.outbuf);

	return retval;
}

#include <stdio.h>
static int mpform_parse_content(mpform_parse_t *parse) {

	fflush(stdout);
	if (parse->expect_header) {

		gchar *header;

		header = g_strndup(parse->outbuf, parse->outbufpos);
		g_strstrip(header);

		if (strlen(header)==0) {
			parse->expect_header=0;
			printf("DATA FOLLOWS:\n");
		} else {

			printf("HEADER: %s\n", header);
		}

		g_free(header);
	} else {

		write(1, parse->outbuf, parse->outbufpos);
	}

	parse->outbufpos=0;

	return 0;
}

