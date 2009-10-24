/*    $Id: YSM_ToolBox.h,v 1.23 2004/08/22 00:12:03 rad2k Exp $    */
/*
-======================== ysmICQ client ============================-
        Having fun with a boring Protocol
-======================== YSM_ToolBox.h ============================-

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

#ifndef _TOOLBOX_H_
#define _TOOLBOX_H_

struct YSM_EVENTS
{
    /* path to the binary used to play sounds */
    int8_t    sbinpath[MAX_PATH];
    /* event definitions */
#define EVENT_INCOMINGMESSAGE        0
    int8_t    execincoming[MAX_DATA_LEN+1];
    int8_t    insound;
#define EVENT_OUTGOINGMESSAGE        1
    int8_t    execoutgoing[MAX_DATA_LEN+1];
    int8_t    outsound;
#define EVENT_ONCOMINGUSER        2
    int8_t    execoncoming[MAX_DATA_LEN+1];
    int8_t    onsound;
#define EVENT_OFFGOINGUSER        3
    int8_t    execoffgoing[MAX_DATA_LEN+1];
    int8_t    offsound;
#define EVENT_LOGOFF            4
    int8_t    execlogoff[MAX_DATA_LEN+1];
    int8_t    logoffsound;
#define EVENT_PREINCOMINGMESSAGE     5
};

extern struct YSM_EVENTS g_events;

void YSM_Event( int8_t    event_t,
    uin_t        r_uin,
    int8_t        *r_nick,
    int32_t        m_len,
    int8_t        *m_data,
    u_int8_t    m_flags );

int32_t YSM_PlaySound( int8_t *filename );

void YSM_Error( int32_t level, int8_t *file, int32_t line, int8_t verbose );

int32_t YSM_LookupStatus( int8_t *name );
void YSM_WriteFingerPrint( int client, char *buf );
void YSM_WriteStatus(u_int16_t status, int8_t *buf);
int32_t YSM_IsValidStatus(u_int16_t status);
int8_t * YSM_GetColorStatus(int8_t *status, int8_t *override);
int8_t * YSM_GetColorByName(int8_t *color);

FILE *YSM_OpenFile( char *fname, char *attr );

int32_t YSM_AppendTopFile(int8_t *filename, int8_t *data);
int32_t YSM_AppendBotFile(int8_t *filename, int8_t *data);

int8_t * YSM_trim(int8_t *str);
ssize_t YSM_tokenize(char *str, const char *sep, char **arr, ssize_t count);

void YSM_GenerateLogEntry(
	int8_t *nick,
	uin_t   uinA,
	uin_t   uinB,
	int8_t *message,
	int32_t mlen);

int32_t YSM_DumpLogFile(int8_t *fname, int8_t *data);
void YSM_Print_Uptime(void);
void YSM_CheckSecurity(void);

u_int32_t Chars_2_DW(u_int8_t *buf);
u_int32_t Chars_2_DWb(u_int8_t *buf);
u_int16_t Chars_2_Word(u_int8_t *buf);
u_int16_t Chars_2_Wordb(u_int8_t *buf);

void DW_2_Chars(u_int8_t *buf, u_int32_t num);
void DW_2_Charsb(u_int8_t *buf, u_int32_t num);
void Word_2_Chars(u_int8_t *buf, const int num);
void Word_2_Charsb(u_int8_t *buf, const int num);

void EncryptPassword(char *Password, char *output);

#define FD_KEYBOARD   0
#define FD_DIRECTCON  1
#define FD_NETWORK    2

void FD_Init(int8_t whichfd);
void FD_Timeout(u_int32_t sec, u_int32_t usec);
void FD_Add(int32_t sock, int8_t whichfd);
void FD_Del(int32_t sock, int8_t whichfd);
int  FD_IsSet(int32_t sock, int8_t whichfd);
int  FD_Select(int8_t whichfd);

long YSM_GetMicroTime(long input);

void YSM_Thread_Sleep(unsigned long seconds, unsigned long ms);

char * YSM_gettime(time_t Time, char *Buffer, size_t Length);
void YSM_CheckCommandsFile(void);

#endif /* _TOOLBOX_H_ */
