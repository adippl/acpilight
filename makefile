prefix = /usr
datarootdir = $(prefix)/share
datadir = $(datarootdir)
exec_prefix = $(prefix)
bindir = $(exec_prefix)/bin
mandir = $(datarootdir)/man
man1dir = $(mandir)/man1
sysconfdir = /etc

ifeq ($(DESTDIR),)
 DESTDIR := 
 PREFIX := /usr/local
else
 ifeq ($(PREFIX),)
  PREFIX := /usr
 endif
endif

ifeq ($(CFLAGS),)
 CFLAGS := -Wall -pedantic -O2 -g 
endif

xbacklight: 
	cc $(CFLAGS) xbacklight.c -o xbacklight

clean:
	rm -rf xbacklight

install: xbacklight xbacklight.1 90-backlight.rules
	install -D -m 755 xbacklight ${DESTDIR}${PREFIX}/bin/xbacklight
	install -D -m 644 xbacklight ${DESTDIR}/usr/share/man/man1/xbacklight.1
	install -D -m 644 90-backlight.rules $(DESTDIR)/etc/udev/rules.d/90-backlight.rules 
	udevadm trigger -s backlight -c add

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/xbacklight
	rm -f ${DESTDIR}/usr/share/man/man1/xbacklight.1
	rm -f $(DESTDIR)/etc/udev/rules.d/90-backlight.rules 

.PHONY: install uninstall all clean p

p:
	git push --all origin
	git push --all github




