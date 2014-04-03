
#ifdef RESOW_MYSQL_SUPPORT

#include <string.h>
#include <mysql/mysql.h>

#include "db_mysql.h"

#include "../debug/debug.h"

int db_mysql_setup(sqldb_t *db, const gchar *hostname,
	const gchar *username, const gchar *password, const gchar *dbname) {

	my_bool flag=1;

	if (mysql_options(DB_MYSQL(db), MYSQL_OPT_RECONNECT, &flag)!=0) {
		DEBUG_PRINT("mysql_options (MYSQL_OPT_RECONNECT) error");
		return -1;
	}

	if ((db->_dbhandle=mysql_real_connect(DB_MYSQL(db), hostname, username,
		password, dbname, 0, NULL, 0))==NULL) {

		DEBUG_PRINT("mysql_real_connect error");
		return -1;
	}

	if (mysql_autocommit(DB_MYSQL(db), 0)!=0) {
		DEBUG_PRINT("mysql_autocommit disable error");
		mysql_close(db->_dbhandle);
		return -1;
	}

	db->close = db_mysql_close;
	db->query = db_mysql_query;
	db->command = db_mysql_command;

	db->resultset_free = db_mysql_resultset_free;
	db->resultset_row_next = db_mysql_resultset_row_next;
	db->resultset_row_free = db_mysql_resultset_row_free;
	db->resultset_row_count = db_mysql_resultset_row_count;
	db->escape_string = db_mysql_escape_string;
	db->transaction_begin = db_mysql_transaction_begin;
	db->transaction_commit = db_mysql_transaction_commit;

	return 0;
}

void db_mysql_close(struct sqldb_t *db) {

	mysql_close(DB_MYSQL(db));
}

int db_mysql_transaction_begin(struct sqldb_t *db) {
	return db_mysql_command(db, "BEGIN");
}

int db_mysql_transaction_commit(struct sqldb_t *db) {
	return db_mysql_command(db, "COMMIT");
}

sqldb_resultset_t * db_mysql_query(struct sqldb_t *db, const gchar * statement) {

	int ret;
	MYSQL_RES *result;
	sqldb_resultset_t *rs;

	ret = db_mysql_command(db, statement);

	if (ret!=SQLDB_OK) {
		DEBUG_PRINT("Error in SQL statement");
		return NULL;
	}

	/* fetch result */
	result = mysql_store_result(DB_MYSQL(db));

	rs = (sqldb_resultset_t *)g_malloc(sizeof(sqldb_resultset_t));
	if (rs==NULL) {
		DEBUG_PRINT("Error in retrieval of SQL result");
		mysql_free_result(result);
		return NULL;
	}

	rs->handle = result;

	return rs;
}

sqldb_status_t db_mysql_command(struct sqldb_t *db, const gchar * statement) {

	int ret;

	ret = mysql_real_query(DB_MYSQL(db), statement, strlen(statement));

	if (ret!=0) {
		DEBUG_PRINT("Error in SQL statement");
		DEBUG_PRINT(mysql_error(DB_MYSQL(db)));
		return SQLDB_ERROR;
	}

	return SQLDB_OK;
}

void db_mysql_resultset_free(sqldb_resultset_t *resultset) {

	assert(resultset!=NULL);

	if (resultset->handle!=NULL)
		mysql_free_result((MYSQL_RES *)resultset->handle);
	g_free(resultset);
}

GSList * db_mysql_resultset_row_next(sqldb_resultset_t *resultset) {

	unsigned int num_fields, i;
	MYSQL_ROW row;
	GSList *fields=NULL;
	unsigned long *lengths;
	sqldb_field_t *f;

	num_fields = mysql_num_fields((MYSQL_RES *)resultset->handle);
	row = mysql_fetch_row((MYSQL_RES *)resultset->handle);
	if (row==NULL) {
		/* Empty result, do not produce noise here (normal operation) */
		return NULL;
	}

	lengths = mysql_fetch_lengths(resultset->handle);

	for (i=0; i<num_fields; i++) {

		f=(sqldb_field_t *)g_malloc(sizeof(sqldb_field_t));
		if (f==NULL) {
			DEBUG_PRINT("Out of memory");
			db_mysql_resultset_row_free(fields);
			return NULL;
		}

		f->datalen = (size_t)lengths[i];

		if (f->datalen>0) {
			f->data = (gchar *)g_malloc(f->datalen);
			if (f->data==NULL) {
				DEBUG_PRINT("Out of memory");
#ifdef DEBUG
				fprintf(stderr, "Tried to allocate %lu bytes.\n", f->datalen);
#endif
				g_free(f);
				db_mysql_resultset_row_free(fields);
				return NULL;
			}
			memcpy(f->data, row[i], f->datalen);
		} else {
			f->data=NULL;
		}
		/* fprintf(stderr, "Field %u: %s\n", i+1, f->data); */

		fields=g_slist_append(fields, (gpointer)f);
	}

	/* fprintf(stderr, "Field number: %u\n", num_fields); */
	return fields;
}

void db_mysql_resultset_row_free(GSList *row) {

	GSList *list;

	/* free list data */
	list = row;
	while (list != NULL) {

		sqldb_field_t *field;

		field = (sqldb_field_t *)list->data;
		assert(field != NULL);

		/* field->data might be NULL when field is NULL */
		if (field->data!=NULL) g_free(field->data);
		g_free(field);

		list = g_slist_next(list);
	}

	/* free list structure */
	g_slist_free(row);
}

size_t db_mysql_resultset_row_count(sqldb_resultset_t *resultset) {
	/*
		XXX
		
		This works only, because we use mysql_store_result(),
		instead of mysql_use_result().
	*/
	return mysql_num_rows((MYSQL_RES *)resultset->handle);
}

gchar * db_mysql_escape_string(struct sqldb_t *db, const gchar *s) {

	size_t srclen;
	unsigned long ret;
	gchar *to;

	srclen = strlen(s);

	to = g_malloc(srclen*2+1); /* see MySQL docs for explanation */
	if (to==NULL) return NULL;

	ret=mysql_real_escape_string(DB_MYSQL(db), to, s, srclen);

	return to;
}

#endif

