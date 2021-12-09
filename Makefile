# Released under Apache License v2.0

VERSION=0.2

DEBUG=-g -pedantic #-pg #-fprofile-arcs
LDFLAGS=-lncurses $(DEBUG)
CFLAGS+=-O2 -Wall -DVERSION=\"$(VERSION)\" $(DEBUG)

OBJS=banihstypos.o

all: banihstypos

banihstypos: $(OBJS)
	$(CC) -Wall $(OBJS) $(LDFLAGS) -o banihstypos

install: banihstypos
	cp banihstypos $(DESTDIR)/usr/local/bin

uninstall: clean
	rm -f $(DESTDIR)/usr/bin/banihstypos

clean:
	rm -f $(OBJS) banihstypos core gmon.out *.da

package: clean
	# source package
	rm -rf banihstypos-$(VERSION)*
	mkdir banihstypos-$(VERSION)
	cp *.c Makefile readme.txt banihstypos-$(VERSION)
	tar czf banihstypos-$(VERSION).tgz banihstypos-$(VERSION)
	rm -rf banihstypos-$(VERSION)
