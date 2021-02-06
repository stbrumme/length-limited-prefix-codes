# simple Makefile
MAKEFLAGS += --silent

# compiler settings
CC       = gcc
CFLAGS   = -O3 -s
CFLAGS  += -Wall -Wextra -Wshadow -Wstrict-aliasing -pedantic

# input/output
INCLUDES = packagemerge.h moffat.h limitedjpegdeflate.h limitedbzip2.h limitedkraft.h limitedkraftheap.h
SRC      = packagemerge.c moffat.c limitedjpegdeflate.c limitedbzip2.c limitedkraft.c limitedkraftheap.c
MAIN     = benchmark.c
TARGET   = benchmark
MAIN2    = histogram.c
TARGET2  = histogram

# rules
.PHONY: default clean rebuild

default: $(TARGET) $(TARGET2)

$(TARGET): $(SRC) $(MAIN) $(INCLUDES) Makefile
	$(CC) $(CFLAGS) $(SRC) $(MAIN) -o $@

$(TARGET2): $(MAIN2) Makefile
	$(CC) $(CFLAGS) $(MAIN2) -o $@

clean:
	-rm -f $(TARGET) $(TARGET2)

rebuild: clean $(TARGET) $(TARGET2)
