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

int8_t YSM_cfgfile[MAX_PATH];
int8_t YSM_cfgdir[MAX_PATH];
static int8_t YSM_DefaultAFKMessage[MAX_DATA_LEN+1];
int8_t YSM_DefaultCHATMessage[MAX_DATA_LEN+1];

extern short  YSM_AFKCount;
extern time_t YSM_AFK_Time;
extern struct YSM_MODEL YSM_USER;

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
    if (YSM_cfgfile[0] == '\0' || YSM_cfgdir[0] == '\0')
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

    PRINTF(VERBOSE_MOATA, "Retrieving dlave data from config.");
    init_slaves();

    return 0;
}

void init_config(void)
{
    FILE *fd = NULL;

    if ((fd = ysm_fopen(YSM_cfgfile, "r")) != NULL)
    {
        YSM_ReadConfig(fd, 0);
        ysm_fclose(fd);
    }
    else
    {
        PRINTF(VERBOSE_BASE, "Couldn't open config file.\n");
        exit(1);
    }
}

void YSM_SetupHomeDirectory(void)
{
    int8_t    *homep = NULL, *homep2 = NULL;
    int32_t    homepsize = 0, homep2size = 0;

    homepsize = MAX_PATH;
    homep = ysm_calloc(1, homepsize, __FILE__, __LINE__);

    /* no need to finish it by hand, called Calloc() */
    strncpy(homep, getenv("HOME"), homepsize - 1);

    /* homep + slash + zero */
    homep2size = strlen(homep) + 2;
    homep2 = ysm_calloc(1, homep2size, __FILE__, __LINE__);

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

    YSM_FREE(homep);
    YSM_FREE(homep2);
}

void init_slaves(void)
{
    FILE *fd;

    if ((fd = ysm_fopen(YSM_cfgfile, "r")) != NULL)
    {
        YSM_ReadSlaves(fd);
        ysm_fclose(fd);
    }
    else
    {
        PRINTF( VERBOSE_BASE,
            "Contact list couldn't be read!. "
            "File not found.\n" );
            YSM_ERROR(ERROR_CRITICAL, 0);
    }
}

void YSM_ReadConfig(FILE *fd, char reload)
{
    int8_t YSM_CFGEND = FALSE, buf[MAX_PATH], *auxb = NULL, *aux = NULL;

    strncpy( YSM_DefaultAFKMessage,
        YSM_AFK_MESSAGE,
        sizeof(YSM_DefaultAFKMessage) - 1);

    strncpy( YSM_DefaultCHATMessage,
        YSM_CHAT_MESSAGE,
        sizeof(YSM_DefaultCHATMessage) - 1);

    YSM_USER.status_flags |= STATUS_FLDC_CONT;

#if defined (YSM_USE_CHARCONV)
    memset(g_cfg.charset_trans,0,MAX_CHARSET+4);
    memset(g_cfg.charset_local,0,MAX_CHARSET+4);
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
                g_cfg.antisocial =
                        atoi(strtok(NULL," \n\t"));

            else if(!strcasecmp(aux,"UPDATENICKS"))
                g_cfg.updatenicks =
                        atoi(strtok(NULL," \n\t"));

            else if (!strcasecmp(aux,"DC_DISABLE")) {
                if ((aux=strtok(NULL," \n\t"))!= NULL)
                    g_cfg.dcdisable = atoi(aux);
            }

            else if (!strcasecmp(aux,"DC_LAN")) {
                if ((aux=strtok(NULL," \n\t"))!= NULL)
                    g_cfg.dclan = atoi(aux);
            }

            else if (!strcasecmp(aux,"DC_PORT1")) {
                if ((aux=strtok(NULL," \n\t"))!= NULL)
                    g_cfg.dcport1 = atoi(aux);
            }

            else if (!strcasecmp(aux,"DC_PORT2")) {
                if ((aux=strtok(NULL," \n\t"))!= NULL)
                    g_cfg.dcport2 = atoi(aux);
            }

            else if(!strcasecmp(aux,"VERBOSE")) {
                if ((aux=strtok(NULL," \n\t"))!= NULL)
                    g_cfg.verbose = atoi(aux);
            }

            else if(!strcasecmp(aux,"LOGALL")) {
                if ((aux=strtok(NULL," \n\t"))!= NULL)
                    g_cfg.logall = atoi(aux);
            }

            else if(!strcasecmp(aux,"NEWLOGSFIRST")) {
                if ((aux=strtok(NULL," \n\t"))!= NULL)
                    g_cfg.newlogsfirst = atoi(aux);
            }

            else if (!strcasecmp(aux, "AWAYTIME")) {
                if ((aux=strtok(NULL," \n\t"))!= NULL)
                    g_cfg.awaytime = atoi(aux);
            }

            else if(!strcasecmp(aux,"AFKMESSAGE")) {
                if ((aux=strtok(NULL,"\n\t"))!= NULL)
                    strncpy( YSM_DefaultAFKMessage, aux,
                    sizeof(YSM_DefaultAFKMessage)-1);
            }

            else if(!strcasecmp(aux,"AFKMAXSHOWN")) {
                if ((aux=strtok(NULL," \n\t"))!= NULL)
                    g_cfg.afkmaxshown = atoi(aux);
            }

            else if(!strcasecmp(aux,"AFKMINIMUMWAIT")) {
                if ((aux=strtok(NULL," \n\t"))!= NULL)
                    g_cfg.afkminimumwait = atoi(aux);
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
                    g_cfg.verbose = atoi(aux);
            }

            else if(!strcasecmp(aux,"VERSION_CHECK")) {
                if ((aux=strtok(NULL, " \n\t")) != NULL)
                    g_cfg.version_check = atoi(aux);
            }

#ifdef YSM_USE_CHARCONV
            else if(!strcasecmp(aux,"CHARSET_TRANS")) {
                if ((aux=strtok(NULL," \n\t"))!= NULL)
                    strncpy( g_cfg.charset_trans, aux,
                    sizeof(g_cfg.charset_trans) - 1);

            } else if(!strcasecmp(aux,"CHARSET_LOCAL")) {
                if ((aux=strtok(NULL," \n\t"))!= NULL)
                    strncpy( g_cfg.charset_local, aux,
                    sizeof(g_cfg.charset_local) - 1);
            }
#endif

            else if (!strcasecmp(aux,"BROWSER")) {
                if ((aux=strtok(NULL,"\n\t"))!= NULL)
                    strncpy( g_cfg.BrowserPath, aux,
                        sizeof(g_cfg.BrowserPath) - 1);
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

            else if(!strcasecmp(aux,SLAVES_TAG))
                YSM_CFGEND=TRUE;

            else if( *aux )
            {
                PRINTF( VERBOSE_BASE,
                    "UNKNOWN cfg directive '%s' , "
                    "ignoring...\n" ,
                    aux );
            }
        }
    }

    /*    Before leaving check there's at least the    */
    /*    minimum required fields */

    if (!YSM_USER.Uin)
    {
        PRINTF(VERBOSE_BASE,
            "\nMissing UIN in config. Can't continue.\n");
        exit(0);
    }
    else if (YSM_USER.network.auth_host[0] == '\0')
    {
        PRINTF(VERBOSE_BASE,
            "\nMissing ICQ Server in config. Can't continue.\n");
        exit(0);
    }
    else if (!YSM_USER.network.auth_port)
    {
        PRINTF(VERBOSE_BASE,
            "\nMissing ICQ Server port in config. Can't continue.\n");
        exit(0);
    }
}

#define YSMOPENCONFIG(rwx)    (fd = ysm_fopen(YSM_cfgfile,rwx))
#define YSMCLOSECONFIG()    ysm_fclose(fd)

void YSM_ReadSlaves(FILE *fd)
{
    int8_t YSM_tmpbuf[MAX_PATH], *aux = NULL;
    int8_t *auxnick = NULL, *auxuin = NULL, *auxkey = NULL, *auxflags = NULL;
    int8_t *auxcol = NULL;
    int8_t *next = NULL;
    int field = 0;

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

                default: /* Trailing garbage */
                    break;
            }

            aux = next;
            if (aux) next = strchr(aux, ':');
            if (next) *next++ = '\0';
        }

        if (auxnick == NULL    /* No name */
        || auxuin == NULL    /* No UIN */
        || strlen(auxuin) < 5)    /* Invalid UIN */
            continue;

                YSM_AddSlaveToList( auxnick,
                                atoi(auxuin),
                                auxflags,
                                auxkey,
                                0,
                                0,
                                0,
                                0 );

        }

        PRINTF( VERBOSE_MOATA,
                "%s%d]\n",
                MSG_READ_SLAVES,
                g_slave_list.length );
}

slave_t * YSM_QuerySlaves(
	unsigned short TYPE,
	unsigned char    *Extra,
	uin_t        Uin,
	unsigned int    reqid )
{
    u_int32_t  x;
    slave_t *node = (slave_t *) g_slave_list.start;

    for (x = 0; x < g_slave_list.length; x++)
    {
        if (!node) break;

        if (TYPE == SLAVE_NAME) {
            if(!strcasecmp(node->info.NickName, Extra))
                return node;
        } else if ( TYPE == SLAVE_UIN ) {
            if (node->uin == Uin)
                return node;
        } else if ( TYPE == SLAVE_REQID ) {
            if (node->ReqID == reqid)
                return node;
        } else {
            YSM_ERROR(ERROR_CODE, 1);
        }

        node = (slave_t *) node->suc;
    }

    return NULL;
}

void YSM_AddSlave(char *Name, uin_t Uin)
{
    slave_t *result = NULL;

    result = YSM_AddSlaveToList( Name,
                Uin,
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
            "\nSLAVE ALREADY exists in your list!.\n");
        return;
    }

    PRINTF( VERBOSE_BASE,
        "Adding a SLAVE with #%d. Call him %s from now on.\n",
        result->uin,
        result->info.NickName );

    YSM_AddSlaveToDisk( result );
}

void YSM_AddSlaveToDisk(slave_t *victim)
{
    FILE     *YSM_tmp = NULL, *fd = NULL;
    int8_t    YSMBuff[MAX_PATH];
    u_int32_t x = 0;

    fd = ysm_fopen(YSM_cfgfile, "r");
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
            fprintf(YSM_tmp,
                "%s:%d:",
                victim->info.NickName,
                (int)victim->uin);

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

        ysm_fclose(fd);

    rewind(YSM_tmp);

        fd = ysm_fopen(YSM_cfgfile,"w");
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

         ysm_fclose(fd);
         ysm_fclose(YSM_tmp);
}


/* the fl flag determinates whether the slave must be deleted from the
    disk & list (TRUE) or it just needs to be deleted from disk. (FALSE) */

void YSM_DelSlave( slave_t *victim, int fl)
{
    FILE  *YSM_tmp = NULL, *fd = NULL;
    int8_t YSMBuff[MAX_PATH], *auxnick = NULL, *theuin = NULL, *therest = NULL;
    int8_t slave_tDELETED = FALSE, fl_slavedeleted = FALSE;

    fd = ysm_fopen(YSM_cfgfile,"r");
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

                ysm_fclose(fd);

                fd = ysm_fopen(YSM_cfgfile,"w");
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

                ysm_fclose(fd);
                ysm_fclose(YSM_tmp);
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
        if (!strcasecmp(validate, table[x].str))
        YSM_USER.status = table[x].val;
        return;
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
    PRINTF(VERBOSE_BASE,"%s\n",MSG_AFK_MODE_ON);
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

    size = strlen(FileName)+strlen(YSM_cfgdir)+2;
    rfilename = ysm_calloc(1, size, __FILE__, __LINE__);
    snprintf(rfilename,size,"%s/%s", YSM_cfgdir, FileName);
    rfilename[size - 1] = 0x00;

    if ((logfile = ysm_fopen(rfilename,"r")) == NULL) {
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
                    ysm_fclose(logfile);
                    logfile = ysm_fopen(rfilename, "w");
                    if (logfile != NULL) {
                        ysm_fclose(logfile);
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
                    ysm_fclose(logfile);

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
            ysm_fclose(logfile);
            return;
        }

        mnum++; tnum++;

        if (snum > 0) snum--;

        memset( buff, '\0', sizeof(buff) );

    }

    g_promptstatus.flags &= ~FL_BUSYDISPLAY;
    PRINTF(VERBOSE_BASE, "\nEnd of Messages\n");

    ysm_fclose( logfile );
    ysm_free(rfilename, __FILE__, __LINE__);
    rfilename = NULL;
}

/* YSM_DisplayLogEntry - returns 0 if a parsing error occurs.
 */

int YSM_DisplayLogEntry(int8_t *buf, int32_t messageNum)
{
char*    part[3+1];    /* 1 more than 3 to distinguish 3 from >3 fields */
ssize_t    fields;

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
