#include <stdio.h>
#include <stdlib.h>

#define LOG_PREFIX "te-plugin-db845c-browsing"
#include "log.h"
#include "threshold.h"
#include "plugin.h"

struct plugin_descriptor plugin_descriptor = {
	.version	= "0.0.1",
	.profile	= "browsing",
	.compatibles	= { "te-plugin-db845c", NULL },
};

static void *init_db845c(void)
{
	INFO("Initialized plugin compatible='%s', " \
	     "profile='%s', version='%s'\n",
	     plugin_descriptor.compatibles[0],
	     plugin_descriptor.profile,
	     plugin_descriptor.version);

	return malloc(sizeof(int));
}

static void exit_db845c(void *data)
{
	free(data);
}

static int trip_high_db845c(int tz_id, int temperature, void *data)
{
	DEBUG("Plugin trip high callback, tz_id=%d, temperature=%d\n",
	      tz_id, temperature);

	return 0;
}

static int trip_low_db845c(int tz_id, int temperature, void *data)
{
	DEBUG("Plugin trip low callback, tz_id=%d, temperature=%d\n",
	      tz_id, temperature);

	return 0;
}

static int reset_db845c(int tz_id, int temperature, void *data)
{
	DEBUG("Reset callback, tz_id=%d, temperature=%d\n",
	      tz_id, temperature);

	return 0;
}

struct plugin_ops plugin_ops = {
	.init = init_db845c,
	.exit = exit_db845c,
	.trip_low = trip_low_db845c,
	.trip_high = trip_high_db845c,
	.reset = reset_db845c,
};
