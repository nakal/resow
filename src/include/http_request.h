
#ifndef RESOW_REQUEST_INCLUDED
#define RESOW_REQUEST_INCLUDED

#include <glib.h>

typedef gint http_method_t;
#define	HTTP_METHOD_UNDEFINED		0
#define	HTTP_METHOD_GET			1
#define	HTTP_METHOD_PUT			2
#define	HTTP_METHOD_POST		3
#define	HTTP_METHOD_DELETE		4
#define	HTTP_METHOD_HEAD		5
#define	HTTP_METHOD_OPTIONS		6

/*
 * In HTTP headers you find quite often specifications in form:
 * property; q=xxx
 */
#define RESOW_CGI_QUALITY_MIN 0
#define RESOW_CGI_QUALITY_MAX 1000
typedef struct {

	/* HTTP property (e.g. encoding, data type, language) */
	gchar *property;

	/* 0 till 1000 per mil, according to:
	 * http://www.w3.org/Protocols/rfc2616/rfc2616-sec3.html#sec3.9 */
	unsigned int quality_per_mil;

} request_property_quality_t;

/*
	HTTP request structure
*/
typedef struct {

	gchar *hostname; /* server hostname */
	unsigned int port; /* server port */

	gchar *uri; /* full request string */
	gchar *uri_path; /* request string without parameters */
	gchar *path_part; /* path part of request (after CGI, before "?") */
	gchar *resow_root; /* resow base path */
	gchar *service_root; /* resow service root path */
	gchar *service_path; /* path relative to the resolved service */

	http_method_t method; /* HTTP method */
	GSList *accept_datatype; /* http_accept header */
	GSList *accept_language; /* response language */
	GSList *accept_encoding; /* response encoding */
	GHashTable *query_parameters; /* part of request (after "?") */

	gchar *content_type;
	size_t content_length; /* length of content sent with the request */
	GHashTable *form_parameters; /* parsed form parameters */

	GHashTable *app_parameters; /* application settings for the binding */

	gchar *if_match;	/* ETag conditionals */
	gchar *if_none_match;

} request_cgi_env_t;


extern request_cgi_env_t * request_init_cgi_env(void);
extern void request_free_cgi_env(request_cgi_env_t *env);
extern unsigned int http_query_parameter_ulong(const request_cgi_env_t *env,
	const gchar *param);
extern unsigned int http_form_parameter_ulong(const request_cgi_env_t *env,
	const gchar *param);

#define HTTP_QUERY_PARAMETER(env,param) \
((env)->query_parameters ? \
(const gchar *)g_hash_table_lookup((env)->query_parameters, (param)) : \
NULL)

#define HTTP_FORM_PARAMETER(env,param) \
((env)->form_parameters ? \
(const gchar *)g_hash_table_lookup((env)->form_parameters, (param)) : \
NULL)

#define HTTP_QUERY_PARAMETER_ULONG(env,param) \
	http_query_parameter_ulong(env,param)

#define HTTP_FORM_PARAMETER_ULONG(env,param) \
	http_form_parameter_ulong(env,param)

#endif

