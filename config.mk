# feuille version
VERSION = 1.6.20

# paths (customize them to fit your system)
PREFIX = /usr/local
MAN    = $(PREFIX)/share/man

# includes and libs
INCS = -I/usr/include -I.
LIBS = -L/usr/lib -lc

# OpenBSD / FreeBSD (uncomment)
#MAN  = $(PREFIX)/man
#INCS = -I/usr/share/include -I.
#LIBS = -L/usr/share/lib -lc

# compiler
CC = cc

# debug build
CFLAGS  = -g -std=c99 -Wall -Wextra -Wpedantic -Wno-sign-compare -DVERSION=\"$(VERSION)\" -DDEBUG $(INCS)
LDFLAGS = -g $(LIBS)

# release build
CFLAGS$(DEBUG)  = -std=c99 -Wall -Wextra -Wno-sign-compare -DVERSION=\"$(VERSION)\" -O3 $(INCS)
LDFLAGS$(DEBUG) = -s $(LIBS)

# static build (uncomment)
#LD_FLAGS += -static
