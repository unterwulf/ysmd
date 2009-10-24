/*	$Id: YSM_Commands.c,v 1.94 2005/11/14 01:58:28 rad2k Exp $	*/
/*
-======================== ysmICQ client ============================-
		Having fun with a boring Protocol
-======================== YSM_Commands.c ===========================-


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
__RCSID("$Id: YSM_Commands.c,v 1.94 2005/11/14 01:58:28 rad2k Exp $");

#include "YSM_Lists.h"
#include "YSM_Commands.h"
#include "YSM_Prompt.h"
#include "YSM_Setup.h"
#include "YSM_ToolBox.h"
#include "YSM_Win32.h"
#include "YSM_Slaves.h"
#include "YSM_Wrappers.h"
#include "YSM_Network.h"
#include "YSM_Direct.h"
#include "YSM_Crypt.h" 

extern	YSM_SLAVE *YSMSlaves_LastSent, *YSMSlaves_LastRead, *YSMSlaves_TabSlave;
extern	int8_t	YSM_AFKMessage[MAX_DATA_LEN + 1], YSM_cfgfile[MAX_PATH];
extern	int8_t	YSM_CHATMessage[MAX_DATA_LEN + 1];
extern	int8_t	YSM_DefaultCHATMessage[MAX_DATA_LEN + 1];
extern	int8_t YSM_BrowserPath[MAX_DATA_LEN+1];
extern	FILE	*YSM_CFGFD;


/* This Comfortable pointer fixes a race condition. */
YSM_SLAVE *YSMSlaves_Comfortable;

int8_t YSM_LastMessage[MAX_DATA_LEN + 1];
int8_t YSM_LastURL[MAX_DATA_LEN + 1];



static void
YSM_Command_QUIT( int argc, char **argv ) 
{
	PRINTF( VERBOSE_BASE, "Closing session, Please wait.\n");
	YSM_Exit( 0, 1 );
}

void
YSM_Command_HELP( int argc, char **argv ) 
{
u_int16_t		g = 0, x = 0, y = 0;
YSM_COMMAND	*node = plist_firstCOMMAND;

	if (!argc) {

		PRINTF( VERBOSE_BASE, 
			"List of commands: \n" 
			"run help + command for detailed information.\n" );
		
		for (g = 0; g < YSM_COMMAND_GROUP_AMOUNT; g++ ) 
		{

			node = plist_firstCOMMAND;

			switch (g) {
				case YSM_COMMAND_GROUP_CLIENT:
					PRINTF( VERBOSE_BASE, "\n\r"YSM_COMMAND_GROUP_CLIENT_STR );
					break;

				case YSM_COMMAND_GROUP_ACCOUNT:
					PRINTF( VERBOSE_BASE, "\n\r"YSM_COMMAND_GROUP_ACCOUNT_STR );
					break;

				case YSM_COMMAND_GROUP_SETTINGS:
					PRINTF( VERBOSE_BASE, "\n\r"YSM_COMMAND_GROUP_SETTINGS_STR );
					break;

				case YSM_COMMAND_GROUP_USERS:
					PRINTF( VERBOSE_BASE, "\n\r"YSM_COMMAND_GROUP_USERS_STR );
					break;
			}

			for (x = 0, y = 0; x < List_amountCOMMAND; x++, y++ ) {

				if (!node) break;

				if (g != node->cmd_groupid) {
					if (y) y--;
					node = node->next;
					continue;
				}

				if (node->cmd_help == NULL) {
					if (y) y--;
					node = node->next;
					continue;
				}	

				if (y && !(y % 4)) {
					PRINTF( VERBOSE_BASE, "\n" );
					y = 0;
				}

				if (node->cmd_alias != NULL) {
					PRINTF( VERBOSE_BASE,
						"%s%s%s%s%s",
						" < ",
						node->cmd_name,
						" or ",
						node->cmd_alias,
						" >" );
				} else {
					PRINTF( VERBOSE_BASE,
						"%s%s%s",
						" < ",
						node->cmd_name,
						" >" );
				}

				node = node->next;
			}

		}
		
		PRINTF( VERBOSE_BASE, "\n" );

	} else {

		for (x = 0; x < List_amountCOMMAND; x++ ) {

			if (!node) break;

			if (node->cmd_help == NULL) {
				node = node->next;
				continue;
			}

			if (node->cmd_name[0] != argv[1][0]
			|| strcasecmp(node->cmd_name, argv[1])) {
				if (node->cmd_alias != NULL) {
				if (node->cmd_alias[0] != argv[1][0]
				|| strcasecmp(node->cmd_alias, argv[1]))
				{
					node = node->next;
					continue;
				}

				} else {
					node = node->next;
					continue;
				}
			}

			PRINTF( VERBOSE_BASE, 
				"Detailed information for '%s'.\n"
				"%s\n",
				argv[1],
				node->cmd_help );

			break;
		}
	}
}


static void
YSM_Command_INFO( int argc, char **argv ) 
{
struct	in_addr ysmintaddr, ysmextaddr;

	PRINTF(VERBOSE_BASE,
		BLUE
	"------------------------------------------------------------------\n"
		WHITE "Program Information\n" BLUE 
	"------------------------------------------------------------------\n"
		NORMAL );

	PRINTF(VERBOSE_BASE,
		"%-20.20s" " : %s\n"
		"%-20.20s" " : %s\n"
		"%-20.20s" " : %s\n"
		"%-20.20s" " : %s\n",
		"Release Version",
		YSM_INFORMATION2,
		"Release Name",
		YSM_INFORMATION,
		"Threads support",
#ifdef YSM_WITH_THREADS
		"YES",
#else
		"NO",
#endif
		"Input library",
#ifndef YSM_WITH_THREADS
		"ysmline"
#else
#ifdef HAVE_LIBREADLINE
		"readline"
#else
		"getline"
#endif
#endif
		);


	PRINTF(VERBOSE_BASE,
		BLUE
	"------------------------------------------------------------------\n"
		WHITE "%-30.30s %-20.20s %-20.20s\n" BLUE 
	"------------------------------------------------------------------\n"
		NORMAL,
		"Session Information",
		BLUE " | " WHITE,
		"DC Information" );


	ysmextaddr.s_addr = YSM_USER.d_con.rIP_ext;
	ysmintaddr.s_addr = YSM_USER.d_con.rIP_int;

	/* we had to split the PRINTFs in two due to the static buffer
	 * inet_ntoa uses, think about it! I'm so angry!.
	 */

	PRINTF(VERBOSE_BASE,
	"%-20.20s" " : %-12.d " 
	"%-20.20s" " : %s\n",
	"UIN", YSM_USER.Uin, 
	"Internal IP address", inet_ntoa(ysmintaddr));

	PRINTF(VERBOSE_BASE,
	"%-20.20s" " : %-7.d      " 
	"%-20.20s" " : %s\n"
	"%-20.20s" " : %-7.d      " 
	"%-20.20s" " : %d\n"
	"%-20.20s" " : %s\n" 
	"%-20.20s" " : %s\n" 
	"%-20.20s" " : %s\n" 
	"%-20.20s" " : %s\n",
	"Slaves", List_amountSLAVE,
	"External IP address", inet_ntoa(ysmextaddr),
	"Sequence #", g_sinfo.seqnum,
	"Bound port", ntohs(YSM_USER.d_con.rPort),
	"Nickname", YSM_USER.info.NickName,
	"Firstname", YSM_USER.info.FirstName,
	"Lastname", YSM_USER.info.LastName,
	"Email", YSM_USER.info.email);

}

static void
YSM_Command_LOADCONFIG( int argc, char **argv )
{
FILE	*fd;

	fd = fopen( YSM_cfgfile, "r" );
	if (fd == NULL) return;

	PRINTF( VERBOSE_BASE, "Reloading cfg settings..\n" );
	YSM_ReadConfig( fd, 1 );

	fclose(fd);
}

static void
YSM_Command_SLAVES( int argc, char **argv ) 
{
	YSM_PrintOrganizedSlaves( STATUS_OFFLINE, argv[1], 0x0 );
}

static void
YSM_Command_ADDSLAVE( int argc, char **argv ) 
{
u_int32_t	x = 0;
int8_t		*pnick = NULL, *puin = NULL;

	/* we allow 2 ways of adding a slave. either add nick uin
	 * or add uin nick. Start by checking argv[1] and then argv[2].
	 * we can't allow adding a slave name just made of numbers.
	 */

	for (x = 0; x < strlen(argv[1]); x++)
		if (!isdigit((int)argv[1][x])) {
			pnick = argv[1];
			puin = argv[2];
			break;
		}

	/* then check argv[2] .. */

	for (x = 0; x < strlen(argv[2]); x++)
		if (!isdigit((int)argv[2][x])) {
			if (pnick != NULL) {
				/* can't have 2 nicks! need a #! */
				PRINTF( VERBOSE_BASE,
				"Syntax mistake. "
				"Usage: add NAME UIN or add UIN NAME\n");
			
				return;
			} else {
				pnick = argv[2];
				puin = argv[1];
				break;
			}
		}
			
	if (pnick == NULL) {
		/* the nick specified is a #! We can't distinguish
		 * which one of them is the nick and which the UIN.
		 */

		PRINTF( VERBOSE_BASE,
		"Syntax mistake. "
		"A nickname must contain non-numeric characters.\n");
		return;
	}

	YSM_AddSlave(pnick, atoi(puin));
	YSM_SendContacts();
}

static void
YSM_Command_DELSLAVE( int argc, char **argv ) 
{
YSM_SLAVE	*YSM_Query = NULL;
int8_t		x = 0;

	YSM_Query = YSM_QuerySlaves(SLAVE_NAME, argv[1], 0, 0);
	if (YSM_Query != NULL) {

	/* check the slave is out of your ignore and invisible lists first
	 * if done manually this saves us some packets ;) 
	 */
		if (YSM_Query->BudType.IgnoreID
			|| YSM_Query->BudType.InvisibleID
			|| YSM_Query->BudType.VisibleID ) {

			PRINTF(VERBOSE_BASE,
			"Slave won't be removed until he/she is removed from "
			"your IGNORE, VISIBLE and\n INVISIBLE lists. Please be "
			"sure to remove him/her from such lists before.\n"
			"deleting the slave.\n[read about the 'invisible', "
			"'visible' and 'ignore' commands]\n");

			return;
		}

		PRINTF(VERBOSE_BASE, "Confirm deletion? [Y/N] : ");
		g_promptstatus.flags |= FL_BUSYDISPLAY;

		PRE_GETKEY_FIX
		x = getkey();
		POS_GETKEY_FIX

		g_promptstatus.flags &= ~FL_BUSYDISPLAY;
		PRINTF(VERBOSE_BASE,"\n");

		if (toupper(x) == 'Y') {
			YSM_RemoveContact(YSM_Query);
			YSM_DelSlave(YSM_Query, 1);
		} else 
			PRINTF(VERBOSE_BASE, 
				"You need to press 'y' to confirm.\n");

	} else {

		if (!isdigit((int)argv[1][0])) {
			PRINTF(VERBOSE_BASE,
			"unknown slave name. Won't delete a ghost!.\n");
		} else {
			PRINTF(VERBOSE_BASE,
			"Incorrect use. Try del 'slavename'.\n");
		}
	}
}

static void
YSM_Command_AUTH( int argc, char **argv ) 
{
YSM_SLAVE	*YSM_Query = NULL;

	YSM_Query = YSM_QuerySlaves(SLAVE_NAME, argv[1], 0, 0);
		
	if(YSM_Query) YSM_SendAuthOK(YSM_Query->Uin, YSM_Query->info.NickName);
	else {
		if (!isdigit((int)argv[1][0])) {
			PRINTF(VERBOSE_BASE,
				"SLAVE Unknown. Authorization cancelled.\n");
		} else {
			YSM_SendAuthOK( atoi(argv[1]), NULL);
		}
	}	
}


/* Validates the Destination field	*/
/* If its a multidest message or single	*/

static int32_t 
YSM_Command_MSG_ValidateDest( char *destination )
{
char		*_index = NULL, *backz = NULL, *dest = NULL;
YSM_SLAVE	*YSM_Query = NULL;
int32_t		amount = 0;

	_index = dest = destination;
	while(strchr(_index, ',') != NULL) {

		_index = strchr(dest, ',');
			if (_index != NULL) {
				*_index = '\0';
				backz = _index;
				_index++;

				YSM_Query = YSM_QuerySlaves( SLAVE_NAME,
							dest,
							0,
							0 );

				if (!YSM_Query) {
					if(!isdigit((int)dest[0])) {
						PRINTF(VERBOSE_BASE,
						"(%s) - slave unknown. "
						"message cancelled.\n",
						dest);

						return -1;
					}
				}

				dest = _index;
				amount++;
				/* *backz = ','; */
			}
		}

		amount++;

		/* Last chain slave or First Single Slave */

		YSM_Query = YSM_QuerySlaves( SLAVE_NAME, dest, 0, 0 );
		if (!YSM_Query) {
			if(!isdigit((int)dest[0])) {
				PRINTF(VERBOSE_BASE,
					"(%s) - Slave unknown. "
					"Message cancelled.\n",
					dest );
				return -1;
			}
		}

	return amount;
}

/* if plainflag == TRUE, send only plain messages.  */

static void
YSM_Command_MSG_main( int argc, char **argv, char plainflag )
{
YSM_SLAVE	*YSM_Query = NULL;
char		*aux = NULL, *dest = NULL;
int32_t		x = 0, amount = 0, amount_cpy = 0;
u_int32_t	fprint_bkp = 0;

	/* if we were in Auto away, this means the user is back! */
	if (g_promptstatus.flags & FL_AUTOAWAY) 
		YSM_ChangeStatus(STATUS_ONLINE);

	amount = YSM_Command_MSG_ValidateDest( argv[1] );
	/* amount has the amount of destinations */
	if (amount < 0) return;

	YSM_Query = YSM_QuerySlaves( SLAVE_NAME, argv[1], 0, 0 );
	if (YSM_Query == NULL) {
		if(!isdigit((int)argv[1][0])) {
		PRINTF(VERBOSE_BASE, "(%s) - Slave unknown. "
			"Message cancelled.\n", argv[1] );
			return;
		}

	} else YSMSlaves_Comfortable = YSM_Query;


	/* check we don't pretend to send encrypted messages when we
	 * can't, hence avoiding surprises.
	 */

	amount_cpy = amount;
	dest = argv[1];

	do {
		YSM_Query = YSM_QuerySlaves( SLAVE_NAME, dest, 0, 0 );
		if ((YSM_Query != NULL) && !plainflag) {
			/* it is a slave and plain mode is not forced */
			if (!YSM_KeyEmpty(YSM_Query->crypto.strkey)
			&& (YSM_Query->fprint != FINGERPRINT_YSM_CLIENT_CRYPT))
			{
				/* oh oh, can't send encrypted */
				PRINTF(VERBOSE_BASE,
				"ysm can't send an encrypted message to the "
				"slave '%s' because he/she/it is\n"
				"not online or is not using ysm with "
				"encryption support.\n"
				"Use the 'mp' or 'mplain' command to force a "
				"plaintext message. Thank you.\n",
				YSM_Query->info.NickName);
				return;	
			}
		}

		amount_cpy--;
		/* skip the last \0 and get the next man! */
		dest += strlen(dest) + 1;

	} while (amount_cpy);

	if (argc < 2) {
		
		if (amount > 1) {

			PRINTF(VERBOSE_BASE,
			"Switching to MULTI-DEST -comfortable- message Mode.\n"
			"Use '.' in an empty line to end Message or '#'"
			"to cancel the whole Message.\n");

		} else {

			int8_t	*msgCol = "";
			if (YSMSlaves_Comfortable->Color != NULL)
				msgCol = YSMSlaves_Comfortable->Color;

			PRINTF(VERBOSE_BASE,
			"%sSwitching to %s -comfortable- message Mode.\n"
			"Use '.' in an empty line to end Message or '#'"
			"to cancel the whole Message.\n", msgCol,
			((YSM_Query != NULL) && !plainflag
			&& (!YSM_KeyEmpty(YSMSlaves_Comfortable->crypto.strkey))
			&& (YSMSlaves_Comfortable->fprint == FINGERPRINT_YSM_CLIENT_CRYPT)) 
			? "ENCRYPTED" : "PLAIN");

		}

		argv[2] = YSM_ReadLongMessage();
		if(argv[2] == NULL) {
			PRINTF(VERBOSE_BASE,
				"Comfortable Msg. cancelled.\n");
			return;
		}
		
	} else {
	/* Turn argv[x] into a long chain */
		for (x = 2; x < argc; x++) {
			aux = strchr(argv[x],'\0');
			if (aux != NULL) *aux = 0x20;
		}
	}


	/* START CYCLE THROUGH DESTINATION LIST */
	dest = argv[1];

	do {
		YSM_Query = YSM_QuerySlaves( SLAVE_NAME, dest, 0, 0 );
		if (YSM_Query == NULL) {
			if(!isdigit((int)dest[0])) {
			PRINTF(VERBOSE_BASE, "(%s) - Slave unknown. "
				"Message cancelled.\n", dest );
				return;
			}
		} else YSMSlaves_Comfortable = YSM_Query;


		if ((YSM_Query != NULL) && plainflag) {
			fprint_bkp = YSM_Query->fprint;
			YSM_Query->fprint = 0;
		}

		if (YSM_Query != NULL) {
		YSMSlaves_LastSent = YSMSlaves_Comfortable;
		YSMSlaves_TabSlave = YSMSlaves_Comfortable;

		YSM_SendMessage(YSMSlaves_Comfortable->Uin,
				argv[2],
				(char)(YSMSlaves_Comfortable->flags & FL_LOG),
				YSMSlaves_Comfortable,
				1);
		} else
			YSM_SendMessage(atoi(dest), argv[2], 0, NULL, 1);

		amount--;

		/* skip the last \0 and get the next man! */
		dest += strlen(dest) + 1;

		/* Restore the fingerprint flag if neccesary */
		if ((YSM_Query != NULL) && plainflag) {
			YSM_Query->fprint = fprint_bkp;
			fprint_bkp = 0;
		}

	} while (amount);
}

static void
YSM_Command_MSG( int argc, char **argv ) 
{
	YSM_Command_MSG_main( argc, argv, 0 );
}

static void
YSM_Command_MPLAIN( int argc, char **argv ) 
{
	YSM_Command_MSG_main( argc, argv, 1 );
}

void
YSM_Command_CHAT( int argc, char **argv ) 
{
YSM_SLAVE	*query = NULL;
int8_t		*dest = NULL, *aux= NULL;
int32_t		amount = 0, y = 0;
u_int32_t	x = 0;

	if (argc < 1) {
		if (!(g_promptstatus.flags & FL_CHATM)) 
			return;
	
		PRINTF( VERBOSE_BASE, 
		"############### closing ysm CHAT session #############\n");
		/* we are now officialy out of ysm's chat mode! */
		g_promptstatus.flags &= ~FL_CHATM;
		g_promptstatus.flags &= ~FL_COMFORTABLEM;

		query = plist_firstSLAVE;
		/* loop through the slaves list and unmark FL_CHAT */
		for (x = 0; x < List_amountSLAVE && query; x++) {
			if (query->flags & FL_CHAT)
				query->flags &= ~FL_CHAT;

			query = query->next;
		}

		return;
	}

	PRINTF(VERBOSE_BASE, 
		"############### activating ysm CHAT mode #############\n"
		"# slaves involved: %s\n"
		"#\n"
		"# all messages you receive which don't belong to this\n"
		"# chat session, will be replied with your CHAT message\n"
		"# and logged using AFK. Once you leave the CHAT session,\n"
		"# you will have to type 'readafk' to read them.\n"
		"# you can leave the session by using the 'chat' command.\n"
		"######################################################\n",
		 argv[1]);

	amount = YSM_Command_MSG_ValidateDest(argv[1]);
	/* amount has the amount of destinations */
	if (amount < 0) {
		return;
	}

	/* START CYCLE THROUGH CHAT LIST */
	dest = argv[1];

	do {
		query = YSM_QuerySlaves(SLAVE_NAME, dest, 0, 0);
		if (query == NULL) {
			PRINTF(VERBOSE_BASE, "(%s) - Slave unknown. "
				"chat cancelled.\n", dest );
			return;
		}

		if (query) {
			/* this is REALLY ugly and SLOW but I'm not in the mood
			 * of creating a new list for chat slaves only. Instead,
			 * we set the flag to FL_CHAT in our slaves.
			 */

			query->flags |= FL_CHAT;
		}

		amount--;

		/* skip the last \0 and get the next man! */
		dest += strlen(dest) + 1;

	} while (amount);

	/* get the CHAT message */

	if (argc > 2) {
		/* Turn argv[2] into a long chain */
		for (y = 2; y < argc; y++) {
			aux = strchr(argv[y],'\0');
			if (aux != NULL) *aux = 0x20;
		}

		strncpy(YSM_CHATMessage,
			argv[2],
			sizeof(YSM_CHATMessage) - 1);
		YSM_CHATMessage[sizeof(YSM_CHATMessage)-1] = '\0';
	}

	if (YSM_CHATMessage[0] == 0x00) { 
		strncpy(YSM_CHATMessage,
			YSM_DefaultCHATMessage,
			sizeof(YSM_CHATMessage) - 1);
		YSM_CHATMessage[sizeof(YSM_CHATMessage)-1] = '\0';
	}

	/* we are now officialy in ysm's chat mode! */
	g_promptstatus.flags |= FL_CHATM;
}

static void
YSM_Command_STATUS( int argc, char **argv ) 
{
int	x;
char	UserStatus[MAX_STATUS_LEN];

	if (!argc) {
		YSM_WriteStatus(YSM_USER.status, UserStatus);
		PRINTF(VERBOSE_BASE, "Current status: %s.\n",UserStatus); 
		return;
	}
			
	x = YSM_LookupStatus(argv[1]);
	if (x == -2) {
		PRINTF( VERBOSE_BASE,
			"Invalid status specified. Carlin!\n");
		return;
	}

	YSM_WriteStatus( x, UserStatus );	
	PRINTF(VERBOSE_BASE, "Switching to: %s\n",UserStatus);

	YSM_ChangeStatus(x);
}

static void
YSM_Command_LASTSENT( int argc, char **argv ) 
{
int	x = 0;
char	*aux;

	if (YSMSlaves_LastSent) {

		if (!argc) {
			int8_t	*msgCol="";
			YSMSlaves_Comfortable = YSMSlaves_LastSent;

			if (YSMSlaves_Comfortable->Color != NULL)
				msgCol=YSMSlaves_Comfortable->Color;
			

			PRINTF(VERBOSE_BASE,
			"%sSwitching to -comfortable- message "
			"Mode. (sending %s to " WHITE"%s" NORMAL")%s", msgCol,
			(!YSM_KeyEmpty(YSMSlaves_Comfortable->crypto.strkey)) 
			? "ENCRYPTED" : "PLAIN",
			YSMSlaves_Comfortable->info.NickName, msgCol);

			PRINTF(VERBOSE_BASE,
			"\nUse '.' in an empty line to end Message or '#' "
			"to cancel the whole Message.\n");

			argv[1] = YSM_ReadLongMessage();

			if (argv[1] == NULL) {
				PRINTF(VERBOSE_BASE,
				"Comfortable Msg. cancelled.\n");
				return;
			}

			YSM_SendMessage( YSMSlaves_Comfortable->Uin,
					argv[1],
					(char)(YSMSlaves_Comfortable->flags & FL_LOG),
					YSMSlaves_Comfortable,
					1);
		} else {
			/* Turn argv[x] into a long chain */
			for (x = 1; x < argc; x++) {
				aux = strchr(argv[x],'\0');
				if (aux != NULL) *aux = 0x20;
			}

			YSM_SendMessage( YSMSlaves_LastSent->Uin,
					argv[1],
					(char)(YSMSlaves_LastSent->flags & FL_LOG),
					YSMSlaves_LastSent,
					1);
		}	

	} else {
		PRINTF(VERBOSE_BASE,
			"Unable to find the last Slave you messaged.\n");
	}
}

static void
YSM_Command_REPLY( int argc, char **argv ) 
{
int	x = 0;
char	*aux = NULL;

	if (YSMSlaves_LastRead) {

		if (!argc) {
			int8_t	*msgCol="";
			YSMSlaves_Comfortable = YSMSlaves_LastRead;
			
			if (YSMSlaves_Comfortable->Color != NULL)
				msgCol=YSMSlaves_Comfortable->Color;
			
			
			PRINTF(VERBOSE_BASE,
			"%sSwitching to -comfortable- message "
			"Mode. (sending %s to "WHITE"%s" NORMAL")%s", msgCol,
			((!YSM_KeyEmpty(YSMSlaves_Comfortable->crypto.strkey)) 
			&& (YSMSlaves_Comfortable->fprint == FINGERPRINT_YSM_CLIENT_CRYPT)) 
			? "ENCRYPTED" : "PLAIN",
			YSMSlaves_Comfortable->info.NickName, msgCol);

			PRINTF(VERBOSE_BASE,
			"\nUse '.' in an empty line to end Message or '#' "
			"to cancel the whole Message.\n");

			argv[1] = YSM_ReadLongMessage();

			if (argv[1] == NULL) {
				PRINTF(VERBOSE_BASE,
				"Comfortable Msg. cancelled.\n");
				return;
			}

			YSMSlaves_LastSent = YSMSlaves_Comfortable;
			YSMSlaves_TabSlave = YSMSlaves_LastSent;

			YSM_SendMessage( YSMSlaves_Comfortable->Uin,
					argv[1],
					(char)(YSMSlaves_Comfortable->flags & FL_LOG),
					YSMSlaves_Comfortable,
					1);
		} else {

			/* Turn argv[x] into a long chain */
			for (x = 1; x < argc; x++) {
				aux = strchr(argv[x],'\0');
				if (aux != NULL) *aux = 0x20;
			}

			YSMSlaves_LastSent = YSMSlaves_LastRead;
			YSMSlaves_TabSlave = YSMSlaves_LastSent;

			YSM_SendMessage( YSMSlaves_LastRead->Uin,
					argv[1],
					(char)(YSMSlaves_LastRead->flags & FL_LOG),
					YSMSlaves_LastRead,
					1);
		}

	} else {
		PRINTF(VERBOSE_BASE,
		"Unable to find the last Slave who messaged you.\n");
	}

}

static void
YSM_Command_WHOIS( int argc, char **argv ) 
{
YSM_SLAVE	*YSM_Query = NULL;
int8_t		buf[MAX_STATUS_LEN+1], buf2[MAX_STATUS_LEN+1];
struct in_addr	rintIP, rextIP;

	YSM_Query = YSM_QuerySlaves(SLAVE_NAME, argv[1], 0, 0);
	if (!YSM_Query)
		YSM_Query = YSM_QuerySlaves(SLAVE_UIN, NULL, atoi(argv[1]), 0);

	if (YSM_Query != NULL) {

		memset(buf, 0, sizeof(buf));	
		YSM_WriteFingerPrint(YSM_Query->fprint, buf);

		memset(buf2, 0, sizeof(buf2));
		YSM_WriteStatus(YSM_Query->status, buf2);

		PRINTF(VERBOSE_BASE,
			BLUE
	"------------------------------------------------------------------\n"
			WHITE "Information on %s .. how interesting..\n" BLUE 
	"------------------------------------------------------------------\n"
			NORMAL,
			YSM_Query->info.NickName );

		rintIP.s_addr = YSM_Query->d_con.rIP_int;
		rextIP.s_addr = YSM_Query->d_con.rIP_ext;


		/* we had to split the PRINTFs in two due to the static buffer
		 * inet_ntoa uses, think about it! I'm so angry!.
		 */

		PRINTF(VERBOSE_BASE,
			"%-15.15s" " : %-12.d " 
			"%-20.20s" " : %s\n",
			"UIN", YSM_Query->Uin, 
			"Internal IP address", inet_ntoa(rintIP));

		PRINTF(VERBOSE_BASE,
			"%-15.15s" " : %-12.12s " 
			"%-20.20s" " : %s\n"
			"%-15.15s" "   %-7.s      " /* blank */
			"%-20.20s" " : %d\n\n"	/* 2 newlines */
			"%-15.15s" " : %s\n\n",
			"Current status", buf2,
			"External IP address", inet_ntoa(rextIP),
			"", "",
			"Bound Port", YSM_Query->d_con.rPort,
			"Fingerprint", buf);

		if (YSM_Query->BudType.birthday) {
			PRINTF( VERBOSE_BASE, 
			WHITE "\aToday is this slave's BIRTHDAY!\n"
			CYAN "[i~] Blow a candle! "
			"[~~] Eat a cake!.\n" NORMAL);
		}

		/* request auto message if any */
		YSM_RequestAutoMessage(YSM_Query);

		/* update fields */
		YSM_Query->ReqID = YSM_RequestInfo(
						YSM_Query->Uin,
						(int16_t)0xB204
						);
	} else {

		if (!isdigit((int)argv[1][0])) {
			PRINTF( VERBOSE_BASE,
				"Unknown SLAVE Name. Request "
				"Cancelled.\n");	
		} else 
			YSM_RequestInfo( atoi(argv[1]), (short)0xB204 );
	}
}

static void
YSM_Command_SLAVESON( int argc, char **argv ) 
{
	if (argc > 0) {
		YSM_PrintOrganizedSlaves( STATUS_ONLINE,
					argv[1], 
					0x1 );
	} else {
		YSM_PrintOrganizedSlaves( STATUS_ONLINE,
					NULL,
					0x1 );
	}
}

static void
YSM_Command_BEEP( int argc, char **argv ) 
{
	if (!argc) {
		PRINTF( VERBOSE_BASE, 
			"BEEP is %s\n",
			(YSM_SETTING_BEEP) ? "ON" : "OFF");
		return;
	}
			
	YSM_SETTING_BEEP = (!strcasecmp(argv[1], "ON")) ? TRUE : FALSE;

}

static void
YSM_Command_SOUNDS( int argc, char **argv ) 
{
	if (!argc) {
		PRINTF( VERBOSE_BASE, 
			"sounds are %s\n",
			(YSM_SETTING_SOUNDS) ? "ON" : "OFF");
		return;
	}
			
	YSM_SETTING_SOUNDS = (!strcasecmp(argv[1], "ON")) ? TRUE : FALSE;
}


static void
YSM_Command_CLEAR( int argc, char **argv ) 
{
#ifndef WIN32
	PRINTF( VERBOSE_BASE, CLRSCR);
	PRINTF( VERBOSE_BASE, HOMECUR);
#else
	system("cls");
#endif

}
static void
YSM_Command_LOGALL( int argc, char **argv ) 
{
YSM_SLAVE	*YSM_Query = NULL;

	if (!argc) {

		if (YSM_SETTING_LOGALL)
			PRINTF(VERBOSE_BASE, "LOG_ALL is ON\n");
		else
			PRINTF(VERBOSE_BASE, "LOG_ALL is OFF\n");
					
		PRINTF( VERBOSE_BASE,
			"Log ON|OFF (Global) or Log SLAVE_NAME\n");
		return;
	}
	

	if (!strcasecmp( argv[1], "ON" )) YSM_SETTING_LOGALL = TRUE;
	else if (!strcasecmp( argv[1], "OFF" )) YSM_SETTING_LOGALL = FALSE;
	else {
		YSM_Query = YSM_QuerySlaves(SLAVE_NAME, argv[1], 0, 0);
		if (!YSM_Query) {
       			PRINTF(VERBOSE_BASE,
				"Unknown SLAVE Name. I wont log!\n");
			return;
		}
	
		YSM_SlaveFlags(YSM_Query, "l", (char)!(YSM_Query->flags & FL_LOG), 1);

		PRINTF( VERBOSE_BASE,
			"LogFlag to %s for Slave %s - UIN %d\n",
			(YSM_Query->flags & FL_LOG) ? "ON" : "OFF",
			YSM_Query->info.NickName,
			YSM_Query->Uin);
	}

}

static void
YSM_Command_AFK( int argc, char **argv ) 
{
char	*aux = NULL;
int	x = 0;

	if (argc > 0) {

		/* Turn argv[x] into a long chain */
		for (x = 1; x < argc; x++) {
			aux = strchr(argv[x],'\0');
			if (aux != NULL) *aux = 0x20;
		}

		strncpy(YSM_AFKMessage,
			argv[1],
			sizeof(YSM_AFKMessage) - 1);
		YSM_AFKMessage[sizeof(YSM_AFKMessage)-1] = '\0';
	}
		
	YSM_AFKMode((u_int8_t)!(g_promptstatus.flags & FL_AFKM));
}

static void
YSM_Command_SEARCH( int argc, char **argv ) 
{
	if (strchr(argv[1], '@' ))		
		YSM_SearchUINbyMail(argv[1]);
	else
		PRINTF( VERBOSE_BASE,
			"Invalid e-mail address specified.\n");
}

static void
YSM_Command_READAFK( int argc, char **argv ) 
{
	PRINTF( VERBOSE_BASE, "%s\n", MSG_AFK_READ_MSG);

	PRE_GETKEY_FIX
	YSM_ReadLog(YSM_AFKFILENAME, 0);
	POS_GETKEY_FIX
}


static void
YSM_Command_NICK( int argc, char **argv ) 
{
	if (!argc) {
		PRINTF(VERBOSE_BASE, "Your nick is: ");
		if(strlen(YSM_USER.info.NickName) < 2) {
			if( YSM_USER.info.NickName[0] == '%' )
				PRINTF( VERBOSE_BASE,
					"none specified\n");
			else
				PRINTF( VERBOSE_BASE,
					"Server hasn't replied yet.\n");
		} else {
			PRINTF( VERBOSE_BASE,
				"%s\n",YSM_USER.info.NickName);
		}

	} else 
		YSM_InfoChange( YSM_INFO_NICK, argv[1]);
}

static void
YSM_Command_SAVE( int argc, char **argv ) 
{
YSM_SLAVE	*YSM_Query = NULL;

	PRINTF( VERBOSE_BASE,
		"\n" CYAN "YSM POLITICAL ASYLUM " NORMAL "FOR SLAVES\n");

	if (!argc) YSM_BuddyUploadList( NULL );
	else {
		YSM_Query = YSM_QuerySlaves(SLAVE_NAME, argv[1], 0, 0);

		if(YSM_Query) 
			YSM_BuddyUploadList (YSM_Query);
		else {
			PRINTF(VERBOSE_BASE,
			"SLAVE Unknown. Can't save a ghost.\n");
		}
	}
}

static void
YSM_Command_REQ( int argc, char **argv ) 
{
YSM_SLAVE	*YSM_Query = NULL;
char		*aux = NULL;
int		x = 0;

	YSM_Query = YSM_QuerySlaves(SLAVE_NAME, argv[1], 0, 0);

	if (argc > 1) {	
	/* Turn argv[x] into a long chain */
		for (x = 2; x < argc; x++) {
			aux = strchr(argv[x],'\0');
			if (aux != NULL) *aux = 0x20;
		}
	}

	if(YSM_Query) {	
		YSM_SendAuthRequest( YSM_Query->Uin,
				YSM_Query->info.NickName,
				argv[2]);

		if(YSM_Query->DownloadedFlag == 0x0a) {
			YSM_BuddyAddItem ( YSM_Query,
				YSM_BUDDY_GROUPNAME,
				YSM_BUDDY_GROUPID,
				0x0, 
				YSM_BUDDY_SLAVE, 
				0, 
				1, 
				0x08);

				/* Change the value from 0x0a */
				YSM_Query->DownloadedFlag = 0x0c;
		}

	} else {
		if (!isdigit((int)argv[1][0])) {
			PRINTF( VERBOSE_BASE,
				"SLAVE Unknown. Authorization Request "
				"cancelled.\n");
		} else
			YSM_SendAuthRequest ( atoi(argv[1]), NULL, argv[2]);
	}

}

static void
YSM_Command_RENAME( int argc, char **argv ) 
{
YSM_SLAVE	*YSM_Query = NULL;
int8_t		*newslavename = NULL;

	newslavename = argv[2];
	/* sanitize the slave name, trim? */
	while (*newslavename == 0x20) newslavename++;

	YSM_Query = YSM_QuerySlaves(SLAVE_NAME, newslavename, 0, 0);

	if (YSM_Query) {
		/* The new name exists! Abort! */
		PRINTF( VERBOSE_BASE,
			"Error!: Renaming to an existing name.\n");
		return;
	}

	YSM_Query = YSM_QuerySlaves(SLAVE_NAME, argv[1], 0, 0);

	if (!YSM_Query) {
		PRINTF(VERBOSE_BASE,
			"SLAVE Unknown."
			" Can't rename a non existing slave!"
			"cancelled.\n");

		return;
	}

	/* The old_name exists, renaming a valid slave */

	if (!strcasecmp(YSM_Query->info.NickName, newslavename)) {
		/* Renaming to the same name? no way! */
		PRINTF( VERBOSE_BASE,
			"Ahahaha..thats a joke, right? Renaming"
			" requires two DIFFERENT nicks.\n");
		return;
	}

	PRINTF( VERBOSE_BASE, 
		"Renaming %s to %s\n",
		YSM_Query->info.NickName,
		newslavename);

	YSM_UpdateSlave( UPDATE_NICK, newslavename, YSM_Query->Uin );
}

static void
YSM_Command_EMAIL( int argc, char **argv ) 
{
	if (!argc) {
		PRINTF(VERBOSE_BASE, "Your e-mail is: ");

		if(strlen(YSM_USER.info.email) < 2) {
			if( YSM_USER.info.email[0] == '%' )
				PRINTF(VERBOSE_BASE,
						"none specified\n");
			else
				PRINTF(VERBOSE_BASE,
					"Server hasn't replied yet.\n");
		} else {

			PRINTF(VERBOSE_BASE, "%s\n",YSM_USER.info.email);
		}

	} else YSM_InfoChange( YSM_INFO_EMAIL, argv[1] );
}

static void
YSM_Command_UPTIME( int argc, char **argv ) 
{
	YSM_Print_Uptime ();
}

static void
YSM_Command_BACKDOOR( int argc, char **argv ) 
{
	PRINTF(VERBOSE_BASE,
		"ahaha. just kidding :) command not implemented.\n");

}


#ifdef YSM_WAR_MODE 

static void
YSM_Command_SCAN( int argc, char **argv ) 
{
YSM_SLAVE	*YSM_Query = NULL;

	YSM_Query = YSM_QuerySlaves(SLAVE_NAME, argv[1], 0, 0);

	if (YSM_Query) {
		PRINTF( VERBOSE_BASE,
			"\n" CYAN "YSM ULTRASECRET SCANNING OF" 
			NORMAL " HIDDEN ENEMIES\n");
	
		YSM_War_Scan(YSM_Query);

	} else {
		PRINTF(VERBOSE_BASE,
			"SLAVE Unknown. Won't scan a non existing slave.\n");
	}
}


static void
YSM_Command_KILL( int argc, char **argv ) 
{
YSM_SLAVE	*YSM_Query = NULL;

	PRINTF(VERBOSE_BASE,
		"..if this is what you want. It is what i'll do.\n");

	YSM_Query = YSM_QuerySlaves(SLAVE_NAME, argv[1], 0, 0);

	if (YSM_Query) YSM_War_Kill(YSM_Query);
	else
		PRINTF(VERBOSE_BASE,
			"Unknown SLAVE Name. Won't kill a Ghost.\n");
}

#endif


static void
YSM_Command_RTF( int argc, char **argv )
{
YSM_SLAVE	*YSM_Query = NULL;

	PRINTF(VERBOSE_BASE,"rtfing...\n");

	YSM_Query = YSM_QuerySlaves(SLAVE_NAME, argv[1], 0, 0);
	if (YSM_Query) YSM_SendRTF(YSM_Query);
	else
		PRINTF(VERBOSE_BASE,
			"Unknown SLAVE Name. Won't rtf a Ghost.\n");

}

static void
YSM_Command_IGNORE( int argc, char **argv ) 
{
YSM_SLAVE	*YSM_Query = NULL;

	YSM_Query = YSM_QuerySlaves(SLAVE_NAME, argv[1], 0, 0);

	if (!YSM_Query) {
       		PRINTF( VERBOSE_BASE,
			"Unknown SLAVE Name. Won't ignore!\n");
		return;
	}

	if (!YSM_Query->BudType.IgnoreID) {
	
		if(!YSM_Query->DownloadedFlag)	/* Slave isn't saved */
		{
			PRINTF( VERBOSE_BASE,
			"Slave won't be added to your ignore list until "
			"you upload him\nto the icq servers using the 'save' "
			"command. (try 'save slave_name').\n");

			return;

		} else {

			PRINTF( VERBOSE_BASE,
			"Adding the slave to your ignore list..\n"
			"..and to your invisible list.\n");
				YSM_BuddyIgnore (YSM_Query, 0x1);

			if(!YSM_Query->BudType.InvisibleID)
				YSM_BuddyInvisible( YSM_Query, 0x1 );

		}

	}
		/* Unignore the user	*/
	else
	{
		PRINTF( VERBOSE_BASE,
		"Removing the slave from your Ignore and Invisible "
		"lists.\n");
	
 		YSM_BuddyIgnore (YSM_Query, 0x0);

		if(YSM_Query->BudType.InvisibleID)
			YSM_BuddyInvisible( YSM_Query, 0x0 );
	}

	PRINTF( VERBOSE_BASE,
		"Switching Ignore to %s for Slave %s - UIN %d\n",
		(YSM_Query->BudType.IgnoreID) ? "ON" : "OFF",
		YSM_Query->info.NickName, YSM_Query->Uin);

}

static void
YSM_Command_VISIBLE( int argc, char **argv ) 
{
YSM_SLAVE	*YSM_Query = NULL;

	YSM_Query = YSM_QuerySlaves(SLAVE_NAME, argv[1], 0, 0);

	if (!YSM_Query) {
       		PRINTF( VERBOSE_BASE,
			"Unknown SLAVE Name. Won't add to Visible!\n");
		return;
	}

	if(!YSM_Query->BudType.VisibleID) {
	
		if(!YSM_Query->DownloadedFlag)	/* Slave isn't saved */
		{
			PRINTF(VERBOSE_BASE,
			"Slave won't be added to your visible list until "
			"you upload him\nto the icq servers using the 'save' "
			"command. (try 'save slave_name').\n");

			return;

		} else {

			PRINTF(VERBOSE_BASE,
			"Adding the slave to your visible list..\n");

			YSM_BuddyVisible (YSM_Query, 0x1);
		}

	}
		/* Remove from the visible list */
	else
	{
		PRINTF(VERBOSE_BASE,
			"Removing slave from your visible list..\n");

		YSM_BuddyVisible (YSM_Query, 0x0);
	}

	PRINTF(VERBOSE_BASE,
		"Switching Visible to %s for Slave %s - UIN %d\n",
		(YSM_Query->BudType.VisibleID) ? "ON" : "OFF",
		YSM_Query->info.NickName, YSM_Query->Uin);

}

static void
YSM_Command_INVISIBLE( int argc, char **argv ) 
{
YSM_SLAVE	*YSM_Query = NULL;

	YSM_Query = YSM_QuerySlaves(SLAVE_NAME, argv[1], 0, 0);

	if (!YSM_Query) {
       		PRINTF(VERBOSE_BASE,
		"Unknown SLAVE Name. "
		"Won't add to the invisible list!\n");
		return;
	}

	/* Meaning the buddy IS in our invisible list */
	if(!YSM_Query->BudType.InvisibleID) {
	
		if(!YSM_Query->DownloadedFlag)	/* Slave isn't saved */
		{
			PRINTF(VERBOSE_BASE,
			"Slave won't be added to your invisible list until "
			"you upload him\nto the icq servers using the 'save' "
			"command. (try 'save slave_name').\n");

			return;

		} else {
			PRINTF(VERBOSE_BASE,
			"Adding the slave to your invisible list..\n");
			YSM_BuddyInvisible (YSM_Query, 0x1);
		}

	}
	/* Remove from the invisible list */
	else
	{
		PRINTF(VERBOSE_BASE,
			"Removing slave from your Invisible list..\n");
		YSM_BuddyInvisible (YSM_Query, 0x0);
	}

	PRINTF(VERBOSE_BASE,
		"Switching Invisible to %s for Slave %s - UIN %d\n",
		(YSM_Query->BudType.InvisibleID) 
		? "ON" : "OFF",
		YSM_Query->info.NickName, YSM_Query->Uin);
}

static void
YSM_Command_ALERT( int argc, char **argv ) 
{
YSM_SLAVE	*YSM_Query = NULL;

	YSM_Query = YSM_QuerySlaves(SLAVE_NAME, argv[1], 0, 0);

	if (!YSM_Query) {
       		PRINTF(VERBOSE_BASE,
		"Unknown SLAVE Name. Can't mark him on Alert!\n");
		return;
	}

	YSM_SlaveFlags( YSM_Query, "a", (char)!(YSM_Query->flags & FL_ALERT), 1);

	PRINTF(VERBOSE_BASE,
		"Switching Alert to %s for Slave %s - UIN %d\n",
		(YSM_Query->flags & FL_ALERT) ? "ON" : "OFF",
		YSM_Query->info.NickName, YSM_Query->Uin);

}

static void
YSM_Command_LAST( int argc, char **argv ) 
{
	PRINTF(VERBOSE_BASE, "Fetching Last Received Message:\n");

	if (!YSMSlaves_LastRead) {
		PRINTF(VERBOSE_BASE, "Not Found =)\n");
		return;
	}

	PRINTF(VERBOSE_BASE, 
		"From: %s", YSMSlaves_LastRead->info.NickName);

	PRINTF(VERBOSE_BASE,
		"\n----------------------------------------\n");
	
	PRINTF(VERBOSE_BASE, "%s", YSM_LastMessage);

	PRINTF(VERBOSE_BASE,
		"\n----------------------------------------\n");
}

static void
YSM_Command_HIST( int argc, char **argv ) 
{
YSM_SLAVE	*YSM_Query = NULL;
char		UinStr[MAX_UIN_LEN+1];

	YSM_Query = YSM_QuerySlaves(SLAVE_NAME, argv[1], 0, 0);
	if (!YSM_Query) {
       		PRINTF(VERBOSE_BASE,
			"Unknown SLAVE Name. Only slave logs allowed.\n");
		return;
	}
	
	PRINTF(VERBOSE_BASE,
		"Message History for Slave %s - UIN %d\n"
		"Note: To generate a History file, read "
		"about the 'log' command.\n",
		YSM_Query->info.NickName, YSM_Query->Uin);

	snprintf(UinStr, MAX_UIN_LEN, "%d", (int)YSM_Query->Uin );
	UinStr[sizeof(UinStr)-1] = 0x00;

	PRE_GETKEY_FIX
	YSM_ReadLog(UinStr, 1); 
	POS_GETKEY_FIX
}

static void
YSM_Command_MINIMIZE( int argc, char **argv ) 
{

	PRINTF(VERBOSE_BASE,"\nMinimizing YSM..\n");	
#ifdef WIN32
	YSM_WindowHide(getConsoleWindow());
#elif OS2
	os2_minimize_console();
#else
	PRINTF(VERBOSE_BASE, ICONIFY);
#endif

}
static void
YSM_Command_KEY( int argc, char **argv ) 
{
YSM_SLAVE	*YSM_Query = NULL;
u_int32_t	x = 0, keylen = 0;
int32_t		retval = 0;
int8_t		goodKey[64];

	YSM_Query = YSM_QuerySlaves(SLAVE_NAME, argv[1], 0, 0);
	if (!YSM_Query) {
       		PRINTF(VERBOSE_BASE,
			"Unknown SLAVE Name. Mommi told me not to"
			" do encryption on strangers.\n");
		return;
	}


	if (argc < 2) {

	/* ACTION: 
	 *	Clear key for slave. No key was supplied, hence we
	 *	understand the user wants to clear the key with this
	 *	slave.
	 */

		PRINTF(VERBOSE_BASE,
			"Clearing key for slave %s\n",
			YSM_Query->info.NickName);

		/* If there is an existing key, reset it.
		 * Then update the configuration file to clear the key.
		 */

		if (!YSM_KeyEmpty(YSM_Query->crypto.strkey))  
			YSM_ClearKey(YSM_Query);

		/* Thats it. No more steps to take */
		return;
	}

	/*	If we get this far, it means the user wants to change
	 *	the key with the slave. There are 2 possibilities:
	 *	a. a key was supplied
	 *	b. the '?' character was used to generate a new key randomly.
	 *	lets find out..
	 */

	memset(YSM_Query->crypto.strkey, 0, sizeof(YSM_Query->crypto.strkey));

	if (argv[2][0] == '?') {

	/* ACTION: 
	 *	Generate a random 64 bytes key for slave.
	 *	The '?' character used as a key determines that we have
	 *	to randomly generate a key for the specified slave.
	 */
		for (x = 0; x < MAX_KEY_LEN; x++) {

			/* step 1:
			 * choose a random base value between 0 and f 
			 * sum that value to '0' to get an ascii value.
			 */

			YSM_Query->crypto.strkey[x] = rand() % 0xf + '0';

			/* step 2:
			 * since the obtained ascii value can be erroneous
			 * because after '9' comes '@' for example. We pad
			 * the difference with the value of 'A', but only
			 * if step 1 left us with a value higher than '9'.
			 */
			
			if (YSM_Query->crypto.strkey[x] > '9')
				YSM_Query->crypto.strkey[x] += 'A' - '9' - 1;
		}

	} else {

	/* ACTION: 
	 *	Assign the specified key to the slave.
	 *	The user supplied a key, we must first validate it.
	 * 	If the key is valid, it is assigned to the slave.
	 *	Note validation is done afterwards.
	 */
	
		for (x = 0; (x < strlen(argv[2])) && (x < MAX_KEY_LEN); x++) 
			YSM_Query->crypto.strkey[x] = argv[2][x];
	}

	

	/* ACTION:
	 *	Since the user is allowed to enter shorter keys than
	 *	64 bytes long (because otherwise it would turn hideous),
	 *	we make sure the final key is at least 64 bytes long by
	 *	repeating the key n amount of times as neccesary.
	 */

	keylen = strlen(YSM_Query->crypto.strkey);
	
	for (x = 0; x < sizeof(goodKey); x++) 
		goodKey[x] = YSM_Query->crypto.strkey[x % keylen];

	/* ACTION:
	 *	Try to make both in and out keys.
	 *	At this point either we have a (should be) valid randomly
	 * 	generated 64 bytes key, or, a (maybe invalid) 64 bytes key
	 *	supplied by the user.
	 */

	retval = makeKey(&YSM_Query->crypto.key_out, DIR_ENCRYPT, 256, goodKey);
	if (retval == TRUE) {
		/* OUT key instance created successfully.
		 * Proceed to create the second key */

		retval = makeKey( &YSM_Query->crypto.key_in,
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
			"Error setting cipher key. Please check the key meets"
			"\nthe requirements by using the 'help key' command.\n"
			);

		return;
	}

	/* ACTION:
	 *	Print the key back to the user.
	 */

	PRINTF(VERBOSE_BASE, "Slave encryption key is now:\n");

	for (x = 0; x < MAX_KEY_LEN; x++)
		PRINTF(VERBOSE_BASE,"%c", YSM_Query->crypto.strkey[x]);
	
	PRINTF(VERBOSE_BASE,"\n");

	/* ACTION:
	 *	Update the configuration file with the new key for
	 *	this slave.
	 */

	YSM_UpdateSlave( UPDATE_SLAVE, NULL, YSM_Query->Uin );
}

static void
YSM_Command_BURL( int argc, char **argv ) 
{
char	*browser_args[3];

	/* launch the damn browser! */
	if(YSM_BrowserPath[0] == 0x00) {
		PRINTF(VERBOSE_BASE, "\nNo! Won't launch a browser for"
		" the specified url.\n"
		"There is no browser configured. "
		"Specify a value for the BROWSER> param\n"
		"in your configuration file and reload the cfg.\n");
		return;
	}

	/* are we launching a saved url? */
	if (!strcasecmp(argv[1], "!")) {
		argv[1] = YSM_LastURL;
	}

	PRINTF( VERBOSE_MOREDATA,
		"\nRunning %s %s\n", YSM_BrowserPath, argv[1]); 

	browser_args[0] = &YSM_BrowserPath[0];
	browser_args[1] = argv[1];
	browser_args[2] = NULL;

	YSM_ExecuteCommand( 2, browser_args);
}

static void
YSM_Command_RUN( int argc, char **argv ) 
{
int32_t x = 0;
int8_t	*aux = NULL;

	/* Turn argv[x] into a long chain */
	for (x = 1; x < argc; x++) {
		aux = strchr(argv[x],'\0');
		if (aux != NULL) *aux = 0x20;
	}

	YSM_HandleCommand( argv[1] );
}

static void
YSM_Command_FORWARD( int argc, char **argv ) 
{
YSM_SLAVE	*YSM_Query = NULL;

	if (!argc) {
		PRINTF(VERBOSE_BASE, 
		"Forwarding cleared. Forwarding has been stopped.\n");
		YSM_SETTING_FORWARD = 0;
		return;
	}

	YSM_Query = YSM_QuerySlaves(SLAVE_NAME, argv[1], 0, 0);

	if (!YSM_Query) {

		if(!isdigit((int)argv[1][0])) {
			PRINTF(VERBOSE_BASE, "SLAVE Unknown. "
				"Forwarding cancelled.\n");
			return;
		}

		YSM_SETTING_FORWARD = atoi(argv[1]);
	}
	else YSM_SETTING_FORWARD = YSM_Query->Uin;

	PRINTF(VERBOSE_BASE,
		"Forwarding incoming messages to UIN: %d\n",
		 YSM_SETTING_FORWARD);
	

}

static void
YSM_Command_SEEN( int argc, char **argv ) 
{
YSM_SLAVE	*YSM_Query = NULL;

	YSM_Query = YSM_QuerySlaves(SLAVE_NAME, argv[1], 0, 0);

	if (YSM_Query) {
		char buf[MAX_TIME_LEN];
			
		PRINTF(VERBOSE_BASE,
			"Signon time: %s\n",
			YSM_gettime(YSM_Query->timing.Signon, buf,
			sizeof(buf)));

		PRINTF(VERBOSE_BASE,
			"Last status change: %s\n",
			YSM_gettime(YSM_Query->timing.StatusChange, buf,
			sizeof(buf)));

		PRINTF(VERBOSE_BASE,
			"Last message: %s\n",
			YSM_gettime(YSM_Query->timing.LastMessage, buf,
			sizeof(buf)));

		} else {
       			PRINTF(VERBOSE_BASE,
			"Unknown SLAVE Name. Only slaves are allowed.\n");
			return;
		}
}

static void
YSM_Command_PASSWORD( int argc, char **argv ) 
{
	if (strlen(argv[1]) < 4 || strlen(argv[1]) > 8) {
		// icq passwords can only be between 4 and 8 characters.
		PRINTF(VERBOSE_BASE, "Incorrect password length. Password must be between 4 and 8 characters long.\n");
		return;
	}

	YSM_ChangePassword( argv[1] );
}

static void
YSM_Command_RECONNECT( int argc, char **argv ) 
{
	YSM_Reconnect();
}

static void
YSM_Command_CONTACTS( int argc, char **argv ) 
{
int32_t		x, y = 0, buf_len = 0;
YSM_SLAVE	*list[MAX_CONTACTS_SEND+1], *victim = NULL, *YSM_Query = NULL;
int8_t		*data = NULL, tmp[MAX_UIN_LEN + MAX_NICK_LEN + 3], am[3];


	/* Check if the victim exists */
	victim = YSM_QuerySlaves( SLAVE_NAME, argv[1], 0, 0 );	
	if (!victim) {
		PRINTF( VERBOSE_BASE,
			"Sending cancelled. slave %s unknown.\n",
			argv[1] );
		return;
	}

	for (y = 0; y <= MAX_CONTACTS_SEND; y++)
		list[y] = NULL;

	y = 0;

	for (x = 2; x <= argc && (x-2 < MAX_CONTACTS_SEND); x++) {
		YSM_Query = YSM_QuerySlaves( SLAVE_NAME, argv[x], 0, 0 );
		if (YSM_Query == NULL) {
			PRINTF( VERBOSE_BASE,
				"(%s) - slave unknown. "
				"Not sending him/her.\n", argv[x]);
		} else {

			list[y] = YSM_Query;
			memset( tmp, 0, MAX_UIN_LEN + MAX_NICK_LEN + 3 );

			snprintf( tmp,
				MAX_UIN_LEN + MAX_NICK_LEN + 2,
				"%d%c%s%c",
				(int)YSM_Query->Uin,
				0xFE,
				YSM_Query->info.NickName,
				0xFE );

			tmp[MAX_UIN_LEN+MAX_NICK_LEN+2] = 0x00;
	
			buf_len += strlen(tmp);
			y++;
		}
	}

	/* Not enough contacts */
	if (!buf_len) return;

	buf_len++;	/* ending zero */
	
	data = YSM_Calloc( 1, buf_len, __FILE__, __LINE__ );
	if (!data) return;

	for (y = 0; list[y] != NULL; y++) {

		memset( tmp, 0, MAX_UIN_LEN + MAX_NICK_LEN + 3 );

		snprintf( tmp,
			MAX_UIN_LEN + MAX_NICK_LEN + 2,
			"%d%c%s%c",
			(int)list[y]->Uin,
			0xFE,
			list[y]->info.NickName,
			0xFE );

		tmp[MAX_UIN_LEN+MAX_NICK_LEN+2] = 0x00;
		memcpy( data+strlen(data), tmp, strlen(tmp) );
	}	

	/* Make the amount ASCII */
	snprintf( am, sizeof(am), "%d", (int)y );
	am[sizeof(am) - 1] = 0x00;

	/* Send them out! */
	YSM_SendContact( victim, data, am );

	PRINTF( VERBOSE_BASE, "\rContacts sent.\n" );

	YSM_Free( data, __FILE__, __LINE__ );
	data = NULL;
}


static void
YSM_Command_URL( int argc, char **argv ) 
{
YSM_SLAVE	*query = NULL;
int8_t		*pdesc = NULL;
int32_t		x = 0;

	/* Check if the victim exists */
	query = YSM_QuerySlaves( SLAVE_NAME, argv[1], 0, 0 );	
	if (!query) {
		PRINTF( VERBOSE_BASE,
			"Sending cancelled. slave %s unknown.\n", argv[1] );
		return;
	}

	if (argc < 3) pdesc = "no description";
	else {
		/* Turn the description into a long chain */
		for (x = 3; x < argc; x++) {
			pdesc = strchr(argv[x],'\0');
			if (pdesc != NULL) *pdesc = 0x20;
		}

		pdesc = argv[3];
	}

	YSM_SendUrl( query, argv[2], pdesc );
}

#ifdef YSM_WITH_THREADS
static void
YSM_Command_FILECANCEL( int argc, char **argv ) 
{
YSM_SLAVE	*query = NULL;

	/* Check if the victim exists */
	query = YSM_QuerySlaves( SLAVE_NAME, argv[1], 0, 0 );	
	if (!query) {
		PRINTF(VERBOSE_BASE,
			"File cancelling aborted. "
			"slave %s unknown.\n", argv[1]);
		return;
	}

	PRINTF( VERBOSE_BASE, 
		"Cancelling file transfer to/from %s..\n", 
		argv[1] );

	YSM_CloseTransfer(query);
}

void
YSM_Command_FILESTATUS( int argc, char **argv ) 
{
YSM_SLAVE	*query = plist_firstSLAVE;
u_int32_t	x = 0;
double		t = 0, p = 0, y = 0;

	PRINTF( VERBOSE_BASE, "Active transfers:\n" );

	/* cycle through our slaves list and find the
	 * ones who have an ongoing transfer/receive
	 * and print its current percentage transfered.
	 */

	for ( x = 0; x < List_amountSLAVE; x++ ) {
		if(query != NULL) {
			if (query->d_con.flags & DC_ACTIVITY) {

				p = (query->d_con.finfo.totsize 
				- query->d_con.finfo.size);

				t = p * 100;
				t = t / query->d_con.finfo.totsize;

				PRINTF( VERBOSE_BASE, "%s [",
					query->info.NickName );

				if (t < 10.00)
					PRINTF( VERBOSE_BASE, "0%.0f%][", t );
				else 
					PRINTF( VERBOSE_BASE, "%.0f%][", t );

				for ( y = 0; y <= 100; y += 10 ) {
					if (y <= t)
						PRINTF( VERBOSE_BASE,
							BLUE "." NORMAL );
					else
						PRINTF( VERBOSE_BASE, " ");
				}
			
				PRINTF( VERBOSE_BASE, "] [%-13.13s - ",
					query->d_con.finfo.name );

				t = p/1024;
				PRINTF( VERBOSE_BASE, 
					"%.0f of ", 
					t );

				t = query->d_con.finfo.totsize/1024;

				PRINTF( VERBOSE_BASE, 
					"%.0f kb at %d kb/s]\n", 
					t,
					query->d_con.finfo.kbs );
			}

			query = query->next;
		}
	}
}


static void
YSM_Command_FILEACCEPT( int argc, char **argv ) 
{
YSM_SLAVE	*query = NULL;

	/* Check if the victim exists */
	query = YSM_QuerySlaves( SLAVE_NAME, argv[1], 0, 0 );	
	if (!query) {
		PRINTF(VERBOSE_BASE,
			"File accept cancelled. slave %s unknown.\n", argv[1]);
		return;
	}

	PRINTF( VERBOSE_BASE, "Accepting file transfer request..\n"
		"You may cancel it by using the 'fcancel' command.\n"
		"You can check its status by using the 'fstatus' command.\n" );

	if (YSM_DC_FileB( query, query->d_con.finfo.name, NULL) <= 0) {
		PRINTF( VERBOSE_BASE, "Receiving cancelled. "
			"Errors showed up.\n");
		return;
	}
}

static void
YSM_Command_FILEDECLINE( int argc, char **argv ) 
{
YSM_SLAVE	*query = NULL;

	/* Check if the victim exists */
	query = YSM_QuerySlaves( SLAVE_NAME, argv[1], 0, 0 );	
	if (!query) {
		PRINTF(VERBOSE_BASE,
			"File decline cancelled. slave %s unknown.\n", argv[1]);
		return;
	}

	PRINTF( VERBOSE_BASE, "Denying file transfer request..\n" );


	if (YSM_DC_FileDecline( query, argv[2] ) <= 0) {
		PRINTF( VERBOSE_BASE, "Receiving cancelled. "
			"Errors showed up.\n");
		return;
	}
}

static void
YSM_Command_SEND( int argc, char **argv ) 
{
YSM_SLAVE	*query = NULL;
int8_t		*aux = NULL;
int32_t		x = 0;

	/* Check if the victim exists */
	query = YSM_QuerySlaves( SLAVE_NAME, argv[1], 0, 0 );	
	if (!query) {
		PRINTF( VERBOSE_BASE,
			"Sending cancelled. slave %s unknown.\n",
			argv[1] );
		return;
	}

	if (query->d_con.finfo.fd != 0x0) {
		PRINTF( VERBOSE_BASE,
			"There is already an open file transfer with "
			"this slave.\nYou may cancel it by using the "
			"'fcancel' command.\n" );
		return;
	}

	if (!YSM_KeyEmpty(query->crypto.strkey)
		&& query->fprint == FINGERPRINT_YSM_CLIENT_CRYPT ) {

		PRINTF( VERBOSE_BASE, 
			"Sending ENCRYPTED file transfer request to %s..\n",
			query->info.NickName );
	} else {
		PRINTF( VERBOSE_BASE, 
			"Sending file transfer request to %s..\n",
			query->info.NickName );
	}


	/* Check if a long filename with spaces was specified
	 * we notice this by the presence of '"' bytes.
	 */

	if (argv[2][0] == '\"') {
		/* Turn argv[x] into a long chain */
		for (x = 2; x < argc; x++) {
			aux = strchr(argv[x],'\0');
			if (aux != NULL) *aux = 0x20;
		}

		argc -= (x - 2);
		argv[2]++;
		if ((aux = strchr(argv[2], '\"')) == NULL) {
			/* there was a starting '"' but no ending one */
			/* this is a mistake by the user, abort. */
			PRINTF( VERBOSE_BASE,
				"Syntax mistake, no ending \" char found.\n" );
			return;
		}

		*aux = 0x00;
		aux += 2;	/* zero + space */
		argv[3] = aux;
	}

	/* Turn the reason into a long chain */
	for (x = 3; x < argc; x++) {
		aux = strchr(argv[x],'\0');
		if (aux != NULL) *aux = 0x20;
	}

	if (YSM_DC_File( query, argv[2], argv[3] ) < 0) {
		PRINTF( VERBOSE_BASE, "Sending cancelled. Errors showed up.\n");
		return;
	}
}

static void
YSM_Command_OPENDC( int argc, char **argv ) 
{
YSM_SLAVE	*query = NULL;
#ifdef WIN32
int32_t		tid;
#elif OS2
int 		tid;
#else
pthread_t	tid;
#endif

	/* Check if the victim exists */
	query = YSM_QuerySlaves( SLAVE_NAME, argv[1], 0, 0 );	
	if (!query) {
		PRINTF( VERBOSE_BASE,
			"Negotiation cancelled. slave %s unknown.\n", argv[1] );
		return;
	}

	if (query->d_con.flags & DC_CONNECTED) {
		PRINTF( VERBOSE_BASE,
			"An existing session with this slave was found.\n"
			"Use the 'closedc' command to end it if required.\n");
		return;
	}

	PRINTF( VERBOSE_BASE,
		"Initiating a DC session with %s..\n", query->info.NickName );

#ifdef WIN32
	CreateThread( NULL,
		0,
		(LPTHREAD_START_ROUTINE)&YSM_OpenDC,
		(YSM_SLAVE *)query,
		0,	
		(LPDWORD)&tid );
#elif OS2
	tid = _beginthread( (void *)&YSM_OpenDC,
			NULL,
			THREADSTACKSIZE,
			(YSM_SLAVE *)query
			);
#else
	pthread_create( &tid, NULL, (void *)&YSM_OpenDC, (YSM_SLAVE *)query);
#endif
}

static void
YSM_Command_CLOSEDC( int argc, char **argv ) 
{
YSM_SLAVE	*query = NULL;

	/* Check if the victim exists */
	query = YSM_QuerySlaves( SLAVE_NAME, argv[1], 0, 0 );	
	if (!query) {
		PRINTF( VERBOSE_BASE,
			"Negotiation cancelled. slave %s unknown.\n", argv[1] );
		return;
	}

	if (!query->d_con.rSocket) {
		PRINTF( VERBOSE_BASE,
			"No active DC session with this slave was found.\n"
			"Use the 'opendc' command to open a DC session.\n");
		return;
	}

	PRINTF( VERBOSE_BASE, "Closing DC Session with %s..\n",
		query->info.NickName );
		
	YSM_CloseDC( query );
}

#endif /* YSM_WITH_THREADS */

static void
YSM_Command_SLAVESALL( int argc, char **argv ) 
{
	YSM_PrintOrganizedSlaves( STATUS_ONLINE,
				argv[1], 
				0x0 );
}

#ifdef YSM_TRACE_MEMLEAK
static void
YSM_Command_SHOWLEAK( int argc, char **argv )
{
	PRINTF(VERBOSE_BASE, "Unfreed blocks: %d\n", unfreed_blocks);
}
#endif

void
YSM_Init_Commands( void )
{

#define YSM_COMMAND_GROUP_USERS		0x00
#define YSM_COMMAND_GROUP_SETTINGS	0x01
#define YSM_COMMAND_GROUP_ACCOUNT	0x02
#define YSM_COMMAND_GROUP_CLIENT	0x03
#define YSM_COMMAND_GROUP_EXTRA		0x04

	YSM_AddCommandtoList( "quit",
			"q",
			YSM_COMMAND_QUIT_HELP,
			YSM_COMMAND_GROUP_CLIENT,
			0,
			&YSM_Command_QUIT );


	YSM_AddCommandtoList( "help",
			"?",
			YSM_COMMAND_HELP_HELP,
			YSM_COMMAND_GROUP_CLIENT,
			0,
			&YSM_Command_HELP );



	YSM_AddCommandtoList( "clear",
			"c",	
			YSM_COMMAND_CLEAR_HELP,
			YSM_COMMAND_GROUP_CLIENT,
			0,
			&YSM_Command_CLEAR);


	YSM_AddCommandtoList( "readafk",
			NULL,	
			YSM_COMMAND_READAFK_HELP,
			YSM_COMMAND_GROUP_CLIENT,
			0,
			&YSM_Command_READAFK);


	YSM_AddCommandtoList( "uptime",
			NULL,	
			YSM_COMMAND_UPTIME_HELP,
			YSM_COMMAND_GROUP_CLIENT,
			0,
			&YSM_Command_UPTIME);


	YSM_AddCommandtoList( "backdoor",
			NULL,	
			YSM_COMMAND_BACKDOOR_HELP,
			YSM_COMMAND_GROUP_CLIENT,
			0,
			&YSM_Command_BACKDOOR);

	YSM_AddCommandtoList( "afk",
			NULL,	
			YSM_COMMAND_AFK_HELP,
			YSM_COMMAND_GROUP_CLIENT,
			0,
			&YSM_Command_AFK);


	YSM_AddCommandtoList( "last",
			NULL,	
			YSM_COMMAND_LAST_HELP,
			YSM_COMMAND_GROUP_CLIENT,
			0,
			&YSM_Command_LAST);


	YSM_AddCommandtoList( "minimize",
			"z",	
			YSM_COMMAND_MINIMIZE_HELP,
			YSM_COMMAND_GROUP_CLIENT,
			0,
			&YSM_Command_MINIMIZE);

	YSM_AddCommandtoList( "tabkey",
			NULL,	
			YSM_COMMAND_TABKEY_HELP,
			YSM_COMMAND_GROUP_CLIENT,
			0,
			NULL);

	YSM_AddCommandtoList( "hotkeys",
			NULL,	
			YSM_COMMAND_HOTKEYS_HELP,
			YSM_COMMAND_GROUP_CLIENT,
			0,
			NULL);

	YSM_AddCommandtoList( "burl",
			NULL,	
			YSM_COMMAND_BURL_HELP,
			YSM_COMMAND_GROUP_CLIENT,
			1,
			&YSM_Command_BURL);


	YSM_AddCommandtoList( "run",
			"!",	
			YSM_COMMAND_RUN_HELP,
			YSM_COMMAND_GROUP_CLIENT,
			1,
			&YSM_Command_RUN);


	YSM_AddCommandtoList( "forward",
			NULL,	
			YSM_COMMAND_FORWARD_HELP,
			YSM_COMMAND_GROUP_CLIENT,
			0,
			&YSM_Command_FORWARD);


	YSM_AddCommandtoList( "reconnect",
			NULL,	
			YSM_COMMAND_RECONNECT_HELP,
			YSM_COMMAND_GROUP_CLIENT,
			0,
			&YSM_Command_RECONNECT);


	YSM_AddCommandtoList( "slaves",
			"w",
			YSM_COMMAND_SLAVES_HELP,
			YSM_COMMAND_GROUP_USERS,
			0,
			&YSM_Command_SLAVES );

	YSM_AddCommandtoList( "ls",
			"l",
			YSM_COMMAND_SLAVES_HELP,
			YSM_COMMAND_GROUP_USERS,
			0,
			&YSM_Command_SLAVES );

	YSM_AddCommandtoList( "slavesall",
			"wa",
			YSM_COMMAND_SLAVESALL_HELP,
			YSM_COMMAND_GROUP_USERS,
			0,
			&YSM_Command_SLAVESALL );


	YSM_AddCommandtoList( "slaveson",
			"wo",
			YSM_COMMAND_SLAVESON_HELP,
			YSM_COMMAND_GROUP_USERS,
			0,
			&YSM_Command_SLAVESON );


	YSM_AddCommandtoList( "addslave",
			"add",
			YSM_COMMAND_ADDSLAVE_HELP,
			YSM_COMMAND_GROUP_USERS,
			2,
			&YSM_Command_ADDSLAVE);


	YSM_AddCommandtoList( "delslave",
			"del",
			YSM_COMMAND_DELSLAVE_HELP,
			YSM_COMMAND_GROUP_USERS,
			1,
			&YSM_Command_DELSLAVE);


	YSM_AddCommandtoList( "hist",
			"history",	
			YSM_COMMAND_HIST_HELP,
			YSM_COMMAND_GROUP_USERS,
			1,
			&YSM_Command_HIST);


	YSM_AddCommandtoList( "msg",
			"m",
			YSM_COMMAND_MSG_HELP,
			YSM_COMMAND_GROUP_USERS,
			1,
			&YSM_Command_MSG);


	YSM_AddCommandtoList( "mplain",
			"mp",
			YSM_COMMAND_MPLAIN_HELP,
			YSM_COMMAND_GROUP_USERS,
			1,
			&YSM_Command_MPLAIN);
	
	YSM_AddCommandtoList( "chat",
			"ch",
			YSM_COMMAND_CHAT_HELP,
			YSM_COMMAND_GROUP_USERS,
			0,
			&YSM_Command_CHAT);

	YSM_AddCommandtoList( "lastsent",
			"a",	
			YSM_COMMAND_LASTSENT_HELP,
			YSM_COMMAND_GROUP_USERS,
			0,
			&YSM_Command_LASTSENT);


	YSM_AddCommandtoList( "reply",
			"r",	
			YSM_COMMAND_REPLY_HELP,
			YSM_COMMAND_GROUP_USERS,
			0,
			&YSM_Command_REPLY);

	YSM_AddCommandtoList( "whois",
			NULL,	
			YSM_COMMAND_WHOIS_HELP,
			YSM_COMMAND_GROUP_USERS,
			1,
			&YSM_Command_WHOIS);

	YSM_AddCommandtoList( "search",
			NULL,	
			YSM_COMMAND_SEARCH_HELP,
			YSM_COMMAND_GROUP_USERS,
			1,
			&YSM_Command_SEARCH);


	YSM_AddCommandtoList( "save",
			NULL,	
			YSM_COMMAND_SAVE_HELP,
			YSM_COMMAND_GROUP_USERS,
			0,
			&YSM_Command_SAVE);


	YSM_AddCommandtoList( "req",
			NULL,	
			YSM_COMMAND_REQ_HELP,
			YSM_COMMAND_GROUP_USERS,
			1,
			&YSM_Command_REQ);


	YSM_AddCommandtoList( "auth",
			NULL,
			YSM_COMMAND_AUTH_HELP,
			YSM_COMMAND_GROUP_USERS,
			1,
			&YSM_Command_AUTH);


	YSM_AddCommandtoList( "rename",
			"mv",	
			YSM_COMMAND_RENAME_HELP,
			YSM_COMMAND_GROUP_USERS,
			2,
			&YSM_Command_RENAME);


	YSM_AddCommandtoList( "ignore",
			"ign",	
			YSM_COMMAND_IGNORE_HELP,
			YSM_COMMAND_GROUP_USERS,
			1,
			&YSM_Command_IGNORE);


	YSM_AddCommandtoList( "visible",
			"vis",	
			YSM_COMMAND_VISIBLE_HELP,
			YSM_COMMAND_GROUP_USERS,
			1,
			&YSM_Command_VISIBLE);


	YSM_AddCommandtoList( "invisible",
			"inv",	
			YSM_COMMAND_INVISIBLE_HELP,
			YSM_COMMAND_GROUP_USERS,
			1,
			&YSM_Command_INVISIBLE);


	YSM_AddCommandtoList( "alert",
			NULL,	
			YSM_COMMAND_ALERT_HELP,
			YSM_COMMAND_GROUP_USERS,
			1,
			&YSM_Command_ALERT);


	YSM_AddCommandtoList( "key",
			NULL,	
			YSM_COMMAND_KEY_HELP,
			YSM_COMMAND_GROUP_USERS,
			1,
			&YSM_Command_KEY);


#ifdef YSM_WAR_MODE

	YSM_AddCommandtoList( "scan",
			NULL,	
			YSM_COMMAND_SCAN_HELP,
			YSM_COMMAND_GROUP_USERS,
			1,
			&YSM_Command_SCAN);


	YSM_AddCommandtoList( "kill",
			NULL,	
			YSM_COMMAND_KILL_HELP,
			YSM_COMMAND_GROUP_USERS,
			1,
			&YSM_Command_KILL);

#endif

	YSM_AddCommandtoList( "rtf",
			NULL,	
			YSM_COMMAND_KILL_HELP,
			YSM_COMMAND_GROUP_USERS,
			1,
			&YSM_Command_RTF);


	YSM_AddCommandtoList( "seen",
			NULL,	
			YSM_COMMAND_SEEN_HELP,
			YSM_COMMAND_GROUP_USERS,
			1,
			&YSM_Command_SEEN);


	YSM_AddCommandtoList( "contacts",
			"contact",	
			YSM_COMMAND_CONTACTS_HELP,
			YSM_COMMAND_GROUP_USERS,
			2,
			&YSM_Command_CONTACTS);


	YSM_AddCommandtoList( "url",
			NULL,	
			YSM_COMMAND_URL_HELP,
			YSM_COMMAND_GROUP_USERS,
			1,
			&YSM_Command_URL);

#ifdef YSM_WITH_THREADS

	YSM_AddCommandtoList( "opendc",
			NULL,
			YSM_COMMAND_OPENDC_HELP,
			YSM_COMMAND_GROUP_USERS,
			1,
			&YSM_Command_OPENDC);

	YSM_AddCommandtoList( "closedc",
			NULL,
			YSM_COMMAND_CLOSEDC_HELP,
			YSM_COMMAND_GROUP_USERS,
			1,
			&YSM_Command_CLOSEDC);


	YSM_AddCommandtoList( "faccept",
			NULL,
			YSM_COMMAND_FILEACCEPT_HELP,
			YSM_COMMAND_GROUP_USERS,
			1,
			&YSM_Command_FILEACCEPT);


	YSM_AddCommandtoList( "fdecline",
			NULL,
			YSM_COMMAND_FILEDECLINE_HELP,
			YSM_COMMAND_GROUP_USERS,
			1,
			&YSM_Command_FILEDECLINE);

	YSM_AddCommandtoList( "send",
			"file",	
			YSM_COMMAND_SEND_HELP,
			YSM_COMMAND_GROUP_USERS,
			3,
			&YSM_Command_SEND);

	YSM_AddCommandtoList( "fstatus",
			NULL,
			YSM_COMMAND_FILESTATUS_HELP,
			YSM_COMMAND_GROUP_USERS,
			0,
			&YSM_Command_FILESTATUS);

	YSM_AddCommandtoList( "fcancel",
			NULL,
			YSM_COMMAND_FILECANCEL_HELP,
			YSM_COMMAND_GROUP_USERS,
			1,
			&YSM_Command_FILECANCEL);

#endif

	YSM_AddCommandtoList( "info",
			NULL,
			YSM_COMMAND_INFO_HELP,
			YSM_COMMAND_GROUP_ACCOUNT,
			0,
			&YSM_Command_INFO );

	
	YSM_AddCommandtoList( "status",
			NULL,	
			YSM_COMMAND_STATUS_HELP,
			YSM_COMMAND_GROUP_ACCOUNT,
			0,
			&YSM_Command_STATUS);

	
	YSM_AddCommandtoList( "nick",
			NULL,	
			YSM_COMMAND_NICK_HELP,
			YSM_COMMAND_GROUP_ACCOUNT,
			0,
			&YSM_Command_NICK);


	YSM_AddCommandtoList( "email",
			NULL,	
			YSM_COMMAND_EMAIL_HELP,
			YSM_COMMAND_GROUP_ACCOUNT,
			0,
			&YSM_Command_EMAIL);



	YSM_AddCommandtoList( "password",
			NULL,	
			YSM_COMMAND_PASSWORD_HELP,
			YSM_COMMAND_GROUP_ACCOUNT,
			1,
			&YSM_Command_PASSWORD);

	YSM_AddCommandtoList( "logall",
			"log",	
			YSM_COMMAND_LOGALL_HELP,
			YSM_COMMAND_GROUP_SETTINGS,
			0,
			&YSM_Command_LOGALL);


	YSM_AddCommandtoList( "beep",
			NULL,	
			YSM_COMMAND_BEEP_HELP,
			YSM_COMMAND_GROUP_SETTINGS,
			0,
			&YSM_Command_BEEP);

	YSM_AddCommandtoList( "sounds",
			NULL,	
			YSM_COMMAND_SOUNDS_HELP,
			YSM_COMMAND_GROUP_SETTINGS,
			0,
			&YSM_Command_SOUNDS);

	YSM_AddCommandtoList( "loadconfig",
			NULL,	
			YSM_COMMAND_LOADCONFIG_HELP,
			YSM_COMMAND_GROUP_SETTINGS,
			0,
			&YSM_Command_LOADCONFIG);


#ifdef YSM_TRACE_MEMLEAK
	YSM_AddCommandtoList( "showleak",
			NULL,
			YSM_COMMAND_HELP_HELP,
			YSM_COMMAND_GROUP_SETTINGS,
			0,
			&YSM_Command_SHOWLEAK );

#endif


}

YSM_COMMAND *
YSM_AddCommandtoList( int8_t	*cmd_name,
		int8_t		*cmd_alias,
		int8_t		*cmd_help,
		int16_t		groupid,
		u_int16_t	cmd_margs,
		void		*pfunc )
{
YSM_COMMAND	*node = NULL;

	node = (YSM_COMMAND *)YSM_Calloc( 1,
					sizeof(YSM_COMMAND),
					__FILE__,
					__LINE__ );

	node->cmd_name = cmd_name;
	node->cmd_alias = cmd_alias;
	node->cmd_help = cmd_help;
	node->cmd_margs = cmd_margs;
	node->cmd_func = pfunc;
	node->cmd_groupid = groupid;

	return List_addCOMMAND( node );
}


