#include <stdio.h>
#include "list.h"

void list_init(struct list *list)
{
	list->prev = list->next = NULL;
	list->tail = list;
}

struct list *list_tail(struct list *list)
{
	return list ? list->tail : NULL;
}

struct list *list_next(struct list *list)
{
	return list ? list->next : NULL;
}

struct list *list_prev(struct list *list)
{
	return list ? list->prev : NULL;
}

void list_add_head(struct list *head, struct list *new)
{
	new->next = head->next;
	new->prev = head;
	head->next = new;
}

void list_add_tail(struct list *head, struct list *new)
{
	new->prev = head->tail;
	new->next = NULL;
	head->tail->next = new;
	head->tail = new;
}

void list_remove(struct list *list)
{
	if (list->prev)
		list->prev->next = list->next;

	if (list->next)
		list->next->prev = list->prev;
}
