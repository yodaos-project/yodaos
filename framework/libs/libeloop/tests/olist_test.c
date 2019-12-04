#include <olist.h>
#include <eloop.h>

#include <stdio.h>
#include <stdlib.h>

struct int_node {
    struct olist_head list;
    int data;
};

#define arr_len(arr) \
    (sizeof(arr)/sizeof((arr)[0]))

struct int_node *create_node(int data)
{
    struct int_node *node = malloc(sizeof(*node));
    if (!node)
        return NULL;

    node->data = data;

    return node;
}

struct int_node *add_node(struct olist_head *list, int data)
{
    struct int_node *node = create_node(data);
    if (!node) {
        return NULL;
    }

    olist_add(list, (struct olist_head *)node, &node->data);

    return node;
}

static void print_list(struct olist_head *list)
{
    struct olist_head *itr;

    printf("* ");
    olist_for_each(list, itr) {
        printf("%d -> ", ((struct int_node *)itr)->data);
    }
    printf("#\n");
}

static void print_list_rev(struct olist_head *list)
{
    struct olist_head *itr;

    printf("# ");
    olist_for_each_rev(list, itr) {
        printf("%d <- ", ((struct int_node *)itr)->data);
    }
    printf("*\n");
}

static int fill_list(struct olist_head *list, int data[], int len)
{
    int n;

    for (n = 0; n < len; n++) {
        struct int_node *item = add_node(list, data[n]);
        if (!item) {
            printf("<Allocating error>");
            break;
        }
    }

    return n;
}

static void clean_list(struct olist_head *list)
{
    struct olist_head *itr, *item;

    printf("Removed: ");
    olist_for_each_safe(list, itr, item) {
        struct int_node *node = (struct int_node *)item;
        int n = node->data;

        olist_del(item);
        free(node);

        printf("%d ", n);
    }

    printf("#\n");
}

static int remove_node(struct olist_head *list, int data, int *rem)
{
    struct int_node *item =
        (struct int_node *)olist_del_by_key(list, (void *)&data);
    if (!item) {
        return -1;
    }

    *rem = item->data;
    free(item);

    return 0;
}

static void remove_nodes(struct olist_head *list, int arr[], int len)
{
    int n, rem;

    for (n = 0; n < len; n++) {
        if (remove_node(list, arr[n], &rem)) {
            printf("Trying to remove %d, not found\n", arr[n]);
            continue;
        }

        printf("Trying to remove %d, removed %d\n", arr[n], rem);
    }
}

static int int_cmp(void *key1, void *key2)
{
    int val1 = *(int *)key1,
        val2 = *(int *)key2;

    if (val1 > val2)
        return 1;

    if (val1 < val2)
        return -1;

    return 0;
}

int main(int argc, char *argv[])
{
    int data[] = { 7, 18, 4, 10, 12, 6, 11, 20 };
    int removal_list[] = { 4, 13, 20, 10 };
    olist_new(numbers, int_cmp);

    fill_list(&numbers, data, arr_len(data));
    print_list(&numbers);
    print_list_rev(&numbers);

    remove_nodes(&numbers, removal_list, arr_len(removal_list));
    print_list(&numbers);

    clean_list(&numbers);

    return 0;
}
