# SPDX-License-Identifier: LGPL-2.1+
CC=gcc
INCLUDES=-I../include -I/usr/include/libnl3
CFLAGS+=-g -Wall -Wno-unused -fPIC -Wextra -O2 $(INCLUDES)
LDFLAGS=-shared -lnl-3 -lnl-genl-3
DEPS=include/libthermal.h
OBJS=thermal.o thermal_nl.o commands.o events.o sampling.o
LIB=libthermal.so

BINS=$(C_BINS:.c=)

default: $(LIB)

%.o: %.c $(DEPS)
	$(CROSS_COMPILE)$(CC) -c -o $@ $< $(CFLAGS)

$(LIB): $(OBJS)
	$(CROSS_COMPILE)$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)

clean:
	rm -f $(OBJS) $(LIB) *~

.PHONY: check
