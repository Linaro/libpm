// Copyright (C) 2022, Linaro Ltd - Daniel Lezcano <daniel.lezcano@linaro.org>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>

#include "log.h"
#include "timestamp.h"

#define BUFFER_LOG_SIZE	4096

static const char *__ident = "unknown";
static int __options;
static unsigned int __level;

static const char * const loglvl[] = {
	[LOG_DEBUG]	= "DEBUG",
	[LOG_INFO]	= "INFO",
	[LOG_NOTICE]	= "NOTICE",
	[LOG_WARNING]	= "WARN",
	[LOG_ERR]	= "ERROR",
	[LOG_CRIT]	= "CRITICAL",
	[LOG_ALERT]	= "ALERT",
	[LOG_EMERG]	= "EMERG",
};

int log_str2level(const char *lvl)
{
	unsigned int i;

	for (i = 0; i < sizeof(loglvl) / sizeof(loglvl[LOG_DEBUG]); i++)
		if (!strcmp(lvl, loglvl[i]))
			return i;

	return LOG_DEBUG;
}

extern void logit(unsigned int level, const char *log_prefix, const char *format, ...)
{
	static char buffer[BUFFER_LOG_SIZE] = { '\0' };
	size_t idx;
	va_list args;

	if (level >= sizeof(loglvl))
		return;
	
	if (level > __level)
		return;

	if (!format || !strlen(format))
		return;

	idx = strlen(buffer);

	/*
	 * There is nothing in the buffer, it is a new trace. Let's
	 * add the timestamp.
	 */
	if (!idx) {
		unsigned long ts = timestamp();
		idx = sprintf(buffer, "[ %lu.%03lu ] (%s%s%s): ",
			      ts / 1000, ts % 1000, loglvl[level],
			      log_prefix[0] != '\0' ? "@" : "", log_prefix);
	}

	/*
	 * If the logger did not added a '\n', then it wants the log
	 * to be concatenate with the next one. This is useful when we
	 * are logging multiple information without wanting to do that
	 * in multiline.
	 */
	if (format[strlen(format) - 1] != '\n') {
		va_start(args, format);
		vsnprintf(&buffer[idx], BUFFER_LOG_SIZE - idx, format, args);
		va_end(args);
		return;
	}

	/*
	 * We have a previous message in the buffer, we should
	 * concatenate the current one with the buffer.
	 */
	va_start(args, format);
	vsnprintf(&buffer[idx], BUFFER_LOG_SIZE - idx, format, args);		
	va_end(args);

 	if (__options & TO_SYSLOG)
		syslog(level, "%s", buffer);

	if (__options & TO_STDERR)
		fprintf(stderr, "%s", buffer);

	if (__options & TO_STDOUT)
		fprintf(stdout, "%s", buffer);

	buffer[0] = '\0';
}

int log_init(int level, const char *ident, int options)
{
	if (!options)
		return -1;

	if (level > LOG_DEBUG)
		return -1;

	if (!ident)
		return -1;

	if (timestamp_init())
		return -1;

	__ident = ident;
	__options = options;
	__level = level;

	if (options & TO_SYSLOG) {
		openlog(__ident, options | LOG_NDELAY, LOG_USER);
		setlogmask(LOG_UPTO(level));
	}

	return 0;
}

void log_exit(void)
{
	closelog();
}
