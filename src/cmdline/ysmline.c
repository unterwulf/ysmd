/*	$Id: ysmline.c,v 1.5 2004/05/29 18:16:59 rad2k Exp $ */
/*
-======================== ysmICQ client ============================-
		Having fun with a boring Protocol
-========================== ysmline.c ==============================-

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

-------------------------------------------------------------------
This file covers the basic functionality of a readline library
for the unthreaded versions of ysm (which just cant block on readline).
written by <raddy at gmx dot net>
*/

#include "../YSM.h"
__RCSID("$Id: ysmline.c,v 1.5 2004/05/29 18:16:59 rad2k Exp $");

#if !defined(YSM_WITH_THREADS)

#include "../YSM_Win32.h"
#include "../YSM_Wrappers.h"
#include "../YSM_Prompt.h"
#include "ysmline.h"

/*************** tab-key hook *****************************/
ysm_tab_hook_proc ysm_tab_hook = NULL;
/*************** used for special combination of keys *****/
static int8_t	g_inputspecial = 0x00;
/*************** our buffer for the command line **********/
int8_t	g_cmdstring[MAX_CMD_LEN+1];
/*************** old and current lengths of the command line */
static int32_t g_oldcmdlen = 0;
int32_t g_cmdlen = 0;
/*************** command history variables ****************/
static int32_t	g_cmdhistoryindex = 0, g_cmdhistorylast = 0;
static int8_t	g_cmdhistory[MAX_CMD_HIST+1][MAX_CMD_LEN+1];

/*************** pointed to the prompt buffer in ysmreadline() **/
static const int8_t *g_prompt;

/*************** prototypes *****/
static void ysmdrawprompt(void);

static void
ysmdrawprompt(void)
{
	if (g_prompt != NULL) {
		PRINTF( VERBOSE_BASE,
			"\r%s",
			g_prompt );
	}

        fflush(stdout);
}

 /* Command History Function. (UP/DOWN keys)
  * if fl is TRUE, cycle command history UP
  * if fl is FALSE, cycle DOWN
  */

static void
commandhistory( int8_t fl )
{
	if (fl) {
		if (g_cmdhistoryindex > 0) g_cmdhistoryindex--;
		else PRINTF(VERBOSE_BASE, "\a");
	} else {
		if (g_cmdhistoryindex < MAX_CMD_HIST 
		&& strlen(g_cmdhistory[MAX_CMD_HIST - g_cmdhistoryindex]) > 0)
			g_cmdhistoryindex++;

		else PRINTF(VERBOSE_BASE, "\a");
	}

	YSM_ConsoleClearLine(1, g_oldcmdlen);
	memcpy( &g_cmdstring[0],
		&g_cmdhistory[MAX_CMD_HIST - g_cmdhistoryindex][0],
		MAX_CMD_LEN );

	g_cmdlen = strlen(g_cmdstring);
	PRINTF(VERBOSE_BASE, "%s", g_cmdstring);
}

int8_t *
ysmreadline( int8_t *prompt )
{
int8_t input, *outp = NULL;

	/* set the prompt or none */
	if (prompt != NULL) g_prompt = prompt;
	else g_prompt = "";

	/* since this function leaves on every char, only redraw
	 * the prompt if we are in length 0 
	 */
	if (!g_cmdlen) 
		ysmdrawprompt();

	input = getkey();
	if (!input) return outp;

	if (!g_cmdlen) {
		/* is the specified key a known hotkey? */
		if (YSM_ConsoleIsHotKey(input)) return outp;
	}

	switch (input) {

		case '\t':
			g_oldcmdlen = strlen(g_cmdstring);
			if (ysm_tab_hook != NULL)
				ysm_tab_hook();
			break;

		case '\r':
		case '\n':
#ifdef WIN32
			putch('\r');
#endif
			putch('\n');
			g_cmdstring[g_cmdlen] = '\0';
			g_oldcmdlen = g_cmdlen;

			if (strlen(g_cmdstring) > 0) {
				/* do command history backwards */
				memcpy( &g_cmdhistory[MAX_CMD_HIST
					- g_cmdhistorylast][0],
					&g_cmdstring[0],
					MAX_CMD_LEN );

				if (g_cmdhistorylast+1 <= MAX_CMD_HIST) 
					g_cmdhistorylast++;
				else g_cmdhistorylast = 1;

				g_cmdhistoryindex = g_cmdhistorylast;
			}

			outp = YSM_Calloc(1,
					strlen(g_cmdstring)+1,
					__FILE__,
					__LINE__ );

			strncpy(outp, g_cmdstring, strlen(g_cmdstring));

			memset(g_cmdstring, 0, sizeof(g_cmdstring));
			g_cmdlen = 0;
			break;

#ifdef WIN32
		case -32:
#endif
		case 27:	/* may turn into a special key */
			g_inputspecial = 0x01;
			g_oldcmdlen = strlen(g_cmdstring);
			memset(g_cmdstring, 0, sizeof(g_cmdstring));
			g_cmdlen = 0;
			if (g_oldcmdlen) YSM_ConsoleClearLine(1, g_oldcmdlen);
			break;

		case 8:
		case 0x7f:
			if (g_cmdlen) {
				g_cmdlen--;
				g_cmdstring[g_cmdlen] = '\0';
				do_backspace();
			}
			break;

		default:

			/* check if we are expecting a special key */
			if (g_inputspecial == 0x01) {
				g_inputspecial = 0;
				g_oldcmdlen = strlen(g_cmdstring);

				switch (input) {
#ifdef WIN32
					case 'H':	/* Up Arrow */
						commandhistory(TRUE);
						break;

					case 'P':	/* Down Arrow */
						commandhistory(FALSE);
						break;

					case 'I':	/* Page up! */
						scrollconsole(TRUE);
						break;

					case 'Q':	/* Page down! */
						scrollconsole(FALSE);
						break;
#else
					case '[':	/* Pre - Arrow */
						break;
#endif
					default:
					/* Although this might seem wrong, 
					 * we need this default case for not
					 * loosing the next 2 characters 
					 * after an input special. (ESC?)
					 */
						g_inputspecial = 0;
						g_cmdstring[g_cmdlen] = input;
						g_cmdlen++;
						putch(input);
						return NULL;
				}
#ifndef WIN32
				g_inputspecial = 0x02;
#endif
			} else if (g_inputspecial == 0x02) {
				g_oldcmdlen = strlen(g_cmdstring);
				switch (input) {
					case 'A':	/* Up Arrow */
						commandhistory(TRUE);
						break;

					case 'B':	/* Down Arrow */
						commandhistory(FALSE);
						break;
			
					case 'C':	/* Right Arrow */
						break;

					case 'D':	/* Left Arrow */
						break;
				}
				g_inputspecial = 0;

			} else {
				/* this would be the most normal situation :) */
				if (g_cmdlen == MAX_CMD_LEN - 1) {
					PRINTF(VERBOSE_BASE,"\a");
					break;
				}

				g_cmdstring[g_cmdlen] = input;
				g_cmdlen++;
				putch(input);
			}
                break;
	}

	return outp;
}

#endif
