#ifndef __CB_CHAIN_H__
#define __CB_CHAIN_H__

#include "list.h"

typedef void (*callback_t)(int id, void *data);

struct cb_chain {
	struct list list;
	callback_t callback;
	void *data;
};

void cb_chain_init(struct cb_chain *cb_chain);
int cb_chain_add(struct cb_chain *cb_chain, callback_t callback, void *private);
void cb_chain_remove(struct cb_chain *cb_chain, callback_t callback);
void cb_chain_run(int id, struct cb_chain *cb_chain);
void cb_chain_destroy(struct cb_chain *cb_chain);
#endif /* __CB_CHAIN_H__ */
