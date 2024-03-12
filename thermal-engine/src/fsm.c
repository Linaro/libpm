#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>

#include "fsm.h"

struct fsm_state {
	struct cb_chain cb_chain_enter;
	struct cb_chain cb_chain_exit;
	void *data;
};

void fsm_state_init(struct fsm_state *fsm_state)
{
	cb_chain_init(&fsm_state->cb_chain_enter);
	cb_chain_init(&fsm_state->cb_chain_exit);
	fsm_state->data = NULL;
}

int fsm_next(struct fsm *fsm, int from, int to)
{
	struct fsm_state *from_state;
	struct fsm_state *to_state;

	if (from == to)
		return -EINVAL;
	
	to_state = pair_find(&fsm->states, to);
	if (!to_state)
		return -EEXIST;

	from_state = pair_find(&fsm->states, from);
	if (!from_state)
		return -EEXIST;

	cb_chain_run(from, &from_state->cb_chain_exit);
	cb_chain_run(to, &to_state->cb_chain_enter);

	return 0;
}

void fsm_init(struct fsm *fsm)
{
	pair_init(&fsm->states);
}

int fsm_add_state(struct fsm *fsm, int id, void *data)
{
	struct fsm_state *state;
	int ret;

	if (pair_find(&fsm->states, id))
		return -EEXIST;

	state = malloc(sizeof(*state));
	if (!state)
		return -ENOMEM;

	fsm_state_init(state);
	state->data = data;
	
	ret = pair_add(&fsm->states, id, state);
	if (ret) {
		free(state);
		return ret;
	}

	return 0;
}

int fsm_add_action(struct fsm *fsm, int key, callback_t enter, callback_t exit)
{
	struct fsm_state *fsm_state;
	int ret = 0;

	fsm_state = pair_find(&fsm->states, key);
	if (!fsm_state)
		return -EEXIST;

	if (enter) {
		ret = cb_chain_add(&fsm_state->cb_chain_enter, enter, fsm_state->data);
		if (ret)
			return ret;
	}

	if (exit) {
		ret = cb_chain_add(&fsm_state->cb_chain_exit, exit, fsm_state->data);
		if (ret)
			goto out_remove_enter;
	}

	return 0;

out_remove_enter:
	if (enter)
		cb_chain_remove(&fsm_state->cb_chain_enter, enter);
	
	return ret;
}
