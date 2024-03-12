/* SPDX-License-Identifier: LGPL-2.1+ */
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <sys/types.h>

#include "performance.h"

static int tst_global_latency(struct performance_handler *handler)
{
	const int global_latency = 1000;
	int value;

	if (performance_set_global_latency(global_latency) < 0) {
		fprintf(stderr, "Failed to set the global latency\n");
		return -1;
	}

	value = performance_get_global_latency();
	if (value != global_latency) {
		fprintf(stderr, "global latency mismatch %d<>%d\n", value, global_latency);
		return -1;
	}

	if (performance_set_global_latency(INT_MAX) < 0) {
		fprintf(stderr, "Failed to reset the global latency\n");
		return -1;
	}

	value = performance_get_global_latency();
	if (value != INT_MAX) {
		fprintf(stderr, "global latency mismatch %d<>%d\n", value, INT_MAX);
		return -1;
	}

	return 0;
}

static int tst_device_latency_cb(struct performance_handler *handler,
				 const char *device, void *data)
{
	const int latency = 500;
	int id, value, prev_value;

	id = performance_get_device_id(handler, device);
	if (id < 0) {
		fprintf(stderr, "Failed to get device '%s' id\n", device);
		return -1;
	}

	if (!performance_device_latency_supported(handler, id))
		return 0;

	prev_value = performance_get_device_latency(handler, id);
	if (prev_value < 0) {
		fprintf(stderr, "Failed to get previous latency value\n");
		return -1;
	}

	if (performance_set_device_latency(handler, id, latency)) {
		fprintf(stderr, "Failed to set latency\n");
		return -1;
	}

	value = performance_get_device_latency(handler, id);
	if (value < 0) {
		fprintf(stderr, "Failed to get current latency value\n");
		return -1;
	}

	if (value != latency) {
		fprintf(stderr, "Current and set value mismatch\n");
		return -1;
	}

	if (performance_set_device_latency(handler, id, prev_value)) {
		fprintf(stderr, "Failed to set latency to previous value\n");
		return -1;
	}

	value = performance_get_device_latency(handler, id);
	if (value < 0) {
		fprintf(stderr, "Failed to get current latency value\n");
		return -1;
	}

	if (value != prev_value) {
		fprintf(stderr, "Failed to restore previous value\n");
		return -1;
	}

	return 0;
}

static int tst_device_latency(struct performance_handler *handler)
{
	return performance_for_each_device(handler, tst_device_latency_cb, NULL);
}

struct freq_cb_data {
	int nr_freq;
	int *freq;
};

static int tst_device_frequency_fill_cb(int frequency, void *data)
{
	struct freq_cb_data *fcd = (typeof(fcd))data;

	fcd->freq[fcd->nr_freq] = frequency;
	fcd->nr_freq++;

	return 0;
}

static int tst_device_frequency_cb(struct performance_handler *handler,
				   const char *device, void *data)
{
	struct freq_cb_data fcd = { 0 };
	int freq[64];
	int id, i;
	int value;

	fcd.freq = freq;

	id = performance_get_device_id(handler, device);
	if (id < 0) {
		fprintf(stderr, "Failed to get device '%s' id\n", device);
		return -1;
	}

	if (performance_for_each_frequency(handler, id,
					   tst_device_frequency_fill_cb, (void *)&fcd)) {
		fprintf(stderr, "Failed to read frequency for device=%s id=%d\n", device, id);
		return -1;
	}

	/*
	 * For testing the range, we must set the maximum frequency
	 * interval first.
	 */
	if (performance_set_device_min_frequency(handler, id, fcd.freq[0]) < 0) {
		fprintf(stderr, "Failed to set initial minimum frequency for device=%s\n", device);
		return -1;
	}

	if (performance_set_device_max_frequency(handler, id,
						 fcd.freq[fcd.nr_freq - 1]) < 0) {
		fprintf(stderr, "Failed to set initial maximum frequency for device=%s\n",
			device);
		return -1;
	}

	for (i = 0; i < fcd.nr_freq; i++) {
		if (performance_set_device_min_frequency(handler, id, fcd.freq[i])) {
			fprintf(stderr, "Failed to set minimum frequency for device=%s\n",
				device);
			return -1;
		}

		/*
		 * When setting the minimum/maxiumu frequency, there
		 * can be delay before being taken into account and
		 * reflected in sysfs.
		 */
		usleep(1000);

		value = performance_get_device_min_frequency(handler, id);
		if (value != fcd.freq[i]) {
			fprintf(stderr, "Minimum frequency mismatch for device=%s %d <> %d\n",
				device, value, fcd.freq[i]);
			return -1;
		}
	}

	if (performance_set_device_min_frequency(handler, id, fcd.freq[0]) < 0) {
		fprintf(stderr, "Failed to set back minimum frequency for device=%s\n", device);
		return -1;
	}

	usleep(1000);

	if (performance_get_device_min_frequency(handler, id) != fcd.freq[0]) {
		fprintf(stderr, "Unexpected minimum frequency for device=%s\n", device);
		return -1;
	}

	for (i = fcd.nr_freq - 1; i >= 0; i--) {
		if (performance_set_device_max_frequency(handler, id, fcd.freq[i])) {
			fprintf(stderr, "Failed to set maximum frequency for device=%s\n",
				device);
			return -1;
		}

		/*
		 * When setting the minimum/maxiumu frequency, there
		 * can be delay before being taken into account and
		 * reflected in sysfs.
		 */
		usleep(1000);

		if (performance_get_device_max_frequency(handler, id) != fcd.freq[i]) {
			fprintf(stderr, "Maximum frequency mismatch for device=%s\n",
				device);
			return -1;
		}
	}

	if (performance_set_device_max_frequency(handler, id,
						 fcd.freq[fcd.nr_freq - 1]) < 0) {
		fprintf(stderr, "Failed to set back maximum frequency for device=%s\n",
			device);
		return -1;
	}

	usleep(1000);

	if (performance_get_device_max_frequency(handler, id) != fcd.freq[fcd.nr_freq - 1]) {
		fprintf(stderr, "Unexpected maximum frequency for device=%s\n", device);
		return -1;
	}

	return 0;
}

static int tst_device_frequency(struct performance_handler *handler)
{
	return performance_for_each_device(handler, tst_device_frequency_cb, NULL);
}

int main(void)
{
	struct performance_handler *handler;

	handler = performance_create();
	if (!handler) {
		fprintf(stderr, "Failed to initialize the library\n");
		return -1;
	}

	printf("Global latency test: %s\n",
	       tst_global_latency(handler) ? "[Failed]" : "[OK]");

	printf("Device latency test: %s\n",
	       tst_device_latency(handler) ? "[Failed]" : "[OK]");

	printf("Device frequency test: %s\n",
	       tst_device_frequency(handler) ? "[Failed]" : "[OK]");

	performance_destroy(handler);

	return 0;
}
