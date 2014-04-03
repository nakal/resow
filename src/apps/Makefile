
CFLAGS += -Wall -Wstrict-prototypes -Wmissing-prototypes
CFLAGS += `pkg-config --cflags glib-2.0`
CFLAGS += -DRESOW_LD_PATH=\"${RESOW_LD_PATH}\" -I../include -I..

APPLIST = \
binder \
setup \
flatfs \
httplock

.ifdef DEBUG
CFLAGS+=-DDEBUG -g
.endif

OBJS=\
	application.o

all: $(OBJS) apps

clean: apps-clean
	rm -f $(OBJS)

application.o:
	$(CC) $(CFLAGS) -c -o $@ application.c

application.c: application.h
application.h: ../include/sqldb.h

apps:
	@for lib in $(APPLIST); do cd $$lib; $(MAKE) all; cd .. ; done

apps-clean:
	@for lib in $(APPLIST); do cd $$lib; $(MAKE) clean; cd .. ; done

install:
	@for lib in $(APPLIST); do install $$lib/$$lib.so $(RESOW_LD_PATH)/apps/$$lib.so; done
