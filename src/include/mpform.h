
#ifndef HTTP_MPFORM_INCLUDED
#define HTTP_MPFORM_INCLUDED

#include <http_request.h>

#define MPFORM_RECVBUF_SIZE 32768

typedef struct {
	GSList *file_field_names;
	GSList *file_names;
	GSList *file_content_types;
	GSList *file_checksums;
} mpform_t;

extern mpform_t * mpform_read(const request_cgi_env_t *env);
extern void mpform_close(mpform_t *mpform);

#endif

