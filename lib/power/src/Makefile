# SPDX-License-Identifier: LGPL-2.1+
CC=gcc
CFLAGS+=-g -Wall -Wno-unused -I../include -fPIC -Wextra -O2
LDFLAGS=-shared
DEPS = ../include/power.h
OBJS = power.o
LIB=libpower.so

BINS=$(C_BINS:.c=)

default: $(LIB)

%.o: %.c $(DEPS)
	$(CROSS_COMPILE)$(CC) -c -o $@ $< $(CFLAGS)

$(LIB): $(OBJS)
	$(CROSS_COMPILE)$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)

clean:
	rm -f $(OBJS) $(LIB) *~

.PHONY: check
