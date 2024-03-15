#define _GNU_SOURCE
#include <dirent.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <regex.h>

#include "log.h"
#include "thermal-engine.h"
#include "pair.h"
#include "plugin.h"
#include "config.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(__array) (sizeof(__array)/sizeof(__array[0]))
#endif

#ifndef PLUGIN_PATH
#define PLUGIN_PATH "../plugins"
#endif

struct plugin_device {
	const char *device;
	struct list list;
};

struct plugin_power {
	unsigned int power;
	struct list devices;
};

static int plugin_init(struct plugin *plugin, void *handle)
{
	plugin->handle = handle;

	plugin->ops = (typeof(plugin->ops))dlsym(handle, "plugin_ops");
	if (!plugin->ops) {
		ERROR("Failed to find symbol 'plugin_ops'\n");
		return -1;
	}

	plugin->private = plugin->ops->init();

	return 0;
}

static int plugin_match(struct plugin_descriptor *pd, const char *compatible,
			const char *profile, const char *version)
{
	int i;

	for (i = 0; pd->compatibles[i]; i++) {

		DEBUG("Checking compatible='%s', profile='%s', version='%s' vs\n",
		      compatible, profile, version);

		DEBUG("         compatible='%s', profile='%s', version='%s'\n",
		      pd->compatibles[i], pd->profile, pd->version);
		
		if (strcmp(pd->compatibles[i], compatible))
			continue;

		if (profile && strcmp(pd->profile, profile))
			continue;

		if (version && strcmp(pd->version, version))
			continue;

		return 0;
	}

	return -1;
}

static int plugin_descriptor_load(struct plugin *plugin, void *handle)
{
	plugin->descriptor = (typeof(plugin->descriptor))dlsym(handle, "plugin_descriptor");

	if (!plugin->descriptor) {
		ERROR("Failed to find symbol 'plugin_descriptor'\n");
		return -1;
	}
	
	if (!plugin->descriptor->profile)
		NOTICE("No plugin profile specified\n");

	if (!plugin->descriptor->version)
		NOTICE("No plugin version specified\n");

	return 0;
}

static struct plugin *plugin_load(const char *lib, const char *compatible,
				  const char *profile, const char *version)
{
	struct plugin *plugin;
	void *handle;

	handle = dlopen(lib, RTLD_NOW);
	if (!handle) {
		ERROR("Failed to dlopen: %s\n", dlerror());
		return NULL;
	}

	plugin = malloc(sizeof(*plugin));
	if (!plugin)
		goto out_dlclose;

	if (plugin_descriptor_load(plugin, handle)) {
		ERROR("Failed to load plugin descriptor\n");
		goto out_free_plugin;
	}
	
	if (plugin_match(plugin->descriptor, compatible, profile, version)) {
		DEBUG("Plugin '%s' does not match\n", compatible);
		goto out_free_plugin;
	}

	if (plugin_init(plugin, handle)) {
		ERROR("Failed to initialize the plugin\n");
		goto out_free_plugin;
	}

	INFO("Loaded plugin compatible='%s', profile='%s', version='%s'\n",
	     compatible, profile, version);
	
	return plugin;

out_free_plugin:
	free(plugin);
out_dlclose:
	dlclose(handle);

	return NULL;
}

struct plugin_power *plugin_power_alloc(unsigned int power)
{
	struct plugin_power *pwr;

	pwr = calloc(1, sizeof(*pwr));
	if (!pwr)
		return NULL;

	pwr->power = power;
	
	list_init(&pwr->devices);

	return pwr;
}

void plugin_power_free(struct plugin_power *pwr)
{
	free(pwr);
}

int plugin_power_add_device(struct plugin_power *pwr, const char *device)
{
	struct plugin_device *dev;

	dev = malloc(sizeof(*dev));
	if (!dev)
		return -1;

	dev->device = strdup(device);
	if (!dev->device)
		return -1;

	list_add_tail(&pwr->devices, &dev->list);

	return 0;
}

int plugin_power_limit(struct plugin_power *pwr)
{
	if (!pwr) {
		ERROR("Invalid plugin power pointer\n");
		return -1;
	}

	return pwr->power;
}

struct plugin *plugin_open(const char *path, const char *compatible,
			   const char *profile, const char *version)
{
	struct plugin *plugin = NULL;
	struct dirent *dirent;
	regex_t regex;
	DIR *dir;

	if (!compatible) {
		ERROR("Plugin 'compatible' is missing\n");
		return NULL;
	}
	
	dir = opendir(path);
	if (!dir) {
		ERROR("Failed to open plugin directory '%s'\n", path);
		return NULL;
	}

	if (regcomp(&regex, ".*\\.so$", REG_NOSUB))
		return NULL;
	
	while ((dirent = readdir(dir))) {

		char *abspath;
		
		if (regexec(&regex, dirent->d_name, 0, NULL, 0) == REG_NOMATCH)
			continue;

		if (asprintf(&abspath, "%s/%s", path, dirent->d_name) == -1)
			break;

		plugin = plugin_load(abspath, compatible, profile, version);

		free(abspath);
		
		if (plugin)
			break;
	}

	regfree(&regex);

	closedir(dir);

	return plugin;
}

void plugin_close(struct plugin *plugin)
{
	if (!plugin)
		return;

	plugin->ops->exit(plugin->private);
	dlclose(plugin->handle);
	free(plugin);
}

typedef enum {
	TRIP_LOW,
	TRIP_HIGH,
	RESET,
} plugin_ops_t;

static int plugin_ops(struct plugin *plugin, int tz_id, int temperature,
		      void *data, plugin_ops_t ops_t)
{
	struct plugin_ops *ops;

	if (!plugin)
		return -1;

	ops = plugin->ops;

	switch (ops_t) {
	case TRIP_LOW:
		return ops->trip_low(tz_id, temperature, data);
	case TRIP_HIGH:
		return ops->trip_high(tz_id, temperature, data);
	case RESET:
		return ops->reset(tz_id, temperature, data);
	}

	return -1;
}

int plugin_trip_low(struct plugin *plugin, int tz_id, int temperature, void *data)
{
	return plugin_ops(plugin, tz_id, temperature, data, TRIP_LOW);
}

int plugin_trip_high(struct plugin *plugin, int tz_id, int temperature, void *data)
{
	return plugin_ops(plugin, tz_id, temperature, data, TRIP_HIGH);
}

int plugin_reset(struct plugin *plugin, int tz_id, int temperature, void *data)
{
	return plugin_ops(plugin, tz_id, temperature, data, RESET);
}

static int plugin_add(struct list *plugins,
		      const char *path, const char *compatible,
		      const char *profile, const char *version)
{
	struct plugin *plugin;

	plugin = plugin_open(path, compatible, profile, version);
	if (!plugin)
		return -1;

	list_add_tail(plugins, &plugin->list);

	return 0;
}

static int plugin_show(struct plugin *plugin, __maybe_unused void *data)
{
	DEBUG("Plug '%s' loaded\n", plugin->descriptor->compatibles[0]);

	return 0;
}

static int plugin_for_each(struct list *plugins, int (*cb)(struct plugin *, void *),
			   void *data)
{
	struct plugin *plugin;

	plugins = list_next(plugins);

	while (plugins) {
		plugin = container_of(plugins, struct plugin, list);
		cb(plugin, data);

		plugins = list_next(plugins);
	}

	return 0;
}

int plugin_profile_for_each(struct list *plugins, const char *profile,
			    int (*cb)(struct plugin *, void *data), void *data)
{
	struct plugin *plugin;

	plugins = list_next(plugins);

	while (plugins) {
		plugin = container_of(plugins, struct plugin, list);

		if (!strcmp(plugin->descriptor->profile, profile))
			cb(plugin, data);

		plugins = list_next(plugins);
	}

	return 0;
}

void thermal_engine_plugins_exit(struct thermal_engine_data *ted)
{
	free(ted->plugins);
}
	
int thermal_engine_plugins_init(struct thermal_engine_data *ted)
{
	ted->plugins = malloc(sizeof(*ted->plugins));
	if (!ted->plugins)
		return -1;

	list_init(ted->plugins);
	
	if (config_plugins(ted, plugin_add))
		return -1;

	plugin_for_each(ted->plugins, plugin_show, ted);

	return 0;
}
