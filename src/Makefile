
CGI_LIBS=`pkg-config --libs glib-2.0`

.ifdef DEBUG
LDFLAGS+=-g
.endif

CGI_OBJS=\
	cgi/main.o

HTTP_OBJS=\
	http/http_request.o \
	http/http_response.o \
	http/mpform.o \
	http/stream.o

SQLDB_OBJS=\
	sqldb/sqldb.o \
	sqldb/db_mysql.o

APPS_OBJS=\
	apps/application.o

all:
	cd cgi && $(MAKE)
	cd sqldb && $(MAKE)
	cd apps && $(MAKE) RESOW_LD_PATH="${RESOW_LD_PATH}"
	cd http && $(MAKE)

clean:
	cd cgi && $(MAKE) clean
	cd sqldb && $(MAKE) clean
	cd apps && $(MAKE) clean
	cd http && $(MAKE) clean
	rm -f resow

link_cgi:
	$(CC) $(LDFLAGS) -Wl,-export-dynamic -Wl,-rpath,${RESOW_LD_PATH}/lib \
		-o resow $(CGI_OBJS) \
		$(APPS_OBJS) $(HTTP_OBJS) $(CGI_LIBS) -Lsqldb -lsqldb

