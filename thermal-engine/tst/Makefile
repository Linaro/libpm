SRCS = $(wildcard tst_*.c)
BINS = $(SRCS:.c=)

OBJS = $(wildcard ../src/*.o)

CFLAGS   =-g -Wall -Wno-unused
INCLUDES =-I../src

TOPDIR ?= ../..
LIBPATH  = $(TOPDIR)/lib
LIBPOWER_PATH = $(LIBPATH)/power/src
LIBTHERMAL_PATH = $(LIBPATH)/thermal/src
LIBPERFORMANCE_PATH = $(LIBPATH)/performance/src

RPATH  = -Wl,-rpath,$(LIBPOWER_PATH)
RPATH += -Wl,-rpath,$(LIBTHERMAL_PATH)
RPATH += -Wl,-rpath,$(LIBPERFORMANCE_PATH)

LDFLAGS  = -L$(LIBPOWER_PATH)
LDFLAGS += -L$(LIBTHERMAL_PATH)
LDFLAGS += -L$(LIBPERFORMANCE_PATH)
LDFLAGS += -lpower
LDFLAGS += -lthermal
LDFLAGS += -lperformance
LDFLAGS += -lconfig
LDFLAGS += $(RPATH)
LDFLAGS += -ldl
LDFLAGS += -rdynamic

all: $(BINS)

%: %.c $(OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@ $(OBJS) $(LDFLAGS)

check: $(BINS)
	@for i in $(BINS); do \
		echo -n "$$i: "; ./$$i; \
		if [ $$? != 0 ]; then echo "failed"; else echo "ok"; fi; \
	done

clean:
	rm -f $(BINS) *~
