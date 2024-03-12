#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>

#include "fsm.h"

static void fsm_cb_enter(int id, void *data)
{
	int *value = (int *)data;

	*value += 1111;
}

static void fsm_cb_exit(int id, void *data)
{
	int *value = (int *)data;

	*value -= 1111;
}

int fsm_test(void)
{
	struct fsm fsm;
	int value0, value1, value2, value3;

	fsm_init(&fsm);

	if (fsm_add_state(&fsm, 0, &value0))
		return -1;

	if (fsm_add_action(&fsm, 0, fsm_cb_enter, fsm_cb_exit))
		return -1;

	if (fsm_add_action(&fsm, 0, NULL, fsm_cb_exit))
		return -1;

	if (fsm_add_action(&fsm, 0, fsm_cb_enter, NULL))
		return -1;

	if (fsm_add_state(&fsm, 75000, &value1))
		return -1;

	if (fsm_add_action(&fsm, 75000, fsm_cb_enter, fsm_cb_exit))
		return -1;

	if (fsm_add_state(&fsm, 85000, &value2))
		return -1;

	if (fsm_add_action(&fsm, 85000, fsm_cb_enter, fsm_cb_exit))
		return -1;

	if (fsm_add_state(&fsm, 95000, &value3))
		return -1;

	if (fsm_add_action(&fsm, 95000, fsm_cb_enter, fsm_cb_exit))
		return -1;
	
	value0 = 0, value2 = 0;
	fsm_next(&fsm, 0, 85000);
	/*
	 * Given actions:
	 * value0(0) -= 2 x 1111 => -2222
	 * value2(0) += 1 x 1111 = 1111
	 */
	if (value0 != -2222 || value2 != 1111)
		return 1;

	value1 = 0;
	fsm_next(&fsm, 85000, 75000);
	/*
	 * Given actions:
	 * value2(1111) -= 1111 = 0
	 * value1(0) += 1111 = 1111
	 */
	if (value1 != 1111 || value2 != 0)
		return 1;

	fsm_next(&fsm, 75000, 85000);
	/*
	 * Given actions:
	 * value1(1111) -= 1111 = 0
	 * value2(0) += 1111 = 1111
	 */
	if (value1 != 0 || value2 != 1111)
		return 1;

	value3 = 0;
	fsm_next(&fsm, 95000, 75000);
	/*
	 * Given actions:
	 * value1(0) += 1111 = 1111
	 * value3(0) -= 1111 = -1111
	 */
	if (value1 != 1111 || value3 != -1111)
		return 1;

	fsm_next(&fsm, 75000, 95000);
	/*
	 * Given actions:
	 * value1(1111) -= 1111 = 0
	 * value3(1111) += 1111 = 0
	 */
	if (value1 != 0 || value3 != 0)
		return 1;
	
	fsm_next(&fsm, 75000, 0);
	/*
	 * Given actions:
	 * value0(2222) += 2 x 1111 = 0
	 * value1(0) -= 1111 = -1111
	 */
	if (value1 != -1111 || value0 != 0)
		return 1;

	
	return 0;
}

int main(int argc, char *argv[])
{
	if (fsm_test())
		return 1;

	return 0;
}
