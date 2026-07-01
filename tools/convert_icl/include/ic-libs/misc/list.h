
#ifndef __IC_LIBS__MISC__LIST_H
#define __IC_LIBS__MISC__LIST_H

#include <stddef.h> /* для offsetof */

#ifdef __cplusplus
extern "C" {
#endif

#define LIST_POISON1  ((struct list_head *) 0x00000001)
#define LIST_POISON2  ((struct list_head *) 0x00000002)


struct list_head {
	struct list_head *next, *prev;
};


#define LIST_HEAD_INIT(name) { &(name), &(name) }

#define LIST_HEAD(name) \
	struct list_head name = LIST_HEAD_INIT(name)

static inline void INIT_LIST_HEAD(struct list_head *list)
{
	list->next = list;
	list->prev = list;
}

static inline void __list_add(struct list_head *new_item,
			      struct list_head *prev,
			      struct list_head *next)
{
	next->prev = new_item;
	new_item->next = next;
	new_item->prev = prev;
	prev->next = new_item;
}

static inline void list_add(struct list_head *new_item, struct list_head *head)
{
	__list_add(new_item, head, head->next);
}

static inline void list_add_tail(struct list_head *new_item, struct list_head *head)
{
	__list_add(new_item, head->prev, head);
}


static inline void __list_del(struct list_head * prev, struct list_head * next)
{
	next->prev = prev;
	prev->next = next;
}

static inline void list_del(struct list_head *entry)
{
	__list_del(entry->prev, entry->next);
	entry->next = LIST_POISON1;
	entry->prev = LIST_POISON2;
}

static inline int list_empty(const struct list_head *head)
{
	return head->next == head;
}

static inline void __list_splice(const struct list_head *list,
				 struct list_head *prev,
				 struct list_head *next)
{
	struct list_head *first = list->next;
	struct list_head *last = list->prev;

	first->prev = prev;
	prev->next = first;

	last->next = next;
	next->prev = last;
}

static inline void list_splice(const struct list_head *list,
				struct list_head *head)
{
	if (!list_empty(list))
		__list_splice(list, head, head->next);
}

#define list_entry(ptr, type, member) \
	((type *)((char *) ptr - offsetof(type, member)))

#define list_first_entry(ptr, type, member) \
	list_entry((ptr)->next, type, member)


#define list_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); pos = pos->next)

#define list_for_each_prev(pos, head) \
	for (pos = (head)->prev; pos != (head);	pos = pos->prev)

#define list_for_each_safe(pos, n, head) \
	for (pos = (head)->next, n = pos->next; pos != (head); \
		pos = n, n = pos->next)

#define list_for_each_prev_safe(pos, n, head) \
	for (pos = (head)->prev, n = pos->prev; pos != (head); \
	     pos = n, n = pos->prev)

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __IC_LIBS__MISC__LIST_H */
