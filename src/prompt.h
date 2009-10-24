/*    $Id: YSM_Prompt.h,v 1.24 2004/08/22 00:12:03 rad2k Exp $    */
/*
-======================== ysmICQ client ============================-
        Having fun with a boring Protocol
-========================= YSM_Prompt.h ============================-

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

#ifndef _PROMPT_H_
#define _PROMPT_H_

#ifdef HAVE_LIBREADLINE
int32_t YSM_ConsoleIsHotKey(int count, int key);
#else
int32_t YSM_ConsoleIsHotKey(int32_t key);
#endif

void YSM_ConsoleRedrawPrompt(int8_t redrawbuf);
void YSM_ConsoleRestore(void);
void YSM_ConsoleSetup(void);
void YSM_ConsoleReadInit(void);
void YSM_ConsoleTab( int8_t *string, int32_t *size, int32_t maxsize );
void YSM_ConsoleClearLine(int8_t redrawprompt, int32_t cmdlen);
void YSM_ConsoleTabCommand(int32_t argc, int8_t **argv, int32_t *pos, size_t msize);
void YSM_ConsoleTabSlave(int32_t argc, int8_t **argv, int32_t *pos, size_t msize);

void YSM_SendMessage(
    uin_t      r_uin,
    int8_t    *data,
    int8_t     logflag,
    slave_t *slave,
    u_int8_t   verbous );

void YSM_PasswdCheck(void);
void YSM_ConsoleRead(void);
void YSM_ParseCommand(int8_t *_input, int32_t *argc, int8_t *argv[]);
void YSM_DoCommand(char *cmd);
void YSM_DoChatCommand(int8_t *cmd);
int8_t * YSM_ReadLongMessage(void);

void YSM_DisplayMsg(
    int16_t   m_type,
    uin_t     r_uin,
    u_int16_t r_status,
    int32_t   m_len,
    int8_t   *m_data,
    u_int8_t  m_flags,
    int8_t   *r_nick,
    int32_t   log_flag);

int32_t YSM_ParseMessageData(u_int8_t *data, u_int32_t length);

void YSM_PreOutgoing(
    slave_t *victim,
    uin_t      r_uin,
    int8_t   **m_data,
    int32_t   *m_len,
    int8_t     logflag);

void YSM_PostOutgoing(
    uin_t    r_uin,
    int8_t  *r_nick,
    int32_t  m_len,
    int8_t  *m_data);

void YSM_ExecuteLine(
    char *line,
    char *extra1,
    char *extra2,
    char *extra3,
    char *extra4);

#endif
