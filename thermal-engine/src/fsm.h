#ifndef __FSM_H__
#define __FSM_H__

#include "cb_chain.h"
#include "pair.h"

struct fsm {
	struct pair states;
};

struct fsm_state;

void fsm_state_init(struct fsm_state *fsm_state);
int fsm_next(struct fsm *fsm, int from, int to);
void fsm_init(struct fsm *fsm);
int fsm_add_state(struct fsm *fsm, int key, void *data);
int fsm_add_action(struct fsm *fsm, int key, callback_t enter, callback_t exit);

#endif /* __FSM_H__ */
