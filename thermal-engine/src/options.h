#ifndef __THERMAL_ENGINE_OPTIONS_H
#define __THERMAL_ENGINE_OPTIONS_H
struct options {
	const char *config;
	int loglevel;
	int logopt;
	int interactive;
	int daemonize;
};
#endif
