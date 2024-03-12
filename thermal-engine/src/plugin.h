#ifndef __PLUGIN_H__
#define __PLUGIN_H__

#include "list.h"

struct thermal_engine_data;
struct plugin_power;

struct plugin_descriptor {
	const char *version;
	const char *profile;
	const char *compatibles[];
};

struct plugin_ops {
	void *(*init)();
	void (*exit)(void *);
	int (*trip_high)(int tz_id, int temperature, void *data);
	int (*trip_low)(int tz_id, int temperature, void *data);
	int (*reset)(int tz_id, int temperature, void *data);
};

struct plugin {
	void *handle;
	void *private; /* pointer returned from the init function */
	struct plugin_descriptor *descriptor;
	struct plugin_ops *ops;
	struct list list;
};

struct plugin *plugin_open(const char *path, const char *compatible,
			   const char *profile, const char *version);

void plugin_close(struct plugin *plugin);
int plugin_trip_high(struct plugin *plugin, int tz_id, int temperature, void *data);
int plugin_trip_low(struct plugin *plugin, int tz_id, int temperature, void *data);
int plugin_reset(struct plugin *plugin, int tz_id, int temperature, void *data);

struct plugin_power *plugin_power_alloc(unsigned int power);
void plugin_power_free(struct plugin_power *pwr);

int plugin_power_add_device(struct plugin_power *pwr, const char *device);

int plugin_profile_for_each(struct list *plugins, const char *profile,
			    int (*cb)(struct plugin *, void *data), void *data);
#endif
