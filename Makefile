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

RPATH = \$$ORIGIN/$(shell realpath --relative-to="$(bindir)" "$(libdir)")

BIN_CFLAGS =
BIN_LDFLAGS = -l cnoti.$(VERSION_MAJOR) -L ./ -Wl,-R$(RPATH)

ifdef USE_CJSON
BIN_CFLAGS += -DUSE_CJSON $(shell pkg-config --cflags libcjson)
BIN_LDFLAGS += $(shell pkg-config --libs libcjson)
endif

.PHONY: all install uninstall clean check

all: libcnoti.$(VERSION_MAJOR).so noticat

libcnoti.%.so: cnoti.c cnoti.h
	$(CC) cnoti.c $(LIB_CFLAGS) $(LIB_LDFLAGS) -shared -o $@

noticat: libcnoti.$(VERSION_MAJOR).so noticat.c
	$(CC) noticat.c $(BIN_CFLAGS) $(BIN_LDFLAGS) -o $@

cnoti.pc:
	echo 'prefix=$(prefix)' > $@
	echo 'exec_prefix=$${prefix}' >> $@
	echo 'includedir=$${prefix}/$(include_suffix)' >> $@
	echo 'libdir=$${prefix}/$(lib_suffix)' >> $@
	echo '' >> $@
	echo 'Name: Noti' >> $@
	echo 'Description: little DBus cnotification monitor' >> $@
	echo 'Version: $(VERSION_MICRO)' >> $@
	echo 'Requires: dbus-1' >> $@
	echo 'Libs: -L$${libdir} -lcnoti' >> $@
	echo 'Cflags: -I$${includedir}' >> $@

compile_commands.json:
	bear -- make

check: compile_commands.json
	clang-tidy cnoti.c noticat.c
	clang-check cnoti.c noticat.c
	clang-format -i cnoti.c noticat.c cnoti.h
	git diff --exit-code &>/dev/null

install: libcnoti.$(VERSION_MAJOR).so noticat cnoti.pc
	mkdir -p $(DESTDIR)$(libdir) $(DESTDIR)$(bindir) $(DESTDIR)$(includedir) $(DESTDIR)$(PKG_CONFIG_PATH)
	install -t $(DESTDIR)$(libdir) libcnoti.$(VERSION_MAJOR).so
	ln -sf libcnoti.$(VERSION_MAJOR).so $(DESTDIR)$(libdir)/libcnoti.so
	ln -sf libcnoti.$(VERSION_MAJOR).so $(DESTDIR)$(libdir)/libcnoti.$(VERSION_MICRO).so
	install -t $(DESTDIR)$(includedir) cnoti.h
	install -t $(DESTDIR)$(bindir) noticat
	install cnoti.pc $(DESTDIR)$(PKG_CONFIG_PATH)/cnoti.pc

uninstall:
	rm -f $(DESTDIR)$(PKG_CONFIG_PATH)/cnoti.pc
	rm -f $(DESTDIR)$(bindir)/noticat
	rm -f $(DESTDIR)$(includedir)/cnoti.h
	rm -f $(DESTDIR)$(libdir)/libcnoti.so
	rm -f $(DESTDIR)$(libdir)/libcnoti.$(VERSION_MAJOR).so
	rm -f $(DESTDIR)$(libdir)/libcnoti.$(VERSION_MICRO).so
	rmdir --ignore-fail-on-non-empty $(DESTDIR)$(libdir) $(DESTDIR)$(bindir) $(DESTDIR)$(includedir)

clean:
	rm -f noticat libcnoti.*so cnoti.pc compile_commands.json
