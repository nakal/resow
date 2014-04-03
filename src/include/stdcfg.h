
#ifndef RESOW_CONFIG_H_INCLUDED
#define RESOW_CONFIG_H_INCLUDED

#define RESOW_VERSION "0.0.0"

#include <config.h>

#ifndef RESOW_SQLDB_HOSTNAME
#define RESOW_SQLDB_HOSTNAME	"localhost"
#endif

#ifndef RESOW_SQLDB_USERNAME
#define RESOW_SQLDB_USERNAME	"resow"
#endif

#ifndef RESOW_SQLDB_PASSWORD
#define RESOW_SQLDB_PASSWORD	"resow"
#endif

#ifndef RESOW_SQLDB_DBNAME
#define RESOW_SQLDB_DBNAME	"resow"
#endif

#ifndef RESOW_FLATFS_ROOT
#define RESOW_FLATFS_ROOT	"/usr/local/share/resow/flatfs"
#endif

#endif
