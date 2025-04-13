# Makefile pour projet C avec Doxygen et installation contrôlée

### CONFIGURATION #####################################################
CC = gcc
CFLAGS = -Wall -Wextra -g -Wno-sign-compare
LDFLAGS =
TARGET = gestionnairefs
SRCS = main.c file_system.c
HDRS = file_system.h
OBJS = $(SRCS:.c=.o)

# Installation
PREFIX = /usr/local
BINDIR = $(PREFIX)/bin
LOCALDIR = $(HOME)/.local/bin

# Doxygen
DOXYFILE = Doxyfile

### RÈGLES ###########################################################
.PHONY: all clean install uninstall doc

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c $(HDRS)
	$(CC) $(CFLAGS) -c $< -o $@

doc:
	doxygen $(DOXYFILE)

clean:
	rm -f $(OBJS) $(TARGET)

### INSTALLATION INTELLIGENTE #########################################
install: $(TARGET)
	@if [ -w $(PREFIX) ]; then \
		install -d $(BINDIR); \
		install -m 755 $(TARGET) $(BINDIR); \
		echo "Installé dans $(BINDIR)"; \
	else \
		install -d $(LOCALDIR); \
		install -m 755 $(TARGET) $(LOCALDIR); \
		echo "Installé dans $(LOCALDIR) (manque de droits sur $(PREFIX))"; \
	fi

uninstall:
	@if [ -f $(BINDIR)/$(TARGET) ]; then \
		rm -f $(BINDIR)/$(TARGET); \
		echo "Désinstallé de $(BINDIR)"; \
	fi
	@if [ -f $(LOCALDIR)/$(TARGET) ]; then \
		rm -f $(LOCALDIR)/$(TARGET); \
		echo "Désinstallé de $(LOCALDIR)"; \
	fi

### VÉRIFICATIONS ####################################################
check:
	@echo "=== Vérifications ==="
	@which $(CC) >/dev/null || echo "ERREUR: $(CC) non installé"
	@for f in $(SRCS); do \
		[ -f $$f ] || echo "ERREUR: $$f manquant"; \
	done
	@[ -f $(DOXYFILE) ] || echo "ATTENTION: $(DOXYFILE) manquant (doc: make doc)"
