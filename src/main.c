/*

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
#include <locale.h>

ysm_config_t g_cfg;
ysm_state_t  g_state;
dl_list_t    g_slave_list = { NULL, 0 };
dl_list_t    g_command_list = { NULL, 0 };
ysm_model_t  YSM_USER;
pthread_t    t_netid, t_cycleid, t_dcid;

int main(int argc, char **argv)
{
    setlocale(LC_ALL, "");

    g_state.reason_to_suicide = FALSE;
    g_state.reconnecting = FALSE;
    g_state.last_read = NULL;
    g_state.last_sent = NULL;

    YSM_CheckSecurity();
    reset_timer(UPTIME);

    /* Check for arguments - alternate config file */
    if (argc > 2) {
        if (!strcmp(argv[1], "-c"))
        {
            /* Use this configuration file */
            if (argv[2] != NULL)
            {
                char *aux = NULL;

                strncpy(g_state.config_file, argv[2], sizeof(g_state.config_file) - 1);
                g_state.config_file[sizeof(g_state.config_file) - 1] = '\0';

                strncpy(g_state.config_dir, argv[2], sizeof(g_state.config_dir) - 1);
                g_state.config_dir[sizeof(g_state.config_dir) - 1] = '\0';
                aux = strrchr(g_state.config_dir, '/');

                if (NULL != aux)
                    *(aux+1) = '\0';
                else
                {
                    strncpy(g_state.config_dir, "./", sizeof(g_state.config_dir) - 1);
                    g_state.config_dir[sizeof(g_state.config_dir) - 1] = '\0';
                }
            }
        }
    }

    if (initialize() < 0)
        return -1;

    PRINTF(VERBOSE_BASE,
        "\n%s %d ]\n",
        MSG_STARTING_TWO,
        YSM_USER.Uin);

    YSM_PasswdCheck();

    reset_timer(KEEP_ALIVE_TIMEOUT);
    reset_timer(IDLE_TIMEOUT);

    /* daemonize */
    chdir("/");
    unlink(YSM_FIFO);
    mkfifo(YSM_FIFO, 0600);

    if (fork())
        exit(0);

    setsid();

//    for (sig = 1; sig < 32; sig++)
//        signal(sig, fsignal);

    pthread_create(&t_netid, NULL, (void *) &network_thread, NULL);
    pthread_create(&t_cycleid, NULL, (void *) &cycle_thread, NULL);

    if (!g_cfg.dcdisable)
        pthread_create(&t_dcid, NULL, (void *) &dc_thread, NULL);

    /* Take the main thread for the prompt */
    prompt_thread();

    return 0;
}

static void prompt_thread(void)
{
    int fd;

    while (!g_state.reason_to_suicide)
    {
        fd = open(YSM_FIFO, O_RDONLY);
        if (fd == -1)
        {
            printf("Error stdin");
            return;
        }

        YSM_ConsoleRead(fd);
        close(fd);
        YSM_Thread_Sleep(0, 10);
    }
}

static void network_thread(void)
{
    if (YSM_SignIn() < 0)
        YSM_ERROR(ERROR_NETWORK, 0);

    while (!g_state.reason_to_suicide)
    {
        YSM_SrvResponse();
    }
}

static void cycle_thread(void)
{
    while (!g_state.reason_to_suicide)
    {
        YSM_CycleChecks();
        YSM_Thread_Sleep(0, 100);
        YSM_DC_Select();
    }
}

static void dc_thread(void)
{
    slave_t *slave = NULL;

    init_dc();

    while (!g_state.reason_to_suicide)
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
                g_promptstatus.flags |= FL_RAW;
                g_promptstatus.flags |= FL_AUTOAWAY;
            }
        }
    }
}
