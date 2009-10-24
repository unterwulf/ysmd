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

#include "ysm.h"
#include "main.h"
#include "direct.h"
#include "setup.h"
#include "prompt.h"
#include "toolbox.h"
#include "network.h"
#include "commands.h"
#include "timers.h"

short        YSM_Reason_To_Suicide = FALSE;
ysm_config_t g_cfg;
dl_list_t    g_slave_list = { NULL, 0 };
dl_list_t    g_command_list = { NULL, 0 };
dl_list_t    g_filemap_list = { NULL, 0 };

slave_t *YSMSlaves_LastSent=0, *YSMSlaves_LastRead=0;
slave_t *YSMSlaves_TabSlave = NULL;

struct    YSM_MODEL    YSM_USER;

extern char YSM_AFKMessage[MAX_DATA_LEN + 1];
extern char YSM_cfgfile[MAX_PATH];
extern char YSM_cfgdir[MAX_PATH];

#ifdef YSM_WITH_THREADS
pthread_t    t_netid, t_cycleid, t_dcid;
#endif


int main(int argc, char **argv)
{
    char buf[MAX_TIME_LEN];

    YSM_CheckSecurity();
    reset_timer(UPTIME);
    reset_timer(COMMAND_FILE_CHECK_TIMEOUT);

    /* Check for arguments - alternate config file */
    if (argc > 2)
    {
        if (!strcmp(argv[1], "-c"))
        {
            /* Use this configuration file */
            if (argv[2] != NULL)
            {
                char *aux = NULL;

                strncpy(YSM_cfgfile, argv[2], sizeof(YSM_cfgfile) - 1);
                YSM_cfgfile[sizeof(YSM_cfgfile) - 1] = '\0';

                strncpy(YSM_cfgdir, argv[2], sizeof(YSM_cfgdir) - 1);
                YSM_cfgdir[sizeof(YSM_cfgdir) - 1] = '\0';
                aux = strrchr(YSM_cfgdir, '/');

                if (NULL != aux)
                    *(aux+1) = '\0';
                else
                {
                    strncpy(YSM_cfgdir, "./", sizeof(YSM_cfgdir) - 1);
                    YSM_cfgdir[sizeof(YSM_cfgdir) - 1] = '\0';
                }
            }
        }
    }

    if (YSM_Initialize() < 0) return -1;

    YSM_ConsoleSetup();
    YSM_Setup();

    /* Moved direct connections intialization to main() because
     * we need a few configuration file settings loaded before.
     */
#ifdef YSM_WITH_THREADS
    YSM_DC_Init();
#endif

    PRINTF(VERBOSE_BASE,
        "\n%s %d ]\n",
        MSG_STARTING_TWO,
        YSM_USER.Uin );

    PRINTF(VERBOSE_BASE,
        GREEN "love ysm? " NORMAL "make your contribution! " GREEN "(ysmv7.sourceforge.net)" NORMAL "\n");

    YSM_PasswdCheck();

    reset_timer(KEEP_ALIVE_TIMEOUT);
    reset_timer(IDLE_TIMEOUT);

#ifdef DAEMON
    /* daemonize */
    chdir("/");
    unlink(YSM_FIFO);
    mkfifo(YSM_FIFO, 0600);

    if (fork())
        exit(0);

    setsid();

//    for (sig = 1; sig < 32; sig++)
//        signal(sig, fsignal);
#endif

#ifdef YSM_WITH_THREADS
    pthread_create(&t_netid, NULL, (void *) &network_thread, NULL);
    pthread_create(&t_cycleid, NULL, (void *) &YSM_Start_Cycle, NULL);

    if (!g_cfg.dcdisable)
        pthread_create(&t_dcid, NULL, (void *) &dc_thread, NULL);

    /* Take the main thread for the prompt */
    prompt_thread();
#else
    start();
#endif

    return 0;
}


void start_prompt(void)
{
    YSM_ConsoleReadInit();

    FD_Init(FD_KEYBOARD);
    FD_Add(0, FD_KEYBOARD);
    FD_Select(FD_KEYBOARD);
    if (FD_IsSet(0, FD_KEYBOARD))
        YSM_ConsoleRead();
}

static void start_network(void)
{
    FD_Timeout(0, 1000);
    FD_Init(FD_NETWORK);
    if (YSM_USER.network.rSocket)
        FD_Add(YSM_USER.network.rSocket, FD_NETWORK);

    FD_Select(FD_NETWORK);

    if (FD_IsSet(YSM_USER.network.rSocket, FD_NETWORK))
        YSM_SrvResponse();

    YSM_CycleChecks();
}

/*
 * start() used when there is no Threads Support.
 * What it does? Instead of re-writing a big function
 * we only write stuff that needs to be done once, and
 * on each Start function we leave out the while loops.
 * Hence, we make a while in this function. Weird but nice :)
 */

#ifndef YSM_WITH_THREADS
static void start(void)
{
    if (YSM_SignIn() < 0)
        YSM_Error(ERROR_NETWORK, __FILE__, __LINE__, 0);

    while (!YSM_Reason_To_Suicide)
    {
        start_prompt();
        start_network();
    }
}

#else

static void prompt_thread(void)
{
    YSM_ConsoleReadInit();

    while (!YSM_Reason_To_Suicide)
    {
        FD_Init(FD_KEYBOARD);
        FD_Add(0, FD_KEYBOARD);
        YSM_ConsoleRead();
        YSM_Thread_Sleep(0, 10);
    }
}

static void network_thread(void)
{
    if (YSM_SignIn() < 0)
        YSM_Error(ERROR_NETWORK, __FILE__, __LINE__, 0);

    while (!YSM_Reason_To_Suicide)
    {
        YSM_SrvResponse();
    }
}

void YSM_Start_Cycle(void)
{
    while (!YSM_Reason_To_Suicide)
    {
        YSM_CycleChecks();
        YSM_Thread_Sleep(0, 100);
        YSM_DC_Select();
    }
}

static void dc_thread(void)
{
    slave_t *slave = NULL;

    while (!YSM_Reason_To_Suicide)
    {
        slave = YSM_DC_Wait4Client();

        if (slave == NULL)
        {
            PRINTF(VERBOSE_DCON,
                "Incoming DC request failed. "
                "Connection closed.\n");
        }
        else
        {
            PRINTF(VERBOSE_DCON,
                "New DC connection request from slave %s\n",
                slave->info.NickName);
        }
    }
}
#endif


/* SignIn to the ICQ Network             */
/* moduled for being able to 'reconnect' */

int YSM_SignIn(void)
{
    u_int16_t port = 0;

    if (YSM_USER.proxy.proxy_flags & YSM_PROXY_HTTPS)
        port = 443;
    else
        port = YSM_USER.network.auth_port;

    YSM_USER.network.rSocket = YSM_Connect(
        YSM_USER.network.auth_host,
        port,
        0x1);

    if (YSM_USER.network.rSocket < 0)
        return YSM_USER.network.rSocket;

    PRINTF(VERBOSE_BASE, "\rLogging in.. [");
    YSM_Init_LoginA(YSM_USER.Uin, YSM_USER.password);

    return YSM_USER.network.rSocket;
}

/*    This function takes care of the regular checks to be
 *    done every 1, or 2 seconds. cool huh :)
 */

void YSM_CycleChecks(void)
{
    if (g_sinfo.flags & FL_LOGGEDIN)
    {
        /* check if it is time to send keep alive */
        if (get_timer(KEEP_ALIVE_TIMEOUT) > 58)
        {
            reset_timer(KEEP_ALIVE_TIMEOUT);
            YSM_KeepAlive();
        }

        /* check if we have to switch to away status */
        if (g_cfg.awaytime > 0               /* is it enabled? */
        && !(g_promptstatus.flags & FL_AFKM) /* we are not in AFK */
        && (get_timer(IDLE_TIMEOUT)/60       /* minutes */
            >= g_cfg.awaytime))              /* are over */
        {
            /* then check if we are in -online status-
             * OR Free 4 Chat, which would be the same */
            if (YSM_USER.status == STATUS_ONLINE
            || YSM_USER.status == STATUS_FREE_CHAT)
            {
                /* finally. change status */
                YSM_ChangeStatus(STATUS_AWAY);
                g_promptstatus.flags |= FL_REDRAW;
                g_promptstatus.flags |= FL_AUTOAWAY;
            }
        }

        /* check if we have to check the command file */
        if (get_timer(COMMAND_FILE_CHECK_TIMEOUT) >= YSM_COMMANDSTIME)
        {
            YSM_CheckCommandsFile();
            reset_timer(COMMAND_FILE_CHECK_TIMEOUT);
        }

        /* only redraw console if display isn't busy (Threads) */
        if ((g_promptstatus.flags & FL_REDRAW)
        && !(g_promptstatus.flags & FL_BUSYDISPLAY))
        {
            /* only redraw the prompt fully if it was overwritten */
            if (g_promptstatus.flags & FL_OVERWRITTEN)
                YSM_ConsoleRedrawPrompt(TRUE);
            else
                YSM_ConsoleRedrawPrompt(FALSE);

            g_promptstatus.flags &= ~FL_OVERWRITTEN;
            g_promptstatus.flags &= ~FL_REDRAW;
        }
    }
}
