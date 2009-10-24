#include "ysm.h"
#include "direct.h"
#include "setup.h"
#include "prompt.h"
#include "toolbox.h"
#include "timers.h"
#include "output.h"
#include "network.h"
#include "ystring.h"
#include <locale.h>

ysm_config_t g_cfg;
ysm_state_t  g_state;
ysm_model_t  YSM_USER;
pthread_t    t_netid, t_cycleid, t_dcid;
sem_t        semOutput;

/* This function takes care of the regular checks to be
 * done every 1, or 2 seconds. cool huh :)
 */

static void doCycleChecks(void)
{
    if (g_sinfo.flags & FL_LOGGEDIN)
    {
        /* check if it is time to send keep alive */
        if (getTimer(KEEP_ALIVE_TIMEOUT) > 58)
        {
            resetTimer(KEEP_ALIVE_TIMEOUT);
            sendKeepAlive();
        }

        /* check if we have to switch to away status */
        if (g_cfg.awaytime > 0               /* is it enabled? */
        && (getTimer(IDLE_TIMEOUT)/60        /* minutes */
            >= g_cfg.awaytime))              /* are over */
        {
            /* then check if we are in -online status-
             * OR Free 4 Chat, which would be the same */
            if (YSM_USER.status == STATUS_ONLINE
            || YSM_USER.status == STATUS_FREE_CHAT)
            {
                /* finally. change status */
                YSM_ChangeStatus(STATUS_AWAY);
                g_state.promptFlags |= FL_RAW;
                g_state.promptFlags |= FL_AUTOAWAY;
            }
        }
    }
}

static void promptThread(void)
{
    int fd;

    while (!g_state.reasonToSuicide)
    {
        fd = open(YSM_FIFO, O_RDONLY);
        if (fd == -1)
        {
            printf("Error stdin");
            return;
        }

        YSM_ConsoleRead(fd);
        close(fd);
        threadSleep(0, 10);
    }
}

static void networkThread(void)
{
    if (networkSignIn() < 0)
        YSM_ERROR(ERROR_NETWORK, 0);

    while (!g_state.reasonToSuicide)
    {
        serverResponseHandler();
    }
}

static void cycleThread(void)
{
    while (!g_state.reasonToSuicide)
    {
        doCycleChecks();
        threadSleep(0, 100);
        dcSelect();
    }
}

static void dcThread(void)
{
    uin_t      slave = NOT_A_SLAVE;
    string_t  *nick;

    initDC();

    while (!g_state.reasonToSuicide)
    {
        slave = YSM_DC_Wait4Client();

        if (slave == NOT_A_SLAVE)
        {
            printfOutput(VERBOSE_DCON,
                "Incoming DC request failed. "
                "Connection closed.\n");
        }
        else
        {
            getSlaveNick(uin, nick);
            printfOutput(VERBOSE_DCON, "IN DC_REQ %ld %s\n",
                uin,
                getString(nick));
            freeString(nick);
        }
    }
}

static void checkSecurity(void)
{
    if (!getuid())
    {
        printfOutput(VERBOSE_BASE,
            "HOLD IT! I'm sorry, but i WONT let you run ysmd\n"
            "with uid 0. Don't run ysmd as root!. ..fag.\n");

        /* Not using YSM_Exit() here since YSM didn't start */
        exit(-1);
    }
}

int main(int argc, char **argv)
{
    setlocale(LC_ALL, "");

    g_state.reasonToSuicide = FALSE;
    g_state.reconnecting = FALSE;
    g_state.lastRead = 0;
    g_state.lastSent = 0;

    sem_init(&semOutput, 0, 1);
    checkSecurity();
    resetTimer(UPTIME);

    /* Check for arguments - alternate config file */
    if (argc > 2)
    {
        if (!strcmp(argv[1], "-c"))
        {
            /* Use this configuration file */
            if (argv[2] != NULL)
            {
                char *aux = NULL;

                strncpy(g_state.configFile, argv[2], sizeof(g_state.configFile) - 1);
                g_state.configFile[sizeof(g_state.configFile) - 1] = '\0';

                strncpy(g_state.configDir, argv[2], sizeof(g_state.configDir) - 1);
                g_state.configDir[sizeof(g_state.configDir) - 1] = '\0';
                aux = strrchr(g_state.configDir, '/');

                if (NULL != aux)
                    *(aux+1) = '\0';
                else
                {
                    strncpy(g_state.configDir, "./", sizeof(g_state.configDir) - 1);
                    g_state.configDir[sizeof(g_state.configDir) - 1] = '\0';
                }
            }
        }
    }

    if (initialize() < 0)
        return -1;

    printfOutput(VERBOSE_BASE, "INFO STARTING %ld\n", YSM_USER.uin);

    YSM_PasswdCheck();

    resetTimer(KEEP_ALIVE_TIMEOUT);
    resetTimer(IDLE_TIMEOUT);

    /* daemonize */
    chdir("/");
    unlink(YSM_FIFO);
    mkfifo(YSM_FIFO, 0600);

    if (fork())
        exit(0);

    setsid();

//    for (sig = 1; sig < 32; sig++)
//        signal(sig, fsignal);

    pthread_create(&t_netid, NULL, (void *) &networkThread, NULL);
    pthread_create(&t_cycleid, NULL, (void *) &cycleThread, NULL);

    if (!g_cfg.dcdisable)
        pthread_create(&t_dcid, NULL, (void *) &dcThread, NULL);

    /* Take the main thread for the prompt */
    promptThread();

    return 0;
}

