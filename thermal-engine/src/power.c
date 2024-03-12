#include <stdio.h>
#include <power.h>

#include "thermal-engine.h"
#include "log.h"

static int each_dtpm(const char *name, __maybe_unused void *data)
{
	DEBUG("Found power capable name='%s'\n", name);

	return 0;
}

int thermal_engine_power_init(struct thermal_engine_data *ted)
{
	ted->pw = power_create();
	if (!ted->pw)
		return -1;

	power_for_each(ted->pw, each_dtpm, NULL);
	
	return 0;
}

void thermal_engine_power_exit(struct thermal_engine_data *ted)
{
	power_destroy(ted->pw);
}
