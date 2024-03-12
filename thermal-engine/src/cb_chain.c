#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "cb_chain.h"

static struct cb_chain *cb_chain_next(struct cb_chain *cb_chain)
{
	struct list *l;

	l = list_next(&cb_chain->list);
	if (!l)
		return NULL;

	return container_of(l, struct cb_chain, list);
}

void cb_chain_init(struct cb_chain *cb_chain)
{
	list_init(&cb_chain->list);
}

int cb_chain_add(struct cb_chain *cb_chain, callback_t callback, void *data)
{
	struct cb_chain *c;

	if (!cb_chain)
		return -EINVAL;

	if (!callback)
		return -EINVAL;

	c = malloc(sizeof(*c));
	if (!c)
		return -ENOMEM;

	list_init(&c->list);
	c->callback = callback;
	c->data = data;

        list_add_tail(&cb_chain->list, &c->list);
	
	return 0;
}

void cb_chain_remove(struct cb_chain *cb_chain, callback_t callback)
{
	cb_chain = cb_chain_next(cb_chain);

	while (cb_chain) {

		struct cb_chain *c = cb_chain;

		cb_chain = cb_chain_next(cb_chain);
		
		if (c->callback == callback) {
			list_remove(&c->list);
			free(c);
		}
	}
}

void cb_chain_run(int id, struct cb_chain *cb_chain)
{
	if (!cb_chain)
		return;

	cb_chain = cb_chain_next(cb_chain);
	
	while (cb_chain) {
		cb_chain->callback(id, cb_chain->data);
		cb_chain = cb_chain_next(cb_chain);
	}
}

void cb_chain_destroy(struct cb_chain *cb_chain)
{
	if (!cb_chain)
		return;

	cb_chain = cb_chain_next(cb_chain);

	while (cb_chain) {
		struct cb_chain *c = cb_chain;

		cb_chain = cb_chain_next(cb_chain);

		list_remove(&c->list);
		free(c);
	};
}
