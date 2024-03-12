#ifndef __LIST_H__
#define __LIST_H__

#include <stddef.h>

#define container_of(addr, type, field) ({				\
        const __typeof__( ((type *)0)->field ) *__addr = (addr);\
        (type *)( (char *)__addr - (offsetof(type, field)));})

struct list {
	struct list *next;
	struct list *prev;
	struct list *tail;
};

void list_init(struct list *list);
struct list *list_tail(struct list *list);
struct list *list_next(struct list *list);
struct list *list_prev(struct list *list);
void list_add_head(struct list *head, struct list *new);
void list_add_tail(struct list *head, struct list *new);
void list_remove(struct list *list);
#endif /* __LIST_H__ */
