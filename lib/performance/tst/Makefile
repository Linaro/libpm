# SPDX-License-Identifier: LGPL-2.1+
CC=gcc
DEPS = ../include/libperformance.h
LIB=../src/libperformance.so
C_BINS=tst_performance.c
CFLAGS=-g -Wall -Wno-unused

INCLUDES=-I../include

BINS=$(C_BINS:.c=)

tests: $(LIB) $(BINS)

$(BINS): $(C_BINS)
	$(CROSS_COMPILE)$(CC) $(CFLAGS) $< -o $@ -lperformance -L../src -Wl,-rpath=../src $(INCLUDES)

clean:
	rm -f $(BINS) *~

check: $(BINS)
	sudo ./tst_performance
