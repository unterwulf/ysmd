/*	$Id: YSM_FishGUI.h,v 1.6 2004/01/03 20:59:29 rad2k Exp $ */
/*
-======================== ysmICQ client ============================-
		Having fun with a boring Protocol
-======================== YSM_FishGUI.h ===========================-

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

#ifndef _YSMFISHGUIH_
#define _YSMFISHGUIH_

void FishGUI_init(void);
void FishGUI_runcmds(void);

int32_t FishGUI_send_message(int32_t uin, int8_t *data, int32_t len);

void
FishGUI_event(int8_t event,
	int32_t uin,
	int8_t *nick,
	int32_t len,
	int8_t *data,
	int32_t m_flags 
	);

#endif

