#ifndef __CONFIG_THERMAL_ENGINE_H
#define __CONFIG_THERMAL_ENGINE_H

struct config_t;
struct plugin;
struct profile;

int config_plugins(struct thermal_engine_data *ted,
		   int (*cb)(struct list *list,
			     const char *path,
			     const char *compatible,
			     const char *profile,
			     const char *version));

int config_profile(struct thermal_engine_data *ted);

int config_thermal_zone(struct thermal_engine_data *ted);

#endif
