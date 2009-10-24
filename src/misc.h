/*	$Id: YSM_Win32.h,v 1.14 2005/09/04 01:36:48 rad2k Exp $	*/
/*
-======================== ysmICQ client ============================-
		Having fun with a boring Protocol
-========================= YSM_Win32.h =============================-


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

#ifndef _YSMWIN32H_
#define _YSMWIN32H_

int getkey( void );
void do_backspace( void );
char * YSM_fgets( char *str, int size, char hide);
int PRINTF( int verbose_level, char *fmt,... );
void YSM_PrintGreetingBox( void );
void YSM_PrintWizardBox( int8_t *string );
void YSM_SetConsoleTitle(void);

#ifdef WIN32
extern	HWND g_hYSMHiddenWnd, g_hConsoleWnd;

HWND getConsoleWindow( void );
HWND getConsoleWindowHandlebyTitle(void);
void ScrollConsoleVertical( short n );
void YSM_ScrollConsole( int8_t fl );
int YSM_ReadWindowsProxy( void );
void YSM_WindowHide( HWND );
void YSM_WindowShow( HWND );
void YSM_WindowRegisterHotKey( void );
int32_t YSM_CreateHotKeyWindow(void);
int32_t CtrlHandler( int32_t fdwCtrlType );

#else	/* UNIX */
void CtrlHandler(int32_t sig);
int putch( int c );
int getxy( int32_t *x, int32_t *y );

typedef struct _COORD
{
	int16_t	X;
	int16_t	Y;
} COORD, *PCOORD;

typedef struct _SMALL_RECT {
	int16_t	Left;
	int16_t	Top;
	int16_t	Right;
	int16_t	Bottom;
} SMALL_RECT, *PSMALL_RECT;

#endif	

int8_t * YSM_getpass(int8_t *text);
void YSM_WindowAlert( void );

#if defined (WIN32) || defined (BEOS)
char * getpass( const char *prompt );
#endif /* WIN32 || BEOS */

#endif
