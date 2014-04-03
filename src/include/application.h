
#ifndef RESOW_APP_APPLICATION_H_INCLUDED
#define RESOW_APP_APPLICATION_H_INCLUDED

#include <stdcfg.h>
#include <sqldb.h>
#include <http_request.h>
#include <http_response.h>
#include <stream.h>

/*
 * An application needs these methods implemented (use the RESOW_DECL
 * macros to do this).
 */
#define APPLICATION_METHOD_INIT			"init"

/*
 * RESOW_DECL macros declare and define application methods
 * to remove the syntactic noise of C.
 */
#define RESOW_DECL_MAPPINGS \
extern int init(resow_application_t *app); \
int init(resow_application_t *app)

#define RESOW_REGISTER_METHOD(method,httpmethod,outputtype) \
	if (application_register_method(app, method, httpmethod, \
		outputtype)!=0) return -1

#define RESOW_DECL_METHOD(name,env,resp,db) \
extern int name(const request_cgi_env_t *env, http_response_t *resp, \
	sqldb_t *db); \
int name(const request_cgi_env_t *env, http_response_t *resp, sqldb_t *db)

#define RESOW_REGISTER_PARAMETER(name) \
	if (application_register_parameter(app, name)!=0) return -1

typedef int (*resow_application_method_t)(const request_cgi_env_t *env,
	http_response_t *resp, sqldb_t *db);

typedef struct {

	void *dl_handle; /* so library handle */
	request_cgi_env_t *env;
	gchar *name; /* application name */

	GHashTable *method;

	/*
		application parameter names, if no parameters specified (NULL),
		then don't query for them in table (speedup!)
		(allowed parameter names)
	 */
	GSList *parameters;

} resow_application_t;

/*
 * Application control methods (for system use)
 */
extern gchar *application_bindinglookup(sqldb_t *db, const gchar *path);
extern resow_application_t *application_load(const gchar *name,
	request_cgi_env_t *env);
extern void application_unload(resow_application_t *app);
extern int application_exec(resow_application_t *app, sqldb_t *db);
extern int application_register_method(resow_application_t *app,
	resow_application_method_t app_method, http_method_t http_method,
	const gchar *outputtype);
extern GSList * application_list_get(void);
extern void application_list_free(GSList *list);
extern int application_register_parameter(resow_application_t *app,
	const gchar *name);

#endif

