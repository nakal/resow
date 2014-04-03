
LIBNAME=binder

LDFLAGS	+= -shared -Wl,-soname,$(LIBNAME)
CFLAGS	+= -Wall -Wstrict-prototypes -Wmissing-prototypes -pedantic
CFLAGS	+= `pkg-config --cflags glib-2.0`
CFLAGS	+= -fPIC -std=c99 -I../../include -I../..

all:
	@echo ""
	@echo ===\> Application: $(LIBNAME)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(LIBNAME).so $(LIBNAME).c
	@echo ""

clean:
	rm -f $(LIBNAME).so

