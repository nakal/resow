
#ifndef RESOW_SQLDB_RESULTSET_INCLUDE
#define RESOW_SQLDB_RESULTSET_INCLUDE

#include <glib.h>

/*
 * Opaque structure for a result set after a SELECT query
 */
typedef struct {

	void *handle;

} sqldb_resultset_t;

typedef struct {
	gchar *data;
	size_t datalen;
} sqldb_field_t;

typedef void (*sqldb_resultset_free_t)(sqldb_resultset_t *resultset);
typedef GSList * (*sqldb_resultset_row_next_t)(sqldb_resultset_t *resultset);
typedef void (*sqldb_resultset_row_free_t)(GSList *row);
typedef size_t (*sqldb_resultset_row_count_t)(sqldb_resultset_t *resultset);

extern unsigned long sqldb_entry_ulong(GSList *cols, size_t colnr);

/* Macros for result access */
#define SQLDB_ENTRY_STRING(cols,nr) \
(g_strndup(((sqldb_field_t *)g_slist_nth_data((cols), (nr)))->data, \
((sqldb_field_t *)g_slist_nth_data((cols), (nr)))->datalen))
#define SQLDB_ENTRY_ULONG(cols,nr) sqldb_entry_ulong(cols,nr)

#define SQLDB_ENTRY_STRING_FREE(ent) g_free(ent)

#define SQLDB_ENTRY(cols,nr) ((sqldb_field_t *)g_slist_nth_data((cols), (nr)))


#endif

