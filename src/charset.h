/*    $Id: YSM_Charset.h,v 1.6 2004/03/21 02:12:26 rad2k Exp $    */
/*
-======================== ysmICQ client ============================-
        Having fun with a boring Protocol
-======================== YSM_Charset.h ============================-

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

#ifndef _CHARSET_H_
#define _CHARSET_H_

#define CHARSET_INCOMING    0x00
#define CHARSET_OUTGOING    0x01

#ifdef YSM_USE_CHARCONV
#include <iconv.h>
#define YSM_ICONV_MAXLEN MAX_DATA_LEN * 4
#endif

void init_charset(void);

int32_t YSM_Charset(
    int8_t     direction,
    int8_t    *buf_from,
    int32_t   *buf_fromlen,
    int8_t   **buf_to,
    u_int8_t   m_flags);

int8_t * YSM_CharsetConvertOutputString(
    int8_t **stringp,
    int8_t   fl_dofree);

int32_t YSM_CharsetConvertString(
    int8_t **stringp,
    int8_t   direction,
    u_int8_t flags,
    int8_t   fl_dofree);

int8_t * YSM_encode64(const int8_t *str);

#endif
