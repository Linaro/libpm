SRCS = $(wildcard *.c)
PLUGINS = $(SRCS:.c=.so)

#OBJS = $(wildcard ../src/*.o)
OBJS = ""

CFLAGS +=-g -Wall -Wno-unused -fPIC -Wextra -O2
INCLUDES = -I../src
LDFLAGS = -shared

all: $(PLUGINS)

%.so: %.c
	$(CROSS_COMPILE)$(CC) $(CFLAGS) $(INCLUDES) $< -o $@ $(LDFLAGS)

clean:
	rm -f $(PLUGINS) *~

.PHONY: check
