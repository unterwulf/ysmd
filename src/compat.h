/*	$Id: YSM_Compat.h,v 1.3 2005/02/06 01:10:20 rad2k Exp $	*/
/*
-======================== ysmICQ client ============================-
		Having fun with a boring Protocol
-======================== YSM_Compat.h ================================-

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

#ifndef _YSMCOMPATH_
#define _YSMCOMPATH_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>

#ifdef OS2
#include "YSM_os2.h"
#endif

#ifdef WIN32
#include <conio.h>
#include <winsock.h>
#include <sys/timeb.h>
#include <sys/stat.h>
#include <windows.h>
#include <mmsystem.h>
#else
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <signal.h>
#include <termios.h>
#include <unistd.h>
#endif

#if defined(BEOS) || defined(OS2)
#include <sys/select.h>
#endif

#include "YSM_Lang.h"
#include "rijndael/rijndael-api-fst.h"

#ifdef WIN32
#define YSM_USE_CHARCONV
#endif

#ifdef WIN32
#define YSM_WITH_THREADS
#endif

#define THREADS_COUNT	0x3

#if defined(YSM_WITH_THREADS)
#if !defined(WIN32) && (!defined(OS2))
#include <pthread.h>
#endif
#endif

#ifdef WIN32
#define putch		_putch
#define	close		closesocket
#define snprintf	_snprintf
#define strcasecmp	_stricmp
#define strncasecmp	_strnicmp
#endif

/* used in *nix systems lacking posix data types */
#ifdef MISSING_SIGNED	
typedef signed char	int8_t;
typedef signed short	int16_t;
typedef signed int	int32_t;
#endif
#ifdef MISSING_UNSIGNED
typedef	unsigned char	u_int8_t;
typedef unsigned short	u_int16_t;
typedef unsigned int	u_int32_t;
#endif

#ifdef WIN32
typedef signed char	int8_t;
typedef	unsigned char	u_int8_t;
typedef signed short	int16_t;
typedef unsigned short	u_int16_t;
typedef signed int	int32_t;
typedef unsigned int	u_int32_t;
typedef SSIZE_T		ssize_t;
#else
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif

#ifndef __RCSID
#define __RCSID(x)
#endif

#if !defined (WIN32) && !defined (TRUE)
#define FALSE 0
#define TRUE !FALSE 
#endif

#ifdef WIN32
#define SOCK_READ(x,y,z) recv(x,y,z,0)
#define SOCK_WRITE(x,y,z) send(x,y,z,0)
#else
#define SOCK_READ(x,y,z) read(x,y,z)
#define SOCK_WRITE(x,y,z) write(x,y,z)
#endif

#define NUM_ELEM_ARR(arr)	((ssize_t)(sizeof((arr))/sizeof((arr)[0])))

#define WHITE "\x1B[0;1;37m"
#define TERMINAL_DEFAULT "\x1B[0m"
#define BRIGHT_TERMINAL_DEFAULT "\x1B[0;1m"

#ifndef YSM_MONOCHROME
#define BLACK "\x1B[0;30m"
#define RED "\x1B[0;31m"
#define GREEN "\x1B[0;32m"
#define BROWN "\x1B[0;33m"
#define BLUE "\x1B[0;34m"
#define MAGENTA "\x1B[0;35m"
#define CYAN "\x1B[0;36m"
#define GRAY "\x1B[1;30m"
#define NORMAL "\x1B[0;37m"

#define BRIGHT_BLACK	"\x1B[0;1;30m"
#define BRIGHT_RED	"\x1B[0;1;31m"
#define BRIGHT_GREEN	"\x1B[0;1;32m"
#define BRIGHT_BROWN	"\x1B[0;1;33m"
#define BRIGHT_BLUE	"\x1B[0;1;34m"
#define BRIGHT_MAGENTA	"\x1B[0;1;35m"
#define BRIGHT_CYAN	"\x1B[0;1;36m"
#define BRIGHT_GRAY	"\x1B[0;0;37m"
#define BRIGHT_WHITE	"\x1B[0;1;37m"
#else
#define RED	WHITE
#define GREEN	WHITE
#define BROWN	WHITE
#define BLUE	WHITE
#define MAGENTA	WHITE
#define CYAN	WHITE
#define GRAY	WHITE
#define NORMAL	WHITE

#define BRIGHT_BLACK	WHITE
#define BRIGHT_RED	WHITE
#define BRIGHT_GREEN	WHITE
#define BRIGHT_BROWN	WHITE
#define BRIGHT_BLUE	WHITE
#define BRIGHT_MAGENTA	WHITE
#define BRIGHT_CYAN	WHITE
#define BRIGHT_GRAY	WHITE
#define BRIGHT_WHITE	WHITE
#endif

#define CLRSCR		"\033[2J"
#define HOMECUR		"\033[0;0H"
#define BOLD		"\x1B[1m"

#define DECONIFY	"\x1B[1;1;1t"
#define ICONIFY		"\x1B[2;2;2t"

#endif
