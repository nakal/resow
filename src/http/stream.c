
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>

#include <stdcfg.h>
#include <stream.h>

#include "../debug/debug.h"

static void stream_write_headers(http_response_t *r);
static void stream_write_buffer(resow_stream_t *stream);

int stream_open(http_response_t *r,
	const http_status_t http_status,
	int use_buffer) {

	resow_stream_t *stream;

	if (http_status!=HTTP_STATUS_NO_CONTENT) {
		if (r->content_type!=HTTP_RESPONSE_EMPTY &&
			strchr(r->content_type, '*')!=NULL) {

			DEBUG_PRINT("Content-Type is a placeholder.");
			return RESOW_STREAM_ERROR;
		}
	} else {
		g_free(r->content_type);
		r->content_type=HTTP_RESPONSE_EMPTY;
	}

	stream = g_malloc(sizeof(resow_stream_t));
	if (stream==NULL) return RESOW_STREAM_ERROR;

	r->stream = (struct resow_stream_t *)stream;
	stream->http_status = http_status;
	stream->is_buffered = use_buffer;

	if (use_buffer) {
		stream->buffer=g_malloc(STREAM_MIN_BUFFERLEN);
		if (stream->buffer==NULL) {
			g_free(stream);
			return RESOW_STREAM_ERROR;
		}
		stream->buffer_len=STREAM_MIN_BUFFERLEN;
		stream->buffer_pos=0;
	} else {
		stream->buffer=NULL;
		stream->buffer_len=0;
		stream->buffer_pos=0;

		stream_write_headers(r);
	}

	return RESOW_STREAM_OK;
}

void stream_close(http_response_t *r) {

	resow_stream_t *stream;

	stream = (resow_stream_t *)r->stream;
	if (stream->is_buffered) {
		/*
		 * Write out headers and buffer contents
		 * free buffer
		 */
		stream_write_headers(r);
		stream_write_buffer(stream);
		g_free(stream->buffer);
	}

	/* g_free(stream->mime_type); */
	g_free(stream);
	r->stream=NULL;
}

int stream_printf(http_response_t *r, const char *fmt, ...) {

	va_list ap;
	resow_stream_t *stream;

	if (r->content_type==HTTP_RESPONSE_EMPTY) return -1;

	va_start(ap, fmt);
	stream = (resow_stream_t *)r->stream;
	if (stream->is_buffered) {
		size_t maxlen;
		int retlen;

		maxlen = stream->buffer_len-stream->buffer_pos;
		retlen = vsnprintf(stream->buffer+stream->buffer_pos, maxlen,
			fmt, ap);
		/*printf("retlen(1)::: %s\n", stream->buffer+stream->buffer_pos);*/
		assert(retlen>=0);
		if (retlen>=maxlen) {
			size_t newsize;
			gpointer p;

			newsize = (stream->buffer_len+retlen)*2;
			p=g_realloc(stream->buffer, newsize);
			if (p==NULL) return RESOW_STREAM_ERROR;
			stream->buffer=p;
			stream->buffer_len=newsize;
			retlen = vsnprintf(stream->buffer+stream->buffer_pos,
				maxlen, fmt, ap);
			/*printf("retlen(2)::: %s\n", stream->buffer+stream->buffer_pos);*/
		}

		assert(retlen>=0 &&
			retlen<stream->buffer_len-stream->buffer_pos);
		stream->buffer_pos+=retlen;
	} else {
		vprintf(fmt, ap);
	}
	va_end(ap);

	return RESOW_STREAM_OK;
}

int stream_write(http_response_t *r, const char *buf, size_t size) {

	resow_stream_t *stream;

	if (r->content_type==HTTP_RESPONSE_EMPTY) return -1;

	stream = (resow_stream_t *)r->stream;
	if (stream->is_buffered) {

		size_t maxlen;

		maxlen = stream->buffer_len-stream->buffer_pos;
		if (size>maxlen) {
			size_t newsize;
			gpointer p;

			newsize = (stream->buffer_len+size)*2;
			p=g_realloc(stream->buffer, newsize);
			if (p==NULL) return RESOW_STREAM_ERROR;

			stream->buffer=p;
			stream->buffer_len=newsize;
		}
		assert(stream->buffer_pos+size<stream->buffer_len);
		memcpy(stream->buffer+stream->buffer_pos, buf, size);
		stream->buffer_pos+=size;
	} else {
		write(0, buf, size);
	}

	return RESOW_STREAM_OK;
}

static void stream_write_headers(http_response_t *r) {

	resow_stream_t *stream;
	GSList *h;

	stream = (resow_stream_t *)r->stream;

	/* mandatory headers */
	assert(stream!=NULL);
	printf("Status: %03d %s\r\n",
		http_response_reason[stream->http_status].code,
		http_response_reason[stream->http_status].reason);
	r->stream_written=1;
	if (r->port==80) printf("Host: %s\r\n", r->hostname);
	else printf("Host: %s:%u\r\n", r->hostname, r->port);

	if (r->content_type!=HTTP_RESPONSE_EMPTY) {
		if (g_str_has_prefix(r->content_type, "text/"))
			printf("Content-Type: %s; charset=%s\r\n", r->content_type,
				HTTP_RESPONSE_CHARSET);
		else printf("Content-Type: %s\r\n", r->content_type);

		if (stream->is_buffered)
			printf("Content-Length: %lu\r\n", stream->buffer_pos);
	}

	/* optional headers */
	r->headers=g_slist_reverse(r->headers);
	h=r->headers;
	while (h!=NULL) {

		resow_http_header_t *header;

		header = (resow_http_header_t *)h->data;
		printf("%s: %s\r\n", header->property, header->value);

		g_free(header->property);
		g_free(header->value);
		g_free(header);

		h=h->next;
	}
	g_slist_free(r->headers);
	r->headers=NULL;

	/* TODO: cache tagging, validity */

	/* end of headers */
	printf("\r\n");
	fflush(stdout);
}

static void stream_write_buffer(resow_stream_t *stream) {

	size_t pos=0;

	while (pos<stream->buffer_pos) {

		size_t len;

		len = stream->buffer_pos-pos;
		if (len>STREAM_BUFFER_WRITE_SIZE) len=STREAM_BUFFER_WRITE_SIZE;

		write(1, stream->buffer+pos, len);
		pos+=len;
	}
}

