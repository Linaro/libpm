// Copyright (C) 2022, Linaro Ltd - Daniel Lezcano <daniel.lezcano@linaro.org>
#include <stdio.h>
#include <sys/time.h>

static struct timeval timestamp_offset;

int timestamp_init(void)
{
	gettimeofday(&timestamp_offset, NULL);

	return 0;
}

unsigned long timestamp(void)
{
	struct timeval tv;

	gettimeofday(&tv, NULL);

	return ((tv.tv_sec - timestamp_offset.tv_sec) * 1000) +
		((tv.tv_usec - timestamp_offset.tv_usec) / 1000);
}
