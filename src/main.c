#include "ysm.h"
#include "direct.h"
#include "setup.h"
#include "prompt.h"
#include "toolbox.h"
#include "timers.h"
#include "output.h"
#include "network.h"
#include "ystring.h"
#include "slaves.h"
#include "control.h"
#include <locale.h>

ysm_config_t g_cfg;
ysm_state_t  g_state;
ysm_model_t  g_model;
pthread_t    t_netid, t_cycleid, t_dcid;
sem_t        semOutput;

/* This function takes care of the regular checks to be
 * done every 1, or 2 seconds. cool huh :)
 */

static void doCycleChecks(void)
{
    if (g_state.connected)
    {
        /* check if it is time to send keep alive */
        if (getTimer(KEEP_ALIVE_TIMEOUT) > 58)
        {
            resetTimer(KEEP_ALIVE_TIMEOUT);
            sendKeepAlive();
        }
    }
}

static void promptThread(void)
{
    initCtl();

    while (!g_state.reasonToSuicide)
    {
        ctlHandler();
    }

    closeCtl();
}

static void networkThread(void)
{
    while (!g_state.reasonToSuicide)
    {
        if (g_state.connected)
            serverResponseHandler();
        else
            threadSleep(0, 100);
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
    const slave_t *slave;
    uin_t          uin;

    initDC();

    while (!g_state.reasonToSuicide)
    {
        uin = YSM_DC_Wait4Client();
        lockSlaveList();
        slave = getSlaveByUin(uin);

        if (!slave)
        {
            printfOutput(VERBOSE_DCON,
                "Incoming DC request failed. "
                "Connection closed.\n");
        }
        else
        {
            printfOutput(VERBOSE_DCON, "IN DC_REQ %ld %s\n",
                    slave->uin, slave->info.nickName);
        }

        unlockSlaveList();
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
    g_state.connected = FALSE;

    sem_init(&semOutput, 0, 1);
    checkSecurity();
    resetTimer(UPTIME);
    resetTimer(KEEP_ALIVE_TIMEOUT);
    resetTimer(IDLE_TIMEOUT);

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

    printfOutput(VERBOSE_BASE, "INFO STARTING %ld\n", g_model.uin);

#if 0
    /* daemonize */
    chdir("/");
    unlink(YSM_FIFO);
    mkfifo(YSM_FIFO, 0600);

    if (fork())
        exit(0);

    setsid();
#endif

//    for (sig = 1; sig < 32; sig++)
//        signal(sig, fsignal);

    if (pthread_create(&t_netid, NULL, (void *) &networkThread, NULL) != 0)
        return -1;

    if (pthread_create(&t_cycleid, NULL, (void *) &cycleThread, NULL) != 0)
	    return -1;

    if (!g_cfg.dcdisable)
        if (pthread_create(&t_dcid, NULL, (void *) &dcThread, NULL) != 0)
            return -1;

    /* Take the main thread for the prompt */
    promptThread();

    /* exit here */
    threadSleep(0, 200);
    ysm_exit(0);

    return 0;
}

