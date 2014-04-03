
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <http_response.h>
#include <stdcfg.h>

#include "../debug/debug.h"

const http_status_code_t http_response_reason[] = {
	{ 200, "OK" },
	{ 100, "Continue" },
	{ 201, "Created" },
	{ 400, "Bad request" },
	{ 404, "Not found" },
	{ 500, "Internal server error" },
	{ 405, "Method not allowed" },
	{ 415, "Unsupported media type" },
	{ 501, "Not implemented" },
	{ 503, "Service unavailable" },
	{ 202, "Accepted" },
	{ 406, "Not Acceptable" },
	{ 204, "No Content" },
	{ 307, "Temporary Redirect" },
	{ 302, "Found" },
	{ 410, "Gone" }
};

static void http_response_output_accept_properties(const gchar *msg,
	const gchar *env, GSList *list);

void http_response_send_error(http_status_t status, const char *fmt, ...) {

	va_list ap;

	va_start(ap, fmt);
	printf("Status: %03u %s\r\n", http_response_reason[status].code,
		http_response_reason[status].reason);
	printf("Content-Type: text/html\r\n");
	printf("Connection: close\r\n\r\n");
	printf("<html><head><title>Error %03u</title></head><body><h1>"
		"Error %03u</h1><h2>Reason</h2><p>%s</p><h2>Details</h2><p>",
		http_response_reason[status].code,
		http_response_reason[status].code,
		http_response_reason[status].reason
		);
	if (fmt!=NULL) vprintf(fmt, ap);
	printf("</p><hr /><p><em>resow-%s</em></p></body></html>\n",
		RESOW_VERSION);
	va_end(ap);
}

void http_response_send_status(http_status_t status,
	const gchar *content_type) {

	/*
		XXX needs a status fix, because some status needs extra
		headers under certain conditions (see RFC!)

		XXX might be neat to have content-type and actual
		status message
	*/

	printf("Status: %03u %s\r\n", http_response_reason[status].code,
			http_response_reason[status].reason);
	printf("Connection: close\r\n");

	if (strcmp(content_type, "text/html")==0) {
		printf("Content-Type: text/html\r\n\r\n");
		printf("<html><head><title>Status %03u</title></head><body><h1>"
				"Status %03u</h1><h2>Reason</h2><p>%s</p>",
				http_response_reason[status].code,
				http_response_reason[status].code,
				http_response_reason[status].reason
		      );
		printf("<hr /><p><em>resow-%s</em></p></body></html>\n",
				RESOW_VERSION);
	} else if (strcmp(content_type, "text/ascii")==0) {
			printf("Content-Type: text/ascii\r\n\r\n");
			printf("%03u: %s\r\n",
					http_response_reason[status].code,
					http_response_reason[status].reason
			      );
			printf("resow-%s\r\n", RESOW_VERSION);
	} else printf("\r\n");
}

http_response_t * http_response_init(const gchar *content_type,
	const request_cgi_env_t *env) {

	http_response_t *r;

	r = g_malloc(sizeof(http_response_t));
	if (r == NULL) {
		DEBUG_PRINT("Out of memory");
		return NULL;
	}

	r->headers = NULL;
	r->stream = NULL;
	r->stream_written = 0;
	r->content_type = g_strdup(content_type);
	if (r->content_type == NULL) {
		DEBUG_PRINT("Out of memory");
		g_free(r);
		return NULL;
	}

	r->hostname = env->hostname;
	r->port = env->port;

	r->default_location = g_strdup(env->uri);

	return r;
}

void http_response_close(http_response_t *r) {

	/* notice:
	 * Headers are cleaned in stream.c (stream_write_headers).
	 * Need only traverse the list once.
	 */

	if (r!=NULL) {
		g_free(r->default_location);
		if (r->content_type) g_free(r->content_type);
		r->headers=NULL;
	}
	g_free(r);
}

int http_response_add_header(http_response_t *r, const gchar *property,
	const gchar *valuefmt, ...) {

	va_list ap;
	gchar *h, *v;
	resow_http_header_t *header;

	if (property==NULL || valuefmt==NULL) return -1;

	/* protect certain headers */
	if (g_ascii_strcasecmp(property, "Status")==0 ||
		g_ascii_strcasecmp(property, "Content-Type")==0) return -1;

	header=(resow_http_header_t *)g_malloc(sizeof(resow_http_header_t));
	if (header==NULL) return -1;

	h=g_strdup(property);
	if (h==NULL) {
		g_free(header);
		return -1;
	}

	va_start(ap, valuefmt);
	v=g_strdup_vprintf(valuefmt, ap);
	va_end(ap);
	if (v==NULL) {
		g_free(h);
		g_free(header);
		return -1;
	}

	header->property = h;
	header->value = v;
	r->headers = g_slist_prepend(r->headers, header);

	return 0;
}

int http_response_set_content_type(http_response_t *r,
	const gchar *content_type) {

	if (r->content_type!=NULL) g_free(r->content_type);

	if (content_type!=HTTP_RESPONSE_EMPTY) {
		r->content_type = g_strdup(content_type);
		return r->content_type==NULL;
	}

	r->content_type=HTTP_RESPONSE_EMPTY;
	return 0;
}

int http_response_set_etag(http_response_t *r, const gchar *etag) {
	return http_response_add_header(r, "ETag", etag);
}

int http_response_set_last_modified(http_response_t *r, time_t timestamp) {

	gchar *timestr;
	int ret;

	timestr = http_response_make_time(timestamp);
	ret = http_response_add_header(r, "Last-Modified", timestr);
	g_free(timestr);

	return ret;
}

int http_response_set_location(http_response_t *r, const gchar *location) {

	return http_response_add_header(r, "Location",
		location ? location : r->default_location);
}

gchar * http_response_uri_escape_string(const gchar *str) {

	gchar *retstr;

	if (str==NULL) return NULL;

	retstr = g_uri_escape_string(str, "", TRUE);
	return retstr;
}

gchar * http_response_html_escape_string(const gchar *str) {

	/* TODO verify this method */
	const gchar *find="<>&\"";
	const gchar *replace[]={ "&lt;", "&gt;", "&amp;", "&quot;" };
	size_t len, findlen, i;
	gchar *outstr, *dstr;
	const gchar *p;

	if (str==NULL) return NULL;

	len = strlen(str)*6+1; /* longest replacement is &quot; */

	outstr=(gchar *)g_malloc(len);
	if (outstr==NULL) return NULL;

	findlen = strlen(find);
	dstr = outstr;
	for (p=str; *p; p++) {

		for (i=0; i<findlen && *p!=find[i]; i++);

		if (i<findlen) {
			const gchar *q;

			q=replace[i]; do { *dstr++=*q++; } while (*q);
		} else *dstr++=*p;
	}
	*dstr=0;

	return outstr;
}

gchar * http_response_make_time(time_t ts) {

	/* TODO verify this method */

	struct tm t;
	gchar *retstr;

	gmtime_r(&ts, &t);
	retstr = g_malloc(32);

	if (strftime(retstr, 32, "%a, %d %b %Y %T GMT", &t)==0) {
		g_free(retstr);
		return NULL;
	} else return retstr;
}

void http_response_print_env(const request_cgi_env_t *env) {

	GHashTableIter iter;
	gpointer key, value;
	char buf[1024];
	ssize_t rd;

	printf("Content-Type: text/html\r\n");
	printf("Connection: close\r\n\r\n");

	printf("<html><head><title>resow request debug output"
		"</title></head>\n<body><h1>resow request debug output</h1><p>"
		"HTTP method: <tt>%s</tt><br />\n"
		"HTTP request: <tt>%s</tt><br />\n"
		"HTTP path: <tt>%s</tt><br />\n",
		env->method==HTTP_METHOD_GET ? "GET" : env->method==HTTP_METHOD_POST ? "POST" : env->method==HTTP_METHOD_PUT ? "PUT" : env->method==HTTP_METHOD_DELETE ? "DELETE" : "unknown",
		env->uri, env->path_part);

	printf("resow service root: <tt>%s</tt><br />\n", env->service_root);
	printf("resow service path: <tt>%s</tt><br />\n", env->service_path);

	printf("<br />GET Parameters:<ul>\n");

	g_hash_table_iter_init (&iter, env->query_parameters);
	while (g_hash_table_iter_next (&iter, &key, &value))
		printf("<li><tt>%s</tt>: <tt>%s</tt></li>\n",
			(gchar *)key, (gchar *)value);
	printf("</ul>\n");
	printf("<br />POST Parameters:<ul>\n");

	g_hash_table_iter_init (&iter, env->form_parameters);
	while (g_hash_table_iter_next (&iter, &key, &value))
		printf("<li><tt>%s</tt>: <tt>%s</tt></li>\n",
			(gchar *)key, (gchar *)value);
	printf("</ul>\n");

	http_response_output_accept_properties
	("Output format", "HTTP_ACCEPT", env->accept_datatype);
	http_response_output_accept_properties
	("Output language", "HTTP_ACCEPT_LANGUAGE", env->accept_language);
	http_response_output_accept_properties
	("Output encoding", "HTTP_ACCEPT_ENCODING", env->accept_encoding);

	printf("resow content length: <tt>%lu</tt><br />\n", env->content_length);

	if (env->content_length>0) {
		printf("resow content type: <tt>%s</tt><br /><br />\n", env->content_type);
		printf("<pre>\n");
		fflush(stdout);
		while ((rd=read(0, buf, sizeof(buf)))>0) {
			write(1, buf, rd);
		}
		printf("</pre>\n");
	}

	printf("</p></body></html>");
}

void http_response_output_accept_properties(const gchar *msg, const gchar *env,
	GSList *list) {

	printf("%s: <tt>%s</tt><ul>", msg, getenv(env));
	while (list) {
		request_property_quality_t *prop;

		prop = (request_property_quality_t *)list->data;
		printf("<li>%s (q: %u per mil)</li>\n", prop->property,
			prop->quality_per_mil);
		list = g_slist_next(list);
	}
	printf("</ul>\n");
}

