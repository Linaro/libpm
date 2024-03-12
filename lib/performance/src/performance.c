// SPDX-License-Identifier: LGPL-2.1+
// Copyright (C) 2023, Linaro Ltd - Daniel Lezcano <daniel.lezcano@linaro.org>
#define _GNU_SOURCE
#include <dirent.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <fcntl.h>
#include <limits.h>

#include "performance.h"

#define SYS_CLASS_DEVFREQ		"/sys/class/devfreq"
#define SYS_DEVICE_SYSTEM_CPU		"/sys/devices/system/cpu"
#define SYS_DEVICE_POWER_LATENCY_US	"power/pm_qos_resume_latency_us"

#define CPU_MIN_FREQ			"cpufreq/scaling_min_freq"
#define CPU_MAX_FREQ			"cpufreq/scaling_max_freq"
#define CPU_CUR_FREQ			"cpufreq/scaling_cur_freq"
#define CPU_SET_FREQ			"cpufreq/scaling_setspeed"
#define CPU_FREQUENCIES			"cpufreq/scaling_available_frequencies"

#define DEV_MIN_FREQ			"min_freq"
#define DEV_MAX_FREQ			"max_freq"
#define DEV_CUR_FREQ			"cur_freq"
#define DEV_SET_FREQ			"target_freq"
#define DEV_FREQUENCIES			"available_frequencies"

#define MAX_FREQUENCIES			30

#define CPU_DMA_LATENCY_DEV		"/dev/cpu_dma_latency"

static int cpu_dma_latency_fd = -1;

/*
 * The opening of a file is the most costly operation with files. So
 * instead of open / read / close, let's open the file descriptor and
 * keep it opened during all the life time of the library. However, we
 * don't want those fd to be leaked in child processes application, so
 * we should the close-on-exec flag.
 */
typedef enum {
	LATENCY,
	MIN_FREQ,
	MAX_FREQ,
	CUR_FREQ,
	SET_FREQ,
	MAX_ATTRS,
} perf_type_t;

#define HZ_UNIT	1
#define KHZ_UNIT 1000
#define MHZ_UNIT (KHZ_UNIT * 1000)
#define GHZ_UNIT (MHZ_UNIT * 1000)

#define KHZ_TO_MHZ(__value) (__value / 1000)
#define MHZ_TO_KHZ(__value) (__value * 1000)
#define HZ_TO_MHZ(__value) (KHZ_TO_MHZ(__value) / 1000)

struct dev_sysfs_perf {
	char device[PATH_MAX];
	int fds[MAX_ATTRS];
	int frequencies[MAX_FREQUENCIES];
	int nr_frequency;
};

struct attrs {
	const char *attr;
	mode_t mode;
};

struct performance_handler {
	struct dev_sysfs_perf *dev_sysfs_perfs;
	int count;
};

static int set_device_perf(struct performance_handler *handler,
			   int id, perf_type_t perf_type, int value)
{
	int len = 128;
	char value_str[len];

	len = snprintf(value_str, len - 1, "%d\n", value);
	if (len < 0)
		return -1;

	if (pwrite(handler->dev_sysfs_perfs[id].fds[perf_type], value_str, len, 0) < 0)
		return -1;

	return 0;
}

static int get_device_perf(struct performance_handler *handler,
			   int id, perf_type_t perf_type)
{
	int len = 128;
	char value_str[len];

	if (pread(handler->dev_sysfs_perfs[id].fds[perf_type], value_str, len, 0) < 0)
		return -1;

	return atoi(value_str);
}

static int is_device_perf_supported(struct performance_handler *handler,
				    int id, perf_type_t perf_type)
{
	return (handler->dev_sysfs_perfs[id].fds[perf_type] != -1);
}

int performance_set_device_latency(struct performance_handler *handler,
				   int id, int latency_us)
{
	return set_device_perf(handler, id, LATENCY, latency_us);
}

int performance_get_device_latency(struct performance_handler *handler, int id)
{
	return get_device_perf(handler, id, LATENCY);
}

int performance_device_latency_supported(struct performance_handler *handler, int id)
{
	return is_device_perf_supported(handler, id, LATENCY);
}

int performance_set_device_min_frequency(struct performance_handler *handler,
					 int id, int frequency)
{
	return set_device_perf(handler, id, MIN_FREQ, frequency);
}

int performance_get_device_min_frequency(struct performance_handler *handler, int id)
{
	return get_device_perf(handler, id, MIN_FREQ);
}

int performance_set_device_max_frequency(struct performance_handler *handler,
					 int id, int frequency)
{
	return set_device_perf(handler, id, MAX_FREQ, frequency);
}

int performance_get_device_max_frequency(struct performance_handler *handler, int id)
{
	return get_device_perf(handler, id, MAX_FREQ);
}

int performance_set_device_frequency(struct performance_handler *handler,
				     int id, int frequency)
{
	return set_device_perf(handler, id, SET_FREQ, frequency);
}

int performance_get_device_frequency(struct performance_handler *handler, int id)
{
	return get_device_perf(handler, id, CUR_FREQ);
}

int performance_set_global_latency(int latency_us)
{
	int ret;

	if (latency_us == INT_MAX) {
		close(cpu_dma_latency_fd);
		cpu_dma_latency_fd = -1;
		return 0;
	}

	if (cpu_dma_latency_fd == -1) {
		cpu_dma_latency_fd = open(CPU_DMA_LATENCY_DEV, O_CLOEXEC | O_RDWR);
		if (cpu_dma_latency_fd < 0)
			return -1;
	}

	ret = pwrite(cpu_dma_latency_fd, &latency_us, sizeof(latency_us), 0);
	if (ret < 0) {
		close(cpu_dma_latency_fd);
		cpu_dma_latency_fd = -1;
	}

	return 0;
}

int performance_get_global_latency(void)
{
	int latency_us;
	int ret;

	if (cpu_dma_latency_fd == -1)
		return INT_MAX;

	ret = pread(cpu_dma_latency_fd, &latency_us, sizeof(latency_us), 0);
	if (ret < 0)
		return ret;

	return latency_us;
}

int performance_get_device_id(struct performance_handler *handler,
			      const char *device)
{
	int i;

	for (i = 0; i < handler->count; i++) {
		if (!strcmp(device, handler->dev_sysfs_perfs[i].device))
			return i;
	}

	return -1;
}

int performance_for_each_device(struct performance_handler *handler,
				int (*cb)(struct performance_handler *handler,
					  const char *device, void *data), void *data)
{
	int i;

	for (i = 0; i < handler->count; i++) {
		if (cb(handler, handler->dev_sysfs_perfs[i].device, data) < 0)
			return -1;
	}

	return 0;
}

int performance_for_each_frequency(struct performance_handler *handler,
				   int id, int (*cb)(int frequency, void *data), void *data)
{
	int i;

	for (i = 0; i < handler->dev_sysfs_perfs[id].nr_frequency; i++)
		if (cb(handler->dev_sysfs_perfs[id].frequencies[i], data) < 0)
			return -1;

	return 0;
}

static int __dev_sysfs_get_frequencies(int dirfd, struct dev_sysfs_perf *perf,
				       const char *frequencies)
{
	/*
	 * The maximum sysfs file size is a page size. Even if the
	 * content would be largely under this value, we stick to the
	 * sysfs standard.
	 */
	size_t size = getpagesize();
	char buffer[size];
	char *saveptr, *token;
	FILE *f;
	int fd, ret = -1;

	perf->nr_frequency = 0;

	/*
	 * Retrieve the list of the available frequencies
	 */
	fd = openat(dirfd, frequencies, O_RDONLY);
	if (fd < 0)
		return -1;

	f = fdopen(fd, "r");
	if (!f)
		goto out;

	if (!fgets(buffer, size, f))
		goto out;

	token = strtok_r(buffer, " ", &saveptr);
	while (token) {
		perf->frequencies[perf->nr_frequency] = atoi(token);
		perf->nr_frequency++;
		token = strtok_r(NULL, " \n", &saveptr);
	}

	ret = 0;
out:
	fclose(f);
	close(fd);

	return ret;
}

static int __dev_sysfs_perf_init(int dirfd,
				 struct dev_sysfs_perf *perf,
				 struct attrs attrs[],
				 const char *frequencies)
{
	int i;

	for (i = 0; i < MAX_ATTRS; i++) {

		perf->fds[i] = -1;

		if (attrs[i].attr[0] == '\0')
			continue;

		perf->fds[i] = openat(dirfd, attrs[i].attr,
				      attrs[i].mode | O_CLOEXEC);

		/*
		 * We don't check if the file was correctly opened or
		 * not. There may be some permissions preventing us to
		 * access those files. For instance on Android,
		 * setting the current frequency is a file belonging
		 * to 'root' for writing but the services are uid
		 * 'system'.
		 *
		 * The usage of the different API will fail giving us
		 * the indication the feature is not accessible.
		 */
	}

	return __dev_sysfs_get_frequencies(dirfd, perf, frequencies);
}

static int devfreq_sysfs_perf_init(struct performance_handler *handler)
{
	DIR *dir;
	int dirfd;
	struct dirent *dirent;

	struct attrs attrs[] = {
		[LATENCY]  = { "", 0 },
		[MIN_FREQ] = { DEV_MIN_FREQ, O_RDWR },
		[MAX_FREQ] = { DEV_MAX_FREQ, O_RDWR },
		[CUR_FREQ] = { DEV_CUR_FREQ, O_RDONLY },
		[SET_FREQ] = { "", 0 }
	};

	struct dev_sysfs_perf *dev_sysfs_perfs = handler->dev_sysfs_perfs;
	
	char path[PATH_MAX];

	dir = opendir(SYS_CLASS_DEVFREQ);
	if (!dir)
		return -1;

	while ((dirent = readdir(dir))) {

		if (dirent->d_name[0] == '.')
			continue;

		dev_sysfs_perfs = realloc(dev_sysfs_perfs,
					  sizeof(*dev_sysfs_perfs) * (handler->count + 1));
		if (!dev_sysfs_perfs)
			return -1;

		handler->dev_sysfs_perfs = dev_sysfs_perfs;
		
		sprintf(handler->dev_sysfs_perfs[handler->count].device,
			"%s", dirent->d_name);

		sprintf(path, "%s/%s", SYS_CLASS_DEVFREQ, dirent->d_name);

		dirfd = open(path, O_DIRECTORY | O_CLOEXEC);
		if (dirfd < 0)
			return -1;

		if (__dev_sysfs_perf_init(dirfd, &dev_sysfs_perfs[handler->count],
					  attrs, DEV_FREQUENCIES))
			return -1;

		close(dirfd);

		handler->count++;
	}

	closedir(dir);

	return 0;
}

static int cpu_sysfs_perf_init(struct performance_handler *handler)
{
	struct attrs attrs[] = {
		[LATENCY]  = { SYS_DEVICE_POWER_LATENCY_US, O_RDWR },
		[MIN_FREQ] = { CPU_MIN_FREQ, O_RDWR },
		[MAX_FREQ] = { CPU_MAX_FREQ, O_RDWR },
		[CUR_FREQ] = { CPU_CUR_FREQ, O_RDONLY },
		[SET_FREQ] = { CPU_SET_FREQ, O_WRONLY }
	};

	struct dev_sysfs_perf *dev_sysfs_perfs = handler->dev_sysfs_perfs;

	int i, nr_cpus, dirfd;
	char *path;

	nr_cpus = get_nprocs_conf();
	if (nr_cpus < 0)
		return -1;

	dev_sysfs_perfs = realloc(dev_sysfs_perfs, sizeof(*dev_sysfs_perfs) * nr_cpus);
	if (!dev_sysfs_perfs)
		return -1;

	for (i = handler->count; i < nr_cpus; i++) {

		sprintf(dev_sysfs_perfs[i].device, "cpu%d", i);

		if (asprintf(&path, "%s/%s", SYS_DEVICE_SYSTEM_CPU,
			     dev_sysfs_perfs[i].device) < 0)
			return -1;

		dirfd = open(path, O_DIRECTORY | O_CLOEXEC);

		free(path);

		if (dirfd < 0)
			return -1;

		if (__dev_sysfs_perf_init(dirfd, &dev_sysfs_perfs[i],
					  attrs, CPU_FREQUENCIES))
			return -1;

		close(dirfd);
	}

	handler->count = nr_cpus;
	handler->dev_sysfs_perfs = dev_sysfs_perfs;

	return 0;
}

struct performance_handler *performance_create(void)
{
	struct performance_handler *handler;

	handler = malloc(sizeof(*handler));
	if (!handler)
		return NULL;

	memset(handler, 0, sizeof(*handler));
	
	if (cpu_sysfs_perf_init(handler))
		goto out;

	if (devfreq_sysfs_perf_init(handler))
		goto out;

	return handler;
out:
	free(handler);
	return NULL;
}

void performance_destroy(struct performance_handler *handler)
{
	free(handler->dev_sysfs_perfs);
	free(handler);
}
