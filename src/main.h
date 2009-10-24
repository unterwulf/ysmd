/*	$Id: YSM_Main.h,v 1.3 2003/05/11 04:50:13 rad2k Exp $	*/
/*
-======================== ysmICQ client ============================-
		Having fun with a boring Protocol
-========================= YSM_Main.h ==============================-

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

#ifndef _MAIN_H_
#define _MAIN_H_

static void start(void);
static void start_prompt(void);
static void start_network(void);

static void prompt_thread(void);
static void network_thread(void);
static void dc_thread(void);
static void YSM_Start_Cycle(void);

int YSM_SignIn(void);
void YSM_CycleChecks(void);

#endif
