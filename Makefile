VERSION := 0.0.1

PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin
MANDIR ?= $(PREFIX)/share/man

CFLAGS = -std=c99 -Wall -Wextra -pedantic -O2 -pipe -DVERSION=\"$(VERSION)\"

all: parmap
.PHONY: clean debug doc fmt install test uninstall

clean:
	rm -f parmap

debug: VERSION := $(VERSION)-debug
debug: CFLAGS += -O0 -g -fsanitize=address
debug: clean parmap

doc: parmap.1

fmt:
	clang-format -i --Werror --style=file parmap.c

install: parmap
	mkdir -p $(DESTDIR)$(BINDIR) $(DESTDIR)$(MANDIR)/man1
	install -m755 parmap $(DESTDIR)$(BINDIR)/parmap
	install -m644 parmap.1 $(DESTDIR)$(MANDIR)/man1/parmap.1

parmap: parmap.c
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@

parmap.1: parmap.1.md
	pandoc -s -t man parmap.1.md -o parmap.1

test: debug
	cram --shell zsh -v tests/*.t

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/parmap $(DESTDIR)$(MANDIR)/man1/parmap.1
	-rmdir -p $(DESTDIR)$(BINDIR) $(DESTDIR)$(MANDIR)/man1
