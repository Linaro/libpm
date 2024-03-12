#define _GNU_SOURCE
#include <dirent.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <regex.h>

#include "power.h"

#define DTPM_PATH "/sys/class/powercap"

struct dtpm {
	struct dtpm *next;
	unsigned int hash;
	char *name;
	int set_power_fd;
	int get_power_fd;
};

struct power_handler {
	struct dtpm *dtpm;
};

#define for_each_dtpm(__dtpm__, __iter__) \
	for (__iter__ = __dtpm__; __iter__; __iter__ = __iter__->next)

static unsigned int hash_string(const char *string)
{
        unsigned int hash = 5381;
        int c;

        while ((c = *string++))
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

        return hash;
}

static struct dtpm *power_dtpm_find(struct power_handler *handler,
				    const char *name)
{
	unsigned int hash = hash_string(name);
	struct dtpm *dtpm;

	for_each_dtpm(handler->dtpm, dtpm) {
		if (dtpm->hash == hash)
			return dtpm;
	}

	return NULL;
}

int power_limit_get(struct power_handler *handler, const char *name,
		    unsigned int constraint)
{
	struct dtpm *dtpm;
	unsigned long power_uw;

	dtpm = power_dtpm_find(handler, name);
	if (!dtpm)
		return -1;

	if (pread(dtpm->get_power_fd, &power_uw,
		  sizeof(power_uw), 0) != sizeof(power_uw))
		return -1;
	
	return power_uw / 1000;
}
	
int power_limit_set(struct power_handler *handler, const char *name,
		    unsigned int constraint, unsigned int power_mw)
{
	struct dtpm *dtpm;
	unsigned long power_uw = power_mw * 1000;

	dtpm = power_dtpm_find(handler, name);
	if (!dtpm)
		return -1;

	if (pwrite(dtpm->set_power_fd, &power_uw,
		   sizeof(power_uw), 0) != sizeof(power_uw))
		return -1;

	if (pread(dtpm->get_power_fd, &power_uw,
		  sizeof(power_uw), 0) != sizeof(power_uw))
		return -1;
	
	return power_uw / 1000;
}

int power_limit_reset(struct power_handler *handler, const char *name,
		      unsigned int constraint)
{
	return power_limit_set(handler, name, constraint, 0);
}

int power_usage_get(struct power_handler *handler, const char *name,
		    unsigned int constraint)
{
	struct dtpm *dtpm;
	unsigned long power_uw;

	dtpm = power_dtpm_find(handler, name);
	if (!dtpm)
		return -1;

	if (pread(dtpm->get_power_fd, &power_uw,
		  sizeof(power_uw), 0) != sizeof(power_uw))
		return -1;

	return power_uw / 1000;
}

static int power_dtpm_init(struct dtpm *dtpm, int dirfd, const char *dirname)
{
	struct stat s;
	FILE *file;
	char *buffer;
	int fd, ret = -1;

	buffer = malloc(PATH_MAX);
	if (!buffer)
		return -1;

	snprintf(buffer, PATH_MAX, "%s/name", dirname);

	fd = openat(dirfd, buffer, 0);
	if (fd < 0)
		goto out_free;

	if (fstat(fd, &s) < 0)
		goto out_close;

	dtpm->name = malloc(s.st_size + 1);
	if (!dtpm->name)
		goto out_close;
	
	file = fdopen(fd, "r");
	if (!file)
		goto out_close;

	if (fscanf(file, "%s", dtpm->name) == EOF)
		goto out_fclose;

	dtpm->hash = hash_string(dtpm->name);

	snprintf(buffer, PATH_MAX, "%s/constraint_0_power_limit_uw", dirname);
	dtpm->set_power_fd = openat(dirfd, buffer, 0);
	if (dtpm->set_power_fd < 0)
		goto out_fclose;

	snprintf(buffer, PATH_MAX, "%s/power_uw", dirname);
	dtpm->get_power_fd = openat(dirfd, buffer, 0);
	if (dtpm->get_power_fd < 0) {
		close(dtpm->set_power_fd);
		goto out_fclose;
	}

	ret = 0;
out_fclose:
	fclose(file);
out_close:
	close(fd);
out_free:
	free(buffer);

	return ret;
}

static int power_dtpm_initialize(struct power_handler *handler)
{
	struct dirent *dirent;
	regex_t regex;
	DIR *dir;

	if (regcomp(&regex, "^dtpm:", REG_NOSUB))
		return -1;

	dir = opendir(DTPM_PATH);
	if (!dir)
		return -1;

	/*
	 * The dtpm powercap hierarchy can be also accessed in a flat
	 * manner. As the caller should give the name of the dtpm
	 * node, we can use this information to retrieve the desired
	 * one.
	 */
	while ((dirent = readdir(dir))) {

		struct dtpm *dtpm;
		
		if (regexec(&regex, dirent->d_name, 0, NULL, 0) == REG_NOMATCH)
			continue;

		dtpm = malloc(sizeof(*dtpm));
		if (!dtpm)
			return -1;

		if (power_dtpm_init(dtpm, dirfd(dir), dirent->d_name))
			return -1;

		dtpm->next = handler->dtpm;
		handler->dtpm = dtpm;
	}

	regfree(&regex);

	closedir(dir);

	return 0;
}

int power_for_each(struct power_handler *handler,
		   int (*cb)(const char *name, void *data), void *data)
{
	struct dtpm *dtpm;

	for_each_dtpm(handler->dtpm, dtpm)
		cb(dtpm->name, data);

	return 0;
}

struct power_handler *power_create(void)
{
	struct power_handler *handler;
	struct dtpm *dtpm;

	handler = calloc(1, sizeof(*handler));
	if (!handler)
		return NULL;

	if (power_dtpm_initialize(handler))
		goto out_free;

	return handler;

out_free:
	free(handler);
	return NULL;
}

void power_destroy(struct power_handler *power)
{
	free(power);
}
