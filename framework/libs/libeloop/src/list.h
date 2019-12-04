#ifndef __LIST_H__
#define __LIST_H__

struct list_head {
    struct list_head *next;
    struct list_head *prev;
};

#define list_new(list) \
    struct list_head list = { .next = &(list), .prev = &(list) }

#define list_init(list) \
    { (list)->next = list; (list)->prev = list; }

#define list_is_empty(list) \
    ((list)->next == list)

#define list_for_each(head, itr) \
    for (itr = (head)->next; itr != (head); itr = itr->next)

#define list_for_each_rev(head, itr) \
    for (itr = (head)->prev; itr != (head); itr = itr->prev)

#define list_for_each_safe(head, itr, item) \
    for (item = (head)->next, itr = item->next; \
            item != (head); item = itr, itr = itr->next)

#define list_first(head) \
    ((head)->next)

#define list_is_last(head, item) \
    ((item)->next == (head))

#define list_is_first(head, item) \
    ((item)->prev == (head))

void list_add(struct list_head *head, struct list_head *item);
void list_del(struct list_head *item);

#define list_add_last(head, item) \
    list_add((head), (item))

#define list_add_first(head, item) \
    list_add((head)->next, (item))

#endif /* __LIST_H__ */
