#include "olist.h"
#include <stdlib.h>

void olist_add(struct olist_head *head,
               struct olist_head *item, void *key)
{
    struct olist_head *itr;
    int done = 0;

    item->cmp_cb = head->cmp_cb;
    item->key = key;
    olist_for_each(head, itr) {
        if (head->cmp_cb(itr->key, key) < 0)
            continue;

        list_add_first(itr->head.prev, &item->head);
        done = 1;
        break;
    }

    if (!done)
        list_add_last(&head->head, &item->head);
}

struct olist_head *olist_find(struct olist_head *head, void *key)
{
    struct olist_head *item = NULL, *itr;

    olist_for_each(head, itr) {
        int res = head->cmp_cb(itr->key, key);
        if (res < 0)
            continue;

        if (!res)
            item = itr;

        break;
    }

    return item;
}

struct olist_head *olist_del_by_key(struct olist_head *head, void *key)
{
    struct olist_head *item = olist_find(head, key);
    if (item)
        olist_del(item);

    return item;
}

