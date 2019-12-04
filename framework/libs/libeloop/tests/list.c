#include <list.h>

#include <eloop.h>
#include <stdio.h>
#include <stdlib.h>

#define arr_len(arr) \
    (sizeof(arr)/sizeof((arr)[0]))

struct int_node {
    struct list_head list;
    int data;
};

struct int_node *create_node(int data)
{
    struct int_node *node = malloc(sizeof(*node));
    if (!node)
        return NULL;

    node->data = data;

    return node;
}

struct int_node *add_node_last(struct list_head *list, int data)
{
    struct int_node *node = create_node(data);
    if (!node) {
        return NULL;
    }

    list_add_last(list, (struct list_head *)node);

    return node;
}

struct int_node *add_node_first(struct list_head *list, int data)
{
    struct int_node *node = create_node(data);
    if (!node) {
        return NULL;
    }

    list_add_first(list, (struct list_head *)node);

    return node;
}

static void print_list(struct list_head *list)
{
    struct list_head *itr;

    printf("* ");
    list_for_each(list, itr) {
        printf("%d -> ", ((struct int_node *)itr)->data);
    }
    printf("#\n");
}

static void print_list_rev(struct list_head *list)
{
    struct list_head *itr;

    printf("# ");
    list_for_each_rev(list, itr) {
        printf("%d <- ", ((struct int_node *)itr)->data);
    }
    printf("*\n");
}

static int fill_list(struct list_head *list, int data[], int len)
{
    int n;

    printf("Inserting: ");
    for (n = 0; n < len; n++) {
        if (!add_node_last(list, data[n])) {
            printf("<Allocation error>");
            break;
        }

        printf("%d ", data[n]);
    }

    printf("#\n");

    return n;
}

static void clean_list(struct list_head *list)
{
    struct list_head *itr, *item;

    printf("Removed: ");
    list_for_each_safe(list, itr, item) {
        struct int_node *node = (struct int_node *)item;
        int n = node->data;

        list_del(item);
        free(node);

        printf("%d ", n);
    }

    printf("#\n");
}

int main(int argc, char *argv[])
{
    int data[] = {4, 7, 18, 10, 12, 6, 11};
    list_new(numbers);

    fill_list(&numbers, data, arr_len(data));
    print_list(&numbers);

    add_node_last(&numbers, 14);
    print_list(&numbers);

    add_node_first(&numbers, 3);
    print_list(&numbers);

    print_list_rev(&numbers);
    clean_list(&numbers);

    return 0;
}
