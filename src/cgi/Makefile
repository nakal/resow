
CFLAGS += -Wall -Wstrict-prototypes -Wmissing-prototypes -pedantic
CFLAGS += `pkg-config --cflags glib-2.0`
CFLAGS += -std=c99 -I../include -I..

.ifdef DEBUG
CFLAGS+=-DDEBUG -g
.endif

MAIN_OBJS=\
	main.o

all: $(MAIN_OBJS)

clean:
	rm -f $(MAIN_OBJS)

main.o: main.c
	$(CC) $(CFLAGS) -c -o $@ $<

