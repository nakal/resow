
#ifndef RESOW_SQLDB_INCLUDE
#define RESOW_SQLDB_INCLUDE

#include <glib.h>

typedef enum {
	SQLDB_ERROR = 0,
	SQLDB_OK = 1
} sqldb_status_t;

/* supported database driver types */
typedef enum {
	SQLDB_MYSQL
} sqldb_type_t;

struct sqldb_t;

/* import resultset method types */
#include <sqldbresultset.h>

/* SQL method types */
typedef void (*sqldb_close_t)(struct sqldb_t *);
typedef sqldb_resultset_t * (*sqldb_query_t)(struct sqldb_t *, const gchar *);
typedef sqldb_status_t (*sqldb_command_t)(struct sqldb_t *, const gchar *);
typedef gchar * (*sqldb_escape_string_t)
	(struct sqldb_t *db, const gchar *string);
typedef int (*sqldb_transaction_begin_t)(struct sqldb_t *);
typedef int (*sqldb_transaction_commit_t)(struct sqldb_t *);

/*
 * Main sqldb_t struct equivalent to a transparent database handle.
 *
 * When changing this struct, also look at the assertions in sqldb.c
 */
typedef struct {

	sqldb_type_t type;

	void *_dbhandle;

	sqldb_close_t close;
	sqldb_query_t query;
	sqldb_command_t command;

	sqldb_resultset_free_t resultset_free;
	sqldb_resultset_row_next_t resultset_row_next;
	sqldb_resultset_row_free_t resultset_row_free;
	sqldb_resultset_row_count_t resultset_row_count;
	sqldb_escape_string_t escape_string;
	sqldb_transaction_begin_t transaction_begin;
	sqldb_transaction_commit_t transaction_commit;

} sqldb_t;

/* wrapper SQL methods */

extern sqldb_t * sqldb_init(sqldb_type_t type, const gchar *hostname,
	const gchar *username, const gchar *password, const gchar *dbname);
extern void sqldb_close(sqldb_t *db);
extern sqldb_resultset_t * sqldb_query(sqldb_t *db, const gchar *s);
extern sqldb_status_t sqldb_command(sqldb_t *db, const gchar *s);

extern void sqldb_resultset_free(sqldb_t *db, sqldb_resultset_t *resultset);
extern GSList * sqldb_resultset_row_next(sqldb_t *db,
	sqldb_resultset_t *resultset);
extern void sqldb_resultset_row_free(sqldb_t *db, GSList *row);
extern size_t sqldb_resultset_row_count(sqldb_t *db,
	sqldb_resultset_t *resultset);
extern gchar * sqldb_escape_string(sqldb_t *db, const gchar *s);
extern int sqldb_transaction_begin(sqldb_t *db);
extern int sqldb_transaction_commit(sqldb_t *db);

#endif

