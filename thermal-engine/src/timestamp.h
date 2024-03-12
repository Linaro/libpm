/* Copyright (C) 2022, Linaro Ltd - Daniel Lezcano <daniel.lezcano@linaro.org> */
#ifndef __THERMAL_ENGINE_UPTIMEOFDAY_H
#define __THERMAL_ENGINE_UPTIMEOFDAY_H
#include <sys/time.h>

int timestamp_init(void);
unsigned long timestamp(void);
struct timespec msec_to_timespec(int msec);

#endif
