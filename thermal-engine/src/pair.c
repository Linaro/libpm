#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <errno.h>

#include "pair.h"

static struct pair *pair_next(struct pair *pair)
{
	struct list *l;

	l = list_next(&pair->list);
	if (!l)
		return NULL;

	return container_of(l, struct pair, list);
}

static struct pair *pair_prev(struct pair *pair)
{
	struct list *l;

	l = list_prev(&pair->list);
	if (!l)
		return NULL;

	return container_of(l, struct pair, list);
}

static struct pair *pair_tail(struct pair *pair)
{
	struct list *l;

	l = list_tail(&pair->list);
	if (!l)
		return NULL;

	return container_of(l, struct pair, list);
}

static struct pair *__pair_find(struct pair *pair, int key)
{
	if (!pair)
		return NULL;

	pair = pair_next(pair);

	while (pair) {
		if (pair->key == key)
			return pair;

		pair = pair_next(pair);
	}

	return NULL;
}

void *pair_find(struct pair *pair, int key)
{
	pair = __pair_find(pair, key);
	if (!pair)
		return NULL;

	return pair->data;
}

int pair_add(struct pair *pair, int key, void *data)
{
	struct pair *p;

	if (!pair)
		return -EINVAL;

	p = __pair_find(pair, key);
	if (p)
		return -EEXIST;

	if (!data)
		return -EINVAL;

	p = malloc(sizeof(*p));
	if (!p)
		return -ENOMEM;

	p->key = key;
	p->data = data;

	list_add_tail(&pair->list, &p->list);
	
	return 0;
}

void pair_remove(struct pair *pair, int key)
{
	struct pair *p;

	p = __pair_find(pair, key);
	if (!p)
		return;

	list_remove(&p->list);
	
	free(p);
}

void pair_init(struct pair *pair)
{
	if (pair)
		list_init(&pair->list);
}

void pair_destroy(struct pair *pair)
{
	if (!pair)
		return;

	pair = pair_tail(pair);

	while (pair && pair_prev(pair)) {
		struct pair *p = pair;
		list_remove(&pair->list);
		pair = pair_prev(pair);
		free(p);
	};
}
