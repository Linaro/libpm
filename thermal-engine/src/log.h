#ifndef __THERMAL_ENGINE_LOG_H
#define __THERMAL_ENGINE_LOG_H

#include <syslog.h>

#ifndef __maybe_unused
#define __maybe_unused		__attribute__((__unused__))
#endif

#define TO_SYSLOG 0x1
#define TO_STDOUT 0x2
#define TO_STDERR 0x4

#ifndef LOG_PREFIX
#define LOG_PREFIX ""
#endif

extern void logit(unsigned int level, const char *log_prefix, const char *format, ...);

#define DEBUG(fmt, ...)		logit(LOG_DEBUG, LOG_PREFIX, fmt, ##__VA_ARGS__)
#define INFO(fmt, ...)		logit(LOG_INFO, LOG_PREFIX, fmt, ##__VA_ARGS__)
#define NOTICE(fmt, ...)	logit(LOG_NOTICE, LOG_PREFIX, fmt, ##__VA_ARGS__)
#define WARN(fmt, ...)		logit(LOG_WARNING, LOG_PREFIX, fmt, ##__VA_ARGS__)
#define ERROR(fmt, ...)		logit(LOG_ERR, LOG_PREFIX, fmt, ##__VA_ARGS__)
#define CRITICAL(fmt, ...)	logit(LOG_CRIT, LOG_PREFIX, fmt, ##__VA_ARGS__)
#define ALERT(fmt, ...)		logit(LOG_ALERT, LOG_PREFIX, fmt, ##__VA_ARGS__)
#define EMERG(fmt, ...)		logit(LOG_EMERG, LOG_PREFIX, fmt, ##__VA_ARGS__)

int log_init(int level, const char *ident, int options);
int log_str2level(const char *lvl);
void log_exit(void);

#endif
