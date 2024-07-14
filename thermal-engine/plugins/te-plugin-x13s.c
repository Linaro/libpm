#include <stdio.h>
#include <stdlib.h>

#define LOG_PREFIX "te-plugin-x13s-balanced"
#include "log.h"
#include "threshold.h"
#include "plugin.h"

struct plugin_descriptor plugin_descriptor = {
	.version	= "0.0.1",
	.profile	= "balanced",
	.compatibles	= { "te-plugin-x13s", NULL },
};

static void *init_x13s(void)
{
	INFO("Initialized plugin compatible='%s', " \
	     "profile='%s', version='%s'\n",
	     plugin_descriptor.compatibles[0],
	     plugin_descriptor.profile,
	     plugin_descriptor.version);

	return malloc(sizeof(int));
}

static void exit_x13s(void *data)
{
	free(data);
}

static int trip_high_x13s(int tz_id, int temperature, void *data)
{
	DEBUG("Plugin trip high callback, tz_id=%d, temperature=%d\n",
	      tz_id, temperature);

	return 0;
}

static int trip_low_x13s(int tz_id, int temperature, void *data)
{
	DEBUG("Plugin trip low callback, tz_id=%d, temperature=%d\n",
	      tz_id, temperature);

	return 0;
}

static int reset_x13s(int tz_id, int temperature, void *data)
{
	DEBUG("Reset callback, tz_id=%d, temperature=%d\n",
	      tz_id, temperature);

	return 0;
}

struct plugin_ops plugin_ops = {
	.init = init_x13s,
	.exit = exit_x13s,
	.trip_low = trip_low_x13s,
	.trip_high = trip_high_x13s,
	.reset = reset_x13s,
};
