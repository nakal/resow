
CFLAGS += -Wall -Wstrict-prototypes -Wmissing-prototypes -pedantic
CFLAGS += `pkg-config --cflags glib-2.0`
CFLAGS += -std=c99 -I../include -I..

.ifdef DEBUG
CFLAGS+=-DDEBUG -g
.endif

HTTP_OBJS=\
	http_request.o \
	http_response.o \
	mpform.o \
	stream.o

all: $(HTTP_OBJS)

clean:
	rm -f $(HTTP_OBJS)

http_request.o: http_request.c
	$(CC) $(CFLAGS) -c -o $@ $<

http_response.o: http_response.c
	$(CC) $(CFLAGS) -c -o $@ $<

mpform.o: mpform.c
	$(CC) $(CFLAGS) -c -o $@ $<

stream.o: stream.c
	$(CC) $(CFLAGS) -c -o $@ $<

# header dependencies
http_response.c: ../include/http_response.h
http_request.c: ../include/http_request.h
stream.c: ../include/stream.h

