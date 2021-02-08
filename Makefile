# simple Makefile
MAKEFLAGS += --silent

# compiler settings
CC       = gcc
CFLAGS  += -O3 -s -std=c99
CFLAGS  += -Wall -Wextra -Wshadow -Wstrict-aliasing -pedantic
AFLSTART = AFL_SKIP_CPUFREQ=1
AFLPATH := ../afl-2.57b

# input/output
INCLUDES = packagemerge.h moffat.h limitedjpegdeflate.h limitedbzip2.h limitedkraft.h limitedkraftheap.h
SRC      = packagemerge.c moffat.c limitedjpegdeflate.c limitedbzip2.c limitedkraft.c limitedkraftheap.c
TARGET   = benchmark
TARGET2  = histogram

# rules
.PHONY: default clean rebuild

default: $(TARGET) $(TARGET2)

# benchmark
$(TARGET): $(TARGET).c $(SRC) $(INCLUDES) Makefile
	$(CC) $(CFLAGS) $(TARGET).c $(SRC) -o $@

# histogram
$(TARGET2): $(TARGET2).c Makefile
	$(CC) $(CFLAGS) $(TARGET2).c -o $@

# fuzzing
fuzzer: fuzzer.c $(SRC) $(INCLUDES) Makefile
	$(AFLPATH)/afl-gcc fuzzer.c $(SRC) -o $@

run-fuzzer: fuzzer
	$(AFLSTART) $(AFLPATH)/afl-fuzz -i afl-testcases -o afl-findings -- ./fuzzer

# misc
clean:
	-rm -f $(TARGET) $(TARGET2)

rebuild: clean $(TARGET) $(TARGET2)
