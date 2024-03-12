CFLAGS =-g -Wall -Wextra

TOPDIR ?= $(PWD)/../..
LIBPATH  = $(TOPDIR)/lib
LIBTHERMAL_PATH = $(LIBPATH)/thermal/src
LIBPERFORMANCE_PATH = $(LIBPATH)/performance/src
LIBPOWER_PATH = $(LIBPATH)/power/src

RPATH  = -Wl,-rpath,$(LIBTHERMAL_PATH)
RPATH += -Wl,-rpath,$(LIBPERFORMANCE_PATH)
RPATH += -Wl,-rpath,$(LIBPOWER_PATH)

LDFLAGS  = -L$(LIBTHERMAL_PATH)
LDFLAGS += -L$(LIBPERFORMANCE_PATH)
LDFLAGS += -L$(LIBPOWER_PATH)
LDFLAGS += -lthermal
LDFLAGS += -lperformance
LDFLAGS += -lpower
LDFLAGS += -lconfig
LDFLAGS += $(RPATH)

# plugin specific flags to let them access symbols in the main program
LDFLAGS += -ldl
LDFLAGS += -rdynamic

INCLUDES  =-I$(LIBPATH)/thermal/include
INCLUDES +=-I$(LIBPATH)/performance/include
INCLUDES +=-I$(LIBPATH)/power/include

OBJS = mainloop.o log.o timestamp.o list.o pair.o cb_chain.o fsm.o plugin.o power.o thermal.o threshold.o profile.o performance.o config.o options.o

DEPS  = $(LIBPATH)/thermal/include/thermal.h
DEPS += $(LIBPATH)/thermal/src/libthermal.so

DEPS += $(LIBPATH)/performance/include/performance.h
DEPS += $(LIBPATH)/performance/src/libperformance.so

DEPS += $(LIBPATH)/power/include/power.h
DEPS += $(LIBPATH)/power/src/libpower.so

CONFIG ?= $(TOPDIR)/thermal-engine/src/te.conf
PLUGIN_PATH ?= $(TOPDIR)/thermal-engine/plugins
VERSION = 0.0.1

CBIN = thermal-engine.c
BIN = $(CBIN:.c=)

$(BIN): $(CBIN) $(OBJS)
	$(CC) $(CFLAGS) -DVERSION=\"$(VERSION)\" -DPLUGIN_PATH=\"$(PLUGIN_PATH)\" \
		-DCONFIG=\"$(CONFIG)\" $(INCLUDES) -o $@ $^ $(LDFLAGS)

%.o: %.[ch]
	$(CC) -c -o $@ $< $(CFLAGS) $(INCLUDES)\
		-DVERSION=\"$(VERSION)\" -DPLUGIN_PATH=\"$(PLUGIN_PATH)\" \
		-DCONFIG=\"$(CONFIG)\"

%.so: %.c
	$(CROSS_COMPILE)$(CC) $(CFLAGS) $(INCLUDES) $< -o $@ $(LDFLAGS)

clean:
	$(RM) $(OBJS) $(BIN) *~

.PHONY: check
