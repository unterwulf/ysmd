/*
-================================ lists.h ===================================-

YSM (YouSickMe) ICQ Client. An Original Multi-Platform ICQ client.
Copyright (c) 2007 rad2k Argentina.
Copyright (c) 2008 vs Russia

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

#ifndef _LISTS_H_
#define _LISTS_H_

#define COMMON_LIST \
    struct _dl_list_node_t *pre; \
    struct _dl_list_node_t *suc;

typedef struct _dl_list_node_t
{
    COMMON_LIST
} dl_list_node_t;

typedef struct
{
    dl_list_node_t* start;
    u_int32_t length;
} dl_list_t;

void        freelist(dl_list_t *list);
dl_list_node_t * list_unshift(dl_list_t *list, dl_list_node_t *node);

dl_list_node_t * list_insert_after(
    dl_list_t *list,
    dl_list_node_t *node,
    dl_list_node_t *pre);

void        list_delete(dl_list_t *list, dl_list_node_t *node);

#endif
