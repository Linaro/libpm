#include <stdio.h>
#include <stdlib.h>
#include <libconfig.h>
#include <thermal.h>

#include "thermal-engine.h"
#include "config.h"
#include "options.h"
#include "plugin.h"
#include "profile.h"
#include "threshold.h"
#include "log.h"
#include "fsm.h"

void thermal_engine_config_exit(struct thermal_engine_data *ted)
{
	config_destroy(ted->config);
	free(ted->config);
}

int config_plugins(struct thermal_engine_data *ted,
		   int (*cb)(struct list *list,
			     const char *path,
			     const char *compatible,
			     const char *profile,
			     const char *version))
{
	struct config_t *config = ted->config;
	config_setting_t *plugins;
	config_setting_t *descriptors;
	config_setting_t *descr;
	const char *path;
	int i;

	plugins = config_lookup(config, "plugins");
	if (!plugins) {
		WARN("No plugin defined in configuration");
		return 0;
	}

	if (!config_setting_lookup_string(plugins, "path", &path)) {
		WARN("No plugins path, using default: %s\n", PLUGIN_PATH);
		path = PLUGIN_PATH;
	}

	DEBUG("Loading plugins from '%s'\n", path);
	
	descriptors = config_setting_lookup(plugins, "descriptors");
	if (!descriptors) {
		ERROR("No plugin descriptor found\n");
		return -1;
	}

	for (i = 0; i < config_setting_length(descriptors); i++) {

		const char *compatible, *profile, *version;

		compatible = profile = version = NULL;

		descr = config_setting_get_elem(descriptors, i);
		config_setting_lookup_string(descr, "compatible", &compatible);
		config_setting_lookup_string(descr, "profile", &profile);
		config_setting_lookup_string(descr, "version", &version);

		DEBUG("Configuration plugin descriptor: compatible=%s", compatible);
		if (profile)
			DEBUG(", profile=%s", profile);
		if (version)
			DEBUG(", version=%s", version);
		DEBUG("\n");

		cb(ted->plugins, path, compatible, profile, version);
	}

	return 0;
}

static int config_profile_power(struct thresholds *thresholds,
				struct list *plugins,
				int tz_id, int temperature,
				config_setting_t *profile)
{
	struct plugin_power *pwr;
	config_setting_t *devices;
	const char *prof;
	const char *device;
	int power;
	int j;

	prof = config_setting_get_string_elem(profile, 0);
	power = config_setting_get_int_elem(profile, 1);

	DEBUG("Profile name='%s' sustains power='%d'", prof, power);

	pwr = plugin_power_alloc(power);
	if (!pwr)
		return -1;

	devices = config_setting_get_elem(profile, 2);
	if (!devices) {
		ERROR("Threshold power specified without device(s)\n");
		return -1;
	}

	for (j = 0; j < config_setting_length(devices); j++) {
		device = config_setting_get_string_elem(devices, j);

		if (plugin_power_add_device(pwr, device)) {
			ERROR("Failed to add device='%s' to plugin power\n", device);
			return -1;
		}

		DEBUG(", device=%s", device);
	}

	DEBUG("\n");

	if (threshold_add_action(thresholds, plugins, pwr, prof, tz_id, temperature)) {
		ERROR("Failed to add action on thermal zone id=%d for threshold=%d\n",
		      tz_id, temperature);
		return -1;
	}

	return 0;
}

static int config_threshold(struct thermal_engine_data *ted, int tz_id,
			    config_setting_t *threshold)
{
	config_setting_t *profiles;
	int temperature, hysteresis;
	int i;

	temperature = config_setting_get_int_elem(threshold, 0);
	hysteresis = config_setting_get_int_elem(threshold, 1);

	DEBUG("Found threshold with temperature=%d, hysteresis=%d\n",
	      temperature, hysteresis);

	if (threshold_add(ted->thresholds, tz_id, temperature, hysteresis)) {
		ERROR("Failed to add threshold temp=%d, hyst=%d\n",
		      temperature, hysteresis);
		return -1;
	}

	if (config_setting_length(threshold) == 2)
		return 0;

	profiles = config_setting_get_elem(threshold, 2);
	if (!profiles)
		return 0;

	for (i = 0; i < config_setting_length(profiles); i++) {

		config_setting_t *profile;

		profile = config_setting_get_elem(profiles, i);

		if (config_profile_power(ted->thresholds, ted->plugins, tz_id,
					 temperature, profile))
			return -1;
	}

	return 0;
}

static int config_thresholds(struct thermal_engine_data *ted, int tz_id,
			     config_setting_t *thresholds)
{
	config_setting_t *threshold;
	int i;

	if (threshold_add(ted->thresholds, tz_id, THRESHOLD_DEFAULT_TEMP, 0)) {
		ERROR("Failed to add initial state to fsm\n");
		return -1;
	}
	
	for (i = 0; i < config_setting_length(thresholds); i++) {

		threshold = config_setting_get_elem(thresholds, i);
		if (!threshold) {
			ERROR("Failed to read threshold\n");
			return -1;
		}

		if (config_threshold(ted, tz_id, threshold))
			return -1;
	}

	return 0;
}

int config_thermal_zone(struct thermal_engine_data *ted)
{
	config_setting_t *thermal_zones;
	config_setting_t *thresholds;
	int i;

	thermal_zones = config_lookup(ted->config, "thermal-zones");
	if (!thermal_zones) {
		ERROR("No thermal zones defined in configuration\n");
		return -1;
	}

	for (i = 0; i < config_setting_length(thermal_zones); i++) {

		config_setting_t *thermal_zone;
		const char *name, *type;
		struct thermal_zone *tz;
		name = type = NULL;

		thermal_zone = config_setting_get_elem(thermal_zones, i);
		config_setting_lookup_string(thermal_zone, "name", &name);
		config_setting_lookup_string(thermal_zone, "type", &type);

		tz = thermal_zone_find_by_name(ted->tz, name);
		if (!tz) {
			WARN("No thermal zone associated with configuration '%s'\n",
			     name);
			continue;
		}

		thresholds = config_setting_lookup(thermal_zone, "thresholds");
		if (!thresholds) {
			ERROR("No thresholds defined for thermal zone '%s'\n", name);
			return -1;
		}

		if (config_thresholds(ted, tz->id, thresholds)) {
			ERROR("Failed to configure the thresholds\n");
			return -1;
		}
		
		DEBUG("Found thermal zone name=%s, type=%s\n", name, type);
	}

	return 0;
}

int config_profile(struct thermal_engine_data *ted)
{
	config_setting_t *profile;
	const char *name;

	profile = config_lookup(ted->config, "profile");
	if (!profile) {
		WARN("No default profile specified specified in configuration\n");
		return 0;
	}

	if (!config_setting_lookup_string(profile, "name", &name)) {
		ERROR("Failed to get profile name\n");
		return -1;
	}

	if (profile_set_name(ted->profile, name)) {
		ERROR("Failed to set new profile name\n");
		return -1;
	}

	return 0;
}

int thermal_engine_config_init(struct thermal_engine_data *ted)
{
	ted->config = calloc(1, sizeof(*ted->config));
	if (!ted->config)
		return -1;

	config_init(ted->config);

	INFO("Reading configuration file '%s'\n", ted->options->config);

	if (!config_read_file(ted->config, ted->options->config)) {
		ERROR("Failed to read configuration file: %s - line %d\n",
		      config_error_text(ted->config),
		      config_error_line(ted->config));
		config_destroy(ted->config);
		return -1;
	}

	return 0;
}
