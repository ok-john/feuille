.POSIX:
.SUFFIXES:
.PHONY: all run clean distclean install uninstall cgi

include config.mk

TARGET         = feuille.com
TARGET$(COSMO) = feuille

SRC = feuille.c util.c server.c bin.c
OBJ = $(SRC:%.c=%.o)

all: $(TARGET) feuille.1 cgi

run: $(TARGET)
	./$(TARGET)

clean:
	@printf "%-8s $(OBJ)\n" "rm"
	@rm -f $(OBJ)

distclean:
	@printf "%-8s feuille feuille.com $(OBJ)\n" "rm"
	@rm -f feuille feuille.com $(OBJ)

install: $(TARGET) feuille.1
	@echo "installing executable file to $(PREFIX)/bin"
	@mkdir -p   "$(PREFIX)/bin"
	@cp -f $(TARGET) "$(PREFIX)/bin"
	@chmod 755  "$(PREFIX)/bin/$(TARGET)"

	@echo "installing manpage to $(MAN)/man1"
	@mkdir -p $(MAN)/man1
	@cp -f feuille.1 $(MAN)/man1
	@chmod 644 $(MAN)/man1/feuille.1

uninstall: $(PREFIX)/bin/$(TARGET) $(MAN)/man1/feuille.1
	@echo "removing executable file from $(PREFIX)/bin"
	@rm -f "$(PREFIX)/bin/$(TARGET)"

	@echo "removing manpage from $(MAN)/man1"
	@rm -f $(MAN)/man1/feuille.1

feuille.1: feuille.1.md config.mk
	@printf "%-8s feuille.1.md -o feuille.1\n" "pandoc"
	@sed "s/{VERSION}/$(VERSION)/g" feuille.1.md | pandoc -s -t man -o feuille.1

feuille: $(OBJ)
	@printf "%-8s $(OBJ) -o feuille\n" "$(CC)"
	@$(CC) $(OBJ) -o feuille $(LDFLAGS)

# cosmopolitan libc
feuille.com: cosmopolitan feuille
	@printf "%-8s feuille -o feuille.com\n" "objcopy"
	@objcopy -S -O binary feuille feuille.com

cosmopolitan:
	@if [ ! -d cosmopolitan ]; then                                                                 \
	     printf "%-8s https://justine.lol/cosmopolitan/cosmopolitan-amalgamation-2.2.zip\n" "curl" ;\
	     curl -sO "https://justine.lol/cosmopolitan/cosmopolitan-amalgamation-2.2.zip"             ;\
                                                                                                    \
	     printf "%-8s cosmopolitan-amalgamation-2.2.zip\n" "unzip"                                 ;\
	     unzip -q cosmopolitan-amalgamation-2.2.zip -d cosmopolitan                                ;\
                                                                                                    \
	     rm -rf cosmopolitan-amalgamation-*                                                        ;\
	fi

# CGI script
ADDR = 127.0.0.1
PORT = 9999

cgi: cgi/feuille.cgi

cgi/feuille.cgi: cgi/feuille.cgi.c
	@printf "%-8s cgi/feuille.cgi.c -o cgi/feuille.cgi\n" "$(CC)"
	@$(CC) cgi/feuille.cgi.c -o cgi/feuille.cgi -std=c99 -O3 -static -Wall -Wextra \
                                                -DADDR=\"$(ADDR)\" -DPORT=$(PORT)  \
                                                $(INCS) $(LIBS)

.SUFFIXES: .c .o
.c.o:
	@printf "%-8s $<\n" "$(CC)"
	@$(CC) -c $< $(CFLAGS)
