C_BINS=tst_thermal.c

CFLAGS=-Wall -Wno-unused

LDFLAGS  = -L../src -Wl,-rpath=../src
LDFLAGS += -lthermal
LDFLAGS += -lnl-genl-3 -lnl-3

INCLUDES=-I../include

DEPS  =../include/thermal.h
DEPS +=../src/libthermal.so

BINS = $(C_BINS:.c=)

tests: $(DEPS) $(BINS)

$(BINS): $(C_BINS)
	$(CC) $(CFLAGS) $(INCLUDES) $< $(LDFLAGS) -o $@

clean:
	rm -f $(BINS) *~

check: $(BINS)
	./tst_thermal
