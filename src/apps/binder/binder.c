
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <application.h>

RESOW_DECL_METHOD(binder_create_form,env,resp,db) {

	GSList *applist, *p;

	stream_open(resp, HTTP_STATUS_OK, 1);

	stream_printf(resp, "<html><head><title>Resow define new binding</title></head>\n");
	stream_printf(resp, "<body>\n");
	stream_printf(resp, "<h1>Resow define new binding</h1>\n");

	stream_printf(resp, "<table><form method=\"POST\">\n");
	stream_printf(resp, "<tr><td><input type=\"text\" name=\"path\" value=\"\"></input></td><td><select name=\"app\">\n");

	p = applist = application_list_get();
	while (p!=NULL) {
		stream_printf(resp, "\t<option>%s</option>\n", (gchar *)p->data);
		p = p->next;
	}
	application_list_free(applist);

	stream_printf(resp, "</select></td></tr>\n");
	stream_printf(resp, "<tr><td><input type=\"submit\" value=\"Add\"></td></td></tr>\n");
	stream_printf(resp, "</form></table>\n");

	stream_printf(resp, "</body></html>\n");
	stream_close(resp);
	return HTTP_STATUS_OK;
}

RESOW_DECL_METHOD(binder_get,env,resp,db) {

	sqldb_resultset_t *rs;

	if (HTTP_QUERY_PARAMETER(env, "create"))
		return binder_create_form(env, resp, db);

	stream_open(resp, HTTP_STATUS_OK, 1);

	stream_printf(resp, "<html><head><title>Resow Bindings</title></head>\n");
	stream_printf(resp, "<body>\n");
	stream_printf(resp, "<h1>Resow Bindings</h1>\n");

	rs = sqldb_query(db, "SELECT id,urlpath,application FROM bindings");
	if (rs==NULL) {
		stream_printf(resp, "<p>Error in DB</p>\n");
	} else {
		GSList *cols;

		stream_printf(resp, "<table>\n");
		while ((cols=sqldb_resultset_row_next(db, rs)) != NULL) {

			gchar *id;
			gchar *path, *app, *e_path, *e_app;

			id=SQLDB_ENTRY_STRING(cols,0);
			path=SQLDB_ENTRY_STRING(cols,1);
			app=SQLDB_ENTRY_STRING(cols,2);

			e_path=http_response_html_escape_string(path);
			e_app=http_response_html_escape_string(app);
			SQLDB_ENTRY_STRING_FREE(path);
			SQLDB_ENTRY_STRING_FREE(app);

			/*
				TODO: app setup might be bound somewhere else!
				needs a reverse lookup
			*/
			stream_printf(resp, "<form method=\"POST\"><input type=\"hidden\" name=\"id\" value=\"%s\"></input><tr><td><input type=\"text\" name=\"path\" value=\"%s\" size=\"20\"></input></td><td><input type=\"text\" name=\"app\" value=\"%s\" size=\"20\" readonly></input></td><td><input type=\"submit\" name=\"Update\" value=\"Update\"></input>&nbsp;<input type=\"submit\" name=\"Delete\" value=\"Delete\"%s></input></td><td>&nbsp;<a href=\"%s%s\">Execute <tt>%s</tt></a></td><td>&nbsp;<a href=\"%s/setup?path=%s&app=%s\">Setup</a></td></tr></form>\n", id, e_path, e_app, strcmp(path, env->path_part)==0 ? " disabled" : "", env->resow_root, e_path, e_app, env->resow_root, e_path, e_app);

			SQLDB_ENTRY_STRING_FREE(id);
			g_free(e_path); g_free(e_app);
			sqldb_resultset_row_free(db, cols);
		}

		sqldb_resultset_free(db, rs);

		stream_printf(resp, "<form method=\"GET\"><tr><td><input type=\"hidden\" name=\"create\"></input><input type=\"submit\" value=\"Add new\"></input></td><td></td></tr></form>");
		stream_printf(resp, "</table>\n");
	}

	stream_printf(resp, "</body></html>\n");
	stream_close(resp);
	return HTTP_STATUS_OK;
}

RESOW_DECL_METHOD(binder_post,env,resp,db) {

	enum form_mode_t { BINDER_CREATE, BINDER_DELETE, BINDER_UPDATE } mode;
	gchar *stat, *e_path=NULL, *e_app=NULL;
	unsigned long id;
	sqldb_status_t result;

	if (HTTP_QUERY_PARAMETER(env, "create") != NULL) {
		mode=BINDER_CREATE;
	} else {
		if (HTTP_FORM_PARAMETER(env, "Delete") != NULL) {
			mode=BINDER_DELETE;
		} else if (HTTP_FORM_PARAMETER(env, "Update") != NULL) {
			mode=BINDER_UPDATE;
		} else return HTTP_STATUS_NOT_ACCEPTABLE;
	}

	switch (mode) {
		case BINDER_CREATE:
		e_path = sqldb_escape_string(db,
			HTTP_FORM_PARAMETER(env,"path"));
		e_app = sqldb_escape_string(db, HTTP_FORM_PARAMETER(env,"app"));
		stat = g_strdup_printf("INSERT INTO bindings (urlpath,application) VALUES (\"%s\",\"%s\")", e_path, e_app);
		g_free(e_path); g_free(e_app);
		break;
		case BINDER_DELETE:
		id = strtoul(HTTP_FORM_PARAMETER(env,"id"), NULL, 10);
		stat = g_strdup_printf("DELETE FROM bindings WHERE id=%lu", id);
		break;
		case BINDER_UPDATE:
		id = strtoul(HTTP_FORM_PARAMETER(env,"id"), NULL, 10);
		e_path = sqldb_escape_string(db,
			HTTP_FORM_PARAMETER(env,"path"));
		stat = g_strdup_printf("UPDATE bindings SET urlpath='%s' WHERE id=%lu", e_path, id);
		g_free(e_path);
		break;
	}

	sqldb_transaction_begin(db);
	result=sqldb_command(db, stat);
	sqldb_transaction_commit(db);
	g_free(stat);

	stream_open(resp, HTTP_STATUS_OK, 1);

	stream_printf(resp, "<html><head><title>Binder result</title></head><body><p>%s</p>", result==SQLDB_ERROR? "Action failed." : "Action succeeded.");

	stream_printf(resp, "<p><a href=\"%s\">Back to bindings list</a></body></html>", env->service_root);
	stream_close(resp);
	return HTTP_STATUS_OK;
}

RESOW_DECL_MAPPINGS {

	RESOW_REGISTER_METHOD(binder_get, HTTP_METHOD_GET, "text/html");
	RESOW_REGISTER_METHOD(binder_post, HTTP_METHOD_POST, "text/html");

	return 0;
}

