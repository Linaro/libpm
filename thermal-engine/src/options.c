#include <getopt.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "thermal-engine.h"
#include "log.h"
#include "options.h"

static void usage(const char *cmd)
{
	printf("%s : A thermal monitoring engine based on notifications\n", cmd);
	printf("Usage: %s [options]\n", cmd);
	printf("\t-h, --help\t\tthis help\n");
	printf("\t-d, --daemonize\n");
	printf("\t-l <level>, --loglevel <level>\tlog level: ");
	printf("DEBUG, INFO, NOTICE, WARN, ERROR\n");
	printf("\t-c <config_file>, --config <config_file\n");
	printf("\t-s, --syslog\t\toutput to syslog\n");
	printf("\n");
	exit(0);
}

void thermal_engine_options_exit(struct thermal_engine_data *ted)
{
	free(ted->options);
}

int thermal_engine_options_init(int argc, char *argv[], struct thermal_engine_data *ted)
{
	int opt;

	struct options *options;
	struct option long_options[] = {
		{ "help",	no_argument, NULL, 'h' },
		{ "daemonize",	no_argument, NULL, 'd' },
		{ "syslog",	no_argument, NULL, 's' },
		{ "loglevel",	required_argument, NULL, 'l' },
		{ "config",	required_argument, NULL, 'c' },
		{ 0, 0, 0, 0 }
	};

	options = malloc(sizeof(*options));
	if (!options)
		return -1;

	memset(options, 0, sizeof(*options));

	options->config = CONFIG;
	options->loglevel = LOG_INFO;
	options->logopt = TO_STDOUT;

	ted->options = options;

	while (1) {

		int optindex = 0;

		opt = getopt_long(argc, argv, "c:l:dhs", long_options, &optindex);
		if (opt == -1)
			break;

		switch (opt) {
		case 'c':
			options->config = optarg;
			break;
		case 'l':
			options->loglevel = log_str2level(optarg);
			break;
		case 'd':
			options->daemonize = 1;
			break;
		case 's':
			options->logopt = TO_SYSLOG;
			break;
		case 'h':
			usage(basename(argv[0]));
			break;
		default: /* '?' */
			usage(basename(argv[0]));
			return -1;
		}
	}

	return 0;
}
