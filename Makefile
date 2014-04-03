
INSTALLPATH=/usr/local/www/apache22/resow
RESOW_LD_PATH=/usr/local/share/resow

DEBUG=yes

all:
	cd src && $(MAKE) RESOW_LD_PATH="${RESOW_LD_PATH}" DEBUG="$(DEBUG)"
	cd src && $(MAKE) link_cgi RESOW_LD_PATH="${RESOW_LD_PATH}"

clean:
	cd src && $(MAKE) clean

install: install-libs install-apps
	install src/resow $(INSTALLPATH)/resow

install-apps:
	@echo "===> Installing apps"
	install -d $(RESOW_LD_PATH)/apps
	cd src/apps && $(MAKE) install RESOW_LD_PATH="${RESOW_LD_PATH}"

install-libs:
	@echo "===> Installing libs"
	install -d $(RESOW_LD_PATH)/lib
	install src/sqldb/libsqldb.so ${RESOW_LD_PATH}/lib/libsqldb.so
