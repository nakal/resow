
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <application.h>

RESOW_DECL_METHOD(setup_create_form,env,resp,db) {

	gchar *path;
	gchar *appname;
	resow_application_t *app;
	request_cgi_env_t env_a;
	GSList *i;

	path=http_response_html_escape_string(HTTP_QUERY_PARAMETER(env, "path"));
	appname=http_response_html_escape_string(HTTP_QUERY_PARAMETER(env, "app"));

	memset(&env_a, 0, sizeof(request_cgi_env_t));
	app = application_load(appname, &env_a);
	if (app==NULL) {
		return HTTP_STATUS_BAD_REQUEST;
	}

	stream_open(resp, HTTP_STATUS_OK, 1);

	stream_printf(resp, "<html><head><title>Define new parameter for %s (at %s)</title></head>\n", app, path);
	stream_printf(resp, "<body>\n");
	stream_printf(resp, "<h1>Define new parameter for %s (at %s)</h1>\n",
		appname, path);

	stream_printf(resp, "<table><form method=\"POST\">\n");
	stream_printf(resp, "<tr><th></th><th>Parameter</th><th>Value</th></tr>\n");
	stream_printf(resp, "<tr><td><input type=\"hidden\" name=\"path\" value=\"%s\"></input><input type=\"hidden\" name=\"app\" value=\"%s\"></input></td>\n", path, appname);

	i = app->parameters;
	stream_printf(resp, "<td><select name=\"param\"%s>\n", i==NULL? " disabled" : "");
	while (i!=NULL) {
		stream_printf(resp, "\t<option>%s</option>\n", i->data);
		i=i->next;
	}
	stream_printf(resp, "</select></td>\n");
	stream_printf(resp, "<td><input type=\"text\" name=\"value\"></input></td>\n");
	stream_printf(resp, "</tr>\n");
	stream_printf(resp, "<tr><td></td><td><input type=\"submit\" value=\"Add\"></td></td></tr>\n");
	stream_printf(resp, "</form></table>\n");
	if (app->parameters==NULL)
		stream_printf(resp, "<p>There are no parameters to set in application <tt>%s</tt></p>\n", app->name);

	stream_printf(resp, "</body></html>\n");
	stream_close(resp);

	application_unload(app);

	g_free(path); g_free(appname);

	return HTTP_STATUS_OK;
}

RESOW_DECL_METHOD(setup_get,env,resp,db) {

	sqldb_resultset_t *rs;
	gchar *path;
	gchar *app;
	gchar *query;

	if (HTTP_QUERY_PARAMETER(env, "create"))
		return setup_create_form(env, resp, db);

	path=http_response_html_escape_string(HTTP_QUERY_PARAMETER(env, "path"));
	app=http_response_html_escape_string(HTTP_QUERY_PARAMETER(env, "app"));

	stream_open(resp, HTTP_STATUS_OK, 1);

	stream_printf(resp, "<html><head><title>Application parameters for %s (at %s)</title></head>\n", app, path);
	stream_printf(resp, "<body>\n");
	stream_printf(resp, "<h1>Application parameters for %s (at %s)</h1>\n", app, path);

	query = g_strdup_printf("SELECT id,urlpath,application,paramname,paramvalue FROM appparams WHERE urlpath=\"%s\" AND application=\"%s\"", path, app);
	rs = sqldb_query(db, query);
	g_free(query);

	if (rs==NULL) {
		stream_printf(resp, "<p>Error in DB</p>\n");
	} else {
		GSList *cols;
		size_t rowcount=0;

		stream_printf(resp, "<table>\n");
		while ((cols=sqldb_resultset_row_next(db, rs)) != NULL) {

			gchar *id;
			gchar *paramname, *paramvalue, *e_param, *e_value;

			id=SQLDB_ENTRY_STRING(cols,0);
			paramname=SQLDB_ENTRY_STRING(cols,3);
			paramvalue=SQLDB_ENTRY_STRING(cols,4);

			e_param=http_response_html_escape_string(paramname);
			e_value=http_response_html_escape_string(paramvalue);
			SQLDB_ENTRY_STRING_FREE(paramname);
			SQLDB_ENTRY_STRING_FREE(paramvalue);

			rowcount++;
			stream_printf(resp, "<form method=\"POST\"><input type=\"hidden\" name=\"app\" value=\"%s\"></input><input type=\"hidden\" name=\"path\" value=\"%s\"></input><input type=\"hidden\" name=\"id\" value=\"%s\"></input><tr><td><input type=\"text\" name=\"paramname\" value=\"%s\" size=\"20\" readonly></input></td><td><input type=\"text\" name=\"paramvalue\" value=\"%s\" size=\"20\"></input></td><td><input type=\"submit\" name=\"Update\" value=\"Update\"></input>&nbsp;<input type=\"submit\" name=\"Delete\" value=\"Delete\"></input></td></tr></form>\n", app, path, id, e_param, e_value);

			SQLDB_ENTRY_STRING_FREE(id);
			g_free(e_param); g_free(e_value);
			sqldb_resultset_row_free(db, cols);
		}

		sqldb_resultset_free(db, rs);

		if (rowcount<=0) stream_printf(resp,
			"<p>No parameters set.</p>");

		stream_printf(resp, "<form method=\"GET\"><tr><td><input type=\"hidden\" name=\"create\"></input><input type=\"hidden\" name=\"path\" value=\"%s\"></input><input type=\"hidden\" name=\"app\" value=\"%s\"></input><input type=\"submit\" value=\"Add new\"></input></td><td></td></tr></form>", path, app);
		stream_printf(resp, "</table>\n");
	}

	stream_printf(resp, "</body></html>\n");
	stream_close(resp);

	g_free(path); g_free(app);
	return HTTP_STATUS_OK;
}

RESOW_DECL_METHOD(setup_post,env,resp,db) {

	enum form_mode_t { setup_CREATE, setup_DELETE, setup_UPDATE } mode;
	gchar *stat, *e_path=NULL, *e_app=NULL, *e_key=NULL, *e_val=NULL;
	unsigned long id;
	sqldb_status_t result;

	if (HTTP_QUERY_PARAMETER(env, "create") != NULL) {
		mode=setup_CREATE;
	} else {
		if (HTTP_FORM_PARAMETER(env, "Delete") != NULL) {
			mode=setup_DELETE;
		} else if (HTTP_FORM_PARAMETER(env, "Update") != NULL) {
			mode=setup_UPDATE;
		} else return HTTP_STATUS_NOT_ACCEPTABLE;
	}


	switch (mode) {
		case setup_CREATE:
		e_path = sqldb_escape_string(db,
			HTTP_FORM_PARAMETER(env,"path"));
		e_app = sqldb_escape_string(db, HTTP_FORM_PARAMETER(env,"app"));
		e_key = sqldb_escape_string(db,
			HTTP_FORM_PARAMETER(env,"param"));
		e_val = sqldb_escape_string(db,
			HTTP_FORM_PARAMETER(env,"value"));
		stat = g_strdup_printf("INSERT INTO appparams (urlpath,application,paramname,paramvalue) VALUES (\"%s\",\"%s\",\"%s\",\"%s\")", e_path, e_app,
			e_key, e_val);
		g_free(e_path); g_free(e_app); g_free(e_key); g_free(e_val);
		break;
		case setup_DELETE:
		id = strtoul(HTTP_FORM_PARAMETER(env,"id"), NULL, 10);
		stat = g_strdup_printf("DELETE FROM appparams WHERE id=%lu", id);
		break;
		case setup_UPDATE:
		id = strtoul(HTTP_FORM_PARAMETER(env,"id"), NULL, 10);
		e_val = sqldb_escape_string(db,
			HTTP_FORM_PARAMETER(env,"paramvalue"));
		stat = g_strdup_printf("UPDATE appparams SET paramvalue='%s' WHERE id=%lu", e_val, id);
		g_free(e_val);
		break;
	}

	sqldb_transaction_begin(db);
	result=sqldb_command(db, stat);
	sqldb_transaction_commit(db);
	g_free(stat);

	stream_open(resp, HTTP_STATUS_OK, 1);

	stream_printf(resp, "<html><head><title>setup result</title></head><body><p>%s</p>", result==SQLDB_ERROR? "Action failed." : "Action succeeded.");

	e_path = http_response_html_escape_string
		(HTTP_FORM_PARAMETER(env,"path"));
	e_app = http_response_html_escape_string
		(HTTP_FORM_PARAMETER(env,"app"));
	stream_printf(resp, "<p><a href=\"%s?app=%s&path=%s\">Back to application parameter list</a></body></html>", env->service_root, e_app, e_path);
	g_free(e_path); g_free(e_app);
	stream_close(resp);
	return HTTP_STATUS_OK;
}

RESOW_DECL_MAPPINGS {

	RESOW_REGISTER_METHOD(setup_get, HTTP_METHOD_GET, "text/html");
	RESOW_REGISTER_METHOD(setup_post, HTTP_METHOD_POST, "text/html");

	return 0;
}

