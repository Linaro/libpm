/* SPDX-License-Identifier: LGPL-2.1+ */
/* Copyright (C) 2023, Linaro Ltd - Daniel Lezcano <daniel.lezcano@linaro.org> */
#ifndef __LIBPOWER_H
#define __LIBPOWER_H

#ifndef LIBPOWER_API
#define LIBPOWER_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif
	struct power_handler;

	int power_limit_set(struct power_handler *handler, const char *name,
			    unsigned int constraint, unsigned int power_mw);

	int power_limit_reset(struct power_handler *handler, const char *name,
			      unsigned int constraint);

	int power_limit_get(struct power_handler *handler, const char *name,
			    unsigned int constraint);
	
	int power_usage_get(struct power_handler *handler, const char *name,
			    unsigned int constraint);

	int power_for_each(struct power_handler *handler,
			   int (*cb)(const char *name, void *data), void *data);
	
	struct power_handler *power_create(void);

	void power_destroy(struct power_handler *handler);
#ifdef __cplusplus
}
#endif

#endif /* __LIBPOWER_H */
