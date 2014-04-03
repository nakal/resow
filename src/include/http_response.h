
#ifndef HTTP_RESPONSE_INCLUDED
#define HTTP_RESPONSE_INCLUDED

#include <time.h>

#include <http_request.h>

#define HTTP_RESPONSE_CHARSET "utf-8"

typedef enum {
	HTTP_STATUS_OK				=	0,
	HTTP_STATUS_CONTINUE			=	1,
	HTTP_STATUS_CREATED			=	2,
	HTTP_STATUS_BAD_REQUEST			=	3,
	HTTP_STATUS_NOT_FOUND			=	4,
	HTTP_STATUS_INTERNAL_SERVER_ERROR	=	5,
	HTTP_STATUS_METHOD_NOT_ALLOWED		=	6,
	HTTP_STATUS_UNSUPPORTED_MEDIA_TYPE	=	7,
	HTTP_STATUS_NOT_IMPLEMENTED		=	8,
	HTTP_STATUS_SERVICE_UNAVAILABLE		=	9,
	HTTP_STATUS_ACCEPTED			=	10,
	HTTP_STATUS_NOT_ACCEPTABLE		=	11,
	HTTP_STATUS_NO_CONTENT			=	12,
	HTTP_STATUS_TEMPORARY_REDIRECT		=	13,
	HTTP_STATUS_FOUND			=	14,
	HTTP_STATUS_GONE			=	15
} http_status_t;

typedef struct {
	unsigned int code;
	char *reason;
} http_status_code_t;

typedef struct {
	gchar *property;
	gchar *value;
} resow_http_header_t;

struct resow_stream_t;
typedef struct {

	gchar *content_type;
	GSList *headers;
	struct resow_stream_t *stream;
	int stream_written;

	const gchar *hostname;
	unsigned int port;

	gchar *default_location;

} http_response_t;

#define HTTP_RESPONSE_EMPTY ((gchar *)NULL)

extern const http_status_code_t http_response_reason[];

extern void http_response_send_error(http_status_t status,
	const char *fmt, ...);
extern void http_response_send_status(http_status_t status,
	const gchar *content_type);

extern http_response_t * http_response_init(const gchar *content_type,
	const request_cgi_env_t *env);
extern void http_response_close(http_response_t *r);
extern int http_response_add_header(http_response_t *r, const gchar *property,
	const gchar *valuefmt, ...);
extern gchar * http_response_uri_escape_string(const gchar *str);
extern gchar * http_response_html_escape_string(const gchar *str);
extern int http_response_set_content_type(http_response_t *r,
	const gchar *content_type);
extern gchar * http_response_make_time(time_t ts);
extern int http_response_set_etag(http_response_t *r, const gchar *etag);
extern int http_response_set_last_modified(http_response_t *r,
	time_t timestamp);
extern int http_response_set_location(http_response_t *r,
	const gchar *location);
extern void http_response_print_env(const request_cgi_env_t *env);

#endif

