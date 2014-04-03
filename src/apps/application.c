
#include <string.h>
#include <dlfcn.h>
#include <glob.h>

#include <application.h>
#include <http_response.h>

#include "../debug/debug.h"

static guint method_hash_func(gconstpointer key);
static gboolean method_hash_equals(gconstpointer a, gconstpointer b);
static resow_application_method_t lookup_method(
	resow_application_t *app,
	const gchar *outputtype);
static void application_setup_parameters(resow_application_t *app,
	sqldb_t *db);

static const gchar *fallbacktypes[]={ "text/ascii", "text/html", NULL };

typedef int (*application_init_t)(resow_application_t *app);

typedef struct {

	http_method_t method;
	gchar outputtype[128];	/* MIME type */

} application_binding_key_t;

typedef struct {
	resow_application_method_t method;
} application_binding_value_t;

/*
 * Return path without prefix and leading "/" for a given local path from
 * the URL
 *
 * path = path part of the request URL
 */
gchar *application_bindinglookup(sqldb_t *db, const gchar *path) {
	/* TODO: name trimming */
	
	sqldb_resultset_t *rs;
	GSList *cols;
	gchar *apppath;
	gchar *query;
	gchar *epath;
	sqldb_field_t *f;

	/* we don't allow empty paths */
	if (path==NULL || path[0]==0) return NULL;

	/* lookup service bound to the path in DB */
	epath = sqldb_escape_string(db, path);
	if (epath == NULL) {
		DEBUG_PRINT("Failure escaping string");
		return NULL;
	}

	query = g_strdup_printf
		("SELECT urlpath,application FROM bindings WHERE "
		"urlpath = '%s'", epath);
	g_free(epath);
	if (query == NULL) {
		DEBUG_PRINT("Out of memory");
		return NULL;
	}

	rs = sqldb_query(db, query);
	g_free(query);

	if (rs == NULL) {
		DEBUG_PRINT("SQL query failure");
		return NULL;
	}

	/* return first match */
	if ((cols=sqldb_resultset_row_next(db, rs)) == NULL) {
		/*
			Do not produce noise when not found, because it's
			normal when searching incrementally for an application.
		 */
		sqldb_resultset_free(db, rs);
		return NULL;
	}
	f = (sqldb_field_t *)g_slist_nth_data(cols, 1);
	if (f == NULL) {
		sqldb_resultset_row_free(db, cols);
		sqldb_resultset_free(db, rs);
		DEBUG_PRINT("SQL column addressing error");
		return NULL;
	}
	apppath=g_strndup((gchar *)f->data, f->datalen);
	sqldb_resultset_row_free(db, cols);
	sqldb_resultset_free(db, rs);

	if (apppath==NULL) {
		DEBUG_PRINT("Returned application path is invalid");
		return NULL;
	}

	return apppath;
}

/*
 * Takes an application path (alphnum and "/" allowed)
 */
resow_application_t *application_load(const gchar *name,
	request_cgi_env_t *env) {

	gchar *libpath;
	void *handle;
	resow_application_t *app;
	size_t i;
	application_init_t init;

	for (i=0; i<strlen(name); i++)
		if (!g_ascii_isalnum(name[i])
			|| (i>0 && name[i]=='/')) {
			DEBUG_PRINT("Invalid application name");
			return NULL; 
		}

	libpath = g_strdup_printf("%s/apps/%s.so", RESOW_LD_PATH, name);
	if (libpath==NULL) {
		DEBUG_PRINT("Out of memory");
		return NULL; 
	}

	handle = dlopen((char *)libpath, RTLD_NOW);

	g_free(libpath);

	if (handle == NULL) {
		DEBUG_PRINT("Failed to load application");
		DEBUG_PRINT(dlerror());
		return NULL;
	}

	app = g_malloc(sizeof(resow_application_t));
	if (app == NULL) {
		DEBUG_PRINT("Out of memory while loading application");
		dlclose(handle);
		return NULL;
	}

	app->dl_handle = handle;

	init = (application_init_t)dlsym(handle, APPLICATION_METHOD_INIT);
	if (init==NULL) {
		DEBUG_PRINT("init symbol not found in application library");
		g_free(app);
		dlclose(handle);
		return NULL;
	}

	/* fprintf(stderr, "application %s loaded\n", name); */
	app->env=env;

	app->method = g_hash_table_new_full
		(method_hash_func, method_hash_equals, g_free, g_free);
	app->parameters = NULL; /* empty list for default applications */
	
	if (app->method==NULL) {
		DEBUG_PRINT("init application: out of memory");
		g_free(app);
		dlclose(handle);
		return NULL;
	}

	if ((app->name = g_strdup(name))==NULL) {
		DEBUG_PRINT("init application: out of memory");
		g_hash_table_destroy(app->method);
		g_free(app);
		dlclose(handle);
		return NULL;
	}

	if (init(app)!=0) {
		DEBUG_PRINT("init application library failed");
		g_free(app->name);
		g_hash_table_destroy(app->method);
		g_free(app);
		dlclose(handle);
		return NULL;
	}

	return app;
}

void application_unload(resow_application_t *app) {

	GSList *i;

	i=app->parameters;
	while (i!=NULL) { g_free(i->data); i=i->next; }
	g_slist_free(app->parameters);

	if (app->method) g_hash_table_destroy(app->method);

	if (app->name) g_free(app->name);

	dlclose(app->dl_handle);

	g_free(app);
}

int application_register_method(resow_application_t *app,
	resow_application_method_t app_method,
	http_method_t http_method, const gchar *outputtype) {

	/*
		key:	http_method:outputtype (application_binding_key_t)
		value:	method
	*/

	application_binding_key_t *key;
	application_binding_value_t *val;

	key = g_malloc(sizeof(application_binding_key_t));
	if (key==NULL) return -1;
	val = g_malloc(sizeof(application_binding_value_t));
	if (val==NULL) {
		g_free(key);
		return -1;
	}

	key->method=http_method;
	strlcpy(key->outputtype, outputtype, sizeof(key->outputtype));
	val->method=app_method;

	g_hash_table_insert(app->method, (gpointer)key, (gpointer)val);

	return 0;
}

int application_register_parameter(resow_application_t *app,
	const gchar *name) {

	app->parameters = g_slist_prepend(app->parameters, g_strdup(name));
	return app->parameters==NULL;
}

/*
	XXX this needs to be extended later

	it won't work for "cat / *" and "* / *" accept headers
*/
int application_exec(resow_application_t *app, sqldb_t *db) {

	resow_application_method_t selectedmethod=NULL;
	const gchar *selectedtype;
	GSList *reqlist;
	int accept_fallback;

	reqlist = app->env->accept_datatype;
	accept_fallback = 0;
	while (reqlist!=NULL && selectedmethod==NULL) {

		selectedtype = ((request_property_quality_t *)reqlist->data)->property;
		selectedmethod = lookup_method(app, selectedtype);
		if (strcmp(((request_property_quality_t *)reqlist->data)->property,"*/*")==0) accept_fallback=1;

		reqlist = reqlist->next;
	}

	/* XXX fallback only for "* / *" */
	if (accept_fallback) {

		int i=0;

		while (fallbacktypes[i]!=NULL && selectedmethod==NULL) {
			selectedtype = fallbacktypes[i];
			selectedmethod = lookup_method(app, selectedtype);
			i++;
		}

		/* no more fallbacks */
	}

	if (selectedmethod==NULL) {
		DEBUG_PRINT("No suitable method/content-type combination found.");
		return HTTP_STATUS_NOT_ACCEPTABLE;
	} else {
		http_status_t retcode;
		http_response_t * resp;

		application_setup_parameters(app, db);

		resp = http_response_init(selectedtype, app->env);
		if (resp == NULL) return HTTP_STATUS_INTERNAL_SERVER_ERROR;

		retcode = selectedmethod(app->env, resp, db);

		/* if stream not closed, close it now */
		if (resp->stream) stream_close(resp);

		if (!resp->stream_written) {
			/*
				write out message with retcode, because the app
				has not done it, yet
			 */
			http_response_send_status(retcode, selectedtype);
		} else http_response_close(resp);

		g_hash_table_destroy(app->env->app_parameters);
	}

	/*
		we can ignore retcode above, it just avoids outputing
		http status twice
	*/
	return HTTP_STATUS_OK;
}

static resow_application_method_t lookup_method(
	resow_application_t *app,
	const gchar *outputtype) {

	application_binding_key_t searchkey;
	gpointer val;
	resow_application_method_t selectedmethod=NULL;

/*
fprintf(stderr, "looking up method (%u) for: %s\n", app->env->method, outputtype);
*/

	searchkey.method = app->env->method;
	strlcpy(searchkey.outputtype, outputtype, sizeof(searchkey.outputtype));
	if ((val = g_hash_table_lookup(app->method, (gconstpointer)&searchkey))!=NULL) {
		selectedmethod = ((application_binding_value_t *)val)->method;
	}

	return selectedmethod;
}

GSList * application_list_get(void) {

	GSList *l=NULL;
	gchar *apppath;
	GDir *dir;

	apppath = g_strdup_printf("%s/apps", RESOW_LD_PATH);

	dir = g_dir_open(apppath, 0, NULL);
	if (dir!=NULL) {

		const gchar *p;

		while ((p = g_dir_read_name(dir)) != NULL) {

			if (g_str_has_suffix(p, ".so")) {
				gchar *elem;

				elem = g_strdup(p);
				*g_strrstr(elem, ".so")=0;
				l = g_slist_prepend(l, elem);
			}
		}

		g_dir_close(dir);
		l = g_slist_reverse(l);
	}

	g_free(apppath);

	return l;
}

void application_list_free(GSList *list) {

	GSList *l;

	l = list;
	while (l!=NULL) {
		g_free(l->data);
		l = l->next;
	}

	g_slist_free(list);
}

static guint method_hash_func(gconstpointer key) {

	const application_binding_key_t *k;

	k = (const application_binding_key_t *)key;

	return g_int_hash(&k->method) + g_str_hash(k->outputtype);
}

static gboolean method_hash_equals(gconstpointer a, gconstpointer b) {

	const application_binding_key_t *k1, *k2;

	k1 = (const application_binding_key_t *)a;
	k2 = (const application_binding_key_t *)b;

	return (k1->method == k2->method) &&
		(strcmp(k1->outputtype, k2->outputtype)==0) ? TRUE : FALSE;
}

static void application_setup_parameters(resow_application_t *app,
	sqldb_t *db) {

	gchar *query;
	sqldb_resultset_t *rs;

	query = g_strdup_printf
		("SELECT urlpath,application,paramname,paramvalue FROM "
		"appparams WHERE urlpath = '%s' AND application = '%s'",
		app->env->service_root, app->name);
	if (query == NULL) {
		DEBUG_PRINT("Out of memory");
		return;
	}

	rs = sqldb_query(db, query);
	g_free(query);

	app->env->app_parameters = g_hash_table_new_full
		(g_str_hash, g_str_equal, g_free, g_free);
	if (rs != NULL) {

		GSList *cols;

		while ((cols=sqldb_resultset_row_next(db, rs)) != NULL) {

			gchar *k, *v;

			k = SQLDB_ENTRY_STRING(cols, 2);
			v = SQLDB_ENTRY_STRING(cols, 3);

			g_hash_table_insert(app->env->app_parameters, k, v);

			sqldb_resultset_row_free(db, cols);
		}

		sqldb_resultset_free(db, rs);
	}
}

