# Makefile for dwlb-status

SHELL := /bin/sh

PROG := dwlb-status
SRC  := dwlb-status.c

CC      ?= gcc
CFLAGS  ?= -O2 -pipe -Wall -Wextra -march=native
LDFLAGS ?=

# Install location: defaults to $HOME/.local/bin
PREFIX ?= $(HOME)/.local
BINDIR ?= $(PREFIX)/bin

.PHONY: all clean install uninstall

all: $(PROG)

$(PROG): $(SRC)
	$(CC) $(CFLAGS) $(CPPFLAGS) $< -o $@ $(LDFLAGS)

install: $(PROG)
	@mkdir -p "$(BINDIR)"
	@install -m 0755 "$(PROG)" "$(BINDIR)/$(PROG)"
	@echo "Installed $(PROG) to $(BINDIR)"

uninstall:
	@rm -f "$(BINDIR)/$(PROG)"
	@echo "Removed $(BINDIR)/$(PROG)"

clean:
	@rm -f "$(PROG)" *.o

