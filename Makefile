#
# Makefile
# Adrian Perez, 2014-01-06 12:23
#

GLIB_CFLAGS := $(shell pkg-config glib-2.0 --cflags)
GLIB_LDLIBS := $(shell pkg-config glib-2.0 --libs)

OPTFLAGS ?= -O2
LDFLAGS  += -lnotmuch $(GLIB_LDLIBS)
CFLAGS    = -Wall $(OPTFLAGS) $(GLIB_CFLAGS) -std=c99

all: notmuch-addrlookup

notmuch-addrlookup: notmuch-addrlookup.o

clean:
	$(RM) notmuch-addrlookup notmuch-addrlookup.o

print-cflags:
	@echo "$(CFLAGS)"

.PHONY: print-cflags

