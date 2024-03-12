#ifndef __PAIR_H__
#define __PAIR_H__

#include "list.h"

struct pair {
	int key;
	void *data;
	struct list list;
};

void *pair_find(struct pair *pair, int key);
int pair_add(struct pair *pair, int key, void *data);
void pair_remove(struct pair *pair, int key);
void pair_init(struct pair *pair);
void pair_destroy(struct pair *pair);
#endif /* __PAIR_H__ */
