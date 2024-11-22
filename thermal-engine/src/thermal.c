// Copyright (C) 2022, Linaro Ltd - Daniel Lezcano <daniel.lezcano@linaro.org>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <thermal.h>
#include <performance.h>

#include "thermal-engine.h"
#include "mainloop.h"
#include "threshold.h"
#include "log.h"
#include "profile.h"

static int show_trip(struct thermal_trip *tt, __maybe_unused void *arg)
{
	struct thermal_handler *th = (typeof(th))arg;
	
	DEBUG("  --> trip id=%d, type=%d, temp=%d, hyst=%d\n",
	     tt->id, tt->type, tt->temp, tt->hyst);

	return 0;
}

struct find_trip_data {
	struct thermal_trip *trip;
	struct thermal_trip *found;
	int trip_id;
};

static int __find_trip(struct thermal_trip *tt, void *data)
{
	struct find_trip_data *ftd = (typeof(ftd))data;
	
	if (tt->id == ftd->trip_id)
		ftd->found = tt;

	return 0;
}

static int find_trip(struct thermal_zone *tz, int trip_id, struct thermal_trip **trip)
{
	struct find_trip_data ftd = { .trip = tz->trip, .trip_id = trip_id, .found = NULL };

	for_each_thermal_trip(tz->trip, __find_trip, &ftd);

	*trip = ftd.found;

	return (*trip) ? 0 : -1;
}



static int show_cdev(struct thermal_cdev *cdev, void *arg)
{
	struct thermal_handler *th = (typeof(th))arg;

	DEBUG("Cooling device '%s', id=%d\n", cdev->name, cdev->id);

	return 0;
}

static int show_tz(struct thermal_zone *tz, void *arg)
{
	DEBUG("Thermal zone '%s', id=%d, governor='%s'\n",
	      tz->name, tz->id, tz->governor);

	for_each_thermal_trip(tz->trip, show_trip, arg);

	return 0;
}

static int tz_create(const char *name, int tz_id, __maybe_unused void *arg)
{
	DEBUG("Thermal zone '%s'/%d created\n", name, tz_id);

	return 0;
}

static int tz_delete(int tz_id, __maybe_unused void *arg)
{
	DEBUG("Thermal zone %d deleted\n", tz_id);

	return 0;
}

static int tz_disable(int tz_id, void *arg)
{
	struct thermal_engine_data *ted = arg;
	struct thermal_zone *tz = thermal_zone_find_by_id(ted->tz, tz_id);

	DEBUG("Thermal zone %d ('%s') disabled\n", tz_id, tz->name);

	return 0;
}

static int tz_enable(int tz_id, void *arg)
{
	struct thermal_engine_data *ted = arg;
	struct thermal_zone *tz = thermal_zone_find_by_id(ted->tz, tz_id);

	DEBUG("Thermal zone %d ('%s') enabled\n", tz_id, tz->name);

	return 0;
}

static int trip_high(int tz_id, int trip_id, int temp, void *arg)
{
	struct thermal_engine_data *ted = arg;
	struct thermal_zone *tz = thermal_zone_find_by_id(ted->tz, tz_id);
	struct thermal_trip *trip;

	DEBUG("Thermal zone %d ('%s'): trip point %d crossed way up with %d m°C\n",
	     tz_id, tz->name, trip_id, temp);

	if (find_trip(tz, trip_id, &trip)) {
		WARN("No trip point found for id=%d\n", trip_id);
		return 0;
	}

	return trip_crossed_up(ted->thresholds, tz_id, trip->temp);
}

static int trip_low(int tz_id, int trip_id, int temp, void *arg)
{
	struct thermal_engine_data *ted = arg;
	struct thermal_zone *tz = thermal_zone_find_by_id(ted->tz, tz_id);
	struct thermal_trip *trip;

	DEBUG("Thermal zone %d ('%s'): trip point %d crossed way down with %d m°C\n",
	     tz_id, tz->name, trip_id, temp);

	if (find_trip(tz, trip_id, &trip)) {
		WARN("No trip point found for id=%d\n", trip_id);
		return 0;
	}

	return trip_crossed_down(ted->thresholds, tz_id, trip->temp);
}

static int trip_add(int tz_id, int trip_id, int type, int temp, int hyst,
		    __maybe_unused void *arg)
{
	DEBUG("Trip point added %d: id=%d, type=%d, temp=%d, hyst=%d\n",
	     tz_id, trip_id, type, temp, hyst);

	return 0;
}

static int trip_delete(int tz_id, int trip_id, __maybe_unused void *arg)
{
	DEBUG("Trip point deleted %d: id=%d\n", tz_id, trip_id);

	return 0;
}

static int trip_change(int tz_id, int trip_id, int type, int temp,
		       int hyst, __maybe_unused void *arg)
{
	struct thermal_engine_data *ted = arg;
	struct thermal_zone *tz = thermal_zone_find_by_id(ted->tz, tz_id);

	DEBUG("Trip point changed %d: id=%d, type=%d, temp=%d, hyst=%d\n",
	     tz_id, trip_id, type, temp, hyst);

	tz->trip[trip_id].type = type;
	tz->trip[trip_id].temp = temp;
	tz->trip[trip_id].hyst = hyst;

	return 0;
}

static int cdev_add(const char *name, int cdev_id, int max_state, __maybe_unused void *arg)
{
	DEBUG("Cooling device '%s'/%d (max state=%d) added\n", name, cdev_id, max_state);

	return 0;
}

static int cdev_delete(int cdev_id, __maybe_unused void *arg)
{
	DEBUG("Cooling device %d deleted", cdev_id);

	return 0;
}

static int cdev_update(int cdev_id, int cur_state, __maybe_unused void *arg)
{
	DEBUG("cdev:%d state:%d\n", cdev_id, cur_state);

	return 0;
}

static int gov_change(int tz_id, const char *name, __maybe_unused void *arg)
{
	struct thermal_engine_data *ted = arg;
	struct thermal_zone *tz = thermal_zone_find_by_id(ted->tz, tz_id);

	DEBUG("%s: governor changed %s -> %s\n", tz->name, tz->governor, name);

	strcpy(tz->governor, name);

	return 0;
}

static struct thermal_ops ops = {
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

static int thermal_event(__maybe_unused int fd, __maybe_unused void *arg)
{
	struct thermal_engine_data *ted = arg;

	return thermal_events_handle(ted->th, ted);
}

void thermal_engine_thermal_exit(struct thermal_engine_data *ted)
{
	mainloop_del(ted->ml, thermal_events_fd(ted->th));

	/* 
	 * FIXME: seems like genl unsubscribe is broken
	 * thermal_exit(ted->th);
	 */
	free(ted->tz);
	free(ted->cdev);
}

int thermal_engine_thermal_init(struct thermal_engine_data *ted)
{
	ted->th = thermal_init(&ops);
	if (!ted->th) {
		ERROR("Failed to initialize the thermal library\n");
		return -1;
	}

	ted->tz = thermal_zone_discover(ted->th);
	if (!ted->tz) {
		ERROR("No thermal zone available\n");
		return -1;
	}

	if (thermal_cmd_get_cdev(ted->th, &ted->cdev)) {
		ERROR("Failed to get cooling device list\n");
		return -1;
	}

	if (mainloop_add(ted->ml, thermal_events_fd(ted->th), thermal_event, ted)) {
		ERROR("Failed to setup the mainloop\n");
		return -1;
	}

	for_each_thermal_zone(ted->tz, show_tz, ted->th);

	for_each_thermal_cdev(ted->cdev, show_cdev, ted->th);
	
	return 0;
}
