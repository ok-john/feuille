# feuille version
VERSION = 2.0.0

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

# cosmopolitan libc flags
CCFLAGS  = -std=c99 -nostdinc -fno-pie -fno-omit-frame-pointer \
           -mno-tls-direct-seg-refs -mno-red-zone              \
           -DVERSION=\"$(VERSION)\" -DCOSMOPOLITAN             \
           -I. -include cosmopolitan/cosmopolitan.h

CLDFLAGS = -std=c99 -static -nostdlib -fno-pie -fno-omit-frame-pointer \
           -mno-tls-direct-seg-refs -mno-red-zone                      \
           -fuse-ld=bfd -Wl,-T,cosmopolitan/ape.lds -Wl,--gc-sections  \
           cosmopolitan/crt.o cosmopolitan/ape-no-modify-self.o cosmopolitan/cosmopolitan.a

# standard libc flags
CCFLAGS$(COSMO)  = -std=c99 -DVERSION=\"$(VERSION)\" $(INCS)
CLDFLAGS$(COSMO) = $(LIBS)

# debug flags
CFLAGS  = -g -Wall -Wextra -Wno-sign-compare -DDEBUG $(CCFLAGS)
LDFLAGS = -g $(CLDFLAGS)

# release flags
CFLAGS$(DEBUG)  = -O3 -Wall -Wextra -Wno-sign-compare $(CCFLAGS)
LDFLAGS$(DEBUG) = -s $(CLDFLAGS)

# static build (uncomment)
#LD_FLAGS += -static
