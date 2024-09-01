/**
 * \file
 * \brief Double linked list interface implementation
 * \author Vladimir Petrigo
 */

#ifndef LIST_H
#define LIST_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#ifdef __GNUC__
#define member_type(type, member) __typeof__(((type *)0)->member)
#else
#define member_type(type, member) const void
#endif

/**
 * Casts a member of a structure out to the containing structure
 * \param ptr the pointer to the member.
 * \param type the type of the container struct this is embedded in.
 * \param member the name of the member within the struct.
 */
#define container_of(ptr, type, member)                                                            \
    ((type *)((char *)(member_type(type, member) *){ptr} - offsetof(type, member)))


struct list_head {
    struct list_node *first;
};

struct list_node {
    struct list_node *next;
    struct list_node **prev;
};

#define LIST_HEAD_INIT_VAR   {.first = NULL}
#define LIST_HEAD_INIT(name) ((name)->first = NULL)
#define LIST_HEAD(name)      struct list_head name = LIST_HEAD_INIT_VAR
#define LIST_HEAD_VAR(name)  struct list_head name

#define LIST_NODE_INIT(node)        ((node)->next = NULL, (node)->prev = NULL)
#define LIST_NODE_IS_DANGLING(node) ((node)->next == NULL && (node)->prev == NULL)

#define LIST_ENTRY(ptr, type, member) container_of(ptr, type, member)
#define LIST_FOR_EACH(ptr, head)      for ((ptr) = (head)->first; (ptr) != NULL; (ptr) = (ptr)->next)

static inline int list_head_empty(const struct list_head *head)
{
    return head->first == NULL;
}

static inline void list_head_add(struct list_head *head, struct list_node *node)
{
    struct list_node *h = head->first;

    node->next = h;

    if (h != NULL) {
        h->prev = &node->next;
    }

    head->first = node;
    node->prev = &head->first;
}

static inline void list_head_del(struct list_node *node)
{
    struct list_node *next = node->next;
    struct list_node **prev = node->prev;

    *prev = next;

    if (next != NULL) {
        next->prev = prev;
    }

    LIST_NODE_INIT(node);
}

#ifdef __cplusplus
}
#endif

#endif /* LIST_H */
