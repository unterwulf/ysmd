/*	$Id: YSM_os2.h,v 1.3 2004/03/26 01:17:53 rad2k Exp $	*/
/*
-======================== ysmICQ client ============================-
		Having fun with a boring Protocol
-========================== YSM_os2.h ==============================-


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
#ifdef OS2

struct KeyPacket {
	int ext_key;
	int shift_key;
	unsigned char	key;
	unsigned char	scancode;
};

#define THREADSTACKSIZE 64*1024

void os2_set_edit_cursor(int);
void os2_readkbd(struct KeyPacket *);
void os2_set_title(char *title);
char* os2_get_clip(void);
void os2_set_clip(char *text);
int os2_init(void);
int os2_startsession(char **,int argc);
void os2_deinit(void);
void os2_minimize_console(void);
void YSM_WindowAlert(void);
int os2_showin(void);
int os2_flashchange(int);
void os2_flashtitle(void);

#define INCL_DOSSESMGR /* For DosStartSession */
#define INCL_DOSPROCESS
#define INCL_VIO
#define INCL_WIN
#define INCL_KBD

#include <os2.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define MAXPATH 1024

#endif

