
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <http_request.h>

#include "../debug/debug.h"

/* maximal form size */
#define REQUEST_MAX_FORM_LEN 1048576

static int request_parse_query(const gchar *query, GHashTable *params);
static GSList * request_parse_accepts_properties(const gchar *qstr,
	const gchar *def_property);
static gint request_quality_compare(gconstpointer a, gconstpointer b);
static void request_free_accepts_properties(GSList *list);
static void request_free_property_quality(gpointer elem, gpointer data);
static gchar *request_get_form_data(ssize_t input_size);
static int http_request_get_uri_path(request_cgi_env_t *env);

request_cgi_env_t * request_init_cgi_env(void) {

	request_cgi_env_t *env;
	const gchar *e;

	env = g_malloc0(sizeof(request_cgi_env_t));
	if (env == NULL) {
		DEBUG_PRINT("Out of memory (env)");
		return NULL;
	}

	/* get hostname and port */
	e = g_getenv("HTTP_HOST");
	if (e == NULL) {
		DEBUG_PRINT("No hostname given");
		request_free_cgi_env(env);
		return NULL;
	}
	env->hostname = g_strdup(e);
	if (env->hostname == NULL) {
		DEBUG_PRINT("Out of memory for HTTP_HOST");
		request_free_cgi_env(env);
		return NULL;
	}
	e = g_getenv("SERVER_PORT");
	if (e == NULL) {
		env->port=80;
	} else {
		env->port = (unsigned int)strtoul(e, NULL, 10);

		/* XXX fix up... port 0 is mostly reserved */
		if (env->port==0) env->port=80;
	}

	/* get resow root path */
	e = g_getenv("SCRIPT_NAME");
	if (e == NULL) {
		DEBUG_PRINT("SCRIPT_NAME (base path) not set");
		request_free_cgi_env(env);
		return NULL;
	}
	env->resow_root = g_strdup(e);
	if (env->resow_root == NULL) {
		DEBUG_PRINT("Out of memory for SCRIPT_NAME");
		request_free_cgi_env(env);
		return NULL;
	}

	/* determine request method */
	e = g_getenv("REQUEST_METHOD");
	if (e == NULL) {
		DEBUG_PRINT("No request method");
		request_free_cgi_env(env);
		return NULL;
	}

	env->method = 0;
	if (!g_ascii_strcasecmp(e, "GET")) env->method = HTTP_METHOD_GET;
	else if (!g_ascii_strcasecmp(e, "POST")) env->method = HTTP_METHOD_POST;
	else if (!g_ascii_strcasecmp(e, "PUT")) env->method = HTTP_METHOD_PUT;
	else if (!g_ascii_strcasecmp(e, "DELETE"))
		env->method = HTTP_METHOD_DELETE;
	else {
		DEBUG_PRINT("Unknown HTTP method");
		request_free_cgi_env(env);
		return NULL;
	}

	/* store full request */
	e = g_getenv("REQUEST_URI");
	if (e == NULL) {
		DEBUG_PRINT("Unknown REQUEST_URI not set");
		request_free_cgi_env(env);
		return NULL;
	}
	env->uri = g_strdup(e);
	if (env->uri == NULL) {
		DEBUG_PRINT("Out of memory for REQUEST_URI");
		request_free_cgi_env(env);
		return NULL;
	}
	
	/* store path part */
	e = g_getenv("PATH_INFO");
	if (e == NULL) e="";
	env->path_part = g_strdup(e);
	if (env->path_part == NULL) {
		DEBUG_PRINT("Out of memory for PATH_INFO");
		request_free_cgi_env(env);
		return NULL;
	}

	/* calc uri path */
	if (http_request_get_uri_path(env)!=0) {
		DEBUG_PRINT("Out of memory for uri path");
		request_free_cgi_env(env);
		return NULL;
	}
	
	/* store parameters into a hash structure */
	e = g_getenv("QUERY_STRING");
	if (e == NULL) e="";
	env->query_parameters = g_hash_table_new_full
		(g_str_hash, g_str_equal, g_free, g_free);
	if (env->query_parameters == NULL) {
		DEBUG_PRINT("Out of memory for QUERY_STRING");
		request_free_cgi_env(env);
		return NULL;
	}
	if (request_parse_query(e, env->query_parameters)!=0) {
		DEBUG_PRINT("Out of memory for QUERY_STRING in parser");
		request_free_cgi_env(env);
		return NULL;
	}

	/* parse HTTP_ACCEPT header */
	e = g_getenv("HTTP_ACCEPT");
	if (e == NULL) e="";
	env->accept_datatype = request_parse_accepts_properties(e, "*/*");
	if (env->accept_datatype == NULL) {
		DEBUG_PRINT("request_parse_accepts_properties on HTTP_ACCEPT returned NULL");
		request_free_cgi_env(env);
		return NULL;
	}

	/* parse HTTP_ACCEPT_LANGUAGE header */
	e = g_getenv("HTTP_ACCEPT_LANGUAGE");
	if (e == NULL) e="";
	env->accept_language = request_parse_accepts_properties(e, "en");
	if (env->accept_language == NULL) {
		DEBUG_PRINT("request_parse_accepts_properties on HTTP_ACCEPT_LANGUAGE returned NULL");
		request_free_cgi_env(env);
		return NULL;
	}

	/* parse HTTP_ACCEPT_ENCODING header */
	e = g_getenv("HTTP_ACCEPT_ENCODING");
	if (e == NULL) e="";
	env->accept_encoding = request_parse_accepts_properties(e, NULL);

	/* content length */
	e = g_getenv("CONTENT_LENGTH");
	if (e!=NULL) {
		env->content_length=strtoul(e, NULL, 10);
	} else env->content_length=0;
	/*
fprintf(stderr, "CL: %li\n", env->content_length);
fprintf(stderr, "R: (%u) %s\n", env->method, env->uri);
system("env > /tmp/env");
*/

	e = g_getenv("CONTENT_TYPE");
	if (e != NULL) {
		env->content_type=g_strdup(e);
		if (strcmp(e,"application/x-www-form-urlencoded")==0) {
			/* parse form data */
			gchar *formdata;

			formdata=request_get_form_data(env->content_length);
			if (formdata == NULL) {
				DEBUG_PRINT("Out of memory for form data");
				request_free_cgi_env(env);
				return NULL;
			}
			env->form_parameters = g_hash_table_new_full
				(g_str_hash, g_str_equal, g_free, g_free);
			if (env->form_parameters == NULL) {
				DEBUG_PRINT("Out of memory for form");
				g_free(formdata);
				request_free_cgi_env(env);
				return NULL;
			}
			if (request_parse_query(formdata,
				env->form_parameters)!=0) {
				DEBUG_PRINT("Out of memory for parsing form");
				g_free(formdata);
				request_free_cgi_env(env);
				return NULL;
			}
			env->content_length=0; /* clear; nothing more to read */
			g_free(formdata);
		} else env->form_parameters=NULL;
	} else {
		env->content_type=NULL;
		env->form_parameters=NULL;
	}

	if ((e = g_getenv("HTTP_IF_NONE_MATCH"))!=NULL) {
		env->if_none_match=g_strdup(e);
	}
	if ((e = g_getenv("HTTP_IF_MATCH"))!=NULL) {
		env->if_match=g_strdup(e);
	}

	return env;
}

void request_free_cgi_env(request_cgi_env_t *env) {

	if (env != NULL) {
		if (env->hostname!=NULL) g_free(env->hostname);
		if (env->resow_root!=NULL) g_free(env->resow_root);
		if (env->service_root!=NULL) g_free(env->service_root);
		if (env->service_path!=NULL) g_free(env->service_path);
		if (env->uri != NULL) g_free(env->uri);
		if (env->uri_path != NULL) g_free(env->uri_path);
		if (env->path_part != NULL) g_free(env->path_part);
		if (env->content_type != NULL) g_free(env->path_part);
		if (env->query_parameters != NULL)
			g_hash_table_destroy(env->query_parameters);
		if (env->form_parameters != NULL)
			g_hash_table_destroy(env->form_parameters);

		if (env->if_none_match!=NULL) g_free(env->if_none_match);
		if (env->if_match!=NULL) g_free(env->if_match);
		g_free(env);
	}
}

unsigned int http_query_parameter_ulong(const request_cgi_env_t *env,
	const gchar *param) {

	const gchar *str;

	str = HTTP_QUERY_PARAMETER(env,param);

	return str==NULL ? 0 : strtoul(str, NULL, 10);
}

unsigned int http_form_parameter_ulong(const request_cgi_env_t *env,
	const gchar *param) {

	const gchar *str;

	str = HTTP_FORM_PARAMETER(env,param);

	return str==NULL ? 0 : strtoul(str, NULL, 10);
}

static gchar *request_get_form_data(ssize_t input_size) {

	gchar *ret;
	ssize_t rlen;

	if (input_size>REQUEST_MAX_FORM_LEN || input_size<=0) {
#ifdef DEBUG
		fprintf(stderr, "Form data size error: requested: %li bytes.\n",
			input_size);
#endif
		return NULL;
	}

	ret = (gchar *)g_malloc(input_size);
	if (ret==NULL) {
#ifdef DEBUG
		fprintf(stderr, "Failed to allocate memory.\n");
#endif
	}

	rlen=read(0, ret, input_size);
	if (rlen!=input_size) {
#ifdef DEBUG
		fprintf(stderr, "Read error: rlen=%li, input_size=%li\n",
			rlen, input_size);
#endif
		g_free(ret);
		ret=NULL;
	}

	return ret;
}

static int request_parse_query(const gchar *query, GHashTable *params) {

	gchar **v, **p;
	
	v=g_strsplit(query, "&", 0);
	if (v==NULL) return -1;

	for (p=v; *p; p++) {

		gchar *key=NULL, *val=NULL, *uval=NULL;
		gchar **vv=NULL;
		
		vv=g_strsplit_set(*p, "=", 2);
		if (vv==NULL) {
			DEBUG_PRINT("g_strsplit_set returned NULL");
			continue;
		}

		key=g_strdup(*vv);
		if (key==NULL) {
			DEBUG_PRINT("g_strdup returned NULL");
			g_strfreev(vv);
			continue;
		}

		val= g_strdup(*(vv+1)!=NULL ? *(vv+1) : "");
		if (val==NULL) {
			DEBUG_PRINT("g_strdup returned NULL");
			g_free(key);
			g_strfreev(vv);
			continue;
		}

		g_strfreev(vv);

		uval = g_uri_unescape_string(val, NULL);

		if (uval==NULL) {
			DEBUG_PRINT("g_uri_unescape_string returned NULL");
			g_free(key); g_free(val);
			continue;
		}
		g_free(val);

#ifdef DEBUG
fprintf(stderr, "Parameter: %s -> %s\n", key, uval);
#endif
		g_hash_table_insert(params, key, uval);
	}

	g_strfreev(v);

	return 0;
}

static GSList * request_parse_accepts_properties(const gchar *qstr,
	const gchar *def_property) {

	GSList *list = NULL;
	gchar **v, **p;

	v = g_strsplit_set(qstr, ",\n", 0);
	if (v==NULL) {
		DEBUG_PRINT("g_strsplit returned NULL");
		return NULL;
	}

	for (p=v; *p; p++) {

		gchar **vv=NULL;
		gchar *prop, *qual;
		request_property_quality_t *e;

		e=g_malloc(sizeof(request_property_quality_t));
		if (e==NULL) {
			request_free_accepts_properties(list);
			g_strfreev(v);
			return NULL;
		}

		vv=g_strsplit(*p, ";", 2);
		if (vv==NULL) {
			DEBUG_PRINT("g_strsplit returned NULL");
			g_free(e);
			continue;
		}

		prop=g_strdup(*vv);
		if (prop==NULL) {
			DEBUG_PRINT("g_strdup returned NULL");
			g_strfreev(vv);
			g_free(e);
			continue;
		}

		qual=g_strdup(*(vv+1)!=NULL ? *(vv+1): "1.0");
		if (prop==NULL) {
			DEBUG_PRINT("g_strdup returned NULL");
			g_free(prop);
			g_strfreev(vv);
			g_free(e);
			continue;
		}

		g_strfreev(vv);

		g_strchug(qual); if (strlen(qual)>5) qual[5]=0;
		e->property = prop;
		if (strncmp(qual, "q=0.", 2)==0) {
			unsigned int mul=100, d=3;

			e->quality_per_mil=0;
			while (g_ascii_isdigit(qual[++d])) {
				e->quality_per_mil+=
					(unsigned int)(qual[d]-'0')*mul;
				mul/=10;
			}
		} else {
			/* XXX else we assume 1.xxx for everything invalid */
			e->quality_per_mil = 1000;
		}

		list=g_slist_insert_sorted(list, e, request_quality_compare);

		g_free(qual);
	}

	g_strfreev(v);

	/* we append default with quality 1000, if nothing requested */
	if (list==NULL && def_property!=NULL) {
		request_property_quality_t *e;

		e=g_malloc(sizeof(request_property_quality_t));
		if (e==NULL) return NULL;

		e->property=g_strdup(def_property);
		e->quality_per_mil=RESOW_CGI_QUALITY_MAX;

		list=g_slist_append(list, e);
	}

	return list;
}

static int http_request_get_uri_path(request_cgi_env_t *env) {

	size_t qmpos;
	gchar *p;

	assert(env!=NULL);
	assert(env->uri!=NULL);

	qmpos=0; p=env->uri;
	for (qmpos=0, p=env->uri; *p!='?' && *p!=0; p++, qmpos++); 

	env->uri_path = g_strndup(env->uri, qmpos);

	return env->uri_path==NULL ? -1 : 0;
}

/* sorting relation for properties by quality */
static gint request_quality_compare(gconstpointer a, gconstpointer b) {

	return (((request_property_quality_t *)a)->quality_per_mil>=
		((request_property_quality_t *)b)->quality_per_mil) ? -1 : 1;
}

static void request_free_accepts_properties(GSList *list) {

	g_slist_foreach(list, request_free_property_quality, NULL);
	g_slist_free(list);
}

static void request_free_property_quality(gpointer elem, gpointer data) {

	g_free(((request_property_quality_t *)elem)->property);
}

