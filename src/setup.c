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
#include "lists.h"
#include "setup.h"
#include "wrappers.h"
#include "toolbox.h"
#include "direct.h"
#include "slaves.h"
#include "network.h"
#include "commands.h"
#include "crypt.h"
#include "charset.h"
#include <stdarg.h>

static int8_t YSM_DefaultAFKMessage[MAX_DATA_LEN+1];
int8_t YSM_DefaultCHATMessage[MAX_DATA_LEN+1];

extern short  YSM_AFKCount;
extern time_t YSM_AFK_Time;

void init_default_config(ysm_config_t *cfg)
{
    cfg->verbose = 0x5;
    cfg->logall = FALSE;
    cfg->newlogsfirst = TRUE;
    cfg->spoof = FALSE;
    cfg->awaytime = 5;
    cfg->afkmaxshown = 3;
    cfg->afkminimumwait = MINIMUM_AFK_WAIT;
    cfg->antisocial = FALSE;
    cfg->updatenicks = TRUE;
    cfg->dcdisable = FALSE;
    cfg->dclan = FALSE;

    cfg->dcport1 = 0;
    cfg->dcport2 = 0;

    /* needs to store a 4 bytes UIN */
    cfg->forward = 0;

    memset(&cfg->AFKMessage,   0, sizeof(cfg->AFKMessage));
    memset(&cfg->CHATMessage,  0, sizeof(cfg->CHATMessage));
    memset(&cfg->BrowserPath,  0, sizeof(cfg->BrowserPath));

#if defined (YSM_USE_CHARCONV)
    memset(g_cfg.charset_trans, 0, MAX_CHARSET+4);
    memset(g_cfg.charset_local, 0, MAX_CHARSET+4);
#endif
}

int initialize(void)
{
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, CtrlHandler);

    memset(&YSM_USER, 0, sizeof(YSM_USER));

    /* Commands initialization */
    init_commands();

    /* Network initialization */
    init_network();

    /* Default configuration initialization*/
    init_default_config(&g_cfg);

    /* Setup the config and dir path only if -c was not used before */
    if (g_state.config_file[0] == '\0' || g_state.config_dir[0] == '\0')
    {
        PRINTF(VERBOSE_MOATA, "Setting up Config. file Path.");
        YSM_SetupHomeDirectory();
    }

    PRINTF(VERBOSE_MOATA, "Reading config file.");
    init_config();

#ifdef YSM_USE_CHARCONV
    /* Initialize CodePage/Charsets */
    init_charset();
#endif

    PRINTF(VERBOSE_MOATA, "Retrieving slave data.");
    init_slaves();

    return 0;
}

void init_config(void)
{
    FILE *fd = NULL;

    if ((fd = fopen(g_state.config_file, "r")) != NULL)
    {
        strncpy(YSM_USER.network.auth_host,
                YSM_DEFAULTSRV,
                sizeof(YSM_USER.network.auth_host)-1);
        YSM_USER.network.auth_host[sizeof(YSM_USER.network.auth_host)-1] = '\0';
        YSM_USER.network.auth_port = YSM_DEFAULTPORT;

        read_config(fd, 0);
        fclose(fd);
    }
    else
    {
        PRINTF(VERBOSE_BASE, "Couldn't open config file.\n");
        exit(1);
    }
}

void YSM_SetupHomeDirectory(void)
{
    int8_t *home;

    home = getenv("HOME");

    if (home == NULL)
    {
        PRINTF(VERBOSE_BASE,
            "Couldn't determine the home directory.");
        exit(1);
    }

    snprintf(g_state.config_dir, sizeof(g_state.config_dir),
        "%s/%s", home, YSM_CFGDIRECTORY);

    snprintf(g_state.config_file, sizeof(g_state.config_file),
        "%s/%s", g_state.config_dir, YSM_CFGFILENAME);

    snprintf(g_state.slaves_file, sizeof(g_state.slaves_file),
        "%s/%s", g_state.config_dir, YSM_SLAVESFILENAME);
}

void init_slaves(void)
{
    FILE *fd;

    if ((fd = fopen(g_state.slaves_file, "a")) != NULL)
    {
        YSM_ReadSlaves(fd);
        fclose(fd);
    }
    else
    {
        PRINTF(VERBOSE_BASE,
            "Contact list couldn't be read! File not found.\n" );
        YSM_ERROR(ERROR_CRITICAL, 0);
    }
}

void read_config(FILE *fd, char reload)
{
    int8_t buf[MAX_PATH];
    int8_t *name;
    int8_t *value;
    int8_t *tmp;
    int8_t i;
    static struct {
        int8_t  *name;
        void    *dst;
        enum    {
            PAR_STATUS, PAR_INT8, PAR_INT16, PAR_INT32,
            PAR_STR, PAR_BOOL8, PAR_BOOL16, PAR_END
        } type;
        int8_t  param;
    } directives[] = {
        { "status",         &YSM_USER.status,            PAR_STATUS, 0 },
        { "uin",            &YSM_USER.Uin,               PAR_INT32,  0 },
        { "server",         &YSM_USER.network.auth_host, PAR_STR,    sizeof(YSM_USER.network.auth_host)-1 },
        { "serverport",     &YSM_USER.network.auth_port, PAR_STR,    sizeof(YSM_USER.network.auth_port)-1 },
        { "password",       &YSM_USER.password,          PAR_STR,    sizeof(YSM_USER.password) },
        { "awaytime",       &g_cfg.awaytime,             PAR_INT8,   0 },
        { "afkmaxshown",    &g_cfg.afkmaxshown,          PAR_INT8,   0 },
        { "afkminimumwait", &g_cfg.afkminimumwait,       PAR_INT8,   0 },
        { "afkmessage",     &YSM_DefaultAFKMessage,      PAR_STR,    sizeof(YSM_DefaultAFKMessage)-1 },
        { "chatmessage",    &YSM_DefaultCHATMessage,     PAR_STR,    sizeof(YSM_DefaultCHATMessage)-1 },
        { "proxy",          &YSM_USER.proxy.proxy_host,  PAR_STR,    sizeof(YSM_USER.proxy.proxy_host)-1 },
        { "proxyport",      &YSM_USER.proxy.proxy_port,  PAR_INT16,  0 },
        { "proxyhttps",     &YSM_USER.proxy.proxy_flags, PAR_BOOL8,  YSM_PROXY_HTTPS },
        { "proxyresolve",   &YSM_USER.proxy.proxy_flags, PAR_BOOL8,  YSM_PROXY_RESOLVE },
        { "proxyauth",      &YSM_USER.proxy.proxy_flags, PAR_BOOL8,  YSM_PROXY_AUTH },
        { "proxyusername",  &YSM_USER.proxy.username,    PAR_STR,    sizeof(YSM_USER.proxy.username)-1 },
        { "proxypassword",  &YSM_USER.proxy.password,    PAR_STR,    sizeof(YSM_USER.proxy.password)-1 },
        { "logall",         &g_cfg.logall,               PAR_BOOL8,  1 },
        { "newlogfirst",    &g_cfg.newlogsfirst,         PAR_BOOL8,  1 },
        { "antisocial",     &g_cfg.antisocial,           PAR_BOOL8,  1 },
        { "updatenicks",    &g_cfg.updatenicks,          PAR_BOOL8,  1 },
        { "dcdisable",      &g_cfg.dcdisable,            PAR_BOOL8,  1 },
        { "dclan",          &g_cfg.dclan,                PAR_BOOL8,  1 },
        { "dcport1",        &g_cfg.dcport1,              PAR_INT16,  0 },
        { "dcport2",        &g_cfg.dcport2,              PAR_INT16,  0 },
        { "webaware",       &YSM_USER.status_flags,      PAR_BOOL16, STATUS_FLWEBAWARE },
        { "mybirthday",     &YSM_USER.status_flags,      PAR_BOOL16, STATUS_FLBIRTHDAY },
        { "browser",        &g_cfg.BrowserPath,          PAR_STR,    sizeof(g_cfg.BrowserPath)-1 },
        { "verbose",        &g_cfg.verbose,              PAR_INT8,   0 },
#ifdef YSM_USE_CHARCONV
        { "charsettrans",   &g_cfg.charset_trans,        PAR_STR,    sizeof(g_cfg.charset_trans)-1 },
        { "charsetlocal",   &g_cfg.charset_local,        PAR_STR,    sizeof(g_cfg.charset_local)-1 },
#endif
        { NULL,             NULL,                        PAR_END,    0 }
    };

    strncpy(YSM_DefaultAFKMessage,
        YSM_AFK_MESSAGE,
        sizeof(YSM_DefaultAFKMessage) - 1);

    strncpy(YSM_DefaultCHATMessage,
        YSM_CHAT_MESSAGE,
        sizeof(YSM_DefaultCHATMessage) - 1);

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

        /* if line is not empty */
        if (name[0] != '\0')
        {
            for (i = 0; directives[i].name != NULL; i++)
            {
                if (strcmp(name, directives[i].name) == 0)
                {
                    switch (directives[i].type)
                    {
                        case PAR_STR:
                            strncpy(directives[i].dst, value, (size_t) directives[i].param);
                            break;

                        case PAR_INT8:
                            *((int8_t *) directives[i].dst) = atoi(value);
                            break;

                        case PAR_INT16:
                            *((int16_t *) directives[i].dst) = atoi(value);
                            break;

                        case PAR_INT32:
                            *((int32_t *) directives[i].dst) = atoi(value);
                            break;

                        case PAR_BOOL8:
                            if (strcasecmp(value, "yes") == 0)
                                *((int8_t *) directives[i].dst) |= directives[i].param;
                            else if (strcasecmp(value, "no") == 0)
                                *((int8_t *) directives[i].dst) &= ~directives[i].param;
                            else
                            {
                                PRINTF(VERBOSE_BASE,
                                    "Directive %s has been ignored due to wrong value %s. "
                                    "Should be yes or no.\n", name, value);
                            }
                            break;

                        case PAR_BOOL16:
                            if (strcasecmp(value, "yes") == 0)
                                *((int16_t *) directives[i].dst) |= directives[i].param;
                            else if (strcasecmp(value, "no") == 0)
                                *((int16_t *) directives[i].dst) &= ~directives[i].param;
                            else
                            {
                                PRINTF(VERBOSE_BASE,
                                    "Directive %s has been ignored due to wrong value %s. "
                                    "Should be yes or no.\n", name, value);
                            }
                            break;

                        case PAR_STATUS:
                            YSM_CFGStatus(value);
                            break;

                        default:
                            YSM_ERROR(ERROR_CRITICAL, 0);
                    }
                    break;
                }
            }
            if (directives[i].name == NULL)
                PRINTF(VERBOSE_BASE,
                    "Unknown config directive %s has been ingored.\n", name);

        }
/*
            else if (!strcasecmp(aux,"EXEC_INCOMING")) {
                if ((aux=strtok(NULL," \n\t")) != NULL)
                    strncpy(g_events.execincoming, aux,
                    sizeof(g_events.execincoming) - 1);
            }

            else if (!strcasecmp(aux,"EXEC_OUTGOING")) {
                if ((aux=strtok(NULL," \n\t")) != NULL)
                    strncpy(g_events.execoutgoing, aux,
                    sizeof(g_events.execoutgoing) - 1);
            }

            else if (!strcasecmp(aux,"EXEC_ONCOMING")) {
                if ((aux=strtok(NULL," \n\t")) != NULL)
                    strncpy(g_events.execoncoming, aux,
                    sizeof(g_events.execoncoming) - 1);
            }

            else if (!strcasecmp(aux,"EXEC_OFFGOING")) {
                if ((aux=strtok(NULL," \n\t")) != NULL)
                    strncpy(g_events.execoffgoing, aux,
                    sizeof(g_events.execoffgoing) - 1);
            }

            else if (!strcasecmp(aux,"EXEC_LOGOFF")) {
                if ((aux=strtok(NULL," \n\t")) != NULL)
                    strncpy(g_events.execlogoff, aux,
                    sizeof(g_events.execlogoff) - 1);
            }
*/
    }

    /* Before leaving check there's at least the    */
    /* minimum required fields */

    if (!YSM_USER.Uin)
    {
        PRINTF(VERBOSE_BASE,
            "Missing UIN in config. Can't continue.\n");
        exit(0);
    }
    else if (YSM_USER.network.auth_host[0] == '\0')
    {
        PRINTF(VERBOSE_BASE,
            "Missing ICQ Server in config. Can't continue.\n");
        exit(0);
    }
    else if (!YSM_USER.network.auth_port)
    {
        PRINTF(VERBOSE_BASE,
            "Missing ICQ Server port in config. Can't continue.\n");
        exit(0);
    }
}

void YSM_ReadSlaves(FILE *fd)
{
    int8_t YSM_tmpbuf[MAX_PATH], *aux = NULL;
    int8_t *auxnick = NULL, *auxuin = NULL, *auxkey = NULL, *auxflags = NULL;
    int8_t *auxcol = NULL;
    int8_t *next = NULL;
    int field = 0;

    if (fd == NULL)
        return;

    /* parse slaves */
    while (memset(YSM_tmpbuf, '\0', sizeof(YSM_tmpbuf))
    && fgets(YSM_tmpbuf, sizeof(YSM_tmpbuf)-1, fd) != NULL)
    {
        YSM_trim(YSM_tmpbuf);
        if (YSM_tmpbuf[0]=='#' ) continue; /* ignore comments */

        aux = YSM_tmpbuf;
        next = strchr(aux, ':');
        if (next) *next++ = '\0';

        auxnick = auxuin = auxkey = auxflags = auxcol = NULL;
        field = 0;

        while (aux != NULL)
        {
            YSM_trim(aux);

            switch (field++)
            {
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

                default: /* Trailing garbage */
                    break;
            }

            aux = next;
            if (aux) next = strchr(aux, ':');
            if (next) *next++ = '\0';
        }

        if (auxnick == NULL    /* No name */
        || auxuin == NULL      /* No UIN */
        || strlen(auxuin) < 5) /* Invalid UIN */
            continue;

        YSM_AddSlaveToList(auxnick, atoi(auxuin), auxflags, auxkey, 0, 0, 0, 0);
    }

    PRINTF(VERBOSE_MOATA, "%s%d]\n", MSG_READ_SLAVES, g_slave_list.length);
}

slave_t * YSM_QuerySlaves(
	unsigned short  type,
	unsigned char  *extra,
	uin_t           uin,
	unsigned int    reqid)
{
    slave_t *node = NULL;

    for (node = (slave_t *) g_slave_list.start;
         node != NULL;
         node = (slave_t *) node->suc)
    {
        switch (type)
        {
            case SLAVE_NAME:
                if (!strcasecmp(node->info.NickName, extra)) return node;
                break;

            case SLAVE_UIN:
                if (node->uin == uin) return node;
                break;

            case SLAVE_REQID:
                if (node->ReqID == reqid) return node;
                break;

            default:
                YSM_ERROR(ERROR_CODE, 1);
        }
    }

    return NULL;
}

void YSM_AddSlave(char *name, uin_t uin)
{
    slave_t *result = NULL;

    result = YSM_AddSlaveToList(name, uin, NULL, NULL, 0, 0, 0, 0);

    if (result == NULL)
    {
        PRINTF(VERBOSE_BASE,
            "NO! Illegal Slave Cloning detected..perv!\n"
            "SLAVE ALREADY exists in your list!.\n");
        return;
    }

    PRINTF(VERBOSE_BASE,
        "Adding a SLAVE with #%d. Call him %s from now on.\n",
        result->uin,
        result->info.NickName );

    YSM_AddSlaveToDisk(result);
}

void YSM_AddSlaveToDisk(slave_t *victim)
{
    FILE     *YSM_tmp = NULL, *fd = NULL;
    int8_t    YSMBuff[MAX_PATH];
    u_int32_t x = 0;

    fd = fopen(g_state.slaves_file, "r");

    if (fd == NULL) {
        /* ERR_FOPEN */
        return;
    }

    YSM_tmp = tmpfile();
    if (YSM_tmp == NULL) /* ERR_FILE */
        return;

    fprintf(YSM_tmp,"%s", YSMBuff);

    /* Fill Name and UIN */
    fprintf(YSM_tmp, "%s:%d:", victim->info.NickName, (int)victim->uin);

    /* Fill Key */
    if (!YSM_KeyEmpty(victim->crypto.strkey))
    {
        for(x = 0; x < strlen(victim->crypto.strkey); x++)
            fprintf(YSM_tmp, "%c", victim->crypto.strkey[x]);
    }

    fprintf(YSM_tmp, ":" );

    /* Fill Flags */
    if (victim->flags & FL_ALERT)
        fprintf(YSM_tmp, "a");

    if (victim->flags & FL_LOG)
        fprintf(YSM_tmp, "l");

    fprintf(YSM_tmp, "\n");

    while (!feof(fd))
    {
        memset(YSMBuff,'\0',MAX_PATH);
        fgets(YSMBuff,sizeof(YSMBuff)-1,fd);
        fprintf(YSM_tmp, "%s", YSMBuff);
    }

    fclose(fd);

    rewind(YSM_tmp);

    fd = fopen(g_state.slaves_file, "w");
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


/* the fl flag determinates whether the slave must be deleted from the
    disk & list (TRUE) or it just needs to be deleted from disk. (FALSE) */

void YSM_DelSlave( slave_t *victim, int fl)
{
    FILE  *YSM_tmp = NULL, *fd = NULL;
    int8_t YSMBuff[MAX_PATH], *auxnick = NULL, *theuin = NULL, *therest = NULL;
    int8_t slave_tDELETED = FALSE, fl_slavedeleted = FALSE;

    fd = fopen(g_state.slaves_file,"r");
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

                while (!slave_tDELETED) {
                            if (fgets( YSMBuff,
                        sizeof(YSMBuff)-1,
                        fd) == NULL) {
                                    slave_tDELETED = TRUE;
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
                            YSM_DeleteSlaveFromList
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

                fclose(fd);

                fd = fopen(g_state.slaves_file,"w");
        if (fd == NULL) {
            /* ERR_FILE */
            return;
        }

        rewind(YSM_tmp);
        memset(YSMBuff, '\0', MAX_PATH);

        while (!feof(YSM_tmp))
        {
            memset(YSMBuff, '\0', MAX_PATH);
            fgets(YSMBuff, sizeof(YSMBuff)-1, YSM_tmp);
            fprintf(fd, "%s", YSMBuff);
        }

        fclose(fd);
        fclose(YSM_tmp);
}


void YSM_CFGStatus(char *validate)
{
    static struct
    {
        const char* str;
        const u_int16_t val; 
    } table[] = {
        {"ONLINE",    STATUS_ONLINE},
        {"OFFLINE",   STATUS_OFFLINE},
        {"AWAY",      STATUS_AWAY},
        {"NA",        STATUS_NA},
        {"DND",       STATUS_DND},
        {"OCCUPIED",  STATUS_OCCUPIED},
        {"FREECHAT",  STATUS_FREE_CHAT},
        {"INVISIBLE", STATUS_INVISIBLE},
        {NULL,        0}
    };
    u_int32_t x;

    for (x = 0; validate[x] != '\0'; x++)
        validate[x] = toupper(validate[x]);

    for (x = 0; table[x].str != NULL; x++)
    {
        if (!strcmp(validate, table[x].str))
        {
            YSM_USER.status = table[x].val;
            return;
        }
    }

    /* default value */
    YSM_USER.status = STATUS_ONLINE;
}

void YSM_AFKMode(u_int8_t turnflag)
{
    if (!turnflag)
    {
#ifndef COMPACT_DISPLAY
        PRINTF(VERBOSE_BASE,
            "%s %d %s",
            MSG_AFK_MODE_OFF1,
            ((time(NULL)-YSM_AFK_Time)/60),
            MSG_AFK_MODE_OFF2);

        PRINTF(VERBOSE_BASE,
            "%d %s %s %s",
            YSM_AFKCount,
            MSG_AFK_MODE_OFF3,
            YSM_AFKFILENAME,
            MSG_AFK_MODE_OFF4);
#endif

        g_promptstatus.flags &= ~FL_AFKM;

        /* Only change the status back to online */
        /* if the user was in 'away' status.     */
        if (YSM_USER.status == STATUS_AWAY)
            YSM_ChangeStatus(STATUS_ONLINE);

        return;
    }

#ifndef COMPACT_DISPLAY
    PRINTF(VERBOSE_BASE, "%s\n", MSG_AFK_MODE_ON);
#endif

    time(&YSM_AFK_Time);

    YSM_AFKCount = 0;
    g_promptstatus.flags |= FL_AFKM;

    if (!(strlen(g_cfg.AFKMessage)))
    {
        strncpy(g_cfg.AFKMessage,
                YSM_DefaultAFKMessage,
                sizeof(g_cfg.AFKMessage) - 1);
        g_cfg.AFKMessage[sizeof(g_cfg.AFKMessage)-1] = '\0';
    }

    /* Only change the status to AWAY if the user is */
    /* in status ONLINE so we don't mess with their status */

    if (YSM_USER.status == STATUS_ONLINE || YSM_USER.status == STATUS_FREE_CHAT)
    {
        YSM_ChangeStatus(STATUS_AWAY);
    }
}

/* logtype:    0 => AFK
 *        1 => Slave
 */

void YSM_ReadLog(char *FileName, int logtype)
{
    FILE    *logfile = NULL;
    int8_t     *rfilename = NULL;
    int32_t    mnum = 1, tnum = 1, snum = 0, size = 0;
    int8_t    q, *rtime = NULL, *aux = NULL, *auxb = NULL;
    /* 64 bytes extra for YSM_LOG_SEPARATORs, date, etc */
    int8_t    buff[ MAX_DATA_LEN+MAX_NICK_LEN+MAX_UIN_LEN+64 ];

    if (FileName == NULL) return;

    size = strlen(FileName)+strlen(g_state.config_dir)+2;
    rfilename = ysm_calloc(1, size, __FILE__, __LINE__);
    snprintf(rfilename,size,"%s/%s", g_state.config_dir, FileName);
    rfilename[size - 1] = 0x00;

    if ((logfile = fopen(rfilename,"r")) == NULL) {
        PRINTF(VERBOSE_BASE,
            "\nNo Messages Found :: filename not found.\n");

        ysm_free(rfilename, __FILE__, __LINE__);
        rfilename = NULL;
        return;
    }

    memset( buff, '\0', sizeof(buff) );

    g_promptstatus.flags |= FL_BUSYDISPLAY;
    while (!feof(logfile)) {

        /* we use mnum > YSM.. since it starts as 1 already */
        if ((fgets(buff, sizeof(buff), logfile)) == NULL ||
                (mnum > g_cfg.afkmaxshown && !snum)) {

            g_promptstatus.flags &= ~FL_BUSYDISPLAY;
            PRINTF(VERBOSE_BASE,
            "\n[N]ext %d  "
                     "[S]kip 10  "
                     "[C]lear all  "
                     "[Q]uit\n"
                        ,
                        g_cfg.afkmaxshown);

            g_promptstatus.flags |= FL_BUSYDISPLAY;
            q = getkey();

            switch(toupper(q))
            {
                case 'C':
                    g_promptstatus.flags &= ~FL_BUSYDISPLAY;
                    PRINTF(VERBOSE_BASE, "\nClearing..\n");
                    fclose(logfile);
                    logfile = fopen(rfilename, "w");
                    if (logfile != NULL) {
                        fclose(logfile);
                    }

                    ysm_free(rfilename, __FILE__, __LINE__);
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
                    fclose(logfile);

                    ysm_free(rfilename, __FILE__, __LINE__);

                    return;
                    break;
            }
        }

        YSM_trim(buff);

        if (*buff != '\0' && !YSM_DisplayLogEntry(buff, tnum))
        {
            g_promptstatus.flags &= ~FL_BUSYDISPLAY;
            PRINTF(VERBOSE_BASE,"\nreadlog :: parsing error.\n");
            ysm_free(rfilename, __FILE__, __LINE__);
            fclose(logfile);
            return;
        }

        mnum++; tnum++;

        if (snum > 0) snum--;

        memset( buff, '\0', sizeof(buff) );
    }

    g_promptstatus.flags &= ~FL_BUSYDISPLAY;
    PRINTF(VERBOSE_BASE, "\nEnd of Messages\n");

    fclose(logfile);
    YSM_FREE(rfilename);
}

/* YSM_DisplayLogEntry - returns 0 if a parsing error occurs.
 */

int YSM_DisplayLogEntry(int8_t *buf, int32_t messageNum)
{
    char*    part[3+1];    /* 1 more than 3 to distinguish 3 from >3 fields */
    ssize_t  fields;

    fields = YSM_tokenize( buf,
            YSM_LOG_SEPARATOR,
            part,
            NUM_ELEM_ARR(part));

    if (fields != 3)
        return 0;

    g_promptstatus.flags &= ~FL_BUSYDISPLAY;

    PRINTF(VERBOSE_BASE,
        "\n" "Msg #%i: - From: %s",
        messageNum,
        part[0]);

    PRINTF(VERBOSE_BASE, "\nTime: %s", part[2]);
    PRINTF(VERBOSE_BASE, "\nData: %s\n", part[1]);

    g_promptstatus.flags |= FL_BUSYDISPLAY;
    return 1;
}

void YSM_ExecuteCommand(int argc, char **argv)
{
    pid_t e2pid;

    /* unix exec code */
    if ((e2pid = fork()) == 0)    /* child proc */
    {
        int i;
        pid_t epid;

        for (i = 0; i < 256; i++)
            close(i);

        epid = fork();
        if (epid == 0)
        {
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
        }
        else if (epid > 0)
        {
            while (waitpid(-1, NULL, WNOHANG) > 0);
        }

        exit(0);
    }
    else if (e2pid > 0)
        waitpid(e2pid, NULL, 0);
}
