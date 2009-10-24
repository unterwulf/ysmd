#include "ysm.h"
#include "ylist.h"
#include "wrappers.h"
#include "toolbox.h"

void freeList(list_t *list)
{
    list_node_t *suc, *node = list->start;

    while (list->length > 0)
    {
        if (!node) break;
        suc = node->suc;
        deleteListNode(list, node);
        node = suc;
    }
}

list_node_t * insertListNodeAfter(
    list_t      *list,
    list_node_t *node,
    list_node_t *pre)
{
    if (list->start == NULL)
    {
        YSM_ERROR(ERROR_CRITICAL, 1);
    }
    else
    {
        /* add between two nodes or at the end of list */
        node->suc = pre->suc;
        pre->suc = node;
        if (node->suc)
            node->suc->pre = node;
    }

    list->length++;

    return node;
}

list_node_t * unshiftListNode(list_t *list, list_node_t *node)
{
    if (list->start == NULL)
    {
        /* list empty, insert as first */
        node->suc = NULL;
        node->pre = NULL;
        list->start = node;
    }
    else
    {
        /* add as head */
        node->suc = list->start;
        node->suc->pre = node;
        list->start = node;
    }

    list->length++;

    return node;
}

void deleteListNode(list_t *list, list_node_t *node)
{
    if (node != list->start)
    {
        node->pre->suc = node->suc;

        // could be the last element in the list..therefore suc might be null.
        if (node->suc != NULL)
            node->suc->pre = node->pre;
    }
    else
    {
        /* check if this is the last one in list */
        if (node->suc != NULL)
            node->suc->pre = NULL;

        list->start = node->suc;
    }

    list->length--;
}
