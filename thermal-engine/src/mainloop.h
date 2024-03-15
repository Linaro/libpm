/* SPDX-License-Identifier: LGPL-2.1+ */
/* Copyright (C) 2024, Linaro Ltd - Daniel Lezcano <daniel.lezcano@linaro.org> */
#ifndef __THERMAL_ENGINE_MAINLOOP_H
#define __THERMAL_ENGINE_MAINLOOP_H

typedef int (*mainloop_callback_t)(int fd, void *data);

struct mainloop;

extern int mainloop(struct mainloop *mainloop, unsigned int timeout);
extern int mainloop_add(struct mainloop *mainloop, int fd,
			mainloop_callback_t cb, void *data);
extern int mainloop_del(struct mainloop *mainloop, int fd);
extern void mainloop_exit(struct mainloop *mainloop);
extern void mainloop_fini(struct mainloop *mainloop);
extern struct mainloop *mainloop_init(void);

#endif
