
#ifndef RESOW_STREAM_H_INCLUDED
#define RESOW_STREAM_H_INCLUDED

#include <http_request.h>
#include <http_response.h>

#define STREAM_MIN_BUFFERLEN 16384
#define STREAM_BUFFER_WRITE_SIZE 1024
typedef struct {
	http_status_t http_status;
	const gchar *mime_type;

	int is_buffered;
	char *buffer;
	size_t buffer_len;
	size_t buffer_pos;
} resow_stream_t;

extern int stream_open(http_response_t *r, const http_status_t http_status,
	int use_buffer);
extern void stream_close(http_response_t *r);
extern int stream_printf(http_response_t *r, const char *fmt, ...);
extern int stream_write(http_response_t *r, const char *buf, size_t size);

#define RESOW_STREAM_OK		0
#define RESOW_STREAM_ERROR	-1

#endif

