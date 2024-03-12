/*
 * Thermal engine
 *
 * Copyright (C) 2023 Linaro Ltd.
 *
 * Author: Daniel Lezcano <daniel.lezcano@linaro.org>
 */
#define _GNU_SOURCE
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <syslog.h>
#include <sys/signalfd.h>

#include "thermal-engine.h"
#include "log.h"
#include "options.h"
#include "mainloop.h"
#include "options.h"

#define THERMAL_ENGINE_VERSION "Thermal Engine version: " VERSION ", "
#define THERMAL_ENGINE_COPYRIGHT "Copyright Linaro Ltd (2023)"

#define THERMAL_ENGINE_BANNER			     \
	THERMAL_ENGINE_VERSION			     \
	THERMAL_ENGINE_COPYRIGHT		     \

static void banner(void)
{
	char *line;
	size_t len;
	const char* sep =
		"------------------------------------------------------------------------";
	
	len = asprintf(&line, "| %s |", THERMAL_ENGINE_BANNER);
	
	INFO("%.*s\n", len, sep);
	INFO("%s\n", line);
	INFO("%.*s\n", len, sep);

	free(line);
}

static void thermal_engine_exit(int rc, void *arg)
{
	struct thermal_engine_data *ted = (typeof(ted))arg;

	INFO("Thermal engine exiting with error code=%d\n", rc);

	thermal_engine_options_exit(ted);
	thermal_engine_config_exit(ted);
	thermal_engine_threshold_exit(ted);
	thermal_engine_plugins_exit(ted);
	thermal_engine_profile_exit(ted);
	thermal_engine_power_exit(ted);
	thermal_engine_thermal_exit(ted);
	thermal_engine_performance_exit(ted);
	mainloop_fini(ted->ml);
	log_exit();
}

static int thermal_engine_signal_handler(int fd, void *data)
{
	struct thermal_engine_data *ted = data;
	struct signalfd_siginfo si;

	if (read(fd , &si, sizeof(si)) != sizeof(si))
		return -1;

	if (si.ssi_signo == SIGINT ||
	    si.ssi_signo == SIGTERM) {
		mainloop_del(ted->ml, fd);
		mainloop_exit(ted->ml);
	}

	return 0;
}

static int thermal_engine_signal_init(struct thermal_engine_data *ted)
{
	int fd;
	sigset_t mask;

	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	sigaddset(&mask, SIGQUIT);

	if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1)
		return -1;

	fd = signalfd(-1, &mask, 0);
	if (fd < 0)
		return -1;

	if (mainloop_add(ted->ml, fd, thermal_engine_signal_handler, ted))
		return -1;

	return 0;
}


enum {
	THERMAL_ENGINE_SUCCESS = 0,
	THERMAL_ENGINE_MEMORY_ERROR,
	THERMAL_ENGINE_OPTION_ERROR,
	THERMAL_ENGINE_DAEMON_ERROR,
	THERMAL_ENGINE_LOG_ERROR,
	THERMAL_ENGINE_PROFILE_ERROR,
	THERMAL_ENGINE_CONFIG_ERROR,
	THERMAL_ENGINE_THERMAL_ERROR,
	THERMAL_ENGINE_PERFORMANCE_ERROR,
	THERMAL_ENGINE_PLUGINS_ERROR,
	THERMAL_ENGINE_THRESHOLD_ERROR,
	THERMAL_ENGINE_MAINLOOP_ERROR,
	THERMAL_ENGINE_SYSTEM_ERROR,
};

int main(int argc, char *argv[])
{
	struct thermal_engine_data *ted;

	ted = malloc(sizeof(*ted));
	if (!ted)
		return THERMAL_ENGINE_MEMORY_ERROR;

	memset(ted, 0, sizeof(*ted));
	
	if (thermal_engine_options_init(argc, argv, ted)) {
		ERROR("Failed to initialize the options\n");
		return THERMAL_ENGINE_OPTION_ERROR;
	}

	if (ted->options->daemonize && daemon(0, 0)) {
		ERROR("Failed to daemonize: %p\n");
		return THERMAL_ENGINE_DAEMON_ERROR;
	}

	if (log_init(ted->options->loglevel, basename(argv[0]), ted->options->logopt)) {
		ERROR("Failed to initialize logging facility\n");
		return THERMAL_ENGINE_LOG_ERROR;
	}

	banner();
	
	ted->ml = mainloop_init();
	if (!ted->ml) {
		ERROR("Failed to initialize the mainloop\n");
		return THERMAL_ENGINE_MAINLOOP_ERROR;
	}

	if (thermal_engine_power_init(ted)) {
		ERROR("Failed to initialize the power library");
		return THERMAL_ENGINE_THERMAL_ERROR;
	}

	if (thermal_engine_thermal_init(ted)) {
		ERROR("Failed to initialize the thermal library\n");
		return THERMAL_ENGINE_THERMAL_ERROR;
	}

	if (thermal_engine_performance_init(ted)) {
		ERROR("Failed to initialize the performance library\n");
		return THERMAL_ENGINE_PERFORMANCE_ERROR;
	}

	if (thermal_engine_config_init(ted)) {
		ERROR("Failed to initialize the configuration\n");
		return THERMAL_ENGINE_CONFIG_ERROR;
	}

	if (thermal_engine_profile_init(ted)) {
		ERROR("Failed to initialize the profile\n");
		return THERMAL_ENGINE_PROFILE_ERROR;
	};

	if (thermal_engine_plugins_init(ted)) {
		ERROR("Failed to initialize the plugins\n");
		return THERMAL_ENGINE_PLUGINS_ERROR;
	}

	if (thermal_engine_threshold_init(ted)) {
		ERROR("Failed to initialize the thresholds\n");
		return THERMAL_ENGINE_THRESHOLD_ERROR;
	}

	if (thermal_engine_signal_init(ted)) {
		ERROR("Failed to configure signal handlers: %p\n");
		return THERMAL_ENGINE_SYSTEM_ERROR;
	}

	INFO("Thermal engine successfuly initialized\n");

	if (on_exit(thermal_engine_exit, ted)) {
		ERROR("Failed to set on_exit callback\n");
		return THERMAL_ENGINE_SYSTEM_ERROR;
	}
	
	if (mainloop(ted->ml, -1)) {
		ERROR("Mainloop failed\n");
		return THERMAL_ENGINE_MAINLOOP_ERROR;
	}

	return THERMAL_ENGINE_SUCCESS;
}
