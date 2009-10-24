/*    $Id: YSM_Compat.h,v 1.3 2005/02/06 01:10:20 rad2k Exp $    */
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

#ifndef _COMPAT_H_
#define _COMPAT_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>

#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <signal.h>
#include <termios.h>
#include <unistd.h>

#include "lang.h"
#include "rijndael/rijndael-api-fst.h"

#define THREADS_COUNT    0x3

#if defined(YSM_WITH_THREADS)
#include <pthread.h>
#endif

/* used in *nix systems lacking posix data types */
#ifdef MISSING_SIGNED
typedef signed char    int8_t;
typedef signed short   int16_t;
typedef signed int     int32_t;
#endif

#ifdef MISSING_UNSIGNED
typedef unsigned char  u_int8_t;
typedef unsigned short u_int16_t;
typedef unsigned int   u_int32_t;
#endif

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>

#ifndef __RCSID
#define __RCSID(x)
#endif

#if !defined (TRUE)
#define FALSE 0
#define TRUE !FALSE
#endif

#define SOCK_READ(x,y,z) read(x,y,z)
#define SOCK_WRITE(x,y,z) write(x,y,z)

#define NUM_ELEM_ARR(arr)    ((ssize_t)(sizeof((arr))/sizeof((arr)[0])))

#endif
