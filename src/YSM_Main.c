/*	$Id: YSM_Main.c,v 1.104 2004/08/22 00:12:03 rad2k Exp $	*/
/*
-======================== ysmICQ client ============================-
		Having fun with a boring Protocol
-========================= YSM_Main.c ==============================-

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
__RCSID("$Id: YSM_Main.c,v 1.104 2004/08/22 00:12:03 rad2k Exp $");

#include "YSM_Main.h"
#include "YSM_Direct.h"
#include "YSM_Setup.h"
#include "YSM_Prompt.h"
#include "YSM_Win32.h"
#include "YSM_ToolBox.h"
#include "YSM_Network.h"
#include "YSM_Commands.h"
#include "YSM_FishGUI.h"

short	YSM_Reason_To_Suicide = FALSE;

short	YSM_SETTING_VERBOSE = 0x5;
short	YSM_SETTING_BEEP = 1;
short	YSM_SETTING_SOUNDS = TRUE;
short	YSM_SETTING_LOGALL = FALSE;
short	YSM_SETTING_NEWLOGSFIRST = TRUE;
short	YSM_SETTING_SPOOF = FALSE;
short	YSM_SETTING_AWAYTIME = 5;
short	YSM_SETTING_AFKMAXSHOWN = 3;
short	YSM_SETTING_AFKMINIMUMWAIT = MINIMUM_AFK_WAIT;
short	YSM_SETTING_WINALERT = 0x3;
short	YSM_SETTING_VERSION_CHECK = TRUE;
short	YSM_SETTING_ANTISOCIAL = 0;
short	YSM_SETTING_UPDATENICKS = TRUE;

short	YSM_SETTING_DCDISABLE = FALSE;
short	YSM_SETTING_DCLAN = FALSE;

int8_t	*YSM_SETTING_COLOR_MESSAGE = GREEN;
int8_t	*YSM_SETTING_COLOR_TEXT	= NORMAL;
int8_t	*YSM_SETTING_COLOR_STATUSCHANGENAME = BLUE;
int8_t	*YSM_SETTING_COLOR_STATUSCHANGESTATUS = NULL;
int8_t	*YSM_SETTING_COLOR_TEXT2 = MAGENTA;

u_int16_t YSM_SETTING_DCPORT1 = 0;
u_int16_t YSM_SETTING_DCPORT2 = 0;

/* needs to store a 4 bytes UIN */
uin_t	YSM_SETTING_FORWARD = 0;

char	YSM_SETTING_HOT_KEY_MAXIMIZE = 's';
char	YSM_SETTING_CHARSET_TRANS[MAX_CHARSET + 4];
char	YSM_SETTING_CHARSET_LOCAL[MAX_CHARSET + 4];

extern	char	YSM_AFKMessage[MAX_DATA_LEN + 1];
extern	char	YSM_cfgfile[MAX_PATH];
extern	char	YSM_cfgdir[MAX_PATH];

#ifdef WIN32
extern	HANDLE	g_hSuicideEvent, g_hThreadDiedEvent;
#endif

YSM_SLAVE *YSMSlaves_LastSent=0, *YSMSlaves_LastRead=0;
YSM_SLAVE *YSMSlaves_TabSlave = NULL;

struct	YSM_MODEL	YSM_USER;

time_t	YSM_LastKA, YSM_idletime;
time_t	YSM_StartTime, YSM_LastCmdsTime;

#ifdef YSM_WITH_THREADS

#ifdef WIN32
HANDLE	_thHandle[THREADS_COUNT];
DWORD	_thid[THREADS_COUNT];
#elif OS2
int	t_netid, t_cycleid, t_dcid;
#else
pthread_t	t_netid, t_cycleid, t_dcid;
#endif

#endif


int
main( int argc, char **argv )
{

char buf[MAX_TIME_LEN];

#if !defined (WIN32) && !defined (OS2)
	YSM_CheckSecurity();
#endif

#ifdef OS2
	/* init of the OS2 PM function */
	os2_init();	
#endif

	YSM_StartTime = YSM_LastCmdsTime = time( NULL );

	/* Check for arguments - alternate config file */
	if (argc > 2) {
		if (!strcmp( argv[1],"-c" )) {
			/* Use this configuration file */
			if (argv[2] != NULL) {
				char *aux = NULL;

				strncpy( YSM_cfgfile,
					argv[2],
					sizeof(YSM_cfgfile) - 1);
				YSM_cfgfile[sizeof(YSM_cfgfile)-1] = '\0';

				strncpy( YSM_cfgdir,
					argv[2],
					sizeof(YSM_cfgdir) - 1);
				YSM_cfgdir[sizeof(YSM_cfgdir)-1] = '\0';
#ifdef WIN32
				aux = strrchr( YSM_cfgdir,'\\' );
#else
				aux = strrchr( YSM_cfgdir,'/' );
#endif
				if (NULL != aux) *(aux+1) = '\0';
				else {
#ifdef WIN32
					strncpy( YSM_cfgdir,
						".\\",
						sizeof(YSM_cfgdir) - 1);
#else
					strncpy( YSM_cfgdir,
						"./",
						sizeof(YSM_cfgdir) - 1);
#endif
					YSM_cfgdir[sizeof(YSM_cfgdir)-1] = '\0';
				}
					
			}
		}
	}

	if (YSM_Initialize() < 0) return -1;

	YSM_ConsoleSetup();
	YSM_PrintGreetingBox();
	YSM_Setup();

	/* Moved direct connections intialization to main() because
	 * we need a few configuration file settings loaded before.
	 */
#ifdef YSM_WITH_THREADS
	YSM_DC_Init();
#endif

	YSM_SetConsoleTitle();

        PRINTF( VERBOSE_BASE,
		"\n%s %d ]\n",
		MSG_STARTING_TWO,
		YSM_USER.Uin );

	PRINTF( VERBOSE_BASE,
		"Cronica TV hora Actual : %s\n",
		YSM_gettime(YSM_StartTime, buf, sizeof(buf)));

	PRINTF( VERBOSE_BASE,
		GREEN "love ysm? " NORMAL "make your contribution! " GREEN "(ysmv7.sourceforge.net)" NORMAL "\n");


	YSM_PasswdCheck();

	if (YSM_SETTING_VERSION_CHECK) YSM_VersionCheck();

#ifdef YSM_WITH_THREADS
#ifdef WIN32
		_thHandle[0] = CreateThread(
				NULL,
				0,
				(LPTHREAD_START_ROUTINE)&YSM_Start_Network,
				NULL,
				0,
				(LPDWORD)&_thid[0] );

		_thHandle[1] = CreateThread(
				NULL,
				0,
				(LPTHREAD_START_ROUTINE)&YSM_Start_Cycle,
				NULL,
				0,
				(LPDWORD)&_thid[1] );

	if (!YSM_SETTING_DCDISABLE)
		_thHandle[2] = CreateThread(
				NULL,
				0,
				(LPTHREAD_START_ROUTINE)&YSM_Start_DC,
				NULL,
				0,
				&_thid[2] );

#elif OS2
	t_netid = _beginthread( (void *)&YSM_Start_Network,
				NULL,
				THREADSTACKSIZE,
				NULL
				);

	t_cycleid = _beginthread( (void *)&YSM_Start_Cycle,
				NULL,
				THREADSTACKSIZE,
				NULL
				);

	if (!YSM_SETTING_DCDISABLE)
		t_dcid = _beginthread( (void *)&YSM_Start_DC,
				NULL,
				THREADSTACKSIZE,
				NULL
				);
#else
	pthread_create( &t_netid, NULL, (void *)&YSM_Start_Network, NULL );
	pthread_create( &t_cycleid, NULL, (void *)&YSM_Start_Cycle, NULL );
	
	if (!YSM_SETTING_DCDISABLE)
		pthread_create( &t_dcid, NULL, (void *)&YSM_Start_DC, NULL );

#endif
	/* Take the main thread for the prompt */
	YSM_Start_Prompt();
#else
	YSM_Start ();
#endif

	return( 0 );
}


void
YSM_Start_Prompt( void )
{

	YSM_ConsoleReadInit();

#ifdef YSM_WITH_THREADS
	YSM_LastKA = YSM_idletime = time(NULL);
	
	while (!YSM_Reason_To_Suicide) {

#endif
		FD_Init(FD_KEYBOARD);
		FD_Add(0, FD_KEYBOARD);

#ifndef YSM_WITH_THREADS

		FD_Select(FD_KEYBOARD);

		if (FD_IsSet(0, FD_KEYBOARD))
#endif
			YSM_ConsoleRead();

#ifdef YSM_WITH_THREADS
		YSM_Thread_Sleep( 0, 10 );
#ifdef WIN32
		/* This function will make the thread exit cleanly	*/
		/* in case YSM is called to exit			*/
		YSM_CommitSuicide();
#endif
	}
#endif

}

void
YSM_Start_Network( void )
{

#ifdef YSM_WITH_THREADS

	if (YSM_SignIn() < 0) YSM_Error(ERROR_NETWORK, __FILE__, __LINE__, 0);

	while (!YSM_Reason_To_Suicide) {
#ifdef OS2
		/* fixes an infinite loop with 100% cpu? */
		usleep(1);
#endif
#else
		FD_Timeout(0, 1000);
		FD_Init(FD_NETWORK);
		if (YSM_USER.network.rSocket)
			FD_Add(YSM_USER.network.rSocket, FD_NETWORK);

		FD_Select(FD_NETWORK);

		if (FD_IsSet(YSM_USER.network.rSocket, FD_NETWORK))
#endif
			YSM_SrvResponse();

#ifndef YSM_WITH_THREADS
		YSM_CycleChecks ();
#endif

#ifdef YSM_WITH_THREADS
#ifdef WIN32
		/* This function will make the thread exit cleanly	*/
		/* in case YSM is called to exit			*/
		YSM_CommitSuicide();
#endif
	}
#endif

}

/* 
 * YSM_Start()  used when there is no Threads Support.
 * What it does? Instead of re-writing a big function
 * we only write stuff that needs to be done once, and
 * on each Start function we leave out the while loops.
 * Hence, we make a while in this function. Weird but nice :)
 */

#ifndef YSM_WITH_THREADS
void
YSM_Start( void )
{
	YSM_LastKA = YSM_idletime = time(NULL);

	if (YSM_SignIn() < 0) YSM_Error(ERROR_NETWORK, __FILE__, __LINE__, 0);

	while (!YSM_Reason_To_Suicide) {
		YSM_Start_Prompt ();
		YSM_Start_Network ();
	}
}

#else
void 
YSM_Start_Cycle( void )
{
#ifdef WIN32

	if (YSM_USER.fishgui.port 
		&& YSM_USER.fishgui.socket > 0 
		&& YSM_USER.fishgui.hide_console) {

		/* Hide the console for FishGUI */
		HWND YSM_HWND = getConsoleWindowHandlebyTitle();

		if (YSM_HWND != NULL) 
			ShowWindow(YSM_HWND, SW_HIDE);

	} else {
		if (YSM_CreateHotKeyWindow() >= 0) {
			/* register our YSM super dope hot key! */
			YSM_WindowRegisterHotKey();
		}
	}

#endif
	while(!YSM_Reason_To_Suicide) {
	
		YSM_CycleChecks();
		YSM_Thread_Sleep(0, 100);

		YSM_DC_Select();

#ifdef WIN32
		/* This function will make the thread exit cleanly	*/
		/* in case YSM is called to exit			*/
		YSM_CommitSuicide();
#endif
	}

}

void 
YSM_Start_DC( void )
{
YSM_SLAVE *slave = NULL;

	while(!YSM_Reason_To_Suicide) {

		slave = YSM_DC_Wait4Client();

		if ( slave == NULL ) {
			PRINTF( VERBOSE_DCON,
				"Incoming DC request failed. "
				"Connection closed.\n" );
		} else {
			PRINTF( VERBOSE_DCON,
				"New DC connection request from slave %s\n",
				slave->info.NickName );
		}

#ifdef WIN32
		/* This function will make the thread exit cleanly	*/
		/* in case YSM is called to exit			*/
		YSM_CommitSuicide();
#endif

	}
}

#endif


/* SignIn to the ICQ Network			*/
/* moduled for being able to 'reconnect'	*/

int
YSM_SignIn( void ) 
{
u_int16_t	port = 0;

	if (YSM_USER.proxy.proxy_flags & YSM_PROXY_HTTPS)
		port = 443;
	else
		port = YSM_USER.network.auth_port;

	YSM_USER.network.rSocket = YSM_Connect(
				YSM_USER.network.auth_host,
				port,
				0x1
				);

	if (YSM_USER.network.rSocket < 0) return YSM_USER.network.rSocket;

	PRINTF(VERBOSE_BASE, "\rLogging in.. [");
	YSM_Init_LoginA( YSM_USER.Uin, YSM_USER.password );

	return YSM_USER.network.rSocket;
}

/*	This function takes care of the regular checks to be	
 *	done every 1, or 2 seconds. cool huh :) 	
 */

void
YSM_CycleChecks( void )
{
#ifdef WIN32
	MSG		msg;
#endif

	if (g_sinfo.flags & FL_LOGGEDIN) {
		YSM_KeepAlive ();

		/********** check if we have to switch to away status *****/
		if (YSM_SETTING_AWAYTIME > 0	/* is it enabled? */
		&& !(g_promptstatus.flags & FL_AFKM) /* we are not in AFK */	
		&& ((time(NULL) - YSM_idletime)/60	/* minutes */
			>= YSM_SETTING_AWAYTIME))	/* are over */
		{
			/* then check if we are in -online status- 
			 * OR Free 4 Chat, which would be the same */
			if (YSM_USER.status == STATUS_ONLINE 
			|| YSM_USER.status == STATUS_FREE_CHAT) {
				/* finally. change status */
				YSM_ChangeStatus(STATUS_AWAY);
				g_promptstatus.flags |= FL_REDRAW;
				g_promptstatus.flags |= FL_AUTOAWAY;
			}
			
		}

		/*************** read commands from the fish gui ****/
		FishGUI_runcmds();

		if ((time(NULL) - YSM_LastCmdsTime) >= YSM_COMMANDSTIME) {
			YSM_CheckCommandsFile();
			YSM_LastCmdsTime = time(NULL);
		}

		/* Only redraw console if display isn't busy (Threads) */
		if ((g_promptstatus.flags & FL_REDRAW)
			&& !(g_promptstatus.flags & FL_BUSYDISPLAY)) {

			/* only redraw the prompt fully if it was overwritten */
			if (g_promptstatus.flags & FL_OVERWRITTEN)
				YSM_ConsoleRedrawPrompt(TRUE);
			else
				YSM_ConsoleRedrawPrompt(FALSE);

			g_promptstatus.flags &= ~FL_OVERWRITTEN;
			g_promptstatus.flags &= ~FL_REDRAW;
		}

	}

#ifdef WIN32
	if ( PeekMessage( &msg, g_hYSMHiddenWnd, 0, 0, PM_REMOVE ) ) {		
			TranslateMessage( &msg );
			DispatchMessage( &msg );
	}
#endif
	
}


