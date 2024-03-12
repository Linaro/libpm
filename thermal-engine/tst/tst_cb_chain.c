#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>

#include "cb_chain.h"

static void cb_chain_cb1(int id, void *data)
{
	int *value = (int *)data;

	(*value) += 1;
}

static void cb_chain_cb2(int id, void *data)
{
	int *value = (int *)data;

	(*value) += 2;
}

static void cb_chain_cb3(int id, void *data)
{
	int *value = (int *)data;

	(*value) += 3;
}

int cb_chain_test(void)
{
	struct cb_chain cb_chain;
	int value = 0;

	cb_chain_init(&cb_chain);

	cb_chain_add(&cb_chain, cb_chain_cb1, &value);
	cb_chain_add(&cb_chain, cb_chain_cb2, &value);
	cb_chain_add(&cb_chain, cb_chain_cb3, &value);

	/*
	 * Chain runs callback:
	 * 1 + 2 + 3 = 6
	 */
	cb_chain_run(0, &cb_chain);
	if (value != 6) {
		fprintf(stderr, "value=%d\n", value);
		return -1;
	}

	cb_chain_remove(&cb_chain, cb_chain_cb2);

	value = 0;
	
	/*
	 * Chain runs callback:
	 * 1 + 3 = 4
	 */
	cb_chain_run(0, &cb_chain);
	if (value != 4) {
		fprintf(stderr, "value=%d\n", value);
		return -1;
	}

	cb_chain_add(&cb_chain, cb_chain_cb1, &value);
	cb_chain_add(&cb_chain, cb_chain_cb1, &value);
	cb_chain_add(&cb_chain, cb_chain_cb1, &value);

	value = 0;
	
	/**

	 * Chain runs callback:
	 * 1 + 1 + 1 + 1 + 3 = 7
	 */
	cb_chain_run(0, &cb_chain);
	if (value != 7) {
		fprintf(stderr, "value=%d\n", value);
		return -1;
	}

	cb_chain_remove(&cb_chain, cb_chain_cb1);
	
	value = 0;
	
	/*
	 * Chain runs callback:
	 * 3 = 3
	 */
	cb_chain_run(0, &cb_chain);
	if (value != 3) {
		fprintf(stderr, "value=%d\n", value);
		return -1;
	}

	cb_chain_destroy(&cb_chain);

	value = 0xDEADBEEF;

	/*
	 * Chain runs 0 callback, value is unchanged
	 */
	cb_chain_run(0, &cb_chain);
	if (value != 0xDEADBEEF) {
		fprintf(stderr, "value=%d\n", value);
		return -1;
	}

	return 0;
}

int main(int argc, char *argv[])
{
	if (cb_chain_test())
		return 1;

	return 0;
}
