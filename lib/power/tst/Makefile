# SPDX-License-Identifier: LGPL-2.1+
CC=gcc
DEPS = ../include/power.h
LIB=../src/libpower.so
C_BINS=tst_power.c
CFLAGS=-g -Wall -Wno-unused

INCLUDES=-I../include

BINS=$(C_BINS:.c=)

$(BINS): $(C_BINS)
	$(CROSS_COMPILE)$(CC) $(CFLAGS) $< -o $@ -lpower -L../src -Wl,-rpath=../src $(INCLUDES)

clean:
	rm -f $(BINS) *~

check: $(BINS)
	sudo ./tst_power
