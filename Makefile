libnoti.so: noti.c noti.h
	gcc noti.c -I /usr/include/dbus-1.0/ -I /usr/lib/dbus-1.0/include/ -l dbus-1 -shared -o $@

noticat: libnoti.so noticat.c
	gcc noticat.c -l noti -L./ -o $@

all: libnoti.so noticat
