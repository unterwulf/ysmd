/*	$Id: YSM_Setup.c,v 1.147 2005/12/26 23:44:58 rad2k Exp $	*/
/*
-======================== ysmICQ client ============================-
		Having fun with a boring Protocol
-========================== YSM_Setup.c ============================-

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
__RCSID("$Id: YSM_Setup.c,v 1.147 2005/12/26 23:44:58 rad2k Exp $");

#include "YSM_Lists.h"
#include "YSM_Setup.h"
#include "YSM_Wrappers.h"
#include "YSM_ToolBox.h"
#include "YSM_Direct.h"
#include "YSM_Slaves.h"
#include "YSM_Network.h"
#include "YSM_Win32.h"
#include "YSM_Commands.h"
#include "YSM_Crypt.h" 
#include "YSM_Charset.h"
#include "YSM_FishGUI.h"
#include "stdarg.h"

#if defined(WIN32)
int
mkdir (
	const char	*path,
	short		mode
	);

extern HANDLE g_hSuicideEvent, g_hThreadDiedEvent;
#endif

int8_t	YSM_cfgfile[MAX_PATH];
int8_t	YSM_cfgdir[MAX_PATH];
int8_t	YSM_AFKMessage[MAX_DATA_LEN+1];
int8_t	YSM_CHATMessage[MAX_DATA_LEN+1];
int8_t	YSM_DefaultAFKMessage[MAX_DATA_LEN+1];
int8_t	YSM_DefaultCHATMessage[MAX_DATA_LEN+1];
int8_t	YSM_BrowserPath[MAX_DATA_LEN+1];
int8_t	YSM_CommandsFile[MAX_DATA_LEN+1];

extern	short	YSM_AFKCount;
extern	time_t	YSM_AFK_Time;
extern	struct	YSM_MODEL YSM_USER;

int
YSM_Initialize( void )
{
#ifdef WIN32
WSADATA wsaData;

	if (WSAStartup( 0x0101, &wsaData ) != 0) {
		PRINTF(VERBOSE_BASE,
			"Can't open MS. Windows sockets, exiting..\n");

		return -1;
	}

	g_hSuicideEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
	g_hThreadDiedEvent = CreateEvent( NULL, TRUE, FALSE, NULL );

	/* Set the control Handler. This is required to exit cleanly in win32
	 * since any unclean exit can lead to threads corruption and the system
	 * wont be able to boot normally.
	 */
	if (SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, TRUE) <= 0) {
		PRINTF(VERBOSE_BASE,
			"Unable to set the CtrlHandler, exiting..\n");
		return -1;
	}
#else
	signal( SIGPIPE, SIG_IGN );	
	signal( SIGINT, CtrlHandler);	
#endif
	memset( &YSM_USER, 0, sizeof(YSM_USER) );
	memset( &g_events, 0, sizeof(g_events) );

	/* Lists Initialization */
	List_init();

	/* Commands Initialization */
	YSM_Init_Commands();

	/* Network Initialization */
	YSM_NetworkInit();

	return 0;
}

void
YSM_SetupConfigurationFile(void)
{
FILE	*fd = NULL;

	if ((fd = YSM_fopen(YSM_cfgfile,"r")) != NULL) {
		YSM_ReadConfig(fd, 0);
		YSM_fclose(fd);

	} else {

		YSM_PrintWizardBox("<<< ysmICQ Configuration Wizard >>>");

	       	PRINTF( VERBOSE_BASE,
			"As this is your first time running YSM"
			", you will be\nprompted to fill some "
			"information required to create your\n"
			"configuration file. This file will be"
			" stored in your home\n"
			"directory, or the local directory (in "
			"case you used the '-c'\nparameter.) "
			"The filename will be " BROWN "'%s'" NORMAL ".\n"
			"In case you may want to change/update "
			"any of this information\njust do it "
			"directly in the configuration file."
			"\n\n", YSM_cfgfile);

		PRINTF(VERBOSE_BASE, "So, lets start!:\n");
		YSM_CreateConfig();
		/* read the configuration file settings now */
		if ((fd = YSM_fopen(YSM_cfgfile,"r")) != NULL) {
			YSM_ReadConfig(fd, 0);
			YSM_fclose(fd);
		}
	}
}
			
void
YSM_SetupHomeDirectory(void)
{
int8_t	*homep = NULL, *homep2 = NULL;
int32_t	homepsize = 0, homep2size = 0;

	homepsize = MAX_PATH;
	homep = YSM_Calloc(1, homepsize, __FILE__, __LINE__);
#ifdef WIN32
	if(getenv("USERPROFILE") != NULL) {
		/* no need to finish it by hand, called Calloc() */
		strncpy(homep, getenv("USERPROFILE"), homepsize - 1 );

	} else {
		/* no need to finish it by hand, called Calloc() */
		strncpy(homep,"C:", homepsize - 1);
	}
#else
	/* no need to finish it by hand, called Calloc() */
	strncpy(homep, getenv("HOME"), homepsize - 1);
#endif

	/* homep + slash + zero */
	homep2size = strlen(homep) + 2;
	homep2 = YSM_Calloc(1, homep2size, __FILE__, __LINE__);

	/* no need to finish it by hand, called Calloc() */
	strncpy( homep2, homep, homep2size - 1);
	strncat( homep2, "/", homep2size - strlen(homep2) - 1);

	strncpy( YSM_cfgfile, homep2, sizeof(YSM_cfgfile) - 1);
	YSM_cfgfile[sizeof(YSM_cfgfile)-1] = '\0';

	strncat( YSM_cfgfile,
		YSM_CFGDIRECTORY,
		sizeof(YSM_cfgfile) - strlen(YSM_cfgfile) - 1 );

	strncpy( YSM_cfgdir, YSM_cfgfile, sizeof(YSM_cfgdir) - 1 );
	YSM_cfgdir[sizeof(YSM_cfgdir)-1] = '\0';

	strncat( YSM_cfgfile, 
		"/",
		sizeof(YSM_cfgfile) - strlen(YSM_cfgfile) - 1);

	strncat( YSM_cfgfile,
		YSM_CFGFILENAME,
		sizeof(YSM_cfgfile) - strlen(YSM_cfgfile) - 1);

	YSM_Free(homep, __FILE__, __LINE__);
	homep = NULL;

	YSM_Free(homep2, __FILE__, __LINE__);
	homep2 = NULL;
}


void
YSM_SetupSlaves(void)
{
FILE *fd;

	if ((fd = YSM_fopen(YSM_cfgfile,"r")) != NULL) {
		YSM_ReadSlaves( fd );
		YSM_fclose(fd);
	} else {
		PRINTF( VERBOSE_BASE,
			"Contact list couldn't be read!. "
			"File not found.\n" );
			YSM_Error(ERROR_CRITICAL, __FILE__, __LINE__, 0);
	}
}


void
YSM_Setup(void) 
{
        /* Setup the config and dir path only if -c was not used before */
        if(YSM_cfgfile[0] == '\0' || YSM_cfgdir[0] == '\0') {
                PRINTF(VERBOSE_MOREDATA, "Setting up Config. file Path.");   
		YSM_SetupHomeDirectory();
        }

	PRINTF(VERBOSE_MOREDATA, "Reading or Creating Config file.");
	YSM_SetupConfigurationFile();
	PRINTF(VERBOSE_MOREDATA, "Retrieving Slave DATA from Config.");
	YSM_SetupSlaves();

	/* FishGUI setup */
	FishGUI_init();
}

void
YSM_ReadConfig( FILE *fd, char reload ) 
{
int8_t	YSM_CFGEND = FALSE, buf[MAX_PATH], *auxb = NULL, *aux = NULL;

	memset(&YSM_AFKMessage,0, MAX_DATA_LEN+1);
	memset(&YSM_CHATMessage,0, MAX_DATA_LEN+1);
	memset(&YSM_BrowserPath,0, MAX_DATA_LEN+1);
	memset(&YSM_CommandsFile,0, MAX_DATA_LEN+1);

	strncpy( YSM_DefaultAFKMessage, 
		YSM_AFK_MESSAGE,
		sizeof(YSM_DefaultAFKMessage) - 1);

	strncpy( YSM_DefaultCHATMessage, 
		YSM_CHAT_MESSAGE,
		sizeof(YSM_DefaultCHATMessage) - 1);

	YSM_USER.status_flags |= STATUS_FLDC_CONT;

#if defined (YSM_USE_CHARCONV)
	memset(YSM_SETTING_CHARSET_TRANS,0,MAX_CHARSET+4);
	memset(YSM_SETTING_CHARSET_LOCAL,0,MAX_CHARSET+4);
#endif

	while(!YSM_CFGEND && !feof(fd)) {
		
		memset(buf, '\0', sizeof(buf));
		fgets(buf, sizeof(buf) - 1, fd);

		if ((buf[0] != '#') && (buf[0] != 0)) {

			aux = strtok(buf,">");
			if ((auxb = strchr(aux,'\n'))) *auxb = '\0';
		
		
			if (!strcasecmp(aux,"SERVER")) {
				if ((aux=strtok(NULL," \n\t"))!= NULL) {
				strncpy( YSM_USER.network.auth_host,
					aux,
					sizeof(YSM_USER.network.auth_host) - 1);
				}
			}

			else if(!strcasecmp(aux,"SERVERPORT"))
                       		YSM_USER.network.auth_port = 
						atoi(strtok(NULL," \n\t"));


              		else if(!strcasecmp(aux,"PASSWORD")) {

				if (!reload) {
				if((aux = strtok(NULL," \n\t")) != NULL)
                        		strncpy( YSM_USER.password,
						aux,
						sizeof(YSM_USER.password) - 1);
				}
			}

                	else if(!strcasecmp(aux,"STATUS")) {
				if (!reload) 
					YSM_CFGStatus(strtok (NULL," \n\t"));
			}

                	else if(!strcasecmp(aux,"UIN"))
                       		YSM_USER.Uin = atoi(strtok(NULL," \n\t"));

			else if(!strcasecmp(aux,"ANTISOCIAL"))
				YSM_SETTING_ANTISOCIAL = 
						atoi(strtok(NULL," \n\t"));

			else if(!strcasecmp(aux,"UPDATENICKS"))
				YSM_SETTING_UPDATENICKS = 
						atoi(strtok(NULL," \n\t"));

			else if(!strcasecmp(aux,"COMMANDSFILE")) {
				if ((aux=strtok(NULL,"\n\t")) != NULL) 
					strncpy( YSM_CommandsFile,
						aux,
						sizeof(YSM_CommandsFile) - 1);
			}

			else if (!strcasecmp(aux,"DC_DISABLE")) {
				if ((aux=strtok(NULL," \n\t"))!= NULL) 
					YSM_SETTING_DCDISABLE = atoi(aux);
			}
	
			else if (!strcasecmp(aux,"DC_LAN")) {
				if ((aux=strtok(NULL," \n\t"))!= NULL) 
					YSM_SETTING_DCLAN = atoi(aux);
			}
		
			else if (!strcasecmp(aux,"DC_PORT1")) {
				if ((aux=strtok(NULL," \n\t"))!= NULL) 
					YSM_SETTING_DCPORT1 = atoi(aux);
			}
			
			else if (!strcasecmp(aux,"DC_PORT2")) {
				if ((aux=strtok(NULL," \n\t"))!= NULL) 
					YSM_SETTING_DCPORT2 = atoi(aux);
			}

			else if(!strcasecmp(aux,"VERBOSE")) {
				if ((aux=strtok(NULL," \n\t"))!= NULL) 
					YSM_SETTING_VERBOSE = atoi(aux);
			}

			else if(!strcasecmp(aux,"BEEP")) {
				if ((aux=strtok(NULL," \n\t"))!= NULL) 
					YSM_SETTING_BEEP = atoi(aux);
			}

			else if(!strcasecmp(aux,"LOGALL")) {
				if ((aux=strtok(NULL," \n\t"))!= NULL) 
					YSM_SETTING_LOGALL = atoi(aux);
			}

			else if(!strcasecmp(aux,"NEWLOGSFIRST")) {
				if ((aux=strtok(NULL," \n\t"))!= NULL) 
					YSM_SETTING_NEWLOGSFIRST = atoi(aux);
			}

			else if(!strcasecmp(aux,"WINALERT")) {
				if ((aux=strtok(NULL," \n\t"))!= NULL) 
					YSM_SETTING_WINALERT = atoi(aux);
			}

			else if(!strcasecmp(aux,"WINHOTKEY")) {
				if ((aux=strtok(NULL," \n\t"))!= NULL) 
					YSM_SETTING_HOT_KEY_MAXIMIZE = *aux;
			}

			else if (!strcasecmp(aux, "AWAYTIME")) {
				if ((aux=strtok(NULL," \n\t"))!= NULL) 
					YSM_SETTING_AWAYTIME = atoi(aux);
			}

			else if(!strcasecmp(aux,"AFKMESSAGE")) {
				if ((aux=strtok(NULL,"\n\t"))!= NULL) 
					strncpy( YSM_DefaultAFKMessage, aux,
					sizeof(YSM_DefaultAFKMessage)-1);
			}

			else if(!strcasecmp(aux,"AFKMAXSHOWN")) {
				if ((aux=strtok(NULL," \n\t"))!= NULL) 
					YSM_SETTING_AFKMAXSHOWN = atoi(aux);
			}

			else if(!strcasecmp(aux,"AFKMINIMUMWAIT")) {
				if ((aux=strtok(NULL," \n\t"))!= NULL) 
					YSM_SETTING_AFKMINIMUMWAIT = atoi(aux);
			}

			else if(!strcasecmp(aux,"CHATMESSAGE")) {
				if ((aux=strtok(NULL,"\n\t"))!= NULL) 
					strncpy(YSM_DefaultCHATMessage, aux,
					sizeof(YSM_DefaultCHATMessage)-1);
			}

			else if(!strcasecmp(aux,"PROXY")) {
				if ((aux=strtok(NULL," \n\t"))!= NULL) 
	                       		strncpy( YSM_USER.proxy.proxy_host, aux,
					sizeof(YSM_USER.proxy.proxy_host) - 1);
                       	}	

			else if(!strcasecmp(aux,"PROXY_PORT")) {
				if ((aux=strtok(NULL, " \n\t")) != NULL)
					YSM_USER.proxy.proxy_port = atoi(aux);
			}

			else if(!strcasecmp(aux,"PROXY_HTTPS")) {
				if (atoi(strtok(NULL," \n\t")) > 0) 
					YSM_USER.proxy.proxy_flags |= 
							YSM_PROXY_HTTPS;
				else
					YSM_USER.proxy.proxy_flags &= 
							~YSM_PROXY_HTTPS;
			}

			else if(!strcasecmp(aux,"PROXY_AUTH")) {
				if (atoi(strtok(NULL," \n\t")) > 0) 
					YSM_USER.proxy.proxy_flags |= 
							YSM_PROXY_AUTH;
				else
					YSM_USER.proxy.proxy_flags &= 
							~YSM_PROXY_AUTH;
					
			}

			else if(!strcasecmp(aux,"PROXY_USERNAME")) {
				if ((aux=strtok(NULL," \n\t"))!= NULL) 
	                        	strncpy( YSM_USER.proxy.username, aux,
					sizeof(YSM_USER.proxy.username) - 1);
               		}	

			else if(!strcasecmp(aux,"PROXY_PASSWORD")) {
				if ((aux=strtok(NULL," \n\t"))!= NULL) 
	                       		strncpy( YSM_USER.proxy.password, aux,
					sizeof(YSM_USER.proxy.password) - 1);
                       	}	

			else if(!strcasecmp(aux,"PROXY_RESOLVE")) {
				if (atoi(strtok(NULL," \n\t")) > 0) 
					YSM_USER.proxy.proxy_flags |= 
							YSM_PROXY_RESOLVE;
				else
					YSM_USER.proxy.proxy_flags &= 
							~YSM_PROXY_RESOLVE;
			}

			else if(!strcasecmp(aux,"WEBAWARE")) {
				if (atoi(strtok(NULL," \n\t")) > 0) 
					YSM_USER.status_flags |= 
							STATUS_FLWEBAWARE;
				else
					YSM_USER.status_flags &= 
							~STATUS_FLWEBAWARE;
			}

			else if(!strcasecmp(aux,"MYBIRTHDAY")) {
				if (atoi(strtok(NULL," \n\t")) > 0) 
					YSM_USER.status_flags |= 
							STATUS_FLBIRTHDAY;
				else
					YSM_USER.status_flags &= 
							~STATUS_FLBIRTHDAY;
			}

			else if(!strcasecmp(aux,"VERBOSE")) {
				if ((aux=strtok(NULL, " \n\t")) != NULL)
					YSM_SETTING_VERBOSE = atoi(aux);
			}

			else if(!strcasecmp(aux,"VERSION_CHECK")) {
				if ((aux=strtok(NULL, " \n\t")) != NULL)
					YSM_SETTING_VERSION_CHECK = atoi(aux);
			}
	
#ifdef YSM_USE_CHARCONV
			else if(!strcasecmp(aux,"CHARSET_TRANS")) {
				if ((aux=strtok(NULL," \n\t"))!= NULL) 
					strncpy( YSM_SETTING_CHARSET_TRANS, aux,
					sizeof(YSM_SETTING_CHARSET_TRANS) - 1);

			} else if(!strcasecmp(aux,"CHARSET_LOCAL")) {
				if ((aux=strtok(NULL," \n\t"))!= NULL) 
					strncpy( YSM_SETTING_CHARSET_LOCAL, aux,
					sizeof(YSM_SETTING_CHARSET_LOCAL) - 1);
			}	
#endif

			else if (!strcasecmp(aux,"BROWSER")) {
				if ((aux=strtok(NULL,"\n\t"))!= NULL) 
					strncpy( YSM_BrowserPath, aux,
						sizeof(YSM_BrowserPath) - 1);
			}

			else if (!strcasecmp(aux,"SOUNDS")) {
				if (atoi(strtok(NULL," \n\t")) > 0) 
					YSM_SETTING_SOUNDS = TRUE;
				else
					YSM_SETTING_SOUNDS = FALSE;
			}

#ifndef WIN32
			else if (!strcasecmp(aux,"SOUND_PROGRAM")) {
				if ((aux=strtok(NULL,"\n\t")) != NULL)
					strncpy( g_events.sbinpath, aux,
					sizeof(g_events.sbinpath) - 1);
			}
#endif

			else if (!strcasecmp(aux,"EXEC_INCOMING")) {
				if ((aux=strtok(NULL," \n\t")) != NULL)
					strncpy( g_events.execincoming, aux,
					sizeof(g_events.execincoming) - 1);
			}

			else if (!strcasecmp(aux,"EXEC_OUTGOING")) {
				if ((aux=strtok(NULL," \n\t")) != NULL)
					strncpy( g_events.execoutgoing, aux,
					sizeof(g_events.execoutgoing) - 1);
			}

			else if (!strcasecmp(aux,"EXEC_ONCOMING")) {
				if ((aux=strtok(NULL," \n\t")) != NULL)
					strncpy( g_events.execoncoming, aux,
					sizeof(g_events.execoncoming) - 1);
			}

			else if (!strcasecmp(aux,"EXEC_OFFGOING")) {
				if ((aux=strtok(NULL," \n\t")) != NULL)
					strncpy( g_events.execoffgoing, aux,
					sizeof(g_events.execoffgoing) - 1);
			}

			else if (!strcasecmp(aux,"EXEC_LOGOFF")) {
				if ((aux=strtok(NULL," \n\t")) != NULL)
					strncpy( g_events.execlogoff, aux,
					sizeof(g_events.execlogoff) - 1);
			}

			else if(!strcasecmp(aux,"SOUND_INCOMING")) {
				if (atoi(strtok(NULL," \n\t")) > 0) 
					g_events.insound = 1;
				else
					g_events.insound = 0;
			}

			else if(!strcasecmp(aux,"SOUND_OUTGOING")) {
				if (atoi(strtok(NULL," \n\t")) > 0) 
					g_events.outsound = 1;
				else
					g_events.outsound = 0;
			}

			else if(!strcasecmp(aux, "COLOR_MESSAGE")) {
				if ((aux=strtok(NULL," \n\t")) != NULL) {
					YSM_SETTING_COLOR_MESSAGE = 
						YSM_GetColorByName(aux);
				}
			}

			else if(!strcasecmp(aux, "COLOR_TEXT")) {
				if ((aux=strtok(NULL," \n\t")) != NULL) {
					YSM_SETTING_COLOR_TEXT = 
						YSM_GetColorByName(aux);
				}
			}

			else if(!strcasecmp(aux, "COLOR_STATUSCHANGENAME")) {
				if ((aux=strtok(NULL," \n\t")) != NULL) {
					YSM_SETTING_COLOR_STATUSCHANGENAME = 
						YSM_GetColorByName(aux);
				}
			}

			else if(!strcasecmp(aux, "COLOR_STATUSCHANGESTATUS")) {
				if ((aux=strtok(NULL," \n\t")) != NULL) {
					YSM_SETTING_COLOR_STATUSCHANGESTATUS = 
						YSM_GetColorByName(aux);
				}
			}

			else if(!strcasecmp(aux, "COLOR_TEXT2")) {
				if ((aux=strtok(NULL," \n\t")) != NULL) {
					YSM_SETTING_COLOR_TEXT2 = 
						YSM_GetColorByName(aux);
				}
			}

			else if(!strcasecmp(aux,"FISHGUI")) {
				YSM_USER.fishgui.port = (u_int16_t)
						atoi(strtok(NULL," \n\t"));
			}

			else if(!strcasecmp(aux,"FISHGUI_HIDE_CONSOLE")) {
				if (atoi(strtok(NULL," \n\t")) > 0) 
					YSM_USER.fishgui.hide_console = 1;
				else
					YSM_USER.fishgui.hide_console = 0;
			}

			else if(!strcasecmp(aux,"SOUND_ONCOMING")) {
				if (atoi(strtok(NULL," \n\t")) > 0) 
					g_events.onsound = 1;
				else
					g_events.onsound = 0;
			}

			else if(!strcasecmp(aux,"SOUND_OFFGOING")) {
				if (atoi(strtok(NULL," \n\t")) > 0) 
					g_events.offsound = 1;
				else
					g_events.offsound = 0;
			}

			else if(!strcasecmp(aux,"SOUND_LOGOFF")) {
				if (atoi(strtok(NULL," \n\t")) > 0) 
					g_events.logoffsound = 1;
				else
					g_events.logoffsound = 0;
			}

                	else if(!strcasecmp(aux,SLAVES_TAG)) YSM_CFGEND=TRUE;

			else if( *aux ) {
				PRINTF( VERBOSE_BASE,
					RED "UNKNOWN cfg directive '%s' , "
					"ignoring...\n" NORMAL,
					aux );
			}
		}
	}	


	/*	Before leaving check there's at least the	*/
	/*	minimum required fields */

	if ( !YSM_USER.Uin ) {
		PRINTF( VERBOSE_BASE,
		"\nMissing UIN in config. Can't continue.\n"
		"If you want a new account, remove the cfg file.\n");
		exit(0);

	} else if ( YSM_USER.network.auth_host[0] == '\0' ) {

		PRINTF( VERBOSE_BASE,
			"\nMissing ICQ Server in config. Can't continue.\n");
		exit(0);

	} else if (!YSM_USER.network.auth_port) {
		PRINTF( VERBOSE_BASE,
		"\nMissing ICQ Server port in config. Can't continue.\n");
		exit(0);
	}

	/* Initialize CodePage/Charsets */
#ifdef YSM_USE_CHARCONV
	YSM_CharsetInit();
#endif
}

void
YSM_CreateConfig(void) 
{
char	YSM_tmpa[MAX_PWD_LEN+1], YSM_tmpb[MAX_UIN_LEN+1];
char	*p_passwd;
int	tries = 0;
	
	strncpy(YSM_USER.network.auth_host,
		YSM_DEFAULTSRV,
		sizeof(YSM_USER.network.auth_host) - 1) ;

	YSM_USER.network.auth_port = YSM_DEFAULTPORT;
	YSM_USER.Uin = 0;

	memset(YSM_tmpb, 0, MAX_UIN_LEN+1);

	PRINTF( VERBOSE_BASE,
		"\nYour " RED "UIN" NORMAL " [use 0 for new]#: " );

	YSM_fgets(YSM_tmpb, MAX_UIN_LEN, 0);

	YSM_USER.Uin = atoi(YSM_tmpb);

	/* Ask proxy configuration once */
	YSM_AskProxyConfiguration();

	if (YSM_USER.Uin == 0) {

	PRINTF( VERBOSE_BASE, "\nRegister a new ICQ Number.\n");
	PRINTF( VERBOSE_BASE, 
		"Unfortunately, registering an ICQ number with ysmICQ is no longer possible.\n");

	PRINTF(VERBOSE_BASE, "Please head to http://www.icq.com/register/ to register a UIN.\n");

	PRINTF(VERBOSE_BASE, "After you have registered a UIN, come back and type it in again.\n");

		PRINTF( VERBOSE_BASE,
		"\nYour " RED "UIN" NORMAL "#: " );

	YSM_fgets(YSM_tmpb, MAX_UIN_LEN, 0);
	YSM_USER.Uin = atoi(YSM_tmpb);


	} else	{
		PRINTF(VERBOSE_BASE,
		"\nMaximum password length is set to %d.\n", MAX_PWD_LEN);
		PRINTF(VERBOSE_BASE,
		"You may just press the ENTER key\n"
		"to be prompted for your password everytime you start YSM.\n");
	}

	do {
		p_passwd = YSM_getpass("Password: ");
		if (strlen(p_passwd) >= 1) tries++;
	} while(!tries && YSM_USER.Uin == 0);

	strncpy( YSM_USER.password, p_passwd, sizeof(YSM_USER.password) - 1);

	do {
		p_passwd = YSM_getpass("Again?[verify]: ");
		if (strlen(p_passwd) >= 1) tries--;
	} while(tries && YSM_USER.Uin == 0);

	strncpy( YSM_tmpa, p_passwd, sizeof(YSM_tmpa)-1 );

	if (!strcasecmp( YSM_USER.password , YSM_tmpa )) {
	
		YSM_SaveConfig();

		PRINTF(VERBOSE_BASE,
			"\nConfiguration file created.\n");
		return;
	}

	PRINTF( VERBOSE_BASE,
		"\n%s<WRONG>%s - Passwords did not match.\n", RED, NORMAL );

	YSM_Error(ERROR_CRITICAL, __FILE__, __LINE__, 0);
}

#define YSMOPENCONFIG(rwx)	(fd = YSM_fopen(YSM_cfgfile,rwx))
#define YSMCLOSECONFIG()	YSM_fclose(fd)

__inline void
CFGWRITE(FILE *fd, const u_int8_t *foo, ...)
{
va_list	args;
	va_start(args, foo);
	vfprintf(fd, foo, args);
	fprintf(fd, "\n");
	va_end(args);
}

void
YSM_SaveConfig(void) 
{

time_t	YSM_tmpdate;
FILE	*fd;

	/* mkdir returns 0 if success */
	if (mkdir(YSM_cfgdir,0700)) {
		if (errno != EEXIST) {
			PRINTF(VERBOSE_BASE,
				"\nmkdir() directory %s error",YSM_cfgdir);
			YSM_Error(ERROR_CRITICAL, __FILE__, __LINE__, 1);
		}
	}

        if (YSMOPENCONFIG("w") != NULL) {

		/* Initial COMMENTS and NOTES */

		CFGWRITE(fd,"# %s%s .", YSM_INFORMATION, YSM_INFORMATION2);

                YSM_tmpdate = time(NULL);
		CFGWRITE(fd,"# YSM CFG FILE - Created -> %s", ctime(&YSM_tmpdate));

		CFGWRITE(fd,"# VALUES ARE SPECIFIED AFTER A '>' SYMBOL. ");
		CFGWRITE(fd,"# COMMENTS ARE PRECEEDED BY A '#' SYMBOL.");
		CFGWRITE(fd,"# '0' means NO and '1' means YES.\n");
		

		CFGWRITE(fd, "# #######################"
			"###############################################\n");

		/* Settings and COMMENTS */

		CFGWRITE(fd, "# Default Status - When logging in.");
		CFGWRITE(fd, "# Options are: "
				"ONLINE, "
				"AWAY, "
				"DND, "
				"FREECHAT, "
				"NA,"
				" OCCUPIED "
				"and INVISIBLE.");

		CFGWRITE(fd, "STATUS>%s\n", "ONLINE" );
		CFGWRITE(fd, "UIN>%d", YSM_USER.Uin );

		CFGWRITE(fd,"# Leave this PASSWORD setting empty in order to be");
		CFGWRITE(fd,"# prompted for a password when logging in.");

		CFGWRITE(fd, "PASSWORD>%s\n", YSM_USER.password);
		CFGWRITE(fd, "SERVER>%s", YSM_USER.network.auth_host);
		CFGWRITE(fd, "SERVERPORT>%d\n", YSM_USER.network.auth_port);

		CFGWRITE(fd, "# The amount of minutes to wait without keyboard "
			"input before changing\n# your status from 'online' to "
			"away. Use '0' to disable it.");

		CFGWRITE(fd, "AWAYTIME>%d\n", YSM_SETTING_AWAYTIME);

		CFGWRITE(fd,"# The auto-reply message of the AFK mode.");
		CFGWRITE(fd,"AFKMESSAGE>%s\n", YSM_AFK_MESSAGE);
		CFGWRITE(fd,"# Amount of messages to show each time in 'readafk'");
		CFGWRITE(fd,"AFKMAXSHOWN>%d\n", YSM_SETTING_AFKMAXSHOWN);

		CFGWRITE(fd,"# Seconds between AFK notices to each slave.");
		CFGWRITE(fd,"AFKMINIMUMWAIT>%d\n", YSM_SETTING_AFKMINIMUMWAIT);


		CFGWRITE(fd,"# The auto-reply message sent while you are in a CHAT session.");
		CFGWRITE(fd,"CHATMESSAGE>%s\n", YSM_CHAT_MESSAGE);

		CFGWRITE(fd,"# Proxy Configuration. If you want to enable:\n"
		"# HTTPS - use 1 on PROXY_HTTPS.\n"
		"# Note, this is not SSL but a Hack (uses 443 and not 5190).\n" 
		"# RESOLVE - use 1 to resolve hostnames through the proxy.\n"
		"# AUTH  - use 1 on PROXY_AUTH, type a PROXY_USERNAME"
		" and a PROXY_PASSWORD.\n");

		CFGWRITE(fd,"PROXY>%s", 
			!(YSM_USER.proxy.proxy_host[0]) 
			? "0" : (char *)YSM_USER.proxy.proxy_host);

		CFGWRITE(fd,"PROXY_PORT>%d", YSM_USER.proxy.proxy_port);
		CFGWRITE(fd,"PROXY_HTTPS>%d",
			(YSM_USER.proxy.proxy_flags & YSM_PROXY_HTTPS)
			? 1 : 0 );

		CFGWRITE(fd,"PROXY_RESOLVE>%d",
			(YSM_USER.proxy.proxy_flags & YSM_PROXY_RESOLVE)
			? 1 : 0 );

		CFGWRITE(fd,"PROXY_AUTH>%d",
			(YSM_USER.proxy.proxy_flags & YSM_PROXY_AUTH)
			? 1 : 0 );

		CFGWRITE(fd,"PROXY_USERNAME>%s", 
			!(YSM_USER.proxy.username[0]) 
			? "0" : (char *)YSM_USER.proxy.username);

		CFGWRITE(fd,"PROXY_PASSWORD>%s\n", 
			!(YSM_USER.proxy.password[0]) 
			? "0" : (char *)YSM_USER.proxy.password);

		CFGWRITE(fd,"# Enable or disable Beeping.");
		CFGWRITE(fd,"# By specifying a value bigger than 0, you enable "
			"beeping\n# and specify the amount of times to beep."
			" A value of 0 will disable beeping.");
		CFGWRITE(fd,"BEEP>%d\n", 0x1);

		CFGWRITE(fd,"# GLOBAL Logging ON(1) or OFF(0).");
		CFGWRITE(fd,"# Use '1' to log messages into a history "
			"readable by the 'hist' command. ");
		CFGWRITE(fd,"LOGALL>%d\n", 0x1);

		CFGWRITE(fd,"# Put newer logs at the beginning of the file.");
		CFGWRITE(fd,"NEWLOGSFIRST>%d\n", 0x1);

		CFGWRITE(fd,"# Only Receive messages from slaves in your list (1)"
			" or from anyone (0).");
		CFGWRITE(fd,"# Note you will always receive Auth requests in any"
			" of the two modes.");
		CFGWRITE(fd,"ANTISOCIAL>%d\n", 0x0);

		CFGWRITE(fd,"# Update slave nicknames with newer information?.");
		CFGWRITE(fd,"# (This is done with the 'whois' command).");
		CFGWRITE(fd,"UPDATENICKS>%d\n", 0x01);

		CFGWRITE(fd,"# Specify a file from where YSM will "
			"execute client\n# commands every %d seconds. "
			"(Once they are executed, the file is cleared.)",
			YSM_COMMANDSTIME );
		CFGWRITE(fd,"COMMANDSFILE>\n");


		CFGWRITE(fd,"# DC Configuration.\n"
		"# DC_DISABLE - use 1 to disable direct connections.\n"
		"# DC_LAN - use 1 to speed up LAN negotiations.\n"
		"# DC_PORT1 - force a port to listen for incoming DCs.\n"
		"# DC_PORT2 - force a port to deal with File Transfers.\n");

		CFGWRITE(fd,"DC_DISABLE>0"); 
		CFGWRITE(fd,"DC_LAN>0");
		CFGWRITE(fd,"DC_PORT1>0");
		CFGWRITE(fd,"DC_PORT2>0\n");

		CFGWRITE(fd, "# Do you want to make your presence public?");
		CFGWRITE(fd, "WEBAWARE>0\n");

		CFGWRITE(fd, "# Let everyone know its my birthday!");
		CFGWRITE(fd, "MYBIRTHDAY>0\n");

		CFGWRITE(fd,
	"# WINALERT will alert your console window on incoming messages.\n"
	"# If set to \"0\" windows alerts will be disabled.\n"
	"# If set to \"1\" it will only popup (unix/win32/os2).\n"
	"# If set to \"2\" it will only blink (win32 and OS2 only).\n"
	"# If set to \"3\" it will popup and blink (only blinks in win32 and OS2).");
		CFGWRITE(fd,"WINALERT>%d\n", YSM_SETTING_WINALERT);

#ifdef WIN32
		CFGWRITE(fd,
	"# WINHOTKEY is a CTRL+ALT+key combination in charge of\n"
	"# activating a minimized YSM client. Set WINHOTKEY to the key"
	" you desire.");

		CFGWRITE(fd,"WINHOTKEY>%c\n", YSM_SETTING_HOT_KEY_MAXIMIZE);

#endif	/* WIN32 */

		CFGWRITE(fd,"# Specify the path to the browser that will handle");
		CFGWRITE(fd,"# urls for the \"burl\" command.");
		CFGWRITE(fd,"# (Windows users specify full path too)");
		CFGWRITE(fd,"BROWSER>\n");

		CFGWRITE(fd,
		"# [Action Events Configuration]"
		"\n"
		"# specify a command line or shell script to be executed when:"
		"\n"
		"# - a message is received [in EXEC_INCOMING]" "\n"
		"# - a message is sent [in EXEC_OUTGOING]" "\n"
		"# - a slave goes online [in EXEC_ONCOMING]" "\n"
		"# - a slave goes offline [in EXEC_OFFGOING]" "\n"
		"# - you logoff [in EXEC_LOGOFF]" "\n"
		"# The script you specify will receive the following command-line parameters: \n"
		"# [script] remote_uin remote_nick msg_length msg_data"
		"\n" );

		CFGWRITE(fd,"EXEC_INCOMING>" );
		CFGWRITE(fd,"EXEC_OUTGOING>" );
		CFGWRITE(fd,"EXEC_ONCOMING>" );
		CFGWRITE(fd,"EXEC_OFFGOING>" );
		CFGWRITE(fd,"EXEC_LOGOFF>\n" );

		CFGWRITE(fd,
		"# [Sound Events Configuration]"
		"\n"
		"# enable(1) or disable(0) sounds globally.");
		CFGWRITE(fd,"SOUNDS>1\n");
#ifndef WIN32
		CFGWRITE(fd,
		"# specify the path to a program that will handle the" "\n"
		"# playing of the WAVE sounds. Win32 users dont require this.");
		CFGWRITE(fd,"SOUND_PROGRAM>/usr/bin/play" );
#endif

		CFGWRITE(fd, "\n"
		"# enable(1) or disable(0) the playing of sound events." "\n"
		"# sounds are played from inside the sounds/ directory in\n"
		"# your ysm's home directory. \n" );

		CFGWRITE(fd,"SOUND_INCOMING>1" );
		CFGWRITE(fd,"SOUND_OUTGOING>1" );
		CFGWRITE(fd,"SOUND_ONCOMING>1" );
		CFGWRITE(fd,"SOUND_OFFGOING>1" );
		CFGWRITE(fd,"SOUND_LOGOFF>1\n" );

		CFGWRITE(fd, 
		"# Colors configuration. Available colors are:\n"
		"# BLACK RED GREEN BROWN BLUE MAGENTA CYAN GRAY WHITE TERMINAL_DEFAULT\n"
		"# and all mentioned colors in bright as in BRIGHT_colorname.\n"
		"# You can either use defaults or change them below.");
		
		CFGWRITE(fd,"COLOR_TEXT>" );
		CFGWRITE(fd,"COLOR_TEXT2>" );
		CFGWRITE(fd,"COLOR_STATUSCHANGENAME>" );
		CFGWRITE(fd,"COLOR_STATUSCHANGESTATUS>" );
		CFGWRITE(fd,"COLOR_MESSAGE>\n" );

		CFGWRITE(fd,"# FishGUI - Plugable GUI for ysm! (http://sourceforge.net/projects/imfish)\n"
		"# if you have IMFish please specify its listening port.\n"
		"# otherwise, use port '0' to disable it. Thank you.");
		CFGWRITE(fd,"FISHGUI>0\n");

		CFGWRITE(fd,"# FishGUI - Hide ysm console?");
		CFGWRITE(fd,"FISHGUI_HIDE_CONSOLE>0\n");

		CFGWRITE(fd, "# Verbose level. Add or remove output information.\n"
			"# Normal output -> 5\n"
			"# - Remove status changes -> 0\n"
			"# - Remove connecting information -> 1\n"
			"# + Add direct connections information -> 20\n"
			"# + Add data checking information -> 21\n"
			"# + Add incoming/outgoing packets dump -> 22\n"
			"# + Add slaves downloading processing -> 23" );

		CFGWRITE(fd,"VERBOSE>%d\n", YSM_SETTING_VERBOSE);

		CFGWRITE(fd,"# VERSION CHECKING. "
			"Specify 0 if you want to disable it.\n"
			"# This feature lets ysm check for the lastest "
			"available\n"
			"# release in sourceforge, and compares it with the "
			"local.");

		CFGWRITE(fd,"VERSION_CHECK>%d\n", YSM_SETTING_VERSION_CHECK);

#if defined (YSM_USE_CHARCONV)

		CFGWRITE(fd,"# CHARSET_TRANS is charset for "
			"transfering/receiving of messages");

		CFGWRITE(fd,"# CHARSET_LOCAL is charset for "
			"displaying/inputting of messages");

		CFGWRITE(fd,"# Russian Generic (for Unix) are "
			"TRANS: CP1251 LOCAL: KOI8-R");

		CFGWRITE(fd,"# Russian Generic (for Windows) are "
			"TRANS: 1251 LOCAL: 866");

		CFGWRITE(fd,"# Windows users have a list of supported charsets at:"
		"\n# HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\NIs"
		"\\CodePage");
		
		CFGWRITE(fd,"CHARSET_TRANS>");
		CFGWRITE(fd,"CHARSET_LOCAL>");
#endif

                CFGWRITE(fd,"\n\n# your ICQ slaves.\n");
                CFGWRITE(fd,"%s", SLAVES_TAG);
                CFGWRITE(fd,"\n\n\n# <EOF>");

                YSMCLOSECONFIG();

        } else {
                PRINTF(VERBOSE_BASE, "\nERROR creating cfg file.\n");
		YSM_Error(ERROR_CRITICAL, __FILE__, __LINE__, 1);
	}
}	
	

	
void
YSM_ReadSlaves( FILE *fd )
{
int8_t	YSM_tmpbuf[MAX_PATH], *aux = NULL;
int8_t	*auxnick = NULL, *auxuin = NULL, *auxkey = NULL, *auxflags = NULL;
int8_t	*auxcol = NULL;
int8_t	*next = NULL;
int	field = 0;

	if (YSM_IsInvalidPtr(fd))
		return;

	/* find start of SLAVES section */
	while (memset(YSM_tmpbuf, '\0', sizeof(YSM_tmpbuf))
	&& fgets(YSM_tmpbuf, sizeof(YSM_tmpbuf)-1, fd)!= NULL) {

		if (!strcasecmp( YSM_trim(YSM_tmpbuf), SLAVES_TAG )) {
			break;
		}
	}

	/* parse slaves */
	while (memset(YSM_tmpbuf, '\0', sizeof(YSM_tmpbuf))
	&& fgets(YSM_tmpbuf, sizeof(YSM_tmpbuf)-1, fd) != NULL) {

		YSM_trim(YSM_tmpbuf);
		if (YSM_tmpbuf[0]=='#' ) continue; /* ignore comments */

		aux = YSM_tmpbuf;
		next = strchr(aux, ':');
		if (next) *next++ = '\0';

		auxnick = auxuin = auxkey = auxflags = auxcol = NULL;
		field = 0;

		while (aux != NULL) {
			YSM_trim(aux);

			switch (field++) {

				case 0 : /* Nick */
					if (*aux != '\0') auxnick = aux;
					break;

				case 1 : /* UIN */
					if (*aux != '\0') auxuin = aux;
					break;

				case 2 : /* Key */
					if (*aux != '\0') auxkey = aux;
					break;

				case 3: /* Flags */
					if (*aux != '\0') auxflags = aux;
					break;

				case 4: /* Color */
					if (*aux != '\0') auxcol = YSM_GetColorByName(aux);
					break;

				default: /* Trailing garbage */
					break;
			}

			aux = next;
			if (aux) next = strchr(aux, ':');
			if (next) *next++ = '\0';
		}

		if (auxnick == NULL	/* No name */
		|| auxuin == NULL	/* No UIN */
		|| strlen(auxuin) < 5)	/* Invalid UIN */
			continue;

                YSM_AddSlavetoList( auxnick,
                                atoi(auxuin),
                                auxflags,
                                auxkey,
				auxcol,
                                0,
                                0,
                                0,
                                0 );
 
        }
 
        PRINTF( VERBOSE_MOREDATA,
                "%s%d]" NORMAL "\n",
                MSG_READ_SLAVES,
                List_amountSLAVE );
}

YSM_SLAVE *
YSM_QuerySlaves( unsigned short TYPE,
		unsigned char	*Extra,
		uin_t		Uin,
		unsigned int	reqid ) 
{

u_int32_t	x;
YSM_SLAVE	*node = plist_firstSLAVE;

	for ( x = 0; x < List_amountSLAVE; x++ ) {
	
		if(!node) break;

		if ( TYPE == SLAVE_NAME ) {
			if(!strcasecmp(node->info.NickName, Extra))
				return node;
		} else if ( TYPE == SLAVE_UIN ) {
			if (node->Uin == Uin)
				return node;
		} else if ( TYPE == SLAVE_REQID ) {
			if (node->ReqID == reqid)
				return node;
		} else {
			YSM_Error(ERROR_CODE, __FILE__, __LINE__, 1);
		}

		node = node->next;
	}

	return (NULL);
}

void
YSM_AddSlave( char *Name, uin_t Uin ) 
{
YSM_SLAVE *result = NULL;

	result = YSM_AddSlavetoList( Name,
				Uin,
				NULL,
				NULL,
				NULL,
				0,
				0,
				0,
				0);

	if (result == NULL) {
		PRINTF(VERBOSE_BASE,
			"NO! Illegal Slave Cloning detected..perv!.");
		PRINTF(VERBOSE_BASE,
			"\n%sSLAVE ALREADY%s exists in your list!.\n",
								BRIGHT_CYAN,
								NORMAL);
		return;
	}

	PRINTF( VERBOSE_BASE,
		"Adding a SLAVE with #%d. Call him %s from now on.\n",
		result->Uin,
		result->info.NickName );

	YSM_AddSlavetoDisk( result );
}


void
YSM_AddSlavetoDisk( YSM_SLAVE *victim )
{
FILE		*YSM_tmp = NULL, *fd = NULL;
int8_t		YSMBuff[MAX_PATH];
u_int32_t	x = 0;

 	fd = YSM_fopen(YSM_cfgfile, "r");
	if (fd == NULL) {
		/* ERR_FOPEN */
		return;
	}

	YSM_tmp = tmpfile();
	if (YSM_tmp == NULL) /* ERR_FILE */
		return;

	while(!feof(fd)) {
		memset(YSMBuff,'\0',MAX_PATH);
    		fgets(YSMBuff,sizeof(YSMBuff)-1,fd);

		if (strstr(YSMBuff,SLAVES_TAG)) {

			fprintf(YSM_tmp,"%s", YSMBuff);	

			/* Fill Name and UIN */
			fprintf( YSM_tmp, 
				"%s:%d:",
				victim->info.NickName,
				(int)victim->Uin );

			/* Fill Key */
			if (!YSM_KeyEmpty( victim->crypto.strkey )) {
				for( x=0; x < strlen(victim->crypto.strkey ); x++ )
					fprintf( YSM_tmp, 
						"%c",
						victim->crypto.strkey[x]);
			}

			fprintf( YSM_tmp, ":" );

			/* Fill Flags */
			if (victim->flags & FL_ALERT)
				fprintf( YSM_tmp, "a" );
			
			if (victim->flags & FL_LOG)
				fprintf( YSM_tmp, "l" );

			fprintf(YSM_tmp,"\n");

 		} else {
            		fprintf(YSM_tmp,"%s",YSMBuff);
		}
        }

        YSM_fclose(fd);

	rewind(YSM_tmp);

        fd = YSM_fopen(YSM_cfgfile,"w");
	if (fd == NULL) {
		/* ERR_FILE */
		return;
	}

        while(!feof(YSM_tmp)) 
	{
                memset(YSMBuff,'\0',MAX_PATH);
                fgets(YSMBuff,sizeof(YSMBuff),YSM_tmp);
                fprintf(fd,"%s",YSMBuff);
        }

         YSM_fclose(fd);
         YSM_fclose(YSM_tmp);
}


/* the fl flag determinates whether the slave must be deleted from the
	disk & list (TRUE) or it just needs to be deleted from disk. (FALSE) */

void
YSM_DelSlave( YSM_SLAVE *victim, int fl) 
{
FILE	*YSM_tmp = NULL, *fd = NULL;
int8_t	YSMBuff[MAX_PATH], *auxnick = NULL, *theuin = NULL, *therest = NULL;
int8_t	YSM_SLAVEDELETED = FALSE, fl_slavedeleted = FALSE;

                fd = YSM_fopen(YSM_cfgfile,"r");
		if (fd == NULL) {
			/* ERR_FILE */
			return;
		}

                YSM_tmp = tmpfile();
		if (YSM_tmp == NULL) /* ERR_FILE */
			return;

                while (!feof(fd)) {
                        memset(YSMBuff,'\0',MAX_PATH);
                        fgets(YSMBuff,sizeof(YSMBuff),fd);

			if(strstr(YSMBuff,SLAVES_TAG)) {
				/* Cant forget about the SLAVES TAG! */
                       		fprintf(YSM_tmp,"%s",YSMBuff);

       				memset(YSMBuff,'\0',MAX_PATH);

				while (!YSM_SLAVEDELETED) {
                			if (fgets( YSMBuff,
						sizeof(YSMBuff)-1,
						fd) == NULL) {
                        			YSM_SLAVEDELETED = TRUE;
                    				continue;
                        		}
	
				        if ((YSMBuff[0]!='#') &&
						(YSMBuff[0] != 0) && 
						(YSMBuff[0] != '\n')) { 
					
                        			auxnick = strtok(YSMBuff,":");
					if(auxnick) 
						theuin = strtok(NULL,":");
					if(theuin)
						therest = strtok(NULL,"");


					if(theuin[strlen(theuin)-1] == '\n')
						theuin[strlen(theuin)-1] = '\0';

                        		if(auxnick) {

						/* we use fl_slavedeleted
						 * to handle an erroneous
						 * cfg file where there exists
						 * duplicate slave entries.
						 */

						if(!strcasecmp( auxnick,
							victim->info.NickName)
						&& !fl_slavedeleted)
						{
							fl_slavedeleted = TRUE;

	                        			if(auxnick && fl) {
							YSM_DeleteSlavefromList
							(auxnick, atoi(theuin));
							}
						}
						else {

						fprintf(YSM_tmp,"%s:%s:",
							auxnick,theuin);

						if(therest != NULL)
							fprintf(YSM_tmp,"%s",
								therest);
						else
							fprintf(YSM_tmp,"\n");
					

						}
					}
                       			else if(strlen(YSMBuff)>2)
						fprintf(YSM_tmp,"%s",YSMBuff);
				
					}
                       			else if(strlen(YSMBuff)>2)
                       				fprintf(YSM_tmp,"%s",YSMBuff);
				}

			break;


			}
			else fprintf(YSM_tmp,"%s",YSMBuff);

                }

                YSM_fclose(fd);

                fd = YSM_fopen(YSM_cfgfile,"w");
		if (fd == NULL) {
			/* ERR_FILE */
			return;
		}

		rewind(YSM_tmp);
                memset(YSMBuff,'\0',MAX_PATH);

                while(!feof(YSM_tmp)) {
                        memset(YSMBuff,'\0',MAX_PATH);
                        fgets(YSMBuff,sizeof(YSMBuff)-1,YSM_tmp);
                        fprintf(fd,"%s",YSMBuff);
                }

                YSM_fclose(fd);
                YSM_fclose(YSM_tmp);
}


void
YSM_CFGStatus( char *validate ) 
{
u_int32_t x;

	for (x = 0; x <= strlen(validate); x++)
		validate[x] = toupper(validate[x]);

	if (!strcasecmp(validate,"ONLINE"))
		YSM_USER.status = STATUS_ONLINE;

	else if (!strcasecmp(validate,"OFFLINE"))	
		YSM_USER.status = STATUS_OFFLINE;
	
	else if (!strcasecmp(validate,"AWAY"))
		YSM_USER.status = STATUS_AWAY;

	else if (!strcasecmp(validate,"NA"))
		YSM_USER.status = STATUS_NA;
	
	else if (!strcasecmp(validate,"DND"))
		YSM_USER.status = STATUS_DND;

	else if (!strcasecmp(validate,"OCCUPIED"))
		YSM_USER.status = STATUS_OCCUPIED;

	else if (!strcasecmp(validate,"FREECHAT"))
		YSM_USER.status = STATUS_FREE_CHAT;
	
	else if (!strcasecmp(validate,"INVISIBLE"))
		YSM_USER.status = STATUS_INVISIBLE;

	else
		YSM_USER.status = STATUS_ONLINE;
}

void
YSM_AFKMode(u_int8_t turnflag)
{
	if (!turnflag) {

#ifndef COMPACT_DISPLAY
		PRINTF(VERBOSE_BASE,
			"%s %s%d%s %s",	MSG_AFK_MODE_OFF1,
					BRIGHT_BLACK,
					((time(NULL)-YSM_AFK_Time)/60), 
					NORMAL,
					MSG_AFK_MODE_OFF2);

		PRINTF(VERBOSE_BASE,
			"%s%d%s %s %s %s",
					BRIGHT_BLACK,
					YSM_AFKCount,
					NORMAL,
					MSG_AFK_MODE_OFF3,
					YSM_AFKFILENAME,
					MSG_AFK_MODE_OFF4);
#endif

		g_promptstatus.flags &= ~FL_AFKM;
	
		/* Only change the status back to online */
		/* if the user was in 'away' status.	 */
		if (YSM_USER.status == STATUS_AWAY)
			YSM_ChangeStatus(STATUS_ONLINE);

		return;
	}

#ifndef COMPACT_DISPLAY

	PRINTF(VERBOSE_BASE,"%s\n",MSG_AFK_MODE_ON);
#endif

	time(&YSM_AFK_Time);

	YSM_AFKCount = 0;
	g_promptstatus.flags |= FL_AFKM;

	if(!(strlen(YSM_AFKMessage))) { 
		strncpy(YSM_AFKMessage,
			YSM_DefaultAFKMessage,
			sizeof(YSM_AFKMessage) - 1);
		YSM_AFKMessage[sizeof(YSM_AFKMessage)-1] = '\0';
	}

	/* Only change the status to AWAY if the user is */
	/* in status ONLINE so we don't mess with their status */

	if (YSM_USER.status == STATUS_ONLINE || YSM_USER.status == STATUS_FREE_CHAT){
		YSM_ChangeStatus(STATUS_AWAY);
	}
}

/* logtype:	0 => AFK
 *		1 => Slave
 */

void
YSM_ReadLog(char *FileName, int logtype)
{
FILE	*logfile = NULL;
int8_t 	*rfilename = NULL;
int32_t	mnum = 1, tnum = 1, snum = 0, size = 0;
int8_t	q, *rtime = NULL, *aux = NULL, *auxb = NULL;
/* 64 bytes extra for YSM_LOG_SEPARATORs, date, etc */
int8_t	buff[ MAX_DATA_LEN+MAX_NICK_LEN+MAX_UIN_LEN+64 ];

	if (FileName == NULL) return;
	
	size = strlen(FileName)+strlen(YSM_cfgdir)+2;
	rfilename = YSM_Calloc(1, size, __FILE__, __LINE__);
	snprintf(rfilename,size,"%s/%s", YSM_cfgdir, FileName);
	rfilename[size - 1] = 0x00;

	if ((logfile = YSM_fopen(rfilename,"r")) == NULL) {
		PRINTF(VERBOSE_BASE,
			"\nNo Messages Found :: filename not found.\n");

		YSM_Free(rfilename, __FILE__, __LINE__);
		rfilename = NULL;
		return;
	}

	memset( buff, '\0', sizeof(buff) );

	g_promptstatus.flags |= FL_BUSYDISPLAY;
	while (!feof(logfile)) {

		/* we use mnum > YSM.. since it starts as 1 already */
		if ((fgets(buff, sizeof(buff), logfile)) == NULL ||
				(mnum > YSM_SETTING_AFKMAXSHOWN && !snum)) {

			g_promptstatus.flags &= ~FL_BUSYDISPLAY;
			PRINTF(VERBOSE_BASE,
			"\n" BRIGHT_BLUE "[N]ext %d  "
					 "[S]kip 10  "
					 "[C]lear all  "
					 "[Q]uit\n"
						NORMAL, 
						YSM_SETTING_AFKMAXSHOWN);

			g_promptstatus.flags |= FL_BUSYDISPLAY;
			q = getkey();

			switch(toupper(q))
			{
				case 'C':
					g_promptstatus.flags &= ~FL_BUSYDISPLAY;
					PRINTF(VERBOSE_BASE, "\nClearing..\n");
					YSM_fclose(logfile);
					logfile = YSM_fopen(rfilename, "w");
					if (logfile != NULL) {
						YSM_fclose(logfile);
					}

					YSM_Free(rfilename, __FILE__, __LINE__);
					rfilename = NULL;

					if (logtype == 0) YSM_AFKCount = 0;

					return;
	
					break;

				case 'N':
					mnum = 1;
					break;

				case 'S':
					snum = 10;
					break;

				default :
					g_promptstatus.flags &= ~FL_BUSYDISPLAY;
					PRINTF(VERBOSE_BASE,"\n");
					YSM_fclose(logfile);

					YSM_Free (rfilename,
						__FILE__,
						__LINE__);

					rfilename = NULL;

					return;
					break;
			}

		}


		YSM_trim(buff);
		
		if (*buff != '\0' && !YSM_DisplayLogEntry(buff, tnum)) {
			g_promptstatus.flags &= ~FL_BUSYDISPLAY;
			PRINTF(VERBOSE_BASE,"\nreadlog :: parsing error.\n");
			YSM_Free(rfilename, __FILE__, __LINE__);
			rfilename = NULL;
			YSM_fclose(logfile);
			return;
		}
	
		mnum ++; tnum ++;

		if (snum > 0) snum--;

		memset( buff, '\0', sizeof(buff) );

	}

	g_promptstatus.flags &= ~FL_BUSYDISPLAY;
	PRINTF(VERBOSE_BASE, "\nEnd of Messages\n");

	YSM_fclose( logfile );
	YSM_Free(rfilename, __FILE__, __LINE__);
	rfilename = NULL;
}

/* YSM_DisplayLogEntry - returns 0 if a parsing error occurs.
 */

int
YSM_DisplayLogEntry(int8_t *buf, int32_t messageNum)
{
char*	part[3+1];	/* 1 more than 3 to distinguish 3 from >3 fields */
ssize_t	fields;

	fields = YSM_tokenize( buf,
			YSM_LOG_SEPARATOR,
			part,
			NUM_ELEM_ARR(part));

	if (fields != 3)
		return 0;

	g_promptstatus.flags &= ~FL_BUSYDISPLAY;

	PRINTF(VERBOSE_BASE,
		"\n" WHITE "Msg #%i:" NORMAL " - From: %s",
		messageNum,
		part[0]);

	PRINTF(VERBOSE_BASE, "\nTime: %s", part[2]);
	PRINTF(VERBOSE_BASE, "\nData: %s\n", part[1]);

	g_promptstatus.flags |= FL_BUSYDISPLAY;
	return 1;
}

/* The YSM_VersionCheck function is used to check the local client 
 * version against the last release. It re-implements most of the
 * code that already exists inside our Connect procedures.
 */

int32_t
YSM_VersionCheck(void)
{
int8_t	get_string[256], *aux = NULL, *auxb = NULL;
int32_t	mysock = 0, x = 0;

	PRINTF(VERBOSE_BASE, "\n" BLUE "ysmICQ version checking.."NORMAL);

	/* first check if the user has configured a proxy */
	if (atoi(YSM_USER.proxy.proxy_host) != 0x00) {
		mysock = YSM_ProxyHalfConnect(YSM_VERSIONCHECK_HOST, 80, NULL);
	} else {
		mysock = YSM_RawConnect(YSM_VERSIONCHECK_HOST, 80);
	}

	if (mysock < 0) {
		PRINTF(VERBOSE_BASE,
		"\nVersion Check failed. Connection refused.\n\n");
		return -1;
	}


	memset(get_string,'\0',sizeof(get_string));
	snprintf(get_string, sizeof(get_string),
			"GET http://%s/%s HTTP/1.0\r\n\r\n",
			YSM_VERSIONCHECK_HOST,
			YSM_VERSIONCHECK_FILE );

	get_string[sizeof(get_string) - 1] = 0x00;

	SOCK_WRITE(mysock, get_string, strlen(get_string));
	memset(get_string,'\0',sizeof(get_string));

	/* read the first response */
	if (YSM_READ_LN(mysock, get_string, sizeof(get_string)) <= 0) {
		PRINTF( VERBOSE_BASE, "\nVersion Check failed. "
			"Data file not found on host!.\n\n");
		return -1;
	}
			
	if (strstr(get_string,"200 OK") == NULL) {
		PRINTF( VERBOSE_BASE, "\nVersion Check failed. "
			"Data file not found on host!.\n\n");
		return -1;
	}

	/* read until the end of the headers */
	do {
		memset(get_string,'\0',sizeof(get_string));
		x = YSM_READ_LN( mysock, get_string, sizeof(get_string));
	} while (x);

	/* now read the data */
	memset(get_string,'\0',sizeof(get_string));
	SOCK_READ(mysock, get_string, sizeof(get_string)-1);

	if ((aux = strstr(get_string,"YSM_LAST_VERSION")) != NULL) {
		auxb = strtok(aux, "YSM_LAST_VERSION ");
		if (auxb) {
			int8_t *ver = NULL;
			int32_t	n = 0;
			
			aux = strchr(auxb,'\n');
			if (aux) *aux = '\0';

			ver = strchr( YSM_INFORMATION2, '-');
			if (ver == NULL) ver = YSM_INFORMATION2;
			else ver++;

			n = strcasecmp(ver, auxb);
			if (n == 0)
				PRINTF(VERBOSE_BASE,
				GREEN"Version is up-to-date!\n\n"NORMAL);
			else if( n > 0)
				PRINTF(VERBOSE_BASE,
				GREEN"Pre-release version!\n\n"NORMAL);
			else
				PRINTF(VERBOSE_BASE,
				RED "Version isn't last! Go get %s!\n\n"
								NORMAL, auxb);
		} else
			PRINTF( VERBOSE_BASE,
			"\nVersion Check Failed. Data file corrupted.\n\n");

	} else
		PRINTF( VERBOSE_BASE,
			"\nVersion Check Failed. Data file corrupted.\n\n");

	close(mysock);
	return 0;
}

void
YSM_AskProxyConfiguration( void )
{
int8_t	buf[MAX_PATH], def_settings = 0;

	PRINTF(VERBOSE_BASE,
	"\nIf you need to connect to the Internet through a Proxy.");

	PRINTF(VERBOSE_BASE,
	"\nPlease specify the address below.");

	PRINTF(VERBOSE_BASE,
	"\nType '" RED "no" NORMAL "', type '" RED "0" NORMAL "' or just press "
	"the '" RED "enter" NORMAL 
	"' key if you dont want\n"
	"to use a proxy or don't know what this is.\n");

	memset( buf, 0, sizeof(buf) );

#ifdef WIN32
/* Great idea by a co-worker of mine, Riq 
 * Let the user use his default Internet Explorer proxy! */

	if ( YSM_ReadWindowsProxy() ) {
		PRINTF(VERBOSE_BASE,
		"\nYSM found your Internet Explorer proxy settings.\n"
		"Proxy host and port defaults loaded. You can accept them\n"
		"when prompted by using the 'enter' key.\n");
		def_settings = 1;
	}
#endif

	PRINTF(VERBOSE_BASE,
			"\nProxy address [%s]: ", YSM_USER.proxy.proxy_host);

	YSM_fgets(buf, MAX_PATH-1, 0);

	buf[strlen(buf)-1] = '\0';

	/* Cancel proxy configuration? 
	 * Either the user typed 'no', typed '0' or pressed the enter key.
	 * note pressing the enter key when default values are used will
	 * be interpreted as 'accepting them'. 
	 */

	if (!strcasecmp(buf,"no") 
		|| buf[0] == '0'
		|| (buf[0] == '\0' && !def_settings)) {
#ifdef WIN32
		/* clear the proxy_host and proxy_port if it was
		 * filled inside ReadWindowsProxy()
		 */

		memset( YSM_USER.proxy.proxy_host,
			0,
			sizeof(YSM_USER.proxy.proxy_host)
			);

		YSM_USER.proxy.proxy_port = 0x00;
#endif
		return;
	}
	
	if(strlen(buf) > 1) 
		strncpy( YSM_USER.proxy.proxy_host,
			buf,
			sizeof(YSM_USER.proxy.proxy_host) - 1 );

	PRINTF(VERBOSE_BASE, "\nProxy Port? [%d]: ", YSM_USER.proxy.proxy_port);

	memset( buf, 0, sizeof( buf ) );
	YSM_fgets(buf, MAX_PATH-1, 0);

	if(strlen(buf) >= 2 && buf[0] != 0x00)
		YSM_USER.proxy.proxy_port = atoi(buf);

	PRINTF(VERBOSE_BASE, 
		"\nHTTPS hack ? (uses port 443 instead of 5190) [1/0]: ");

	memset(buf,0,sizeof(buf));
	YSM_fgets(buf,MAX_PATH-1, 0);

	if (atoi(&buf[0]) > 0)	/* Set HTTPS in flags */
		YSM_USER.proxy.proxy_flags |= YSM_PROXY_HTTPS;

	PRINTF(VERBOSE_BASE, 
		"\nMake the proxy resolve hostnames? [1/0]: ");

	memset(buf,0,sizeof(buf));
	YSM_fgets(buf,MAX_PATH-1, 0);

	if (atoi(&buf[0]) > 0)	/* Set RESOLVE in flags */
		YSM_USER.proxy.proxy_flags |= YSM_PROXY_RESOLVE;

	PRINTF(VERBOSE_BASE, 
		"\nThe proxy requires authentication? [1/0]: ");

	memset(buf,0,sizeof(buf));
	YSM_fgets(buf,MAX_PATH-1, 0);

	if (atoi(&buf[0]) > 0) {
		YSM_USER.proxy.proxy_flags |= YSM_PROXY_AUTH;
		PRINTF(VERBOSE_BASE, "\nProxy username: ");
		YSM_fgets(buf, MAX_PATH-1, 0);
		buf[strlen(buf)-1] = '\0';
	
		if(strlen(buf) > 1) 
			strncpy( YSM_USER.proxy.username,
				buf,
				sizeof(YSM_USER.proxy.username) - 1 );

		PRINTF(VERBOSE_BASE, "\nProxy password: ");
		YSM_fgets(buf, MAX_PATH-1, 0);
		buf[strlen(buf)-1] = '\0';
	
		if(strlen(buf) > 1) 
			strncpy( YSM_USER.proxy.password,
				buf,
				sizeof(YSM_USER.proxy.password) - 1 );
	}
}


void
YSM_ExecuteCommand( int argc, char **argv )
{
#ifdef WIN32
char	win32_cmd[MAX_DATA_LEN+1];	/* using max data len, bleh */
STARTUPINFO		si;
PROCESS_INFORMATION	pi;
#else
pid_t	e2pid;
#endif

#ifdef WIN32
int	x = 0;

	memset(win32_cmd, 0, MAX_DATA_LEN+1);

	while (argv[x] != NULL && x < argc ) {
		strncat( win32_cmd,
			argv[x],
			sizeof(win32_cmd) - strlen(win32_cmd) - 1 );

		/* separate arguments by a space */
		strncat( win32_cmd,
			" ",
			sizeof(win32_cmd) - strlen(win32_cmd) - 1 );
		x++;
	}

	ZeroMemory( &si, sizeof(si) );
	si.cb = sizeof(si);
	ZeroMemory( &pi, sizeof(pi) );

	CreateProcess(NULL,
		&win32_cmd[0],
		NULL,
		NULL,
		FALSE,
		0,
		NULL,
		NULL,
		&si,
		&pi);
#elif OS2
	os2_startsession(argv, argc);
#else
	/* unix exec code */
	if ((e2pid = fork()) == 0)	/* child proc */
	{
		int i;
		pid_t epid;

		for (i = 0; i < 256; i++)
			close(i);

		epid = fork();
		if (epid == 0) {
#ifdef HAVE_SETENV
			int8_t tmp[MAX_DATA_LEN];
			/* set a couple of useful environment variables */
			memset(tmp, 0, sizeof(tmp));
			/* our own status */
			YSM_WriteStatus(YSM_USER.status, tmp);
			setenv("YSMSTATUS", tmp, 1);
			/* our own UIN */	
			snprintf(tmp, sizeof(tmp)-1, "%d", (int)YSM_USER.Uin);
			tmp[sizeof(tmp)-1] = 0x00;
			setenv("YSMUIN", tmp, 1);
#endif		
			execv(argv[0], argv);
		} else if (epid > 0) {
			while (waitpid(-1, NULL, WNOHANG) > 0);
		}

		exit(0);
	}

	if (e2pid > 0)
		waitpid(e2pid, NULL, 0);
#endif

}


/* The following HandleCommand function makes use */
/* of the system() function. Since its locally user dependent */
/* it does not mean any risk unless the YSM user is drunk and willing */
/* to play with ';' chars :) */

void
YSM_HandleCommand(char *_argone)
{
	/* Sucks huh */
	system(_argone);
}


