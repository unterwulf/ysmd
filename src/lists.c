/*	$Id: YSM_Lists.c,v 1.12 2005/08/01 03:44:03 rad2k Exp $	*/
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

#include "YSM.h"
__RCSID("$Id: YSM_Lists.c,v 1.12 2005/08/01 03:44:03 rad2k Exp $");

#include "YSM_Lists.h"
#include "YSM_Wrappers.h"
#include "YSM_ToolBox.h"

YSM_SLAVE	*plist_firstSLAVE = NULL;
YSM_COMMAND	*plist_firstCOMMAND = NULL;
FileMap		*plist_firstFILEMAP = NULL;

u_int32_t	List_amountSLAVE = 0, List_amountCOMMAND = 0;
u_int32_t	List_amountFILEMAP = 0;

void
List_init( void )
{

	/* COMMAND LIST */

	plist_firstCOMMAND = (YSM_COMMAND *)YSM_Calloc( 1,
						sizeof(YSM_COMMAND),
	    					__FILE__,
						__LINE__ );

	/* NULL CHECK */

	if ( plist_firstCOMMAND == NULL ) {
		fprintf(stderr, "Can't alloc memory: %s.\n", strerror(errno));
		YSM_Error(ERROR_CRITICAL, __FILE__, __LINE__, 1);
		/* NOTREACHED */
	}

}

void
List_freelistSLAVE( void )
{
u_int32_t	x = 0;
YSM_SLAVE	*next, *node = plist_firstSLAVE;

	for ( x = 0; x < List_amountSLAVE; x++ ) {
		if (!node) break;
		next = node->next;
		List_delSLAVE( node );
		node = next;
	}
}

void
List_freelistCOMMAND( void )
{
YSM_COMMAND	*next, *node = plist_firstCOMMAND;
u_int32_t	x = 0;

	for ( x = 0; x < List_amountCOMMAND; x++ ) {
		if (!node) break;
		next = node->next;
		List_delCOMMAND( node );
		node = next;
	}
}

void
List_freelistFILEMAP( void )
{
FileMap		*next, *node = plist_firstFILEMAP;
u_int32_t	x = 0;

	for ( x = 0; x < List_amountFILEMAP; x++ ) {
		if (!node) break;
		next = node->next;
		List_delFILEMAP( node );
		node = next;
	}
}

void
List_freelist( void )
{
	List_freelistCOMMAND();
	List_freelistSLAVE();
	List_freelistFILEMAP();
}

YSM_SLAVE *
List_addSLAVE( YSM_SLAVE *new )	/* inserts ordered */
{

	if (plist_firstSLAVE == NULL) {

		/* list empty, insert as first */
		new->next = NULL; 
		new->back = NULL;
		plist_firstSLAVE = new;

	} else {
		YSM_SLAVE	*n, *nprev;

		nprev = NULL;
		n = plist_firstSLAVE;

		/* find position; if equal, add behind */
		while (n != NULL && strcasecmp(new->info.NickName,
		    n->info.NickName) >= 0) {

			nprev = n;
			n = n->next;
		}

		if (nprev == NULL) {

			/* insert first */
			new->next = n; 
			new->back = NULL; 
			n->back = new;
			plist_firstSLAVE = new;

		} else if (n == NULL) {

			/* insert last */
			nprev->next = new;
			new->back = nprev;
			new->next = NULL; 

		} else {

			/* insert middle */
			nprev->next = new;
			new->back = nprev; 
			new->next = n; 
			n->back = new; 
		}
	}

	List_amountSLAVE++;

	return (new);
}

YSM_COMMAND *
List_addCOMMAND( YSM_COMMAND *node )	/* inserts in head */
{
	node->next = plist_firstCOMMAND; 
	node->next->back = node;

	plist_firstCOMMAND = node;

	List_amountCOMMAND++;
	return node;
}

pFileMap
List_addFILEMAP( pFileMap new )
{
	if (plist_firstFILEMAP == NULL) {
		/* list empty, insert as first */
		new->next = NULL; 
		new->back = NULL;
		plist_firstFILEMAP = new;
	} else {
		/* add as head */
		new->next = plist_firstFILEMAP; 
		new->next->back = new;
		plist_firstFILEMAP = new;
	}

	List_amountFILEMAP++;
	return (new);
}

void
List_delSLAVE( YSM_SLAVE *node )
{

	if (node != plist_firstSLAVE) {

		node->back->next = node->next;

		// could be the last slave in the list..therefore next might be null.
		if (node->next != 0x00)
			node->next->back = node->back;

	} else {

		/* check if this is the last one in list */
		if (node->next != NULL)
			node->next->back = NULL;

		plist_firstSLAVE = node->next;
	}

	YSM_Free(node, __FILE__, __LINE__);
	node = NULL;
	List_amountSLAVE--;
}

void
List_delCOMMAND( YSM_COMMAND *node )
{
YSM_COMMAND	*new = NULL;

	if (node != NULL) {

		if (node != plist_firstCOMMAND) {
			/* nodo sea el siguiente del nodo victima */
			new = node->next;
			node->back->next = new;

			/* Eliminamos el primero */
			YSM_Free(node, __FILE__, __LINE__);
			node = NULL;
		} else {
			new = node->next;
			YSM_Free(node, __FILE__, __LINE__);
			node = new;
			plist_firstCOMMAND = node;
		}
		
		List_amountCOMMAND--;
	}
}

void
List_delFILEMAP( pFileMap node )
{
	if (node != plist_firstFILEMAP) {
		node->back->next = node->next;
		node->next->back = node->back;

	} else {
		/* check if this is the last one in list */
		if (node->next != NULL)
			node->next->back = NULL;

		plist_firstFILEMAP = node->next;
	}

	if (node->data != NULL)
		YSM_Free(node->data, __FILE__, __LINE__);

	YSM_Free(node, __FILE__, __LINE__);
	node = NULL;
	List_amountFILEMAP--;
}

