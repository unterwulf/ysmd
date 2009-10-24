#include "ysm.h"
#include "setup.h"
#include "wrappers.h"
#include "toolbox.h"
#include "direct.h"
#include "slaves.h"
#include "network.h"
#include "crypt.h"
#include "charset.h"
#include "output.h"
#include <stdarg.h>

int8_t YSM_DefaultCHATMessage[MAX_DATA_LEN+1];

void initDefaultConfig(ysm_config_t *cfg)
{
    cfg->verbose = 0x5;
    cfg->spoof = FALSE;
    cfg->awaytime = 5;
    cfg->antisocial = FALSE;
    cfg->updateNicks = TRUE;
    cfg->dcdisable = FALSE;
    cfg->dclan = FALSE;

    cfg->dcport1 = 0;
    cfg->dcport2 = 0;

    /* needs to store a 4 bytes UIN */
    cfg->forward = 0;
    cfg->outputType = OT_STDOUT;

    memset(&cfg->CHATMessage,  0, sizeof(cfg->CHATMessage));
    memset(&cfg->outputPath,   0, sizeof(cfg->outputPath));

#if defined (YSM_USE_CHARCONV)
    memset(g_cfg.charsetTrans, 0, MAX_CHARSET+4);
    memset(g_cfg.charsetLocal, 0, MAX_CHARSET+4);
#endif
}

int initialize(void)
{
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, sigHandler);
    signal(SIGCHLD, sigHandler);

    memset(&YSM_USER, 0, sizeof(YSM_USER));

    /* Network initialization */
    initNetwork();

    /* Default configuration initialization*/
    initDefaultConfig(&g_cfg);

    /* Setup the config and dir path only if -c was not used before */
    if (g_state.configFile[0] == '\0' || g_state.configDir[0] == '\0')
    {
        printfOutput(VERBOSE_MOATA, "Setting up Config. file Path.");
        setupHomeDirectory();
    }

    printfOutput(VERBOSE_MOATA, "Reading config file.");
    initConfig();

#ifdef YSM_USE_CHARCONV
    /* Initialize CodePage/Charsets */
    initCharset();
#endif

    printfOutput(VERBOSE_MOATA, "Retrieving slave data.");
    initSlaves();

    return 0;
}

void initConfig(void)
{
    FILE *fd = NULL;

    if ((fd = fopen(g_state.configFile, "r")) != NULL)
    {
        strncpy(YSM_USER.network.authHost,
                YSM_DEFAULTSRV,
                sizeof(YSM_USER.network.authHost)-1);
        YSM_USER.network.authHost[sizeof(YSM_USER.network.authHost)-1] = '\0';
        YSM_USER.network.authPort = YSM_DEFAULTPORT;

        readConfig(fd, 0);
        fclose(fd);
    }
    else
    {
        printfOutput(VERBOSE_BASE, "Couldn't open config file.\n");
        exit(1);
    }
}

void setupHomeDirectory(void)
{
    int8_t *home;

    home = getenv("HOME");

    if (home == NULL)
    {
        printfOutput(VERBOSE_BASE,
            "Couldn't determine the home directory.");
        exit(1);
    }

    snprintf(g_state.configDir, sizeof(g_state.configDir),
        "%s/%s", home, YSM_CFGDIRECTORY);

    snprintf(g_state.configFile, sizeof(g_state.configFile),
        "%s/%s", g_state.configDir, YSM_CFGFILENAME);

    snprintf(g_state.slavesFile, sizeof(g_state.slavesFile),
        "%s/%s", g_state.configDir, YSM_SLAVESFILENAME);
}

void initSlaves(void)
{
    FILE *fd;

    if ((fd = fopen(g_state.slavesFile, "r")) != NULL)
    {
        readSlaves(fd);
        fclose(fd);
    }
    else
    {
        printfOutput(VERBOSE_BASE,
            "Contact list couldn't be read! File not found.\n" );
        YSM_ERROR(ERROR_CRITICAL, 0);
    }
}

void readConfig(FILE *fd, char reload)
{
    int8_t buf[MAX_PATH];
    int8_t *name;
    int8_t *value;
    int8_t *tmp;
    int8_t i;
    static struct
    {
        int8_t *name;
        void   *dst;
        enum
        {
            PAR_STATUS, PAR_OUTPUT, PAR_INT8, PAR_INT16, PAR_INT32,
            PAR_STR, PAR_BOOL8, PAR_BOOL16, PAR_END
        } type;
        int16_t  param;
    } directives[] =
    {
        { "status",         &YSM_USER.status,            PAR_STATUS, 0 },
        { "uin",            &YSM_USER.uin,               PAR_INT32,  0 },
        { "server",         &YSM_USER.network.authHost,  PAR_STR,    sizeof(YSM_USER.network.authHost) },
        { "serverport",     &YSM_USER.network.authPort,  PAR_STR,    sizeof(YSM_USER.network.authPort) },
        { "password",       &YSM_USER.password,          PAR_STR,    sizeof(YSM_USER.password) },
        { "awaytime",       &g_cfg.awaytime,             PAR_INT8,   0 },
        { "chatmessage",    &YSM_DefaultCHATMessage,     PAR_STR,    sizeof(YSM_DefaultCHATMessage) },
        { "output",         NULL,                        PAR_OUTPUT, 0 },
        { "proxy",          &YSM_USER.proxy.host,        PAR_STR,    sizeof(YSM_USER.proxy.host) },
        { "proxyport",      &YSM_USER.proxy.port,        PAR_INT16,  0 },
        { "proxyhttps",     &YSM_USER.proxy.flags,       PAR_BOOL8,  YSM_PROXY_HTTPS },
        { "proxyresolve",   &YSM_USER.proxy.flags,       PAR_BOOL8,  YSM_PROXY_RESOLVE },
        { "proxyauth",      &YSM_USER.proxy.flags,       PAR_BOOL8,  YSM_PROXY_AUTH },
        { "proxyusername",  &YSM_USER.proxy.username,    PAR_STR,    sizeof(YSM_USER.proxy.username) },
        { "proxypassword",  &YSM_USER.proxy.password,    PAR_STR,    sizeof(YSM_USER.proxy.password) },
        { "antisocial",     &g_cfg.antisocial,           PAR_BOOL8,  1 },
        { "updatenicks",    &g_cfg.updateNicks,          PAR_BOOL8,  1 },
        { "dcdisable",      &g_cfg.dcdisable,            PAR_BOOL8,  1 },
        { "dclan",          &g_cfg.dclan,                PAR_BOOL8,  1 },
        { "dcport1",        &g_cfg.dcport1,              PAR_INT16,  0 },
        { "dcport2",        &g_cfg.dcport2,              PAR_INT16,  0 },
        { "webaware",       &YSM_USER.status_flags,      PAR_BOOL16, STATUS_FLWEBAWARE },
        { "mybirthday",     &YSM_USER.status_flags,      PAR_BOOL16, STATUS_FLBIRTHDAY },
        { "verbose",        &g_cfg.verbose,              PAR_INT8,   0 },
#ifdef YSM_USE_CHARCONV
        { "charsettrans",   &g_cfg.charsetTrans,         PAR_STR,    sizeof(g_cfg.charsetTrans) },
        { "charsetlocal",   &g_cfg.charsetLocal,         PAR_STR,    sizeof(g_cfg.charsetLocal) },
#endif
        { NULL,             NULL,                        PAR_END,    0 }
    };

    snprintf(YSM_DefaultCHATMessage, sizeof(YSM_DefaultCHATMessage), YSM_CHAT_MESSAGE);

    YSM_USER.status_flags |= STATUS_FLDC_CONT;

    while (!feof(fd))
    {
        memset(buf, '\0', sizeof(buf));
        fgets(buf, sizeof(buf) - 1, fd);

        /* crop comments till # to the end of line */
        for (tmp = buf; *tmp != '\0'; tmp++)
        {
            if (*tmp == '#')
            {
                *tmp = '\0';
                break;
            }
        }

        /* crop spaces at the end of line */
        for (tmp = buf + strlen(buf) - 1; tmp >= buf && isspace(*tmp); tmp++)
            *tmp = '\0';

        /* crop spaces at the begin of line */
        for (name = buf; isspace(*name); name++)
            ;

        for (value = name; *value != '\0'; value++)
            if (*value == ' ')
            {
                *value = '\0';
                for (value++; isspace(*value); value++)
                    ;
                break;
            }
            else
                *value = tolower(*value);

        /* if line is empty */
        if (name[0] == '\0')
            continue;

        for (i = 0; directives[i].name != NULL; i++)
        {
            if (strcmp(name, directives[i].name) == 0)
            {
                DEBUG_PRINT("name: %s, value: %s, param: %d",
                    directives[i].name, value, directives[i].param);
                switch (directives[i].type)
                {
                    case PAR_STR:
                        strncpy(directives[i].dst, value, (size_t) directives[i].param - 1);
                        break;

                    case PAR_INT8:
                        *((int8_t *) directives[i].dst) = atoi(value);
                        break;

                    case PAR_INT16:
                        *((int16_t *) directives[i].dst) = atoi(value);
                        break;

                    case PAR_INT32:
                        *((int32_t *) directives[i].dst) = atol(value);
                        break;

                    case PAR_BOOL8:
                        if (strcasecmp(value, "yes") == 0)
                            *((int8_t *) directives[i].dst) |= (int8_t) directives[i].param;
                        else if (strcasecmp(value, "no") == 0)
                            *((int8_t *) directives[i].dst) &= (int8_t) ~directives[i].param;
                        else
                        {
                            printfOutput(VERBOSE_BASE,
                                "Directive %s has been ignored due to wrong value %s. "
                                "Should be yes or no.\n", name, value);
                        }
                        break;

                    case PAR_BOOL16:
                        if (strcasecmp(value, "yes") == 0)
                            *((int16_t *) directives[i].dst) |= (int16_t) directives[i].param;
                        else if (strcasecmp(value, "no") == 0)
                            *((int16_t *) directives[i].dst) &= (int16_t) ~directives[i].param;
                        else
                        {
                            printfOutput(VERBOSE_BASE,
                                "Directive %s has been ignored due to wrong value %s. "
                                "Should be yes or no.\n", name, value);
                        }
                        break;

                    case PAR_STATUS:
                        if (!convertStatus(FROM_STR, (const uint8_t **)&value, &(YSM_USER.status)))
                        {
                            printfOutput(VERBOSE_BASE,
                                "Invalid status value %s.\n", value);
                        }
                        break;

                    case PAR_OUTPUT:
                        if ((tmp = strchr(value, ':')) != NULL)
                        {
                            *tmp++ = '\0';
                            if (strlen(tmp) != 0)
                            {
                                strncpy(g_cfg.outputPath, tmp, sizeof(g_cfg.outputPath) - 1);
                                if (strcasecmp(value, "stdin") == 0)
                                {
                                    g_cfg.outputType = OT_STDIN;
                                    break;
                                }
                                else if (strcasecmp(value, "fifo") == 0)
                                {
                                    g_cfg.outputType = OT_FIFO;
                                    break;
                                }
                            }
                        }
                        else if (strcasecmp(value, "stdout") == 0)
                        {
                            g_cfg.outputType = OT_STDOUT;
                            break;
                        }
                        printf("Invalid value for output directive in config file.\n");
                        exit(1);
                        break;

                    default:
                        YSM_ERROR(ERROR_CRITICAL, 0);
                }
                break;
            }
        }

        if (directives[i].name == NULL)
            printfOutput(VERBOSE_BASE,
                "Unknown config directive %s has been ingored.\n", name);
    }

    /* Before leaving check there's at least the minimum required fields */

    if (!YSM_USER.uin)
    {
        printfOutput(VERBOSE_BASE,
            "Missing UIN in config. Can't continue.\n");
        exit(0);
    }
    else if (YSM_USER.network.authHost[0] == '\0')
    {
        printfOutput(VERBOSE_BASE,
            "Missing ICQ Server in config. Can't continue.\n");
        exit(0);
    }
    else if (!YSM_USER.network.authPort)
    {
        printfOutput(VERBOSE_BASE,
            "Missing ICQ Server port in config. Can't continue.\n");
        exit(0);
    }
}

void readSlaves(FILE *fd)
{
    int8_t   buf[MAX_PATH], *aux = NULL;
    int8_t  *auxnick = NULL, *auxuin = NULL, *auxkey = NULL;
    int8_t  *auxcol = NULL;
    int8_t  *next = NULL;
    uint8_t  field = 0;

    DEBUG_PRINT("");

    if (fd == NULL)
    {
        DEBUG_PRINT("invalid file descriptor");
        return;
    }

    /* parse slaves */
    while (!feof(fd))
    {
        memset(buf, '\0', sizeof(buf));
        fgets(buf, sizeof(buf)-1, fd);

        YSM_trim(buf);
        if (buf[0] == '#')
        {
            continue; /* ignore comments */
        }

        aux = buf;
        next = strchr(aux, ':');
        if (next) *next++ = '\0';

        auxnick = auxuin = auxkey = auxcol = NULL;

        for (field = 0; aux != NULL; field++)
        {
            YSM_trim(aux);

            switch (field)
            {
                case 0: /* Nick */
                    if (*aux != '\0') auxnick = aux;
                    break;

                case 1: /* UIN */
                    if (*aux != '\0') auxuin = aux;
                    break;

                case 2: /* Key */
                    if (*aux != '\0') auxkey = aux;
                    break;

                default: /* Trailing garbage */
                    break;
            }

            aux = next;
            if (aux) next = strchr(aux, ':');
            if (next) *next++ = '\0';
        }

        DEBUG_PRINT("nick: %s, uin: %s", auxnick, auxuin);

        if (auxnick == NULL    /* No name */
        || auxuin == NULL)     /* No UIN */
            continue;

        addSlaveToList(auxnick, atol(auxuin), 0, auxkey, 0, 0, 0);
    }

    DEBUG_PRINT("read %d slaves", getSlavesListLen());
    printfOutput(VERBOSE_MOATA, "%s%d]\n", MSG_READ_SLAVES, getSlavesListLen());
}

void addSlave(char *name, uin_t uin)
{
    slave_t *result = NULL;

    result = addSlaveToList(name, uin, 0, NULL, 0, 0, 0);

    if (result == NULL)
    {
        printfOutput(VERBOSE_BASE,
            "NO! Illegal Slave Cloning detected..perv!\n"
            "SLAVE ALREADY exists in your list!.\n");
        return;
    }

    printfOutput(VERBOSE_BASE,
        "Adding a SLAVE with #%d. Call him %s from now on.\n",
        result->uin,
        result->info.nickName);

    addSlaveToDisk(result);
}

void addSlaveToDisk(slave_t *victim)
{
    FILE     *YSM_tmp = NULL, *fd = NULL;
    int8_t    YSMBuff[MAX_PATH];
    uint32_t x = 0;

    DEBUG_PRINT("%s", victim->info.nickName);

    fd = fopen(g_state.slavesFile, "r");

    if (fd == NULL) {
        /* ERR_FOPEN */
        return;
    }

    YSM_tmp = tmpfile();
    if (YSM_tmp == NULL) /* ERR_FILE */
        return;

    /* Fill Name and UIN */
    fprintf(YSM_tmp, "%s:%d:", victim->info.nickName, (int)victim->uin);

    /* Fill Key */
    if (!isKeyEmpty(victim->crypto.strkey))
    {
        for(x = 0; x < strlen(victim->crypto.strkey); x++)
            fprintf(YSM_tmp, "%c", victim->crypto.strkey[x]);
    }

    fprintf(YSM_tmp, ":" );

    fprintf(YSM_tmp, "\n");

    while (!feof(fd))
    {
        memset(YSMBuff,'\0',MAX_PATH);
        fgets(YSMBuff, sizeof(YSMBuff)-1, fd);
        fprintf(YSM_tmp, "%s", YSMBuff);
    }

    fclose(fd);

    rewind(YSM_tmp);

    fd = fopen(g_state.slavesFile, "w");
    if (fd == NULL) {
        /* ERR_FILE */
        return;
    }

    while (!feof(YSM_tmp))
    {
        memset(YSMBuff, '\0', MAX_PATH);
        fgets(YSMBuff, sizeof(YSMBuff), YSM_tmp);
        fprintf(fd, "%s", YSMBuff);
    }

    fclose(fd);
    fclose(YSM_tmp);
}

void deleteSlaveFromDisk(uin_t uin)
{
    FILE   *YSM_tmp = NULL, *fd = NULL;
    int8_t  YSMBuff[MAX_PATH], *auxNick = NULL, *auxUin = NULL, *rest = NULL;

    DEBUG_PRINT("%ld", uin);

    fd = fopen(g_state.slavesFile, "r");

    if (fd == NULL) {
        /* ERR_FILE */
        return;
    }

    YSM_tmp = tmpfile();
    if (YSM_tmp == NULL) /* ERR_FILE */
        return;

    while (!feof(fd))
    {
        auxNick = NULL;
        auxUin = NULL;
        memset(YSMBuff, '\0', MAX_PATH);
        fgets(YSMBuff, sizeof(YSMBuff)-1, fd);

        switch (YSMBuff[0])
        {
            case '#':           /* comments */
            case '\0':          /* empty line */
            case '\n':          /* empty line */
                fprintf(YSM_tmp, "%s", YSMBuff);
                break;

            default:
                auxNick = strtok(YSMBuff, ":");
                if (auxNick)
                    auxUin = strtok(NULL, ":");
                if (auxUin)
                    rest = strtok(NULL, "");

                if (auxUin[strlen(auxUin)-1] == '\n')
                    auxUin[strlen(auxUin)-1] = '\0';

                if (auxNick && auxUin && atol(auxUin) != uin)
                {
                    fprintf(YSM_tmp, "%s:%s:", auxNick, auxUin);

                    if (rest != NULL)
                        fprintf(YSM_tmp, "%s", rest);
                    else
                        fprintf(YSM_tmp, "\n");
                }
        }
    }

    fclose(fd);

    fd = fopen(g_state.slavesFile, "w");
    if (fd == NULL) {
        /* ERR_FILE */
        return;
    }

    rewind(YSM_tmp);

    while (!feof(YSM_tmp))
    {
        memset(YSMBuff, '\0', MAX_PATH);
        fgets(YSMBuff, sizeof(YSMBuff)-1, YSM_tmp);
        fprintf(fd, "%s", YSMBuff);
    }

    fclose(fd);
    fclose(YSM_tmp);
}
