
#ifndef RESOW_SQLDB_MYSQL_DRIVER_INCLUDE
#define RESOW_SQLDB_MYSQL_DRIVER_INCLUDE

#include <sqldb.h>

#define DB_MYSQL(db) (MYSQL *)((sqldb_t *)db)->_dbhandle

extern int db_mysql_setup(sqldb_t *db, const gchar *hostname,
	const gchar *username, const gchar *password, const gchar *dbname);
extern void db_mysql_close(struct sqldb_t *db);
extern sqldb_resultset_t * db_mysql_query(struct sqldb_t *db, const gchar * statement);
extern sqldb_status_t db_mysql_command(struct sqldb_t *db, const gchar * statement);

extern void db_mysql_resultset_free(sqldb_resultset_t *resultset);
extern GSList * db_mysql_resultset_row_next(sqldb_resultset_t *resultset);
extern void db_mysql_resultset_row_free(GSList *row);
extern size_t db_mysql_resultset_row_count(sqldb_resultset_t *resultset);
extern gchar * db_mysql_escape_string(struct sqldb_t *db, const gchar *s);
extern int db_mysql_transaction_begin(struct sqldb_t *db);
extern int db_mysql_transaction_commit(struct sqldb_t *db);

#endif

