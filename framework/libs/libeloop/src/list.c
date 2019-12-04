#include "list.h"

void list_add(struct list_head *head, struct list_head *item)
{
    item->next = head->prev->next;
    head->prev->next = item;
    item->prev = head->prev;
    head->prev = item;
}

void list_del(struct list_head *item)
{
    item->prev->next = item->next;
    item->next->prev = item->prev;
    item->next = item;
    item->prev = item;
}
