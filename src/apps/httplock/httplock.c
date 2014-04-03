
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <application.h>

#include "../../debug/debug.h"

/*
*
*/
RESOW_DECL_METHOD(httplock_ls,env,resp,db) {

	gchar *stat;
	const gchar *dstloc;
	sqldb_resultset_t *rs;

	if ((dstloc=HTTP_QUERY_PARAMETER(env, "dstloc")) == NULL)
		return HTTP_STATUS_BAD_REQUEST;

	stat = g_strdup_printf("SELECT id,svcid,srcid,dstloc,srcloc,tmout,deleted FROM httplock WHERE svcid=\"%s\" AND deleted='n' AND new='n' AND (dstloc IS NULL OR dstloc=\"%s\")", env->service_root, dstloc);

	if (stat==NULL) return HTTP_STATUS_INTERNAL_SERVER_ERROR;
	rs = sqldb_query(db, stat);
	g_free(stat);

	stream_open(resp, HTTP_STATUS_OK, 1);

	if (rs==NULL) {
		stream_printf(resp, "ERROR\n");
	} else {
		GSList *cols;

		while ((cols=sqldb_resultset_row_next(db, rs)) != NULL) {

			unsigned long id;

			id=SQLDB_ENTRY_ULONG(cols,0);

			stream_printf(resp, "%lu\r\n", id);

			sqldb_resultset_row_free(db, cols);
		}

		sqldb_resultset_free(db, rs);
	}

	stream_close(resp);
	return HTTP_STATUS_OK;
}

RESOW_DECL_METHOD(httplock_get,env,resp,db) {

	sqldb_resultset_t *rs;
	gchar *stat;

	if (strlen(env->service_path)==0) {
		return httplock_ls(env,resp,db);
	} else {
		stat = g_strdup_printf("SELECT id,svcid,srcid,dstloc,srcloc,tmout,deleted FROM httplock WHERE svcid=\"%s\" AND deleted='n' AND new='n' AND resloc=\"%s\"", env->service_root, env->service_path);
	}
	if (stat==NULL) return HTTP_STATUS_INTERNAL_SERVER_ERROR;
	rs = sqldb_query(db, stat);
	g_free(stat);

	if (sqldb_resultset_row_count(db, rs)<=0) return HTTP_STATUS_NOT_FOUND;

	stream_open(resp, HTTP_STATUS_OK, 1);

	if (rs==NULL) {
		stream_printf(resp, "ERROR\n");
	} else {
		GSList *cols;

		while ((cols=sqldb_resultset_row_next(db, rs)) != NULL) {

			unsigned long id;

			id=SQLDB_ENTRY_ULONG(cols,0);

			stream_printf(resp, "%lu\r\n", id);

			sqldb_resultset_row_free(db, cols);
		}

		sqldb_resultset_free(db, rs);
	}

	stream_close(resp);
	return HTTP_STATUS_OK;
}

RESOW_DECL_METHOD(httplock_post_read_tunnel,env,resp,db) {

	gchar *stat, *srcloc=NULL;
	sqldb_status_t result;
	const gchar *dstloc;
	sqldb_resultset_t *rs;
	GSList *cols;
	int existed=1;
	unsigned long id;
	gchar *svcid=NULL;

	dstloc = HTTP_FORM_PARAMETER(env, "dstloc");
	assert(dstloc!=NULL);

	/*
		select statement to find the first match
		first: prefer efficiency over redundancy
	*/
	stat = g_strdup_printf("SELECT id,svcid,srcid,dstloc,srcloc,tmout,deleted FROM httplock WHERE svcid=\"%s\" AND deleted='n' AND new='n' AND dstloc IS NULL ORDER BY id LIMIT 1", env->service_root);

	if (stat==NULL) return HTTP_STATUS_INTERNAL_SERVER_ERROR;
	rs = sqldb_query(db, stat);
	g_free(stat);

	if (rs == NULL) return HTTP_STATUS_INTERNAL_SERVER_ERROR;

	if (sqldb_resultset_row_count(db, rs)<=0) {

		sqldb_resultset_free(db, rs);

		/* then: fall back to redundancy */
		stat = g_strdup_printf("SELECT id,svcid,srcid,dstloc,srcloc,tmout,deleted FROM httplock WHERE svcid=\"%s\" AND deleted='n' AND new='n' AND dstloc=\"%s\" ORDER BY id LIMIT 1", env->service_root, dstloc);
		if (stat==NULL) return HTTP_STATUS_INTERNAL_SERVER_ERROR;
		rs = sqldb_query(db, stat);
		g_free(stat);

		if (rs == NULL) return HTTP_STATUS_INTERNAL_SERVER_ERROR;
		if (sqldb_resultset_row_count(db, rs)<=0) {
			sqldb_resultset_free(db, rs);
			return HTTP_STATUS_NOT_FOUND;
		}
	}

	if ((cols=sqldb_resultset_row_next(db, rs)) != NULL) {

		gchar *c;

		id = SQLDB_ENTRY_ULONG(cols, 0);
		svcid = SQLDB_ENTRY_STRING(cols, 1);
		c=SQLDB_ENTRY_STRING(cols,4);

		srcloc = c==NULL ? NULL : g_strdup(c);
		existed = srcloc!=NULL;

		SQLDB_ENTRY_STRING_FREE(c);
		sqldb_resultset_row_free(db, cols);
	} else {
		sqldb_resultset_free(db, rs);
		return HTTP_STATUS_INTERNAL_SERVER_ERROR;
	}

	sqldb_resultset_free(db, rs);

	/*
		calc hash, when needed (srcloc is not set)
		store hash(id,svcid) in srcloc
	*/
	if (srcloc==NULL) {

		gchar *hstr;
		GChecksum *cs;

		hstr = g_strdup_printf("%lu%s", id, svcid);
		cs = g_checksum_new(G_CHECKSUM_SHA256);
		g_checksum_update(cs, (guchar *)hstr, strlen(hstr));

		srcloc = g_strdup(g_checksum_get_string(cs));
		g_checksum_free(cs);
	}

	SQLDB_ENTRY_STRING_FREE(svcid);

	assert(srcloc!=NULL);
	stat = g_strdup_printf("UPDATE httplock SET dstloc=\"%s\",srcloc=\"%s\" WHERE id=%lu AND deleted='n' AND srcloc IS NULL ORDER BY id LIMIT 1", dstloc, srcloc, id);

	sqldb_transaction_begin(db);
	result=sqldb_command(db, stat);
	sqldb_transaction_commit(db);
	g_free(stat);

	if (result==SQLDB_OK) {

		gchar *loc;

		http_response_set_content_type(resp, HTTP_RESPONSE_EMPTY);

		loc = g_strdup_printf("%s/%s", env->uri_path, srcloc);
		http_response_set_location(resp, loc);
		g_free(loc);

		stream_open(resp, existed ? HTTP_STATUS_OK :
			HTTP_STATUS_CREATED, 1);
		stream_close(resp);
	}

	g_free(srcloc);

	return HTTP_STATUS_OK;
}

RESOW_DECL_METHOD(httplock_post_create,env,resp,db) {

	const gchar *srcid;
	gchar *data, *resloc, *stat;
	sqldb_status_t result;

	srcid = HTTP_FORM_PARAMETER(env, "srcid");
	assert(srcid!=NULL);

	data = g_strdup_printf("%s%s", env->service_root, srcid);
	if (data!=NULL) {

		GChecksum *cs;

		cs = g_checksum_new(G_CHECKSUM_SHA256);
		g_checksum_update(cs, (guchar *)data, strlen(data));

		resloc = g_strdup(g_checksum_get_string(cs));
		g_checksum_free(cs);

		g_free(data);
	} else return HTTP_STATUS_INTERNAL_SERVER_ERROR;

	stat = g_strdup_printf("INSERT INTO httplock (svcid,srcid,resloc) VALUES (\"%s\",\"%s\",\"%s\")", env->service_root, srcid, resloc);
	DEBUG_PRINT(stat);
	g_free(resloc);
	sqldb_transaction_begin(db);
	result=sqldb_command(db, stat);
	sqldb_transaction_commit(db);
	g_free(stat);

	if (result==SQLDB_OK) {

		gchar *loc;

		http_response_set_content_type(resp, HTTP_RESPONSE_EMPTY);

		loc = g_strdup_printf("%s/%s", env->uri_path, resloc);
		http_response_set_location(resp, loc);
		g_free(loc);

		stream_open(resp, HTTP_STATUS_CREATED, 1);
		stream_close(resp);
	}

	return HTTP_STATUS_OK;
}

RESOW_DECL_METHOD(httplock_post,env,resp,db) {

	if (HTTP_FORM_PARAMETER(env, "dstloc") != NULL) {
		return httplock_post_read_tunnel(env,resp,db);
	} else {
		if (HTTP_FORM_PARAMETER(env, "srcid") != NULL) {
			return httplock_post_create(env,resp,db);
		}
	}

	return HTTP_STATUS_BAD_REQUEST;
}

RESOW_DECL_METHOD(httplock_put,env,resp,db) {

	gchar *stat;
	sqldb_status_t result;

	if (strlen(env->service_path)==0) return HTTP_STATUS_NOT_IMPLEMENTED;

	stat = g_strdup_printf("UPDATE httplock SET new='n' WHERE resloc=\"%s\" AND new='y'", env->service_path);

	sqldb_transaction_begin(db);
	result=sqldb_command(db, stat);
	sqldb_transaction_commit(db);
	g_free(stat);

	if (result==SQLDB_OK) {

		http_response_set_content_type(resp, HTTP_RESPONSE_EMPTY);
		stream_open(resp, HTTP_STATUS_OK, 1);
		stream_close(resp);
		return HTTP_STATUS_OK;
	} else {
		return HTTP_STATUS_NOT_FOUND;
	}

	return HTTP_STATUS_NOT_FOUND;
}

RESOW_DECL_METHOD(httplock_del,env,resp,db) {

	gchar *stat;
	sqldb_status_t result;
	int retval=0;

	if (strlen(env->service_path)==0) return HTTP_STATUS_NOT_IMPLEMENTED;

	stat = g_strdup_printf("DELETE FROM httplock WHERE deleted='n' AND srcloc=\"%s\"", env->service_path);

	sqldb_transaction_begin(db);
	result=sqldb_command(db, stat);
	sqldb_transaction_commit(db);
	g_free(stat);

	if (result==SQLDB_OK) {

		http_response_set_content_type(resp, HTTP_RESPONSE_EMPTY);
		stream_open(resp, HTTP_STATUS_OK, 1);
		stream_close(resp);
		return HTTP_STATUS_OK;
	} else {
		sqldb_resultset_t *rs;

		stat = g_strdup_printf("SELECT id,svcid,srcid,dstloc,srcloc,tmout,deleted FROM httplock WHERE svcid=\"%s\" AND deleted='y' AND srcloc=\"%s\"", env->service_root, env->service_path);
		if (stat==NULL) return HTTP_STATUS_INTERNAL_SERVER_ERROR;
		rs = sqldb_query(db, stat);
		g_free(stat);

		if (sqldb_resultset_row_count(db, rs)<=0) retval=HTTP_STATUS_NOT_FOUND;
		else retval=HTTP_STATUS_GONE;

		sqldb_resultset_free(db, rs);
		return retval;
	}

	return HTTP_STATUS_NOT_FOUND;
}

RESOW_DECL_MAPPINGS {

	RESOW_REGISTER_METHOD(httplock_get, HTTP_METHOD_GET, "text/ascii");
	RESOW_REGISTER_METHOD(httplock_post, HTTP_METHOD_POST, "text/ascii");
	RESOW_REGISTER_METHOD(httplock_put, HTTP_METHOD_PUT, "text/ascii");
	RESOW_REGISTER_METHOD(httplock_del, HTTP_METHOD_DELETE, "text/ascii");

	return 0;
}

