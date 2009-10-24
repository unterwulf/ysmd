/*	$Id: YSM_Setup.h,v 1.14 2005/09/04 01:36:48 rad2k Exp $	*/
/*
-======================== ysmICQ client ============================-
		Having fun with a boring Protocol
-========================== YSM_Setup.h ============================-

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

#ifndef _YSMSETUPH_
#define _YSMSETUPH_

int YSM_Initialize( void );
void YSM_Setup(void);

void YSM_ReadConfig( FILE *fd, char reload );

void YSM_CreateConfig(void);

void YSM_SaveConfig(void);
void YSM_ReadSlaves( FILE *fd );

YSM_SLAVE * YSM_QuerySlaves( unsigned short TYPE,
	unsigned char	*Extra,
	uin_t 		Uin,
	unsigned int	reqid ) ;

void YSM_AddSlave ( char *Name, uin_t Uin ) ;
void YSM_AddSlavetoDisk( YSM_SLAVE *victim );
void YSM_DelSlave( YSM_SLAVE *victim, int fl);

FILE * YSM_OpenCFG ( void );

void YSM_CFGStatus ( char *validate ); 
void YSM_AFKMode(u_int8_t turnflag);
void YSM_ReadLog (char *FileName, int logType);
int YSM_DisplayLogEntry(int8_t *buf, int32_t messageNum);

int32_t YSM_VersionCheck(void);

void YSM_AskProxyConfiguration( void );
void YSM_HandleCommand (char *_argone);

void YSM_ExecuteCommand( int argc, char **argv );

#endif
