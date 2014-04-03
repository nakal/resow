
LIBNAME=libsqldb

LDFLAGS	+= -shared -Wl,-soname,$(LIBNAME).so
CFLAGS+=`pkg-config --cflags glib-2.0`
CFLAGS+=-I../include -I/usr/local/include
CFLAGS+=-Wall -Wstrict-prototypes -Wmissing-prototypes -pedantic
CFLAGS+=-std=c99 -fPIC
CFLAGS+=-DRESOW_MYSQL_SUPPORT

MYSQL_LIBS=-L/usr/local/lib/mysql -lmysqlclient

.ifdef DEBUG
CFLAGS+=-DDEBUG -g
.endif

SQLDB_OBJS=\
	sqldb.o \
	sqldbresultset.o \
	db_mysql.o

all: $(SQLDB_OBJS)
	$(CC) $(LDFLAGS) -o $(LIBNAME).so $(SQLDB_OBJS) $(MYSQL_LIBS)

clean:
	rm -f $(SQLDB_OBJS) $(LIBNAME).so

sqldb.o: sqldb.c
	$(CC) $(CFLAGS) -c -o $@ sqldb.c

sqldbresultset.o: sqldbresultset.c
	$(CC) $(CFLAGS) -c -o $@ sqldbresultset.c

db_mysql.o: db_mysql.c
	$(CC) $(CFLAGS) -c -o $@ db_mysql.c

sqldb.c: ../include/sqldb.h
db_mysql.c: db_mysql.h ../include/sqldb.h

