#ifndef __THERMAL_ENGINE_H__
#define __THERMAL_ENGINE_H__

struct thermal_zone;
struct power_handler;
struct thermal_handler;
struct performance_handler;
struct profile;
struct options;
struct config_t;
struct list;
struct thresholds;

struct thermal_engine_data {
	struct config_t *config;
	struct mainloop *ml;
	struct options *options;
	struct profile *profile;
	struct thermal_zone *tz;
	struct thermal_cdev *cdev;
	struct power_handler *pw;
	struct thermal_handler *th;
	struct performance_handler *ph;
	struct list *plugins;
	struct thresholds *thresholds;
};

int thermal_engine_options_init(int argc, char *argv[], struct thermal_engine_data *ted);
void thermal_engine_options_exit(struct thermal_engine_data *ted);
	
int thermal_engine_config_init(struct thermal_engine_data *ted);
void thermal_engine_config_exit(struct thermal_engine_data *ted);

int thermal_engine_threshold_init(struct thermal_engine_data *ted);
void thermal_engine_threshold_exit(struct thermal_engine_data *ted);

int thermal_engine_power_init(struct thermal_engine_data *ted);
void thermal_engine_power_exit(struct thermal_engine_data *ted);

int thermal_engine_thermal_init(struct thermal_engine_data *ted);
void thermal_engine_thermal_exit(struct thermal_engine_data *ted);

int thermal_engine_performance_init(struct thermal_engine_data *ted);
void thermal_engine_performance_exit(struct thermal_engine_data *ted);

int thermal_engine_plugins_init(struct thermal_engine_data *ted);
void thermal_engine_plugins_exit(struct thermal_engine_data *ted);

int thermal_engine_profile_init(struct thermal_engine_data *ted);
void thermal_engine_profile_exit(struct thermal_engine_data *ted);
#endif
