#include <stdio.h>

#include <performance.h>

#include "thermal-engine.h"
#include "log.h"

static int for_each_performance_state_cb(int frequency, __maybe_unused void *data)
{
	DEBUG(" %d ", frequency);

	return 0;
}

static int for_each_performance_state(struct performance_handler *handler,
				      int id, void *data)
{
	if (performance_for_each_frequency(handler, id,
					   for_each_performance_state_cb, data))
		return -1;

	return 0;
}

static int for_each_device_cb(struct performance_handler *handler,
			      const char *device, void *data)
{
	int id;

	id = performance_get_device_id(handler, device);
	if (id < 0)
		return -1;

	DEBUG("Performance device '%s' [", device);

	if (for_each_performance_state(handler, id, data))
		return -1;

	DEBUG("]\n");
	
	return 0;
}

void thermal_engine_performance_exit(struct thermal_engine_data *ted)
{
	performance_destroy(ted->ph);
}

int thermal_engine_performance_init(struct thermal_engine_data *ted)
{
	ted->ph = performance_create();
	if (!ted->ph)
		return -1;

	if (performance_for_each_device(ted->ph, for_each_device_cb, ted))
		return -1;

	return 0;
}
