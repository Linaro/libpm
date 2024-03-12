#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "thermal-engine.h"
#include "config.h"
#include "log.h"
#include "profile.h"

#ifndef DEFAULT_PROFILE
#define DEFAULT_PROFILE "default"
#endif

struct profile {
	char *name;
};

const char *profile_get_name(struct profile *profile)
{
	return profile->name;
}

int profile_set_name(struct profile *profile, const char *name)
{
	char *n;

	if (!profile) {
		ERROR("Profile not allocated\n");
		return -1;
	}

	n = strdup(name);
	if (!n)
		return -1;

	free(profile->name);

	profile->name = n;

	DEBUG("Set new profile name='%s'\n", n);
	
	return 0;
}

int thermal_engine_profile_init(struct thermal_engine_data *ted)
{
	struct profile *profile;

	profile = calloc(1, sizeof(*profile));
	if (!profile)
		return -1;

	ted->profile = profile;

	if (config_profile(ted)) {
		ERROR("Failed to configure profile\n");
		free(profile);
	}

	if (!profile->name) {
		DEBUG("No profile name specified, setting default='%s'\n", DEFAULT_PROFILE);
		profile->name = strdup(DEFAULT_PROFILE);
		if (!profile->name) {
			free(profile);
			return -1;
		}
	}
	
	DEBUG("Profile initialized with name='%s'\n", profile->name);
	
	return 0;
}

void thermal_engine_profile_exit(struct thermal_engine_data *ted)
{
	struct profile *profile = ted->profile;

	free(profile->name);
	free(profile);
}
