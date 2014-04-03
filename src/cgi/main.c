
#include <stdio.h>
#include <stdlib.h>

#ifdef DEBUG
#include <string.h>
#endif

#include <stdcfg.h>
#include <sqldb.h>
#include <application.h>
#include <http_request.h>
#include <http_response.h>

const gchar *emptypath="";

static gchar *binding_lookup(sqldb_t *db, request_cgi_env_t *env);

int main(void) {

	request_cgi_env_t *env;
	sqldb_t *db;
	gchar *appname;
	resow_application_t *app;
	int exitcode;

	env = request_init_cgi_env();

	if (env == NULL) {
		http_response_send_error(HTTP_STATUS_INTERNAL_SERVER_ERROR,
		NULL);
		exit(HTTP_STATUS_INTERNAL_SERVER_ERROR);
	}

#ifdef DEBUG
	if (strcmp(env->path_part, "/debug")==0) {
		http_response_print_env(env);
		exit(HTTP_STATUS_OK);
	}
#endif

	/* db init */
	if ((db=sqldb_init(SQLDB_MYSQL, RESOW_SQLDB_HOSTNAME,
		RESOW_SQLDB_USERNAME, RESOW_SQLDB_PASSWORD,
		RESOW_SQLDB_DBNAME)) == NULL) {
		http_response_send_error(HTTP_STATUS_INTERNAL_SERVER_ERROR,
		"Database problem.");
		request_free_cgi_env(env);
		exit(HTTP_STATUS_INTERNAL_SERVER_ERROR);
	}

	/* application path binding lookup */
	if ((appname=binding_lookup(db, env)) == NULL) {
		http_response_send_error(HTTP_STATUS_NOT_FOUND,
		"No application at '%s'.", env->path_part);
		sqldb_close(db);
		request_free_cgi_env(env);
		exit(HTTP_STATUS_NOT_FOUND);
	}

	/* application load */
	if ((app=application_load(appname, env)) == NULL) {
		http_response_send_error(HTTP_STATUS_SERVICE_UNAVAILABLE,
		"Loading application '%s' failed.", appname);
		sqldb_close(db);
		request_free_cgi_env(env);
		exit(HTTP_STATUS_SERVICE_UNAVAILABLE);
	}
	g_free(appname);

	/* fprintf(stderr, "executing application"); */

	/* application call */
	exitcode=application_exec(app, db);
	if (http_response_reason[exitcode].code>=400) {
		http_response_send_error(exitcode,
		"Application '%s' failed.", appname);
		application_unload(app);
		sqldb_close(db);
		request_free_cgi_env(env);
		exit(exitcode); /* XXX */
	}

	/* tidy up */
	application_unload(app);
	sqldb_close(db);
	request_free_cgi_env(env);

	exit(0);
}

gchar *binding_lookup(sqldb_t *db, request_cgi_env_t *env) {

	gchar *path, *srvpath;
	gchar *appname;

	path = g_strdup(env->path_part);
	if (path==NULL) return NULL;

	srvpath=NULL;
	do {
		/*fprintf(stderr, "Looking up: %s (%s)\n", path, srvpath==NULL ? "" : srvpath+1);*/
		appname = application_bindinglookup(db,
			path[0]==0 ? "/" : path);

		if (appname == NULL) {
			
			gchar *oldsrvpath;

			oldsrvpath=srvpath;
			srvpath=g_strrstr(path, "/");
			if (oldsrvpath!=NULL) *oldsrvpath='/';
			if (srvpath!=NULL) *srvpath=0;
		}

	} while (appname==NULL && srvpath!=NULL);

	/*fprintf(stderr, "Got: resow_root: %s, path: %s (%s), appname: %s\n", env->resow_root, path, srvpath==NULL ? "(null)" : srvpath+1, appname);*/
	if (appname==NULL) {
		env->service_path=NULL;
		env->service_root=NULL;
	} else {
		env->service_path =  g_strdup(srvpath==NULL ? "": srvpath+1);
		env->service_root = g_strconcat(env->resow_root, path, NULL);
	}
	/*fprintf(stderr, "Got: srvroot: %s, srvpath: %s\n", env->service_root ? env->service_root : "(null)",  env->service_path ? env->service_path : "(null)");*/

	g_free(path);

	return appname;
}
