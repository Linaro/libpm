#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>

#include "pair.h"

static int pair_test(void)
{
	struct pair transitions;
	struct pair *pair;
	void *data;

	pair_init(&transitions);
	pair_add(&transitions, 123, (void *)0x123);
	pair_add(&transitions, 456, (void *)0x456);
	pair_add(&transitions, 789, (void *)0x789);

	data = pair_find(&transitions, 000);
	if (data)
		return -1;

	data = pair_find(&transitions, 123);
	if (data != (void *)0x123)
		return -1;

	data = pair_find(&transitions, 456);
	if (data != (void *)0x456)
		return -1;

	data = pair_find(&transitions, 789);
	if (data != (void *)0x789)
		return -1;

	pair_remove(&transitions, 456);

	data = pair_find(&transitions, 456);
	if (data)
		return -1;

	pair_destroy(&transitions);

	return 0;
}

int main(int argc, char *argv[])
{
	if (pair_test())
		return 1;

	return 0;
}
