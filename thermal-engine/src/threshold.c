#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include "thermal-engine.h"
#include "threshold.h"
#include "config.h"
#include "pair.h"
#include "plugin.h"
#include "log.h"

struct plugin_list {
	struct list list;
	struct plugin *plugin;
};

struct threshold {
	struct plugin_power *power;
	struct list plugins;
	int temperature;
	int tz_id;
};

struct thresholds {
	int crossed;
	struct pair threshold;
};

static int threshold_id_encode(int tz_id, int temperature)
{
	/*
	 * Assumption is there won't be more than 512 thermal zones so
	 * we can encode the id on 9 bits for the thermal zone id and
	 * 23 bits for the trip point temperature.
	 * That allows:
	 *  9 bits : 512 thermal zones
	 * 23 bits : 8388°C max temperature
	 */
	return tz_id << 23 | temperature;
}	

static struct threshold *threshold_find(struct thresholds *thresholds,
					int tz_id, int temperature)
{
	int id = threshold_id_encode(tz_id, temperature);

	return pair_find(&thresholds->threshold, id);
}

int threshold_for_each_plugin(struct threshold *threshold,
			      int (*cb)(struct plugin *plugin, struct threshold *threshold))
{
	struct list *l;
	int ret = 0;

	l = list_next(&threshold->plugins);
	while (l) {
		struct plugin_list *pl;
		struct plugin *p;

		pl = container_of(l, struct plugin_list, list);
		p = pl->plugin;

		ret |= cb(p, threshold);
		
		l = list_next(l);
	}

	return ret;
}

static int threshold_crossed(struct thresholds *thresholds, int tz_id, int temperature,
			     int (*cb)(struct plugin *plugin, struct threshold *threshold))
{
	struct threshold *threshold;

	threshold = threshold_find(thresholds, tz_id, temperature);
	if (!threshold)
		return 0;
	
	return threshold_for_each_plugin(threshold, cb);
}

static int __trip_crossed_up(struct plugin *plugin, struct threshold *threshold)
{
	DEBUG("Doing plugin action %s with profile=%s\n",
	      plugin->descriptor->compatibles[0], plugin->descriptor->profile);

	if (plugin_trip_high(plugin, threshold->tz_id, threshold->temperature, NULL))
		WARN("Plugin callback failed\n");

	return 0;
}

static int __trip_crossed_down(struct plugin *plugin, struct threshold *threshold)
{
	DEBUG("Doing plugin action %s with profile=%s\n",
	      plugin->descriptor->compatibles[0], plugin->descriptor->profile);

	if (plugin_trip_low(plugin, threshold->tz_id, threshold->temperature, NULL))
		WARN("Plugin callback failed\n");
		
	return 0;
}

int trip_crossed_up(struct thresholds *thresholds, int tz_id, int temperature)
{
	DEBUG("Detected threshold crossed the way up event, "
	     "tz_id=%d, temperature=%d\n", tz_id, temperature);

	thresholds->crossed++;

	return threshold_crossed(thresholds, tz_id, temperature, __trip_crossed_up);
}

int trip_crossed_down(struct thresholds *thresholds, int tz_id, int temperature)
{
	DEBUG("Detected threshold crossed the way down event, "
	      "tz_id=%d, temperature=%d\n", tz_id, temperature);

	thresholds->crossed--;
	
	return threshold_crossed(thresholds, tz_id, temperature, __trip_crossed_down);
}

static int __threshold_add_action(struct plugin *plugin, void *data)
{
	struct threshold *threshold = data;
	struct plugin_list *pl;

	DEBUG("Added action on tz_id=%d with plugin='%s' on profile='%s'\n",
	      threshold->tz_id, plugin->descriptor->compatibles[0],
	      plugin->descriptor->profile);

	pl = malloc(sizeof(*pl));
	if (!pl)
		return -1;

	list_init(&pl->list);
	pl->plugin = plugin;

	list_add_tail(&threshold->plugins, &pl->list);

	return 0;
}

int threshold_add_action(struct thresholds *thresholds, struct list *plugins,
			 struct plugin_power *power, const char *profile,
			 int tz_id, int temperature)
{
	struct threshold *threshold;

	DEBUG("Adding all actions for profile='%s' on temperature=%d for thermal"
	      " zone id=%d\n", profile, temperature, tz_id);

	threshold = threshold_find(thresholds, tz_id, temperature);
	if (!threshold) {
		ERROR("Failed to add action, threshold not found tz_id=%d, "
		      "temperature=%d\n", tz_id, temperature);
		return 0;
	}

	threshold->power = power;

	return plugin_profile_for_each(plugins, profile,
				       __threshold_add_action, threshold);
}

int threshold_add(struct thresholds *thresholds, int tz_id, int temperature)
{
	int id = threshold_id_encode(tz_id, temperature);
	struct threshold *threshold;

	if (!thresholds) {
		ERROR("Thresholds not initialized\n");
		return -1;
	}

	threshold = malloc(sizeof(*threshold));
	if (!threshold)
		return -1;

	list_init(&threshold->plugins);
	threshold->temperature = temperature;
	threshold->tz_id = tz_id;

	if (pair_add(&thresholds->threshold, id, threshold)) {
		ERROR("Failed to add threshold\n");
		return -1;
	}

	DEBUG("Added threshold id=%d\n", id);

	return 0;
}

int thermal_engine_threshold_init(struct thermal_engine_data *ted)
{
	struct thresholds *thresholds;

	if (!ted)
		return -1;
	
	thresholds = malloc(sizeof(*thresholds));
	if (!thresholds)
		return -1;

	pair_init(&thresholds->threshold);
	thresholds->crossed = 0;

	ted->thresholds = thresholds;

	if (config_thermal_zone(ted)) {
		ERROR("Failed to configure the thermal zones");
		return -1;
	}
	
	return 0;
}

void thermal_engine_threshold_exit(struct thermal_engine_data *ted)
{
	free(ted->thresholds);
}
