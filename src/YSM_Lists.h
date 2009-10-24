/*	$Id: YSM_Lists.h,v 1.4 2003/10/01 01:48:50 rad2k Exp $	*/
/*
-======================== ysmICQ client ============================-
		Having fun with a boring Protocol
-========================= YSM_Lists.h =============================-

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

#ifndef _YSMLISTSH_
#define _YSMLISTSH_

extern YSM_SLAVE	*plist_firstSLAVE;
extern YSM_COMMAND	*plist_firstCOMMAND;
extern FileMap		*plist_firstFILEMAP;
extern u_int32_t List_amountSLAVE, List_amountCOMMAND;

void List_init( void );
void List_freelistSLAVE( void );
void List_freelistCOMMAND( void );
void List_freelistFILEMAP( void );
void List_freelist( void );
YSM_SLAVE * List_addSLAVE( YSM_SLAVE *node );
YSM_COMMAND * List_addCOMMAND( YSM_COMMAND *node );
pFileMap List_addFILEMAP( pFileMap node );
void List_delSLAVE( YSM_SLAVE *node );
void List_delCOMMAND( YSM_COMMAND *node );
void List_delFILEMAP( pFileMap node );

#endif
