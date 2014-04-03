
#include <mysql/mysql.h>

#include <sqldb.h>

#include "../debug/debug.h"

#ifdef RESOW_MYSQL_SUPPORT
#include "db_mysql.h"
#endif

sqldb_t * sqldb_init(sqldb_type_t type, const gchar *hostname,
	const gchar *username, const gchar *password,
	const gchar *dbname) {

	sqldb_t *db;

	db = g_malloc0(sizeof(sqldb_t));
	if (db==NULL) {
		DEBUG_PRINT("Out of memory");
		return NULL;
	}

	db->type = type;

	switch (type) {

#ifdef RESOW_MYSQL_SUPPORT
		case SQLDB_MYSQL:
		db->_dbhandle=(void *)mysql_init(NULL);
		if (db->_dbhandle==NULL) {
			DEBUG_PRINT("Out of memory (mysql_init)");
			g_free(db);
			db = NULL;
			return NULL;
		}
		if (db_mysql_setup(db, hostname, username, password,
			dbname)!=0) {
			DEBUG_PRINT("Could not connect to MySQL database");
			mysql_close((MYSQL *)db->_dbhandle);
			g_free(db);
			db=NULL;
			return NULL;
		}
		break;
#endif

		default:
		g_free(db);
		return NULL;
	}

	/* check sqldb_t consistency */
	assert(db!=NULL);
	assert(db->_dbhandle!=NULL);
	assert(db->close!=NULL);
	assert(db->query!=NULL);
	assert(db->command!=NULL);
	assert(db->resultset_free!=NULL);
	assert(db->resultset_row_next!=NULL);
	assert(db->resultset_row_free!=NULL);
	assert(db->resultset_row_count!=NULL);
	assert(db->escape_string!=NULL);
	assert(db->transaction_begin!=NULL);
	assert(db->transaction_commit!=NULL);

	return db;
}

/* wrapper SQL methods */

void sqldb_close(sqldb_t *db) {
	db->close((struct sqldb_t *)db);
}

int sqldb_transaction_begin(sqldb_t *db) {
	return db->transaction_begin((struct sqldb_t *)db);
}

int sqldb_transaction_commit(sqldb_t *db) {
	return db->transaction_commit((struct sqldb_t *)db);
}

sqldb_resultset_t * sqldb_query(sqldb_t *db, const gchar *s) {
	return db->query((struct sqldb_t *)db, s);
}

sqldb_status_t sqldb_command(sqldb_t *db, const gchar *s) {
	return db->command((struct sqldb_t *)db, s);
}

gchar * sqldb_escape_string(sqldb_t *db, const gchar *s) {
	return db->escape_string((struct sqldb_t *)db, s);
}

