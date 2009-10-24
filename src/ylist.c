/*
-======================== ysmICQ client ============================-
        Having fun with a boring Protocol
-========================= YSM_Lists.c =============================-

YSM (YouSickMe) ICQ Client. An Original Multi-Platform ICQ client.
Copyright (C) 2002 rad2k Argentina.

YSM is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

For Contact information read the AUTHORS file.

*/

#include "ysm.h"
#include "lists.h"
#include "wrappers.h"
#include "toolbox.h"

void freelist(dl_list_t *list)
{
    dl_list_node_t *suc, *node = list->start;

    while (list->length > 0)
    {
        if (!node) break;
        suc = node->suc;
        list_delete(list, node);
        node = suc;
    }
}

dl_list_node_t * list_insert_after(
    dl_list_t      *list,
    dl_list_node_t *node,
    dl_list_node_t *pre)
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

dl_list_node_t * list_unshift(dl_list_t *list, dl_list_node_t *node)
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

void list_delete(dl_list_t *list, dl_list_node_t *node)
{
    if (node != list->start)
    {
        node->pre->suc = node->suc;

        // could be the last element in the list..therefore suc might be null.
        if (node->suc != 0x00)
            node->suc->pre = node->pre;
    }
    else
    {
        /* check if this is the last one in list */
        if (node->suc != NULL)
            node->suc->pre = NULL;

        list->start = node->suc;
    }

    ysm_free(node, __FILE__, __LINE__);
    node = NULL;
    list->length--;
}
