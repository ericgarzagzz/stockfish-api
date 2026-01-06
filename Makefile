SHELL = /bin/sh
CC = gcc
CFLAGS = -Wall -pedantic -Werror -Wextra
LDFLAGS = 
LIBS = 

SRCDIR = src
BUILDDIR = build

SRCS = main.c
OBJS = $(patsubst %.c,$(BUILDDIR)/%.o,$(filter-out main.c,$(SRCS)))
MAIN_OBJ = $(BUILDDIR)/main.o

TARGET = $(BUILDDIR)/stockfish-api

# Pattern rule: build .o files in build/ from .c files in src/
$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(BUILDDIR)
	$(CC) -c $(CFLAGS) $< -o $@

all: $(TARGET)

$(TARGET): $(OBJS) $(MAIN_OBJ)
	@echo Compiling Stockfish API
	@mkdir -p $(BUILDDIR)
	rm -f $@
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(MAIN_OBJ) $(LIBS)

clean:
	rm -f $(OBJS) $(MAIN_OBJ) $(TARGET)
	rm -rf $(BUILDDIR)

.PHONY: all clean
