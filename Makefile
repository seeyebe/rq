CC = gcc
CFLAGS = -std=c17 -Wall -Wextra -Wpedantic -O2 -g
SRCDIR = src
SOURCES = $(SRCDIR)/platform.c $(SRCDIR)/pattern.c $(SRCDIR)/thread_pool.c $(SRCDIR)/criteria.c $(SRCDIR)/search.c $(SRCDIR)/cli.c $(SRCDIR)/utils.c main.c
TARGET = snub.exe
BUILDDIR = build
OUTFILE = $(BUILDDIR)/$(TARGET)
LIBS = -lshlwapi

# Debug build flags
DEBUG_CFLAGS = -std=c17 -Wall -Wextra -Wpedantic -O0 -g -DDEBUG -fsanitize=address,undefined -fno-omit-frame-pointer
DEBUG_LDFLAGS = -fsanitize=address,undefined

.PHONY: all clean install test debug analyze

all: $(OUTFILE)

$(OUTFILE): $(SOURCES)
	@if not exist "$(BUILDDIR)" mkdir "$(BUILDDIR)"
	$(CC) $(CFLAGS) $(SOURCES) $(LIBS) -o $(OUTFILE)

clean:
	@if exist "$(BUILDDIR)" rmdir /s /q "$(BUILDDIR)"

install: $(OUTFILE)
	copy "$(OUTFILE)" "C:\Windows\System32\"
