/*	$Id: YSM_ToolBox.c,v 1.89 2005/09/04 01:36:48 rad2k Exp $	*/
/*
-======================== ysmICQ client ============================-
		Having fun with a boring Protocol
-======================== YSM_ToolBox.c ============================-

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

#include "YSM.h"
__RCSID("$Id: YSM_ToolBox.c,v 1.89 2005/09/04 01:36:48 rad2k Exp $");

#include "YSM_ToolBox.h"
#include "YSM_Wrappers.h"
#include "YSM_Prompt.h"
#include "YSM_Setup.h"
#include "YSM_Win32.h"

#include "YSM_FishGUI.h"

extern int8_t	YSM_cfgdir[MAX_PATH];
extern int8_t	YSM_CommandsFile[MAX_DATA_LEN+1];
extern short	YSM_AFKCount;
extern time_t	YSM_StartTime;

static struct timeval tv;
static fd_set read_fds, dc_fds, net_fds;
static int max_read_fd, max_dc_fd, max_net_fd;
struct YSM_EVENTS g_events;

#ifdef WIN32
HWND
getConsoleWindow (
	void
	);

HANDLE g_hSuicideEvent, g_hThreadDiedEvent;
#endif

void
YSM_Event( int8_t	event_t,
	uin_t		r_uin,
	int8_t		*r_nick,
	int32_t		m_len,
	int8_t		*m_data,
	u_int8_t	m_flags )
{
int8_t	uinstring[MAX_UIN_LEN+1], length[10];
int8_t	*event_action = NULL, *event_sound = NULL, event_senable = 0;

	snprintf(uinstring, sizeof(uinstring), "%d", (int)r_uin);
	uinstring[sizeof(uinstring) - 1] = 0x00;
	snprintf(length, sizeof(length), "%d", (int)m_len);
	length[sizeof(length) - 1] = 0x00;

	switch (event_t) {
		case EVENT_INCOMINGMESSAGE:
			event_action = g_events.execincoming;
			event_sound = "incoming_msg.wav";
			event_senable = g_events.insound;
			break;
		case EVENT_OUTGOINGMESSAGE:
			event_action = g_events.execoutgoing;
			event_sound = "outgoing_msg.wav";
			event_senable = g_events.outsound;
			break;
		case EVENT_ONCOMINGUSER:
			event_action = g_events.execoncoming;
			event_sound = "slaveonline.wav";
			event_senable = g_events.onsound;
			break;
		case EVENT_OFFGOINGUSER:
			event_action = g_events.execoffgoing;
			event_sound = "slaveoffline.wav";
			event_senable = g_events.offsound;
			break;
		case EVENT_LOGOFF:
			event_action = g_events.execlogoff;
			event_sound = "logoff.wav";
			event_senable = g_events.logoffsound;
			break;
		case EVENT_PREINCOMINGMESSAGE:
			/* used by FishGUI only */
			FishGUI_event( event_t,
					r_uin,
					r_nick,
					m_len,
					m_data,
					m_flags );
			break;
		default:
			return;
	}

	FishGUI_event(event_t, r_uin, r_nick, m_len, m_data, m_flags);

	/* do we have to execute a user supplied action? */
	if (event_action != 0x00 && *event_action != 0x00) 
		YSM_ExecuteLine( event_action,
				uinstring,
				r_nick,
				length,
				m_data );

	/* do we have to play a sound for this event? */
	if (YSM_SETTING_SOUNDS 
	&& event_senable 
#ifndef WIN32
	&& (*g_events.sbinpath != 0x00)
#endif
	) 
		YSM_PlaySound(event_sound);
}

int32_t
YSM_PlaySound( int8_t *filename )
{
int32_t		size;
int8_t		*path = NULL, *path2 = NULL;
struct stat	filestat;

	/* get the full path to the soundfile */
	size = strlen(YSM_cfgdir) + 1;
	size += strlen(YSM_SOUNDSDIRECTORY) + 1;
	size += strlen(filename) + 1;
	path = YSM_Calloc(1, size, __FILE__, __LINE__ );

	snprintf( path,
		size,
		"%s/%s/%s",
		YSM_cfgdir,
		YSM_SOUNDSDIRECTORY,
		filename );

	path[size - 1] = 0x00;

	if (stat(path, &filestat)) {
		/* file not found. return an error */
		YSM_Free( path, __FILE__, __LINE__ );
		path = NULL;
		return -1;
	}

#ifdef WIN32
	/* use the win32 api (nice!) to play a wave file.
	 * SND_NODEFAULT tells windows not to play a default sound
	 * if the specified path isn't found. */
	PlaySound(path, NULL, SND_NODEFAULT);
#else	
	/* soundfile found. concatenate the full path (binary+sound) */
	size = strlen(g_events.sbinpath) + 1;	/* +1 == 0x20 */
	size += strlen(path) + 1;
	path2 = YSM_Calloc(1, size, __FILE__, __LINE__ );

	snprintf( path2, size, "%s %s", g_events.sbinpath, path );
	path2[size - 1] = 0x00;

	/* play the god damn sound at once! */
	YSM_ExecuteLine( path2, NULL, NULL, NULL, NULL );
	YSM_Free( path2, __FILE__, __LINE__ );
	path2 = NULL;
#endif
	YSM_Free( path, __FILE__, __LINE__ );
	path = NULL;
	return 0;
}

void
YSM_Error( int32_t level, int8_t *file, int32_t line, int8_t verbose )
{
	switch (level) {
		case ERROR_CODE:
			PRINTF(VERBOSE_BASE, "%s",ERROR_CODE_M);
			break;

		case ERROR_NETWORK:
			PRINTF(VERBOSE_BASE, "%s",ERROR_NETWORK_M);
			break;

		case ERROR_CRITICAL:
			PRINTF(VERBOSE_BASE, "%s",ERROR_CRITICAL_M);
			break;
	}

	if (verbose)
		PRINTF(VERBOSE_BASE,"\nFile: %s Line: %d\n", file, line);

	exit(-1);
}

int32_t
YSM_LookupStatus( int8_t *name )
{
u_int32_t x = 0, y, status = 0;

	y = strlen(name);
	for (x = 0; x<y; x++)
		name[x] = toupper(name[x]);

	if (strstr("ONLINE",name)) return STATUS_ONLINE;
	if (strstr("OFFLINE",name)) return STATUS_OFFLINE;
	if (strstr("INVISIBLE",name)) return STATUS_INVISIBLE;
	if (strstr("NA",name)) return STATUS_NA;
	if (strstr("DND",name)) return STATUS_DND;
	if (strstr("OCCUPIED",name)) return STATUS_OCCUPIED;
	if (strstr("FREECHAT",name)) return STATUS_FREE_CHAT;
	if (strstr("AWAY",name)) return STATUS_AWAY;
	if (sscanf(name, "0X%x", &status)) return status;

	return -2;
}


void
YSM_WriteFingerPrint(int client, char *buf)
{
	switch(client) {

		case FINGERPRINT_YSM_CLIENT:
			strncpy(buf," YSM client.", MAX_STATUS_LEN - 1);
			break;
		
		case FINGERPRINT_YSM_CLIENT_CRYPT:
			strncpy(buf," YSM client w/Encryption.",
				MAX_STATUS_LEN - 1);
			break;
		
		case FINGERPRINT_MIRANDA_CLIENT:
			strncpy(buf," Miranda client.",
				MAX_STATUS_LEN - 1);
			break;

		case FINGERPRINT_TRILLIAN_CLIENT:
			strncpy(buf," Trillian client.",
				MAX_STATUS_LEN - 1);
			break;

		case FINGERPRINT_SIMICQ_CLIENT:
			strncpy(buf," SIM client.",
				MAX_STATUS_LEN - 1);
			break;

		case FINGERPRINT_LIB2K_CLIENT:
			strncpy(buf," centerICQ/Ickle client. (libicq2000)",
				MAX_STATUS_LEN - 1);
			break;

		case FINGERPRINT_M2000_CLIENT:
			strncpy(buf," Mirabilis ICQ 2000 client.",
				MAX_STATUS_LEN - 1);
			break;

		case FINGERPRINT_M20012_CLIENT:
			strncpy(buf," Mirabilis ICQ 2001/2002 client.",
				MAX_STATUS_LEN - 1);
			break;

		case FINGERPRINT_M2002_CLIENT:
			strncpy(buf," Mirabilis ICQ 2002 client.",
				 MAX_STATUS_LEN - 1);
			break;

		case FINGERPRINT_MICQLITE_CLIENT:
			strncpy(buf," Mirabilis ICQ LITE client.",
				 MAX_STATUS_LEN - 1);
			break;

		case FINGERPRINT_MICQ2003A_CLIENT_1:
			strncpy(buf," Mirabilis ICQ Pro 2003 client.",
				 MAX_STATUS_LEN - 1);
			break;

		case FINGERPRINT_MICQ_CLIENT:
			strncpy(buf," mICQ client.", MAX_STATUS_LEN - 1);
			break;

		case FINGERPRINT_STRICQ_CLIENT:
			strncpy(buf," StrICQ client.", MAX_STATUS_LEN - 1);
			break;

		case FINGERPRINT_LICQ_CLIENT:
			strncpy(buf," Licq client.", MAX_STATUS_LEN - 1);
			break;

		case FINGERPRINT_ICQ2GO_CLIENT:
			strncpy(buf," ICQ2Go! client.", MAX_STATUS_LEN - 1);
			break;

		case FINGERPRINT_MISC_CLIENT:
		default:
			strncpy(buf, " Client unmatched. (Mirabilis?)",
				MAX_STATUS_LEN - 1);
			break;
	}

	buf[MAX_STATUS_LEN - 1] = '\0';
}

int32_t
YSM_IsValidStatus(u_int16_t status)
{
	switch (status) {
		
		case STATUS_ONLINE:
		case STATUS_OFFLINE:
		case STATUS_INVISIBLE:
		case STATUS_NA:
		case STATUS_NA2:
		case STATUS_DND:
		case STATUS_OCCUPIED:
		case STATUS_FREE_CHAT:
		case STATUS_AWAY:
			return 1;
		default:
			break;
	}

	return 0;
}

void
YSM_WriteStatus(u_int16_t status, int8_t *buf)
{
	switch(status) {

		case STATUS_ONLINE :
			strncpy(buf, "ONLINE", MAX_STATUS_LEN - 1);
			break;

		case STATUS_OFFLINE :
			strncpy(buf, "OFFLINE", MAX_STATUS_LEN - 1);
			break;

		case STATUS_INVISIBLE :
			strncpy(buf, "INVISIBLE", MAX_STATUS_LEN - 1);
			break;

		case STATUS_NA :
		case STATUS_NA2 :
			strncpy(buf, "NA", MAX_STATUS_LEN - 1);
			break;

		case STATUS_DND :
			strncpy(buf, "DND", MAX_STATUS_LEN - 1);
			break;

		case STATUS_OCCUPIED :
			strncpy(buf, "OCCUPIED", MAX_STATUS_LEN - 1);
			break;

		case STATUS_FREE_CHAT :
			strncpy(buf, "FREE4CHAT", MAX_STATUS_LEN - 1);
			break;

		case STATUS_AWAY :
			strncpy(buf, "AWAY", MAX_STATUS_LEN - 1);
			break;

		default:
			strncpy(buf, "UNK", MAX_STATUS_LEN - 1);
			snprintf(buf, MAX_STATUS_LEN, "UNK(%04X)", status);
			break;
	}

	buf[MAX_STATUS_LEN-1] = '\0';
}

int8_t *
YSM_GetColorStatus(int8_t *status, int8_t* override)
{
	if (override)
		return override;

	if (YSM_IsInvalidPtr(status))
		return GREEN;

	if(!strcmp(status, "ONLINE"))		return GREEN;
	if(!strcmp(status, "OFFLINE"))		return WHITE;
	if(!strcmp(status, "NA"))		return BRIGHT_BLACK;
	if(!strcmp(status, "AWAY"))		return CYAN;
	if(!strcmp(status, "INVISIBLE"))	return BRIGHT_BLUE;	
	if(!strcmp(status, "DND"))		return RED;
	if(!strcmp(status, "OCCUPIED"))		return RED;
	if(!strcmp(status, "FREE4CHAT"))	return GREEN;
	
	return GREEN;
}


int8_t *
YSM_GetColorByName(int8_t *color)
{
	if (color == NULL)
		return GREEN;

	if(!strcasecmp(color, "BLACK"))		return BLACK;
	if(!strcasecmp(color, "RED"))		return RED;
	if(!strcasecmp(color, "GREEN"))		return GREEN;
	if(!strcasecmp(color, "BROWN"))		return BROWN;
	if(!strcasecmp(color, "BLUE"))		return BLUE;
	if(!strcasecmp(color, "MAGENTA"))	return MAGENTA;
	if(!strcasecmp(color, "WHITE"))		return WHITE;
	if(!strcasecmp(color, "CYAN"))		return CYAN;
	if(!strcasecmp(color, "GRAY"))		return GRAY;
	if(!strcasecmp(color, "NORMAL"))	return NORMAL;
	if(!strcasecmp(color, "TERMINAL_DEFAULT"))	return TERMINAL_DEFAULT;
	if(!strcasecmp(color, "BRIGHT_BLACK"))	return BRIGHT_BLACK;
	if(!strcasecmp(color, "BRIGHT_BLUE"))	return BRIGHT_BLUE;	
	if(!strcasecmp(color, "BRIGHT_RED"))	return BRIGHT_RED;	
	if(!strcasecmp(color, "BRIGHT_GREEN"))	return BRIGHT_GREEN;	
	if(!strcasecmp(color, "BRIGHT_BROWN"))	return BRIGHT_BROWN;	
	if(!strcasecmp(color, "BRIGHT_MAGENTA"))	return BRIGHT_MAGENTA;	
	if(!strcasecmp(color, "BRIGHT_CYAN"))	return BRIGHT_CYAN;	
	if(!strcasecmp(color, "BRIGHT_GRAY"))	return BRIGHT_GRAY;
	if(!strcasecmp(color, "BRIGHT_WHITE"))	return BRIGHT_WHITE;	
	if(!strcasecmp(color, "BRIGHT_TERMINAL_DEFAULT"))	return BRIGHT_TERMINAL_DEFAULT;
	
	return GREEN;
}

void
YSM_GenerateLogEntry( int8_t	*nick,
		uin_t		uinA,
		uin_t		uinB,
		int8_t		*message,
		int32_t		mlen )
{
char	log_name[MAX_PATH], log_msg[MAX_DATA_LEN+1], *log_data, *aux = NULL;
time_t	log_time;
int	log_len = 0, a = 0;

	if (uinA <= 0 || uinB <= 0)
		return;

	log_time = time(NULL);

	log_len = MAX_DATA_LEN+MAX_NICK_LEN+MAX_UIN_LEN+2;
	log_data = YSM_Calloc(1, log_len, __FILE__, __LINE__);
	memset(log_name, '\0', MAX_PATH);
	memset(log_msg, '\0', sizeof(log_msg));

	/* we log AFK and CHAT messages in the AFK file, the AFK way */
	if ((g_promptstatus.flags & FL_AFKM) ||
			(g_promptstatus.flags & FL_CHATM)) {
		snprintf(log_name, MAX_PATH, "%s", YSM_AFKFILENAME);
	} else
		snprintf(log_name, MAX_PATH, "%d", (int)uinB);

	log_name[sizeof(log_name) - 1] = 0x00;

	if (mlen >= (int32_t)(sizeof (log_msg) - 1))
		mlen = (sizeof (log_msg) - 1);
	
	memcpy( log_msg, message, mlen );

	for(a = 0; a < mlen; a++)
		if(log_msg[a] == '\r'
		|| log_msg[a] == '\n') log_msg[a] = 0x20;

	do {	/* look for our 2 bytes separator */
		aux = strstr(log_msg, YSM_LOG_SEPARATOR);
		if(aux != NULL) *aux = 0x20;
	} while(aux != NULL);

	snprintf( log_data, 
		log_len,
		"<%s|%d> %s %s %s %s",
		nick,
		(int)uinA,
		YSM_LOG_SEPARATOR,
		log_msg,
		YSM_LOG_SEPARATOR,
		ctime(&log_time) );

	log_data[log_len - 1] = 0x00;

	/* theres a newline by ctime at the end */
	YSM_DumpLogFile(log_name,log_data);
	YSM_Free(log_data, __FILE__, __LINE__);
	log_data = NULL;
}

int32_t
YSM_DumpLogFile(int8_t *fname, int8_t *data)
{
	if (fname == NULL || data == NULL)
		return -1;

	if (YSM_SETTING_NEWLOGSFIRST) 
		return YSM_AppendTopFile(fname, data);

	return YSM_AppendBotFile(fname, data);
}

/* YSM_OpenFile: opens fname from ysm's directory.
 * returns a File descriptor.
 */

FILE *
YSM_OpenFile( char *fname, char *attr )
{
FILE	*fd = NULL;
int8_t	*path = NULL;
int32_t	size = 0;

	/* 1 byte for / and another for the ending 0! */
	size = strlen(fname) + strlen(YSM_cfgdir) + 2;
	path = YSM_Calloc(1, size, __FILE__, __LINE__ );
		
	snprintf(path, size, "%s/%s", YSM_cfgdir,fname);
	path[size - 1] = 0x00;

	fd = YSM_fopen( path, attr );
	
	YSM_Free( path, __FILE__, __LINE__ );
	path = NULL;
	
	return fd;
}

int32_t
YSM_AppendBotFile(int8_t *filename, int8_t *data)
{
FILE	*filefd = NULL;

	if (filename == NULL || data == NULL)
		return -1;

	filefd = YSM_OpenFile( filename, "a" );
	if (filefd == NULL) return -1;

	fprintf(filefd, "%s", data);

	YSM_fclose(filefd);
	return 0;
}

/* AppendTopFile: dumps $data to the top of the file $filename
 * if $filename exists, a temporary fd is created holding
 * its contents and is then appended after the new data.
 */

int32_t
YSM_AppendTopFile(int8_t *filename, int8_t *data)
{
FILE	*filefd = NULL, *tmpfd = NULL;
int8_t	buf[MAX_PATH];

	if (filename == NULL || data == NULL)
		return -1;

	filefd = YSM_OpenFile( filename, "r" );
	if (filefd != NULL) {
		/* the file already has contents. we need to keep them */
		tmpfd = tmpfile();
		if (tmpfd == NULL)
			return -1;

		while (!feof(filefd)) {
			memset(buf, 0, sizeof(buf));
			fgets(buf, sizeof(buf)-1, filefd);
			fprintf(tmpfd, "%s", buf);
		}

		YSM_fclose(filefd);
	}

	/* open the filename for writing now */
	filefd = YSM_OpenFile( filename, "w" );
	if (filefd == NULL) {
		if (tmpfd != NULL) YSM_fclose(tmpfd);
		return -1;
	}

	/* write whatever it is we wanted to write at the top */
	fprintf(filefd, "%s", data);

	/* if we have tmpfd open we have old data to append */
	if (tmpfd != NULL) {
		rewind(tmpfd);
		while (!feof(tmpfd)) {
			memset(buf, 0, sizeof(buf));
			fgets(buf, sizeof(buf)-1, tmpfd);
			fprintf(filefd, "%s", buf);
		}

		YSM_fclose(tmpfd);
	}

	YSM_fclose(filefd);
	return 0;

}

/*
  Removes leading and trailing whitespace from str if str is non-NULL.
  Returns: str, if str != NULL
           NULL, if str == NULL
*/
int8_t *
YSM_trim( int8_t *str )
{
int8_t *str2 = NULL;
int8_t *str3 = NULL;

	if (YSM_IsInvalidPtr(str))
		return NULL;

	str2 = str;

	while (isspace(*str2)) ++str2;

	str3 = str2 + strlen(str2);
	while (str3!=str2) if (!isspace(*(--str3))) break;

	if (*str2=='\0')
		*str='\0';
	else {
		int8_t *str4 = str;
		++str3;
		while (str2!=str3) *str4++ = *str2++;
		*str4 = '\0';
	}

	return str;
}

void
YSM_Print_Uptime( void )
{
int	days = 0, hours = 0 , minutes = 0, seconds = 0;

	seconds = time(NULL) - YSM_StartTime;
	minutes = seconds/60;
	hours = minutes/60;
	days = hours/24;

	seconds -= 60*minutes;
	minutes -= 60*hours;
	hours -= 24*days;

	PRINTF( VERBOSE_BASE,
		"Uptime: %d days %d hours %d minutes %d seconds.\n",
		days,
		hours,
		minutes,
		seconds);
}	

#ifndef WIN32 

void YSM_CheckSecurity (void)
{
	if ( !getuid() ) {
		PRINTF(VERBOSE_BASE,
			RED "HOLD IT!" NORMAL " I'm sorry, but i WONT let you run YSM\n");

		PRINTF(VERBOSE_BASE,
			"with uid 0. Don't run ysm as root!. ..fag.\n");

		/* Not using YSM_Exit() here since YSM didn't start */
		exit(-1);
	}
}

#endif


/* Thanks a lot to mICQ for these convertion Functions. 
		I made some Big Endian ones out of them */

u_int32_t Chars_2_DW( u_int8_t * buf )
{
    u_int32_t i;

    i = buf[3];
    i <<= 8;
    i += buf[2];
    i <<= 8;
    i += buf[1];
    i <<= 8;
    i += buf[0];

    return i;
}

u_int32_t Chars_2_DWb( u_int8_t * buf )
{
    u_int32_t i;

    i = buf[0];
    i <<= 8;
    i += buf[1];
    i <<= 8;
    i += buf[2];
    i <<= 8;
    i += buf[3];

    return i;
}


u_int16_t Chars_2_Word (u_int8_t * buf)
{
    u_int16_t i;

    i = buf[1];
    i <<= 8;
    i += buf[0];

    return i;
}

/* nuevo big endian */
u_int16_t Chars_2_Wordb (u_int8_t * buf)
{
    u_int16_t i;

    i = buf[0];
    i <<= 8;
    i += buf[1];

    return i;
}

void DW_2_Chars (u_int8_t * buf, u_int32_t num)
{
    buf[3] = (u_int8_t) ((num) >> 24) & 0x000000FF;
    buf[2] = (u_int8_t) ((num) >> 16) & 0x000000FF;
    buf[1] = (u_int8_t) ((num) >> 8) & 0x000000FF;
    buf[0] = (u_int8_t) (num) & 0x000000FF;
}

void DW_2_Charsb(u_int8_t * buf, u_int32_t num)
{
    buf[0] = (u_int8_t) ((num) >> 24) & 0x000000FF;
    buf[1] = (u_int8_t) ((num) >> 16) & 0x000000FF;
    buf[2] = (u_int8_t) ((num) >> 8) & 0x000000FF;
    buf[3] = (u_int8_t) (num) & 0x000000FF;
}


/* intel little endian */
void Word_2_Chars (u_int8_t * buf, const int num)
{
    buf[1] = (u_int8_t) (((unsigned) num) >> 8) & 0x00FF;
    buf[0] = (u_int8_t) ((unsigned) num) & 0x00FF;
}

/* big endian code */
void Word_2_Charsb (u_int8_t * buf, const int num)
{
    buf[0] = (u_int8_t) (((unsigned) num) >> 8) & 0x00FF;
    buf[1] = (u_int8_t) ((unsigned) num) & 0x00FF;
}


void EncryptPassword (char *Password, char *output)
{

	unsigned int x;
	static const u_int8_t tablilla[] = 
	{
    		0xF3, 0x26, 0x81, 0xC4, 0x39, 0x86, 0xDB, 0x92, 0x71, 0xA3, 0xB9, 0xE6,
    		0x53, 0x7A, 0x95, 0x7C,
	};

	for(x=0;x<strlen(Password);x++)
		*(output+x) =  Password[x] ^ tablilla[x];
}


/* In the following FD functions we handle n fd_sets:	
 * one for p2p sockets, one for network/keyboard and one for the FishGUI
 * why? Because in threaded versions we want thread safe functions. 
 * the p2p functions operate in a separate thread and we dont want
 * our FDs being cleared in the middle of a procedure.
 */

void
FD_Init( int8_t whichfd )
{
	switch (whichfd)
	{
		case FD_KEYBOARD:
			FD_ZERO(&read_fds);
			max_read_fd = 0;
			break;

		case FD_NETWORK:
			FD_ZERO(&net_fds);
			max_net_fd = 0;
			break;

		case FD_DIRECTCON:
			FD_ZERO(&dc_fds);
    			max_dc_fd = 0;
			break;

		case FD_FISHGUI:
			FD_ZERO(&YSM_USER.fishgui.fish_fds);
    			YSM_USER.fishgui.max_fish_fd = 0;
			break;

		default:
			break;
	}
}

void
FD_Timeout( u_int32_t sec, u_int32_t usec )
{
	tv.tv_sec = sec;
	tv.tv_usec = usec;
}

void
FD_Add( int32_t sock, int8_t whichfd )
{
	if (sock < 0) return;


	switch (whichfd)
	{
		case FD_KEYBOARD:
			FD_SET( sock, &read_fds );
			if (sock > max_read_fd)
				max_read_fd = sock;
			break;

		case FD_NETWORK:
			FD_SET( sock, &net_fds );
			if (sock > max_net_fd)
				max_net_fd = sock;
			break;

		case FD_DIRECTCON:
			FD_SET( sock, &dc_fds );
			if (sock > max_dc_fd)
				max_dc_fd = sock;
			break;

		case FD_FISHGUI:
			FD_SET( sock, &YSM_USER.fishgui.fish_fds );
			if (sock > YSM_USER.fishgui.max_fish_fd)
				YSM_USER.fishgui.max_fish_fd = sock;
			break;

		default:
			break;
	}
}

void
FD_Del( int32_t sock, int8_t whichfd )
{
	if (sock < 0) return;
#ifndef WIN32
	switch (whichfd)
	{
		case FD_KEYBOARD:
			FD_CLR( sock, &read_fds );
			if (sock == max_read_fd) max_read_fd = 0;
			break;
		
		case FD_NETWORK:
			FD_CLR( sock, &net_fds );
			if (sock == max_net_fd) max_net_fd = 0;
			break;

		case FD_DIRECTCON:
			FD_CLR( sock, &dc_fds );
			if (sock == max_dc_fd) max_dc_fd = 0;
			break;

		case FD_FISHGUI:
			FD_CLR( sock, &YSM_USER.fishgui.fish_fds );
			if (sock == YSM_USER.fishgui.max_fish_fd)
				YSM_USER.fishgui.max_fish_fd = 0;
			break;

		default:
			break;
	}
#endif
}

int
FD_IsSet( int32_t sock, int8_t whichfd )
{
	if (sock < 0) return 0;

	switch (whichfd)
	{
		case FD_KEYBOARD:
			return (FD_ISSET(sock, &read_fds));

		case FD_NETWORK:
			return (FD_ISSET(sock, &net_fds));

		case FD_DIRECTCON:
			return (FD_ISSET(sock, &dc_fds));

		case FD_FISHGUI:
			return (FD_ISSET(sock, &YSM_USER.fishgui.fish_fds));

		default:
			return -1;
	}


}

int
FD_Select( int8_t whichfd )
{
int res, max_fd = 0;
fd_set	*fds = NULL;
	
	switch (whichfd)
	{
		case FD_KEYBOARD:
			max_fd = max_read_fd;
			fds = &read_fds;
			break;

		case FD_NETWORK:
			max_fd = max_net_fd;
			fds = &net_fds;
			break;

		case FD_DIRECTCON:
			max_fd = max_dc_fd;
			fds = &dc_fds;
			break;

		case FD_FISHGUI:
			max_fd = YSM_USER.fishgui.max_fish_fd;
			fds = &YSM_USER.fishgui.fish_fds;
			break;

		default:
			break;
	}

	/* don't care about writefds and exceptfds: */
	res = select( max_fd + 1, fds, NULL, NULL, &tv);
	if (res == -1)
		FD_ZERO(fds);

	return res;
}


#if defined(WIN32) || defined(BEOS)
int
gettimeofday(struct timeval *_tval)
{
struct _timeb local_t;

	if(!_tval) return -1;

	_ftime(&local_t);

	_tval->tv_sec = local_t.time + local_t.timezone;
	_tval->tv_usec = local_t.millitm * 1000;

	return 0;
}
#endif

/* Get Time in MicroSeconds, and if value is smaller than the MicroSeconds
 * at the very moment, then return -1  
 */

long
YSM_GetMicroTime( long input )
{

struct timeval rtimeout;

#if defined(WIN32) || defined(BEOS)
	if(!gettimeofday (&rtimeout))
#else
	if(!gettimeofday (&rtimeout, NULL))
#endif
	{
		if(!input || rtimeout.tv_usec < input)
			return rtimeout.tv_usec;
	}
	return -1;
}


void
YSM_Thread_Sleep( unsigned long seconds, unsigned long ms )
{
/* Thanks god Sleep in Win32 and OS/2 sleeps the thread :) */
#ifdef WIN32	
	Sleep( (seconds*1000) + ms );
#elif OS2
	_sleep2( (seconds*1000) + ms);

	struct timespec {
		time_t	tv_sec;		/* seconds */
		long	tv_nsec;	/* nanoseconds */
	};
#else
#ifdef YSM_WITH_THREADS
	struct timeval  now;
	struct timespec expected;
	pthread_mutex_t	condition_mutex;
	pthread_cond_t	condition_cond;

	/* Unix function */
	gettimeofday (&now, NULL);

	expected.tv_sec = now.tv_sec + seconds;
	expected.tv_nsec = (now.tv_usec * 1000) + (ms * 1000000);

	/*** don't let nsec become seconds ***/
	if (expected.tv_nsec >= 1000000000) {
		expected.tv_sec += 1;
		expected.tv_nsec -= 1000000000;
	}
	
	pthread_mutex_init( &condition_mutex, NULL );
	pthread_mutex_lock( &condition_mutex );
	
	/* Now! Go to sleep for a second, y0 arent paid for nuthn boy */
	pthread_cond_init( &condition_cond, NULL );
	pthread_cond_timedwait( &condition_cond, &condition_mutex, &expected );
	pthread_cond_destroy( &condition_cond );

	pthread_mutex_unlock( &condition_mutex );
	pthread_mutex_destroy( &condition_mutex );
#else
	sleep( seconds );
#endif /* YSM_WITH_THREADS */
#endif /* WIN32 */
}

/* Called by all Threads to check if they should exit cleanly */
#if defined(WIN32) 
void
YSM_CommitSuicide( void )
{
#if defined(YSM_WITH_THREADS)
	/* g_hSuicideEvent is signaled to kill cleanly all threads */
	if (WAIT_OBJECT_0 == WaitForSingleObject( g_hSuicideEvent, 0)) {
		SetEvent( g_hThreadDiedEvent );
		ExitThread(0);
	}
#endif
}
#endif

char *
YSM_gettime(time_t Time, char *Buffer, size_t Length)
{
	if (Time) {
		struct tm *tp;

		tp = localtime(&Time);	/* FIXME: not thread safe */
		strftime(Buffer, Length, "%d %b %Y %H:%M:%S", tp);
	} else
		strcpy(Buffer, "Unknown");

	return (Buffer);
}


void
YSM_CheckCommandsFile( void )
{
FILE	*fd;
int8_t	data[MAX_CMD_LEN+1], *aux = NULL;
int8_t	*argv[MAX_CMD_ARGS], *myp = NULL, fl_multi = 0;
int32_t	argc = 0, x = 0;

	if (YSM_CommandsFile[0] == 0x00) return;

	fd = YSM_fopen( YSM_CommandsFile, "rw" );
	if (fd == NULL) return;

	/* Execute each command */
	while (!feof(fd)) {

		fl_multi = 0;
		memset(data, 0, sizeof(data));
		myp = fgets(data, sizeof(data) - 1, fd);

		aux = strchr(data, '\n');
		if (aux != NULL) *aux = 0x00;

		if (data[0] == 0x00) continue;

		for (x = 0; x < MAX_CMD_ARGS; x++) 
			argv[x] = NULL;

		YSM_ParseCommand( data, &argc, argv );
		if (argv[0] == NULL) continue;

		/* check if we have a multiline message first */
		if (argc == 1 && (!strcasecmp(argv[0], "msg") 
		|| !strcasecmp(argv[0], "m") 
		|| !strcasecmp(argv[0], "mp")
		|| !strcasecmp(argv[0], "mplain"))) {
			fl_multi = 1;
		}

		/* Restore parsed spaces */
		for (x = 0; x < argc; x++) {
			aux = strchr(argv[x],'\0');
			if (aux != NULL) *aux = 0x20;
		}

		if (fl_multi) {
			/* seems we have a multiline!
			 * read until we find a '.' or EOF then.  
			 * oh and add a missing space first.
			 */
			strncat(data+strlen(data), 
				" ", 
				sizeof(data) - strlen(data) - 1);
			do {
				myp = fgets( data+strlen(data),
					(sizeof(data) - 1) - strlen(data),
					fd );
	
			} while (myp != NULL && !feof(fd) 
				&& (strlen(myp) != 1 && myp[0] != '.'));

			/* if we have the final '.', step on it! heh */
			if (myp[0] == '.') myp[0] = 0x00;	
		}

		/**** special case check. If we are quitting we will need
		 * to close the fd first and clear the file. otherwise we
		 * will never get out of the DoCommand function..turururu
		 */
	
		if (!strcasecmp(data, "quit")) {
			/* close & clear the file */
			YSM_fclose(fd);
			fd = YSM_fopen( YSM_CommandsFile, "w" );
			if (fd == NULL) return;
			YSM_fclose(fd);
		}	

		PRINTF( VERBOSE_BASE,
			"\n\rRunning command from %s\n",
			YSM_CommandsFile );

		YSM_DoCommand(data);
	}

	/* Clear the file */
	YSM_fclose(fd);
	fd = YSM_fopen( YSM_CommandsFile, "w" );
	if (fd == NULL) return;
	YSM_fclose(fd);
}

/*
  The string str is split into tokens stored in array arr of maximum length
  count.
  Tokens are delimited by sep.
  Occurrences of sep are not treated as part of any token.
  The number of tokens in str is the number of occurrences of sep PLUS 1.
  If str starts with sep, the first token is an empty string. If str ends with
  sep, the last token is an empty string.

  All token string pointers point directly into str which is destroyed
  while tokenizing.

  Returns: -1 if str, sep or arr is NULL or if count<0 or if strlen(sep)==0
           the number of tokens written to arr (never more than count)
*/

ssize_t
YSM_tokenize(char* str, const char* sep, char** arr, ssize_t count)
{
ssize_t ret = 0, len = -1;

	if (sep != NULL) 
		len = strlen(sep);

	if (str == NULL || sep == NULL || arr == NULL || len <= 0 || count < 0)
		return -1;

	while (count>0) {
		*arr = str;
		--count;
		++ret;
		++arr;

		str = strstr(str, sep);
		if (str == NULL) break;
		*str='\0';
		str += len;
	}

	return ret;
}
