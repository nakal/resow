
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <application.h>
#include <mpform.h>

#include "../../debug/debug.h"

/*
	deliver fs file
*/
RESOW_DECL_METHOD(flatfs_file,env,resp,db) {

	sqldb_resultset_t *rs;
	gchar *query;
	gchar *content_type;
	time_t timestamp;
	gchar *checksum;
	gchar *filename;
	FILE *fp;

	GSList *cols;

	query = g_strdup_printf("SELECT service_root,rel_url,contenttype,time,checksum FROM flatfs WHERE service_root=\"%s\" AND rel_url=\"%s\"",
			env->service_root, env->service_path);

	rs = sqldb_query(db, query);
	g_free(query);
	if (rs==NULL) return HTTP_STATUS_INTERNAL_SERVER_ERROR;

	if ((cols=sqldb_resultset_row_next(db, rs)) != NULL) {

		content_type=SQLDB_ENTRY_STRING(cols,2);
		timestamp=SQLDB_ENTRY_ULONG(cols,3);
		checksum=SQLDB_ENTRY_STRING(cols,4);

	} else return HTTP_STATUS_NOT_FOUND;

	sqldb_resultset_free(db, rs);

	if (checksum==NULL) {
		/*
			Empty checksum means that entry is created but not
			uploaded, yet.
		 */
		http_response_set_content_type(resp, "text/ascii");
		http_response_add_header(resp, "Allow", "PUT");
		stream_open(resp, HTTP_STATUS_METHOD_NOT_ALLOWED, 1);
		stream_printf(resp, "Allowed methods: PUT\n");
		stream_close(resp);
		return HTTP_STATUS_OK;
	}

	/* open file here */
	filename = g_strdup_printf("%s/%s/%s", RESOW_FLATFS_ROOT,
		env->service_root, env->service_path);
	fp = fopen(filename, "r");
	if (fp==NULL) {

		DEBUG_PRINT("File<->DB inconsistency detected.");
		DEBUG_PRINT(filename);
		return HTTP_STATUS_INTERNAL_SERVER_ERROR;
	}

	http_response_set_content_type(resp, content_type);
	http_response_set_etag(resp, checksum);
	http_response_set_last_modified(resp, timestamp);
	SQLDB_ENTRY_STRING_FREE(content_type);
	SQLDB_ENTRY_STRING_FREE(checksum);

	stream_open(resp, HTTP_STATUS_OK, 0);

	/* TODO */

	stream_close(resp);

	fclose(fp);

	return HTTP_STATUS_OK;
}

RESOW_DECL_METHOD(flatfs_upload_form,env,resp,db) {

	stream_open(resp, HTTP_STATUS_OK, 1);

	stream_printf(resp, "<html><head><title>Flatfs upload new file</title></head>\n");
	stream_printf(resp, "<body>\n");
	stream_printf(resp, "<h1>Flatfs: upload new file</h1>\n");

	stream_printf(resp, "<table><form method=\"POST\" enctype=\"multipart/form-data\">\n");
	stream_printf(resp, "<tr><td>file:</td><td><input type=\"file\" name=\"file\" value=\"\"></input></td></tr>\n");
	stream_printf(resp, "<tr><td>flatfs path:</td><td><input type=\"text\" name=\"path\" value=\"\"></input></td></tr>\n");
	stream_printf(resp, "<tr><td><input type=\"submit\" value=\"Upload\"></td></td></tr>\n");
	stream_printf(resp, "</form></table>\n");

	stream_printf(resp, "</body></html>\n");
	stream_close(resp);
	return HTTP_STATUS_OK;
}

/*
	deliver fs listing

	XXX also needs E-Tag
*/
RESOW_DECL_METHOD(flatfs_list,env,resp,db) {

	gchar *query;
	sqldb_resultset_t *rs;

	if (env->service_path[0]!=0) return flatfs_file(env,resp,db);
	if (HTTP_QUERY_PARAMETER(env, "upload"))
		return flatfs_upload_form(env, resp, db);

	stream_open(resp, HTTP_STATUS_OK, 1);

	stream_printf(resp, "<html><head><title>flatfs index</title></head>\n");
	stream_printf(resp, "<body>\n");
	stream_printf(resp, "<h1>flatfs at %s</h1>\n", env->service_root);

	query = g_strdup_printf("SELECT service_root,rel_url,contenttype,time FROM flatfs WHERE service_root=\"%s\"", env->service_root);
	rs = sqldb_query(db, query);
	g_free(query);

	if (rs==NULL) {
		stream_printf(resp, "<p>Error in DB</p>\n");
	} else {
		GSList *cols;

		stream_printf(resp, "<ul>\n");
		while ((cols=sqldb_resultset_row_next(db, rs)) != NULL) {

			gchar *path, *url, *e_path, *e_url;
			time_t t;

			t = SQLDB_ENTRY_ULONG(cols,3);

			path=SQLDB_ENTRY_STRING(cols,1);
			url=SQLDB_ENTRY_STRING(cols,2);

			e_path=http_response_html_escape_string(path);
			e_url=http_response_html_escape_string(url);
			SQLDB_ENTRY_STRING_FREE(path);
			SQLDB_ENTRY_STRING_FREE(url);

			stream_printf(resp, "<li><tt><a href=\"%s/%s\">%s</a></tt></li>\n", resp->default_location, e_url, e_path);

			g_free(e_path); g_free(e_url);

			sqldb_resultset_row_free(db, cols);
		}

		sqldb_resultset_free(db, rs);

		stream_printf(resp, "</ul>\n");

		stream_printf(resp, "<p><form method=\"GET\"><input type=\"hidden\" name=\"upload\"></input><input type=\"submit\" value=\"Upload file\"></input></form></p>");
	}
	stream_printf(resp, "</body></html>\n");
	stream_close(resp);
	return HTTP_STATUS_OK;
}

/*
	upload a file
*/
RESOW_DECL_METHOD(flatfs_upload,env,resp,db) {

	mpform_t *form;

	stream_open(resp, HTTP_STATUS_OK, 0);

	stream_printf(resp, "<html><head><title>flatfs upload</title></head>\n");
	stream_printf(resp, "<body>\n");
	stream_printf(resp, "<h1>flatfs upload</h1><pre>\n");

	form = mpform_read(env);
	mpform_close(form);

	stream_printf(resp, "</pre></body></html>\n");
	stream_close(resp);

	return HTTP_STATUS_OK;
}

RESOW_DECL_MAPPINGS {

	RESOW_REGISTER_METHOD(flatfs_list, HTTP_METHOD_GET, "text/html");
	RESOW_REGISTER_METHOD(flatfs_upload, HTTP_METHOD_POST, "text/html");

	RESOW_REGISTER_PARAMETER("flatfs_root");

	return 0;
}

/*
   GChecksum *cs;
   cs = g_checksum_new(G_CHECKSUM_SHA256);
   if (cs==NULL) {
   return HTTP_STATUS_INTERNAL_SERVER_ERROR;
   }

   g_checksum_update(cs, (guchar *)buf, buflen);
   g_checksum_free(cs);
 */
