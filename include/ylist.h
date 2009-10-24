#ifndef _YLIST_H_
#define _YLIST_H_

#define COMMON_LIST \
    struct _list_node_t *pre; \
    struct _list_node_t *suc;

typedef struct _list_node_t
{
    COMMON_LIST
} list_node_t;

typedef struct
{
    list_node_t *start;
    uint32_t     length;
} list_t;

void         freeList(list_t *list);
void         deleteListNode(list_t *list, list_node_t *node);
list_node_t *unshiftListNode(list_t *list, list_node_t *node);

list_node_t *insertListNodeAfter(
    list_t      *list,
    list_node_t *node,
    list_node_t *pre);

#endif /* _YLIST_H_ */
