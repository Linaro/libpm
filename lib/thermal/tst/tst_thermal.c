/* SPDX-License-Identifier: LGPL-2.1+ */
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#include <sys/epoll.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "thermal.h"

#define MAX_EVENTS 10

static int show_trip(struct thermal_trip *tt, void *arg)
{
	printf("trip id=%d, type=%d, temp=%d, hyst=%d\n",
	       tt->id, tt->type, tt->temp, tt->hyst);

	return 0;
}		

static int show_temp(struct thermal_zone *tz, void *arg)
{
	thermal_cmd_get_temp(arg, tz);

	printf("temperature: %d\n", tz->temp);

	return 0;
}

static int show_governor(struct thermal_zone *tz, void *arg)
{
	thermal_cmd_get_governor(arg, tz);

	printf("governor: '%s'\n", tz->governor);
	
	return 0;
}

static int show_tz(struct thermal_zone *tz, void *arg)
{
	printf("thermal zone '%s', id=%d\n", tz->name, tz->id);

	for_each_thermal_trip(tz->trip, show_trip, NULL);

	show_temp(tz, arg);
	
	show_governor(tz, arg);
	
	return 0;
}

static int tz_create(const char *name, int tz_id, void *arg)
{
	printf("Thermal zone '%s'/%d created\n", name, tz_id);

	return 0;
}

static int tz_delete(int tz_id, void *arg)
{
	printf("Thermal zone %d deleted\n", tz_id);

	return 0;
}

static int tz_disable(int tz_id, void *arg)
{
	printf("Thermal zone %d disabled\n", tz_id);

	return 0;
}

static int tz_enable(int tz_id, void *arg)
{
	printf("Thermal zone %d enabled\n", tz_id);

	return 0;
}

static int tz_temp(int tz_id, int temp, void *arg)
{
	printf("Thermal zone %d temperature: %d\n", tz_id, temp);

	return 0;
}

static int trip_high(int tz_id, int trip_id, int temp, void *arg)
{
	printf("Thermal zone %d: trip point %d crossed way up with %d °C\n",
	       tz_id, trip_id, temp);

	return 0;
}

static int trip_low(int tz_id, int trip_id, int temp, void *arg)
{
	printf("Thermal zone %d: trip point %d crossed way down with %d °C\n",
	       tz_id, trip_id, temp);

	return 0;
}

static int trip_add(int tz_id, int trip_id, int type, int temp, int hyst, void *arg)
{
	printf("Trip point added %d: id=%d, type=%d, temp=%d, hyst=%d\n",
	       tz_id, trip_id, type, temp, hyst);

	return 0;
}

static int trip_delete(int tz_id, int trip_id, void *arg)
{
	printf("Trip point deleted %d: id=%d\n", tz_id, trip_id);

	return 0;
}

static int trip_change(int tz_id, int trip_id, int type, int temp, int hyst, void *arg)
{
	printf("Trip point changed %d: id=%d, type=%d, temp=%d, hyst=%d\n",
	       tz_id, trip_id, type, temp, hyst);

	return 0;
}

static int cdev_add(const char *name, int cdev_id, int max_state, void *arg)
{
	printf("Cooling device '%s'/%d (max state=%d) added",
	       name, cdev_id, max_state);

	return 0;
}

static int cdev_delete(int cdev_id, void *arg)
{
	printf("Cooling device %d deleted", cdev_id);

	return 0;
}

static int cdev_update(int cdev_id, int cur_state, void *arg)
{
	printf("cdev:%d state:%d\n", cdev_id, cur_state);

	return 0;
}

static int gov_change(int tz_id, const char *name, void *arg)
{
	printf("tz %d, governor=%s\n", tz_id, name);

	return 0;
}

static struct thermal_ops ops = {
	.sampling.tz_temp	= tz_temp,
	.events.tz_create	= tz_create,
	.events.tz_delete	= tz_delete,
	.events.tz_disable	= tz_disable,
	.events.tz_enable	= tz_enable,
	.events.trip_high	= trip_high,
	.events.trip_low	= trip_low,
	.events.trip_add	= trip_add,
	.events.trip_delete	= trip_delete,
	.events.trip_change	= trip_change,
	.events.cdev_add	= cdev_add,
	.events.cdev_delete	= cdev_delete,
	.events.cdev_update	= cdev_update,
	.events.gov_change	= gov_change
};

static int stop = 0;

static void sighandler(int sig)
{
	stop = 1;
};

int thermal_netlink_get_temp_bench(struct thermal_handler *th,
				   struct thermal_zone *tz)
{
	int nr_messages = 0;
	int nr_secs = 5;
	unsigned long long sum = 0;
	
	printf("Benchmarking netlink... wait %d secs\n", nr_secs);
		
	signal(SIGALRM, sighandler);
	alarm(nr_secs);
	while (!stop) {
		thermal_cmd_get_temp(th, tz);
		sum += tz->temp;
		nr_messages++;
	}
		
	printf("Temperature reading %d msg/sec (%llu usec/msg), avg temp=%llu\n",
	       nr_messages / nr_secs, 1000000ULL / (nr_messages / nr_secs),
	       sum / nr_messages);

	return 0;
}

int thermal_sysfs_get_temp_bench(struct thermal_zone *tz)
{
	int nr_messages = 0;
	int nr_secs = 5;
	unsigned long long sum = 0;
	char path[PATH_MAX];
	int fd;

	snprintf(path, PATH_MAX,
		 "/sys/class/thermal/thermal_zone%d/temp", tz->id);

	fd = open(path, O_RDONLY);
	if (fd < 0)
		return -1;

	printf("Benchmarking sysfs... wait %d secs\n", nr_secs);

	signal(SIGALRM, sighandler);
	alarm(nr_secs);
	stop = 0;
	while (!stop) {
		char buffer[128] = { 0 };

		pread(fd, buffer, 127, 0);
		sum += atoi(buffer);
		nr_messages++;
	}

	if (!nr_messages) {
		fprintf(stderr, "No message read\n");
		return -1;
	}
	
	printf("Temperature reading %d msg/sec (%llu usec/msg), avg temp=%llu\n",
	       nr_messages / nr_secs, 1000000ULL / (nr_messages / nr_secs),
	       sum / nr_messages);

	close(fd);

	return 0;
}

int main(void)
{
	struct thermal_zone *tz;
	struct thermal_handler *th;
	struct epoll_event ev;
	struct epoll_event events[MAX_EVENTS];	
	int epollfd;
	int nfds;
	int i;

	th = thermal_init(&ops);
	if (!th)
		return -1;

	tz = thermal_zone_discover(th);
	if (!tz)
		return -1;

	thermal_netlink_get_temp_bench(th, tz);

	thermal_sysfs_get_temp_bench(tz);

	for_each_thermal_zone(tz, show_tz, th);
	
	epollfd = epoll_create1(0);
	if (epollfd < 0)
		return -1;

	ev.events = EPOLLIN;
	ev.data.ptr = thermal_events_handle;

	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, thermal_events_fd(th), &ev) == -1)
		return -1;

	ev.events = EPOLLIN;
	ev.data.ptr = thermal_sampling_handle;
	
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, thermal_sampling_fd(th), &ev) == -1)
		return -1;
	
	while (1) {

		static int counter = 0;

		nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
		for (i = 0; i < nfds; i++) {
			if (events[i].data.ptr == thermal_events_handle) {
				thermal_events_handle(th, NULL);
			} else if (events[i].data.ptr == thermal_sampling_handle) {
				counter++;
				thermal_sampling_handle(th, NULL);
			}
		}

		if (counter == 5)
			break;
	}

	return 0;
}
