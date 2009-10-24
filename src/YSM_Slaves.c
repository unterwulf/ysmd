/*	$Id: YSM_Slaves.c,v 1.57 2005/09/04 01:36:48 rad2k Exp $	*/
/*
-======================== ysmICQ client ============================-
		Having fun with a boring Protocol
-======================== YSM_Slaves.c =============================-


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
__RCSID("$Id: YSM_Slaves.c,v 1.57 2005/09/04 01:36:48 rad2k Exp $");

#include "YSM_Lists.h"
#include "YSM_Slaves.h"
#include "YSM_Wrappers.h"
#include "YSM_ToolBox.h"
#include "YSM_Setup.h"
#include "YSM_Win32.h"

extern YSM_SLAVE *YSMSlaves_First,*YSMSlaves_LastRead,*YSMSlaves_LastSent;

u_int16_t arr_status[] = {
		STATUS_OFFLINE,
		STATUS_UNKNOWN,
		STATUS_INVISIBLE,
		STATUS_DND,
		STATUS_OCCUPIED,
		STATUS_FREE_CHAT,
		STATUS_NA,
		STATUS_AWAY,
		STATUS_ONLINE	
		};
	

/* 	Instead of behaving the very same way that YSM_PrintSlaves,
	when supplied with YSM_OFFLINE (print them all) the output is
	still organized :) 						
 */

void
YSM_PrintOrganizedSlaves( u_int16_t	FilterStatus,
			int8_t		*Fstring,
			int8_t		FilterIgnore )
{
int32_t x = 0;
#ifdef COMPACT_DISPLAY
int32_t y = 0;
#endif

	/* We increase x so we skip the first array member (offline) */
	if (FilterStatus != STATUS_OFFLINE) x++;

#ifndef COMPACT_DISPLAY
	if (FilterStatus == STATUS_OFFLINE && (Fstring == NULL))
		PRINTF( VERBOSE_BASE,
			"%s %d SLAVES:\n",
			MSG_SLAVES_LIST,
			List_amountSLAVE );

	else if (Fstring != NULL);

	else
		PRINTF( VERBOSE_BASE,
			"%s %d\n",
			MSG_SLAVES_ONLINE,
			g_sinfo.onlineslaves);
#endif


#ifndef COMPACT_DISPLAY
	for ( ; x < NUM_ELEM_ARR(arr_status); x++)
		YSM_PrintSlaves( arr_status[x], Fstring, FilterIgnore );
#else
	for ( y = NUM_ELEM_ARR(arr_status) - 1; y >= x; y--)
		YSM_PrintSlaves( arr_status[y], Fstring, FilterIgnore );
	PRINTF(VERBOSE_BASE, "\n");
#endif
}


/*	Let me explain the idea of Filtering (in YSM_PrintSlaves) */
/*	Since PrintSlaves is usually called from PrintOrganizedSlaves*/
/*	function, YSM_PrintSlaves will always filter and only print */
/*	those slaves with status equal to FilterStatus, UNLESS */
/*	STATUS_OFFLINE is specified. Which means 'PRINT THEM ALL' */
/*	without worring about any order */

/* 	Fstring (FilterString) when specified will make PrintSlaves */
/*	only display those slaves that start with the */
/*	String specified. */

/*	FilterIgnore if true will skip those slaves in your ignore */
/*	list.	*/

void
YSM_PrintSlaves( u_int16_t FilterStatus, int8_t *Fstring, int8_t FilterIgnore )
{
u_int32_t	x = 0, y = 0;
int8_t		SlaveStatus[MAX_STATUS_LEN];
int8_t		*slaveCol = "";
#ifdef COMPACT_DISPLAY
char		sl_buf[MAX_SLAVELIST_LEN];	/* print here first */
#endif
YSM_SLAVE	*node = plist_firstSLAVE;

	if (node == NULL)
		return;		/* empty list! */

#ifdef COMPACT_DISPLAY
	/* print the status string to the status char buffer */
	YSM_WriteStatus(FilterStatus, SlaveStatus);

	/* print the color escape code for this status */
	strncpy(sl_buf, YSM_GetColorStatus(SlaveStatus, NULL), sizeof(sl_buf) - 1);
	sl_buf[sizeof(sl_buf) - 1] = '\0';

	/* print the name of the status */
	strncat(sl_buf, SlaveStatus, sizeof(sl_buf) - 1 - strlen(sl_buf));

	/* print escape to normal color */
	strncat(sl_buf, NORMAL, sizeof(sl_buf) - 1 - strlen(sl_buf));

	/* colon */
	strncat(sl_buf, ": ", sizeof(sl_buf) - 1 - strlen(sl_buf));
#endif

	/* XXX: alejo: cambiar esto a !node->next */
	for( x = 0; x < List_amountSLAVE; x ++ ) {

		if( Fstring != NULL) {
			if (strncasecmp( node->info.NickName,
					Fstring,
					strlen(Fstring) ) != 0) {

			    	if(node->next != 0) {
               				node = node->next;
					continue;
				}	
				else break;
			}
		}

		if (FilterIgnore && node->BudType.IgnoreID) {
			if (node->next != 0) {
				node = node->next;
				continue;
			}
			else return;
		}

		YSM_WriteStatus(node->status, SlaveStatus);

		if ((FilterStatus == STATUS_UNKNOWN 
			&& !YSM_IsValidStatus(node->status))
			|| (node->status == FilterStatus)
			|| (FilterStatus == STATUS_NA && node->status == STATUS_NA2))
#ifndef COMPACT_DISPLAY
		{
			if (y % 3 == 0) PRINTF(VERBOSE_BASE,"\n");
			y++;

			if (node->Color != NULL)
				slaveCol = node->Color;

			PRINTF( VERBOSE_BASE,
			"[%s%-10.10s"
			" "
			"%s%-2.2s" NORMAL
			" "
			"%-1.1s%-1.1s-%-1.1s%-1.1s-%-2.2s]  %s",
			slaveCol,
			node->info.NickName, 
			YSM_GetColorStatus(SlaveStatus, NULL),
			SlaveStatus,
			node->flags & FL_LOG ? "L" : "",
			node->flags & FL_ALERT ? "A" : "",
			node->BudType.VisibleID ? "V" : "",
			node->BudType.InvisibleID ? "I" : "",
			node->BudType.IgnoreID ? "IG" : "",
			(node->BudType.birthday) 
			? "[ " MSG_SLAVES_BIRTHDAY " ]"  : "");
		}
#else
		{

			if (y > 0)
				strncat(sl_buf, ", ", sizeof(sl_buf) - 1
				    - strlen(sl_buf));

			y++;	/* increase count of printed slaves */

			/* print the color escape code for this slave */
			/* so nicks appear in respective slave color */
			if (node->Color != NULL)
				slaveCol = node->Color;

			strncat(sl_buf, slaveCol,
			    sizeof(sl_buf) - 1 - strlen(sl_buf));

			/* nicks */
			strncat(sl_buf, node->info.NickName, sizeof(sl_buf)
			    - 1 - strlen(SlaveStatus));

			/* print escape to normal color */
			strncat(sl_buf, NORMAL, sizeof(sl_buf) - 1
			    - strlen(sl_buf));

			if (node->BudType.IgnoreID ||
			    node->BudType.InvisibleID ||
			    node->BudType.VisibleID ||
			    node->BudType.birthday ) {

				/* print special attrs */
				strncat(sl_buf, "( ", sizeof(sl_buf) - 1
				    - strlen(SlaveStatus));

				if (node->BudType.IgnoreID)
					strncat(sl_buf, "IG ", sizeof(sl_buf)
					    - 1 - strlen(sl_buf));
				if (node->BudType.InvisibleID)
					strncat(sl_buf, "IN ", sizeof(sl_buf)
					    - 1 - strlen(sl_buf));
				if (node->BudType.VisibleID)
					strncat(sl_buf, "VI ", sizeof(sl_buf)
					    - 1 - strlen(sl_buf));
				if (node->BudType.birthday) 
					strncat(sl_buf, MSG_SLAVES_BIRTHDAY " ",
					    sizeof(sl_buf) - 1
					    - strlen(sl_buf));

				/* print special attrs */
				strncat(sl_buf, ")", sizeof(sl_buf) - 1
				    - strlen(SlaveStatus));
			}
		}
#endif

#ifndef COMPACT_DISPLAY
		/* for birthday slaves: dont let anything be after them 
		 * because it screws the listing. Plus, dont let a slave
		 * be in the third column.
		 */
		if (node->BudType.birthday 
		|| (node->next != NULL && node->next->BudType.birthday)) {
		 	while(y % 3) y++;
		}
#endif
	
        	if(node->next != 0)
               		node = node->next;	
		
		else break;
	}

	/* print the line */
	if (y > 0)
#ifndef COMPACT_DISPLAY
		PRINTF(VERBOSE_BASE, "\n");
#else
		PRINTF(VERBOSE_BASE, "%s ", sl_buf);
#endif
}


/* 	The fl parameter is a DownloadedFlag, which:		*/
/*	if TRUE tells the contact was downloaded from the srv. 	*/
	
YSM_SLAVE *
YSM_AddSlavetoList( char *Nick,
		uin_t	Uin,
		char	*flags,
		char	*c_key,
		int8_t	*color,
		int	budID,
		int	grpID,
		int	budtype,
		int	fl )
{
YSM_SLAVE	*new = NULL,	*res = NULL;
u_int32_t	x = 0, keylen = 0;
int32_t		retval = 0;
int8_t		StringUIN[MAX_UIN_LEN+1], goodKey[64];
	
	memset(StringUIN, 0, MAX_UIN_LEN+1);
	snprintf(StringUIN, MAX_UIN_LEN, "%d", (int)Uin);
	StringUIN[sizeof(StringUIN) - 1] = 0x00;

	/* First Seek if theres another Slave with the same UIN (Duh!) */
	if((res = YSM_FindSlaveinList(NULL, Uin)) != NULL) {

		/* User already exists. Since this function is also  
		 * called for downloaded slaves, a slave might be stored 
		 * in the config AND in the server. So if its a downloaded 
		 * slave just update the information from the config one 
		 * else return FALSE, it already exists 
		 */
		if (!fl) return FALSE;

		/* Set it as downloaded ! */
		res->DownloadedFlag = TRUE;

		/* Updating User Special properties	*/
		switch(budtype)
		{
			case YSM_BUDDY_SLAVE_VIS:	/* Buddy Visible */
				res->BudType.VisibleID = budID;
				break;

			case YSM_BUDDY_SLAVE_INV:	/* Buddy Invisible */
				res->BudType.InvisibleID = budID;
				break;

			case YSM_BUDDY_SLAVE_IGN:	/* Buddy ignored */
				res->BudType.IgnoreID = budID;
				break;

			default:			/* Buddy Normal */
				break;
		}

		/* Set its buddy ID! */	
		res->BudType.BudID = budID;

		/* Now set its group ID (required when deleting from srv) */
		res->BudType.grpID = grpID;

		return FALSE;
	}

	/* Now with the same nick (Conflict!) */
	/* XXX: alejo: shouldn't we warn the user ? */
	if(YSM_FindSlaveinList(Nick, 0) != NULL)
		Nick = &StringUIN[0];
	

	new = (YSM_SLAVE *)YSM_Calloc(1, sizeof(YSM_SLAVE), __FILE__, __LINE__);
	new->Uin = Uin;

	/* no need to end in zero, called Calloc before */
	strncpy(new->info.NickName, Nick, sizeof(new->info.NickName) - 1);

	new->status = STATUS_OFFLINE;
	new->flags = 0;
	new->d_con.rSocket = -1;

	new->DownloadedFlag = fl;
	
	new->Color = color;

	/* Set the encryption key if any */
	if (c_key != NULL) {

		/* no need to end in zero, called Calloc before */
		strncpy(new->crypto.strkey, 
			c_key,
			sizeof(new->crypto.strkey) - 1);

		/* ACTION:
		 *	since the key might be smaller than 64 bytes long
		 *	we make sure the final key is at least 64 bytes long by
		 *	repeating the key n amount of times as neccesary.
		 */

		keylen = strlen(new->crypto.strkey);
	
		for (x = 0; x < sizeof(goodKey); x++) 
			goodKey[x] = new->crypto.strkey[x % keylen];

		/* ACTION:
		 *	Try to make both in and out keys.
		 *	At this point we can have a valid or invalid key
		 * 	either because the user added it manually to the 
		 * 	configuration file or due to whatever reason.
		 */

		retval = makeKey(&new->crypto.key_out,
				DIR_ENCRYPT,
				256,
				goodKey);

		if (retval == TRUE) {
			/* OUT key instance created successfully.
			 * Proceed to create the second key */
	
			retval = makeKey( &new->crypto.key_in,
						DIR_DECRYPT,
						256,
						goodKey);
		}

		/* ACTION:
		 *	Check if any of the keys failed creating. We don't mind
		 *	telling at this point which of them failed. The user 
		 *	doesn't really care.
		 */

		if (TRUE != retval) {
			switch (retval) {
				case BAD_KEY_DIR:
					/* bad key direction */
				case BAD_KEY_MAT:
					/* key material length is incorrect */
				case BAD_KEY_INSTANCE:
					/* invalid supplied key */
				default:
					/* unknown */	
					break;
			}

			PRINTF( VERBOSE_BASE, 
				"Error setting cipher key for slave: %s.\n"
				"Please check the key meets the requirements by"
				"\nusing the 'help key' command.\n",
				new->info.NickName
				);
		}
	}

	/* Updating User Special properties	*/
	switch (budtype) {

		case YSM_BUDDY_SLAVE_VIS:	/* Buddy Visible */
			new->BudType.VisibleID = budID;
			break;

		case YSM_BUDDY_SLAVE_INV:	/* Buddy Invisible */
			new->BudType.InvisibleID = budID;
			break;

		case YSM_BUDDY_SLAVE_IGN:	/* Buddy ignored */
			new->BudType.IgnoreID = budID;
			break;

		default:			/* Buddy Normal */
			break;
	}


	/* Set its buddy ID! */	
	new->BudType.BudID = budID;

	/* Now set its group ID (required when deleting from srv) */
	new->BudType.grpID = grpID;

	/* Set slave flags! */
	if (flags) YSM_SlaveFlags( new, flags, 1, 0 );

	return List_addSLAVE( new );
}

/* Free the slave from the linked list of slaves */
void YSM_FreeSlavefromList( YSM_SLAVE *node )
{
	List_delSLAVE( node );
}

void
YSM_DeleteSlavefromList(char *Nick, uin_t Uin)
{
	/* Pretty important. (for the a and r commands specially) */
	if (YSMSlaves_LastSent != NULL) {
		if(!strcasecmp(YSMSlaves_LastSent->info.NickName, Nick))
				YSMSlaves_LastSent = NULL;
	}

	if (YSMSlaves_LastRead != NULL) {
		if(!strcasecmp(YSMSlaves_LastRead->info.NickName, Nick))
					YSMSlaves_LastSent = NULL;
	}

	YSM_FreeSlavefromList(YSM_FindSlaveinList(NULL, Uin));
}

YSM_SLAVE *
YSM_FindSlaveinList( char *Nick, uin_t Uin )
{
u_int32_t	i;
YSM_SLAVE	*node = plist_firstSLAVE;

	for (i = 1; node != NULL; i++) {
		if (Nick == NULL) {
			if(node->Uin == Uin) return node;
		} else {
			if(!strcasecmp(node->info.NickName, Nick))
				return node;
		}

		node = node->next;
	}

	return NULL;
}

int32_t
YSM_ParseSlave( u_int8_t *name )
{
u_int8_t	*token = NULL, *obuf = NULL;
int32_t		size = 0, origsize = 0, x = 0, y = 0;

        size = origsize = strlen(name);
        obuf = YSM_Calloc(1, size + 1, __FILE__, __LINE__);
        token = strtok(name," ");
	
        while(token) {
                strncat( obuf, token, (size+1) - strlen(obuf) - 1);
                token = strtok(NULL," ");
		if (token != NULL) size--;	/* remove the 0x20 */
        }

        memset(name, 0, origsize);

        for (x = 0; x < size; x++) {
		if(isalnum(obuf[x]) && obuf[x] > 32) {
			name[y] = obuf[x];
                        y++;
                }
        }

        YSM_Free(obuf, __FILE__, __LINE__);
	obuf = NULL;
        return y;
}

void
YSM_SlaveFlags( YSM_SLAVE *victim, char *flags, char add, char update )
{
	/* FL_ALERT flag */
	if (strchr(flags, 'a')) {
		if (add) victim->flags |= FL_ALERT;
		else victim->flags ^= FL_ALERT;
	}

	/* FL_LOG flag */
	if (strchr(flags, 'l')) {
		if (add) victim->flags |= FL_LOG;
		else victim->flags ^= FL_LOG;
	}

	if (update) YSM_UpdateSlave( UPDATE_SLAVE, NULL, victim->Uin );
}

int
YSM_UpdateSlave( char type, char *data, uin_t r_uin )
{
YSM_SLAVE	*result = NULL;

	if (type == UPDATE_NICK) {
		/* Can't rename to an existing name */
		result = YSM_FindSlaveinList(data, 0);
		if(result) return -1;
	}

	result = YSM_FindSlaveinList( NULL, r_uin );
	if (!result) return -1;

	/* We remove the slave from the config file but not from memory */
	YSM_DelSlave( result, 0);

	/* We update the information on memory */
	switch (type) 
	{
		case UPDATE_NICK:

		strncpy(result->info.NickName,
			data,
			sizeof(result->info.NickName) - 1);

		result->info.NickName[sizeof(result->info.NickName)-1] = '\0';
		break;

		default:
		break;
	}

	/* We re-add the slave in the config file with the new data */
	YSM_AddSlavetoDisk( result );

	return 0;
}

