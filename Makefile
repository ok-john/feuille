.POSIX:
.SUFFIXES:
.PHONY: all run clean distclean install uninstall cgi

include config.mk

TARGET = feuille
SRC    = feuille.c util.c server.c bin.c
OBJ    = $(SRC:%.c=%.o)

all: $(TARGET) $(TARGET).1 cgi

run: $(TARGET)
	./$(TARGET)

clean:
	@printf "%-8s $(OBJ)\n" "rm"
	@rm -f $(OBJ)

distclean:
	@printf "%-8s $(TARGET) $(OBJ)\n" "rm"
	@rm -f $(TARGET) $(OBJ)

install: $(TARGET) $(TARGET).1
	@echo "installing executable file to $(PREFIX)/bin"
	@mkdir -p   "$(PREFIX)/bin"
	@cp -f $(TARGET) "$(PREFIX)/bin"
	@chmod 755  "$(PREFIX)/bin/$(TARGET)"

	@echo "installing manpage to $(MAN)/man1"
	@mkdir -p $(MAN)/man1
	@mv -f $(TARGET).1 $(MAN)/man1
	@chmod 644 $(MAN)/man1/$(TARGET).1

uninstall: $(PREFIX)/bin/$(TARGET) $(MAN)/man1/$(TARGET).1
	@echo "removing executable file from $(PREFIX)/bin"
	@rm -f "$(PREFIX)/bin/$(TARGET)"

	@echo "removing manpage from $(MAN)/man1"
	@rm -f $(MAN)/man1/$(TARGET).1

$(TARGET): $(OBJ)
	@printf "%-8s $(OBJ) -o $@\n" "$(CC)"
	@$(CC) $(LDFLAGS) $(OBJ) -o $@

$(TARGET).1: $(TARGET).1.md
	@printf "%-8s $(TARGET).1.md -o $@\n" "pandoc"
	@sed "s/{VERSION}/$(VERSION)/g" $(TARGET).1.md | pandoc -s -t man -o $@

ADDR = 127.0.0.1
PORT = 8888

cgi: cgi/feuille.cgi

cgi/feuille.cgi: cgi/feuille.cgi.c
	@printf "%-8s cgi/feuille.cgi.c -o $@\n" "$(CC)"
	@$(CC) $(CFLAGS) $(LDFLAGS) -static -DADDR=\"$(ADDR)\" -DPORT=$(PORT) cgi/feuille.cgi.c -o $@

.SUFFIXES: .c .o
.c.o:
	@printf "%-8s $<\n" "$(CC)"
	@$(CC) $(CFLAGS) -c $<
