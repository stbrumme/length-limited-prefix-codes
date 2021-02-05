# simple Makefile
MAKEFLAGS += --silent

# compiler settings
CC = gcc
CFLAGS = -O3 -Wall -s

# input/output
INCLUDES = packagemerge.h moffat.h limitedjpegdeflate.h limitedbzip2.h limitedkraft.h limitedkraftheap.h
SRC      = packagemerge.c moffat.c limitedjpegdeflate.c limitedbzip2.c limitedkraft.c limitedkraftheap.c
LIBS     =
MAIN     = benchmark.c
TARGET   = benchmark

# rules
.PHONY: default clean rebuild

default: $(TARGET)

$(TARGET): $(SRC) $(MAIN) $(INCLUDES)
	$(CC) $(CFLAGS) $(SRC) $(MAIN) $(LIBS) -o $@

clean:
	-rm -f $(TARGET)

rebuild: clean $(TARGET)
