// Copyright (C) 2022, Linaro Ltd - Daniel Lezcano <daniel.lezcano@linaro.org>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <sys/epoll.h>

#include "pair.h"
#include "log.h"
#include "mainloop.h"

struct mainloop_data {
	mainloop_callback_t cb;
	void *data;
	int fd;
};

struct mainloop {
	int epfd;
	int exit_mainloop;
	struct pair events;
};

#define MAX_EVENTS 10

int mainloop(struct mainloop *mainloop, unsigned int timeout)
{
	int i, nfds;
	struct epoll_event events[MAX_EVENTS];
	struct mainloop_data *md;

	if (mainloop->epfd < 0)
		return -1;

	for (;;) {

		nfds = epoll_wait(mainloop->epfd, events, MAX_EVENTS, timeout);
		if (nfds < 0) {
			if (errno == EINTR)
				continue;
			return -1;
		}

		for (i = 0; i < nfds; i++) {
			md = events[i].data.ptr;

			if (md->cb(md->fd, md->data) > 0)
				return 0;
		}

		if (mainloop->exit_mainloop || !nfds)
			return 0;
	}
}

int mainloop_add(struct mainloop *mainloop, int fd,
		 mainloop_callback_t cb, void *data)
{
	struct epoll_event ev = {
		.events = EPOLLIN,
	};

	struct mainloop_data *md;

	md = malloc(sizeof(*md));
	if (!md)
		return -1;

	md->data = data;
	md->cb = cb;
	md->fd = fd;

	ev.data.ptr = md;

	if (epoll_ctl(mainloop->epfd, EPOLL_CTL_ADD, fd, &ev) < 0) {
		free(md);
		return -1;
	}

	pair_add(&mainloop->events, fd, md);
	
	return 0;
}

int mainloop_del(struct mainloop *mainloop, int fd)
{
	if (epoll_ctl(mainloop->epfd, EPOLL_CTL_DEL, fd, NULL) < 0)
		return -1;

	free(pair_find(&mainloop->events, fd));
	
	return 0;
}

struct mainloop *mainloop_init(void)
{
	struct mainloop *mainloop;

	mainloop = malloc(sizeof(*mainloop));
	if (!mainloop)
		return NULL;

	mainloop->exit_mainloop = 0;

	mainloop->epfd = epoll_create(2);
	if (mainloop->epfd < 0) {
		free(mainloop);
		return NULL;
	}

	pair_init(&mainloop->events);
	
	return mainloop;
}

void mainloop_exit(struct mainloop *mainloop)
{
	mainloop->exit_mainloop = 1;
}

void mainloop_fini(struct mainloop *mainloop)
{
	close(mainloop->epfd);
	free(mainloop);
}
