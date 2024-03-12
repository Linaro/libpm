/* SPDX-License-Identifier: LGPL-2.1+ */
/* Copyright (C) 2023, Linaro Ltd - Daniel Lezcano <daniel.lezcano@linaro.org> */
#ifndef __LIBPERFORMANCE_H
#define __LIBPERFORMANCE_H

#ifndef LIBPERFORMANCE_API
#define LIBPERFORMANCE_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif
	struct performance_handler;

	int performance_set_device_min_frequency(struct performance_handler *handler,
						 int id, int frequency);

	int performance_get_device_min_frequency(struct performance_handler *handler,
						 int id);

	int performance_set_device_max_frequency(struct performance_handler *handler,
						 int id, int frequency);

	int performance_get_device_max_frequency(struct performance_handler *handler,
						 int id);

	int performance_set_device_frequency(struct performance_handler *handler,
					     int id, int frequency);

	int performance_get_device_frequency(struct performance_handler *handler,
					     int id);

	int performance_set_global_latency(int latency_us);

	int performance_get_global_latency(void);

	int performance_set_device_latency(struct performance_handler *handler,
					   int id, int latency_us);

	int performance_get_device_latency(struct performance_handler *handler,
					   int id);

	int performance_device_latency_supported(struct performance_handler *handler,
						 int id);

	int performance_get_device_id(struct performance_handler *handler,
				      const char *device);

	int performance_for_each_frequency(struct performance_handler *handler,
					   int id, int (*cb)(int frequency, void *data),
					   void *data);

	int performance_for_each_device(struct performance_handler *handler,
					int (*cb)(struct performance_handler *handler,
						  const char *device, void *data),
					void *data);

	struct performance_handler *performance_create(void);

	void performance_destroy(struct performance_handler *handler);
#ifdef __cplusplus
}
#endif

#endif /* __LIBPERFORMANCE_H */
