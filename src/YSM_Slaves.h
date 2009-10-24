/*	$Id: YSM_Slaves.h,v 1.8 2004/08/22 00:12:03 rad2k Exp $	*/
/*
-======================== ysmICQ client ============================-
		Having fun with a boring Protocol
-========================= YSM_Slaves.h ============================-

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

#ifndef _YSMSLAVESH_
#define _YSMSLAVESH_

void YSM_PrintOrganizedSlaves ( 
		u_int16_t	FilterStatus,
		int8_t		*Fstring,
		int8_t		FilterIgnore 
		);

void YSM_PrintSlaves ( 
		u_int16_t	FilterStatus,
		int8_t 		*Fstring,
		int8_t		FilterIgnore );

YSM_SLAVE * YSM_AddSlavetoList( char	*Nick,
		uin_t	Uin,
		char	*flags,
		char	*c_key,
		int8_t	*color,
		int	budID,
		int	grpID,
		int	budtype,
		int	fl );

void YSM_FreeSlavefromList( YSM_SLAVE *node );
void YSM_FreeSlavesList( void );
void YSM_DeleteSlavefromList( char *Nick, uin_t Uin);
YSM_SLAVE * YSM_FindSlaveinList( char *Nick, uin_t Uin );
int32_t YSM_ParseSlave( u_int8_t *name );
int YSM_UpdateSlave( char type, char *data, uin_t r_uin );
void YSM_SlaveFlags( YSM_SLAVE *victim, char *flags, char add, char update );


#endif
