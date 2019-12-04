#ifndef __OLIST_H__
#define __OLIST_H__

#include "list.h"

/* cmp_cb returns:
 * 0: keys are equal
 * 1: key1 is greater
 * -1: key2 is greater
 */
typedef int (*olist_cmp_cb)(void *key1, void *key2);

struct olist_head {
    struct list_head head;
    void *key;
    olist_cmp_cb cmp_cb;
};

#define olist_new(olist, cb)    \
    struct olist_head olist = { \
        .head = { .next = &(olist).head, .prev = &(olist).head }, \
        .cmp_cb = cb            \
    }

#define olist_init(olist, cb)               \
    { (olist)->head.next = &(olist)->head;  \
      (olist)->head.prev = &(olist)->head;  \
      (olist)->cmp_cb = cb; }

#define olist_is_empty(olist)   \
    list_is_empty(&(olist)->head)

#define olist_for_each(olist, itr)                      \
    for (itr = (struct olist_head *)(olist)->head.next; \
        itr != (olist); itr = (struct olist_head *)itr->head.next)

#define olist_for_each_rev(olist, itr)                  \
    for (itr = (struct olist_head *)(olist)->head.prev; \
        itr != (olist); itr = (struct olist_head *)itr->head.prev)

#define olist_for_each_safe(olist, itr, item)           \
    for (item = (struct olist_head *)(olist)->head.next,\
        itr = (struct olist_head *)item->head.next;     \
        item != (olist); item = itr,                    \
        itr = (struct olist_head *)itr->head.next)

void olist_add(struct olist_head *head,
               struct olist_head *item, void *key);
struct olist_head *olist_find(struct olist_head *head, void *key);

#define olist_del(item) \
    list_del(&(item)->head);

struct olist_head *olist_del_by_key(struct olist_head *head, void *key);
#endif /* __OLIST_H__ */

