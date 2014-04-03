
#include <stdlib.h>

#include <sqldb.h>

void sqldb_resultset_free(sqldb_t *db, sqldb_resultset_t *resultset) {
	db->resultset_free(resultset);
}

GSList * sqldb_resultset_row_next(sqldb_t *db, sqldb_resultset_t *resultset) {
	return db->resultset_row_next(resultset);
}

void sqldb_resultset_row_free(sqldb_t *db, GSList *row) {
	db->resultset_row_free(row);
}

size_t sqldb_resultset_row_count(sqldb_t *db, sqldb_resultset_t *resultset) {
	return db->resultset_row_count(resultset);
}

unsigned long sqldb_entry_ulong(GSList *cols, size_t colnr) {

	gchar *ret;
	sqldb_field_t *f;
	unsigned long iret;

	f = (sqldb_field_t *)g_slist_nth_data(cols, colnr);
	ret = g_strndup(f->data, f->datalen);
	if (ret==NULL) return 0;

	iret = strtoul(ret, NULL, 10);

	g_free(ret);

	return iret;
}

