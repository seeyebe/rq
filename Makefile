CC = gcc
CFLAGS = -std=c17 -Wall -Wextra -Wpedantic -O2
SRCDIR = src
SOURCES = $(SRCDIR)/search.c $(SRCDIR)/cli.c $(SRCDIR)/utils.c main.c
TARGET = snub.exe
BUILDDIR = build
OUTFILE = $(BUILDDIR)/$(TARGET)

.PHONY: all clean install test

all: $(OUTFILE)

$(OUTFILE): $(SOURCES)
	@if not exist "$(BUILDDIR)" mkdir "$(BUILDDIR)"
	$(CC) $(CFLAGS) $(SOURCES) -o $(OUTFILE)

clean:
	@if exist "$(BUILDDIR)" rmdir /s /q "$(BUILDDIR)"

install: $(OUTFILE)
	copy "$(OUTFILE)" "C:\Windows\System32\"

