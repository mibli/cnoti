DESCRIBE := $(shell git describe --match "v*" --always --tags)
DESCRIBE_PARTS := $(subst -, ,$(DESCRIBE))

VERSION_TAG := $(word 1,$(DESCRIBE_PARTS))
COMMITS_SINCE_TAG := $(word 2,$(DESCRIBE_PARTS))

VERSION ?= $(subst v,,$(VERSION_TAG))
VERSION_PARTS := $(subst ., ,$(VERSION))

VERSION_MAJOR := $(word 1,$(VERSION_PARTS))
VERSION_MINOR := $(word 1,$(VERSION_PARTS)).$(word 2,$(VERSION_PARTS))
VERSION_MICRO := $(word 1,$(VERSION_PARTS)).$(word 2,$(VERSION_PARTS)).$(word 3,$(VERSION_PARTS))

PKG_CONFIG_PATH ?= /usr/share/pkgconfig

prefix ?= /usr/local
exec_prefix ?= $(prefix)

bin_suffix := bin
bindir ?= $(exec_prefix)/$(bin_suffix)
include_suffix := include
includedir ?= $(prefix)/$(include_suffix)
lib_suffix := lib
libdir ?= $(prefix)/$(lib_suffix)

CFLAGS ?=
LDFLAGS ?=
CC ?= gcc

LIB_CFLAGS = $(CFLAGS) $(shell pkg-config --cflags dbus-1)
LIB_LDFLAGS = $(LDFLAGS) $(shell pkg-config --libs dbus-1)

BIN_CFLAGS =
BIN_LDFLAGS = -l noti -L ./

ifdef USE_CJSON
BIN_CFLAGS += -DUSE_CJSON $(shell pkg-config --cflags libcjson)
BIN_LDFLAGS += $(shell pkg-config --libs libcjson)
endif

.PHONY: all install uninstall clean check

all: libnoti.so noticat

libnoti.so: noti.c noti.h
	$(CC) noti.c $(LIB_CFLAGS) $(LIB_LDFLAGS) -shared -o $@

noticat: libnoti.so noticat.c
	$(CC) noticat.c $(BIN_CFLAGS) $(BIN_LDFLAGS) -o $@

noti.pc:
	echo 'prefix=$(prefix)' > $@
	echo 'exec_prefix=$${prefix}' >> $@
	echo 'includedir=$${prefix}/$(include_suffix)' >> $@
	echo 'libdir=$${prefix}/$(lib_suffix)' >> $@
	echo '' >> $@
	echo 'Name: Noti' >> $@
	echo 'Description: little DBus notification monitor' >> $@
	echo 'Version: $(VERSION_MICRO)' >> $@
	echo 'Requires: dbus-1' >> $@
	echo 'Libs: -L$${libdir} -lnoti' >> $@
	echo 'Cflags: -I$${includedir}' >> $@

compile_commands.json:
	bear -- make

check: compile_commands.json
	clang-tidy noti.c noticat.c
	clang-check noti.c noticat.c
	clang-format -i noti.c noticat.c noti.h
	git diff --exit-code &>/dev/null

install: libnoti.so noticat noti.pc
	mkdir -p $(DESTDIR)$(libdir) $(DESTDIR)$(bindir) $(DESTDIR)$(includedir) $(DESTDIR)$(PKG_CONFIG_PATH)
	cp libnoti.so $(DESTDIR)$(libdir)/libnoti.$(VERSION_MICRO).so
	ln -sf libnoti.$(VERSION_MICRO).so $(DESTDIR)$(libdir)/libnoti.so
	ln -sf libnoti.$(VERSION_MICRO).so $(DESTDIR)$(libdir)/libnoti.$(VERSION_MAJOR).so
	cp noti.h $(DESTDIR)$(includedir)/noti
	cp noticat $(DESTDIR)$(bindir)/noticat
	cp noti.pc $(DESTDIR)$(PKG_CONFIG_PATH)/noti.pc

uninstall:
	rm -f $(DESTDIR)$(PKG_CONFIG_PATH)/noti.pc
	rm -f $(DESTDIR)$(bindir)/noticat
	rm -f $(DESTDIR)$(includedir)/noti.h
	rm -f $(DESTDIR)$(libdir)/libnoti.so
	rm -f $(DESTDIR)$(libdir)/libnoti.$(VERSION_MAJOR).so
	rm -f $(DESTDIR)$(libdir)/libnoti.$(VERSION_MICRO).so
	rmdir --ignore-fail-on-non-empty $(DESTDIR)$(libdir) $(DESTDIR)$(bindir) $(DESTDIR)$(includedir)

clean:
	rm -f noticat libnoti.so noti.pc compile_commands.json
