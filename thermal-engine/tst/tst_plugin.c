#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>

#include "plugin.h"

#ifndef PLUGIN_PATH
#define PLUGIN_PATH "../plugins"
#endif

int tst_plugin(const char *plugin_path)
{
 	struct plugin *plugin;

	/*
	 * Plugin does not exists, it must fails
	 */
	plugin = plugin_open(plugin_path, "itdoesnotexist", NULL, NULL);
	if (plugin)
		return -1;

	/*
	 * Plugin exists, must succeed
	 */
	plugin = plugin_open(plugin_path, "te-plugin-compat1", NULL, NULL);
	if (!plugin)
		return -1;
	plugin_close(plugin);

	/*
	 * Plugin exists, must succeed
	 */
	plugin = plugin_open(plugin_path, "te-plugin-compat2", NULL, NULL);
	if (!plugin)
		return -1;
	plugin_close(plugin);

	/*
	 * Plugin exists but not this profile, must fail
	 */
	plugin = plugin_open(plugin_path, "te-plugin-compat1", "power", NULL);
	if (plugin)
		return -1;
	plugin_close(plugin);

	/*
	 * Plugin exists with this profile, must succeed
	 */
	plugin = plugin_open(plugin_path, "te-plugin-compat1", "game", NULL);
	if (!plugin)
		return -1;
	plugin_close(plugin);

	/*
	 * Plugin exists but not this version, must fail
	 */
	plugin = plugin_open(plugin_path, "te-plugin-compat1", "game", "1234.1234.1234");
	if (plugin)
		return -1;
	plugin_close(plugin);

	/*
	 * Plugin exists with this version, must succeed
	 */
	plugin = plugin_open(plugin_path, "te-plugin-compat1", "game", "0.0.1");
	if (!plugin)
		return -1;
	plugin_close(plugin);

	/*
	 * Plugin exists with this version, must succeed
	 */
	plugin = plugin_open(plugin_path, "te-plugin-compat1", NULL, "0.0.1");
	if (!plugin)
		return -1;
	plugin_close(plugin);

	return 0;
}

int main(int argc, char *argv[])
{
	if (tst_plugin(argc < 2 ? PLUGIN_PATH : argv[1]))
		return 1;

	return 0;
}
