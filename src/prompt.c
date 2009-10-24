/*    $Id: YSM_Prompt.c,v 1.202 2006/01/02 00:14:14 rad2k Exp $    */
/*
-======================== ysmICQ client ============================-
        Having fun with a boring Protocol
-========================= YSM_Prompt.c ============================-

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
#include "network.h"
#include "icqv7.h"
#include "charset.h"
#include "direct.h"
#include "prompt.h"
#include "commands.h"
#include "wrappers.h"
#include "toolbox.h"
#include "slaves.h"
#include "setup.h"
#include "crypt.h"
#include "timers.h"

#include <errno.h>
#include <string.h>
#include <stdlib.h>

struct YSM_PROMPTSTATUS g_promptstatus;

int16_t   YSM_AFKCount = 0;
time_t    YSM_AFK_Time = 0;
u_int16_t YSM_TabCount = 1;

extern slave_t    *g_state.last_sent, *g_state.last_read;
extern slave_t    *g_state.last_sent, *g_state.last_read;
extern slave_t    *g_state.last_sent, *g_state.last_read;
extern slave_t    *g_state.last_sent, *g_state.last_read;
extern int8_t      YSM_LastMessage[MAX_DATA_LEN + 1];
extern int8_t      YSM_LastURL[MAX_DATA_LEN + 1];
extern char        YSM_cfgdir[MAX_PATH];

void YSM_Command_HELP(int argc, char **argv);
void YSM_Command_FILESTATUS(int argc, char **argv);
void YSM_Command_CHAT(int argc, char **argv);

/* only ansi stringz messages for this function.
 * encryption, if neccesary, is done inside.
 * if verbous, print any messages.
 */

void YSM_SendMessage( uin_t         r_uin,
        int8_t        *data,
        int8_t        logflag,
        slave_t    *slave,
        u_int8_t    verbous )
{
    int32_t        data_len = 0;
    int8_t         time_string[10], status_string[MAX_STATUS_LEN];
    int8_t        *oldmark1 = NULL, *oldmark2 = NULL, *r_nick = NULL;
    u_int8_t       flags = 0;
    time_t         log_time;
    struct tm     *time_stamp;
    keyInstance   *crypt_key = NULL;

    /* we save these marks to check if we need to free data
     * at the end of this function. */
    oldmark1 = data;

    data_len = strlen(data)+1;
    YSM_PreOutgoing( slave, r_uin, &data, &data_len, logflag );
    oldmark2 = data;

    /* encrypt the message if neccesary */
    crypt_key = YSM_EncryptAnyData( slave,
                    &data,
                    &data_len,
                    MAX_MSGDATA_LEN );

    if (slave != NULL) {
        YSM_WriteStatus(slave->status, status_string);

        if (slave->caps & CAPFL_UTF8) {
            flags |= MFLAGTYPE_UTF8;
        }
    }

    flags |= MFLAGTYPE_NORM;

    if (slave != NULL && slave->caps & CAPFL_SRVRELAY)
        YSM_SendMessage2Client( slave,
                slave->uin,
                0x02,
                YSM_MESSAGE_,
                data,
                data_len,
                0x00,
                flags,
                rand() & 0xffffff7f );
    else
        YSM_SendMessage2Client( slave,
                r_uin,
                0x01,
                YSM_MESSAGE_,
                data,
                data_len,
                0x00,
                flags,
                rand() & 0xffffff7f );

    time(&log_time);
    time_stamp = localtime( &log_time );
    strftime( time_string, 9, "%H:%M:%S", time_stamp );
    time(&log_time);

#ifndef COMPACT_DISPLAY
    if (slave != NULL) {
        if (crypt_key != NULL) {
            if (verbous) {
                PRINTF( VERBOSE_BASE,
                    "%s %s %s"" [%s]. (%d).\n",
                    time_string,
                    MSG_MESSAGE_SENT3,
                    slave->info.NickName,
                    status_string,
                    slave->uin );
            }
        } else {
            if (verbous) {
                PRINTF( VERBOSE_BASE,
                    "%s %s %s"" [%s]. (%d).\n",
                    time_string,
                    MSG_MESSAGE_SENT1,
                    slave->info.NickName,
                    status_string,
                    slave->uin );
            }
        }

    } else {
        if (verbous) {
            PRINTF(VERBOSE_BASE,
                "%s %s %d.\n",
                time_string,
                MSG_MESSAGE_SENT2,
                r_uin );
        }
    }
#endif

    /* what do we need to free? */
    if (data != oldmark1) {
        if (data != oldmark2 && oldmark2 != oldmark1) {
            ysm_free( oldmark2, __FILE__, __LINE__ );
            oldmark2 = NULL;
        }

        ysm_free( data, __FILE__, __LINE__ );
        data = NULL;
    }

    if (slave != NULL) r_nick = slave->info.NickName;
    else r_nick = "NOT a SLAVE";

    YSM_PostOutgoing( r_uin, r_nick, strlen(oldmark1), oldmark1 );
}

void YSM_PasswdCheck(void)
{
    int8_t *p = NULL;

    if (YSM_USER.password[0] == '\0')
    {
        PRINTF(VERBOSE_BASE, "%s\n", MSG_FOUND_SECPASS);

        do {
            p = YSM_getpass("Password: ");
        } while(p[0] == '\0');

        snprintf(YSM_USER.password,
            sizeof(YSM_USER.password),
            "%s",
            p );

        YSM_USER.password[sizeof(YSM_USER.password) - 1] = 0x00;
    }
}

void YSM_ConsoleRead(int fd)
{
    int8_t *tmp = NULL;
    int8_t *retline = NULL;
    int8_t *pos = NULL;
    size_t  size = BUFSIZ;
    ssize_t readsize;

    /* update the idle keyboard timestamp */
    reset_timer(IDLE_TIMEOUT);

    retline = YSM_MALLOC(size);
    if (retline == NULL)
        return;
    pos = retline;

    while ((readsize = read(fd, pos, size - (pos - retline))) > 0)
    {
        if (readsize == size)
        {
            tmp = YSM_MALLOC(size*2);
            if (tmp == NULL)
            {
                YSM_FREE(retline);
                return;
            }
            memcpy(tmp, retline, size);
            YSM_FREE(retline);
            retline = tmp;
            pos = retline + size;
            size *= 2;
        }
        else if (readsize == -1)
        {
            return;
        }
        else
        {
            pos[readsize - 1] = '\0'; // trim ending CR
            break;
        }
    }

    /* did we get a command or not? */
    if (retline == NULL)
        return;

    /* parse and process the command */
    YSM_DoCommand(retline);

    /* dont you dare to leak on me! */
    YSM_FREE(retline);
}

void YSM_ParseCommand(int8_t *_input, int32_t *argc, int8_t *argv[])
{
    char *aux = NULL, *input = NULL, *tmp = NULL, *last = NULL;
    int8_t fl = FALSE, stop_filtering_spaces = FALSE;
    u_int32_t y = 0, z = 0;
    int32_t x = 0;

    input = last = tmp = _input;
    *argc = 0;

    while ((aux = (char *)strchr(input, ' ')) != NULL) {
        if(!fl) argv[*argc] = input;
        input = aux;
        *input = '\0';
        input++;

        if (!strlen(tmp) && !(*tmp)) {
            *tmp = 0x20;
            last = argv[*argc];
            fl = TRUE;
        } else {
            fl = FALSE;
            last = input;
            (*argc)++;
        }

        tmp = input;
    }

    argv[*argc] = last;

    /* Search for empty arguments.
     * The following lines discard those argv arguments which
     * only have spaces in them. We consider them to be empty.
     */

    while (x <= *argc) {

        for (y = 0; y < strlen(argv[x]); y++) {
            if (argv[x][y] != 0x20) goto while_loop_goto_sucks;
            else continue;
        }

        /* empty argv[x] found */
        (*argc)--;

while_loop_goto_sucks:
        x++;
    }

    /* Search for spaced arguments. (only searching cmd + first arg)
     * The following lines remove the beginning spaces from an
     * argument. These situations are usually user mistakes, because
     * its always the USER'S FAULT. ;)
     */

    x = 0;
    while (x <= *argc && x <= 1) {
        for (y = 0, z = 0; y < strlen(argv[x]); y++) {
            if (argv[x][y] != 0x20 || stop_filtering_spaces) {
                argv[x][z] = argv[x][y];
                z++;
                stop_filtering_spaces = TRUE;
            }
            else continue;
        }

        /* finish the new argument with a zero */
        argv[x][z] = 0x00;
        x++;
        stop_filtering_spaces = FALSE;
    }
}

void YSM_DoCommand(char *cmd)
{
    int8_t      *argv[MAX_CMD_ARGS];
    int32_t      argc = 0;
    command_t   *node;

    if (!strlen(cmd)) return;

    memset(argv, 0, sizeof(argv));

    YSM_ParseCommand(cmd, &argc, argv);

    for (node = (command_t *) g_command_list.start;
         node != NULL;
         node = (command_t *) node->suc)
    {
        /* speed up with first checks */
        if (node->cmd_name[0] != (int8_t)tolower(argv[0][0])
        || strcasecmp(node->cmd_name, argv[0]))
        {
            if (node->cmd_alias != NULL)
            {
                if (node->cmd_alias[0] != (int8_t)tolower(argv[0][0])
                || strcasecmp(node->cmd_alias, argv[0])) {
                    continue;
                }
            }
            else
            {
                continue;
            }
        }

        if (argc < node->cmd_margs)
        {
            PRINTF(VERBOSE_BASE,
                "Missing parameters. Use the 'help'"
                " command for detailed information.\n"
                );
        }
        else
        {
            /* use the low caps argv[0], just in case */
            /* who knows when batman may come 2 kill us (?!) */
            argv[0] = node->cmd_name;

            if (node->cmd_func != NULL)
                node->cmd_func(argc, argv);

            g_promptstatus.flags |= FL_RAW;
        }

        return;
    }

    PRINTF(VERBOSE_BASE, "%s: command not found\n", argv[0]);
}

void YSM_DoChatCommand(int8_t *cmd)
{
    slave_t *query = (slave_t *) g_slave_list.start;
    u_int32_t  x = 0;

    if (cmd == NULL || cmd[0] == 0x00) return;

    /* what command do we have? */
    if (!strcasecmp(cmd, "ch") || !strcasecmp(cmd, "chat"))
    {
        YSM_Command_CHAT(0, NULL);
        return;
    }

    /* loop through the slaves list and send the message only to
     * those who have the FL_CHAT marked */
    for (x = 0; x < g_slave_list.length && query; x++)
    {
        if (query->flags & FL_CHAT)
        {
            YSM_SendMessage(query->uin,
                cmd,
                (int8_t)(query->flags & FL_LOG),
                query,
                0);
        }

        query = (slave_t *) query->suc;
    }
}

static void YSM_PreIncoming(
    slave_t      *contact,
    int16_t       m_type,
    int32_t      *m_len,
    uin_t         r_uin,
    int8_t      **m_data,
    int8_t       *r_nick,
    u_int8_t      m_flags,
    keyInstance **key)
{
    int8_t *data_conv = NULL, do_conv = 0;

    /* procedures applied to normal messages only */
    if (m_type == YSM_MESSAGE_)
    {
        /* decrypt the incoming message if neccesary.
         * the function will return < 0 if decryption failed.
         * though it might be because we don't have a key with
         * this contact (or we don't have her on our list).
         */

        if (YSM_DecryptMessage(contact, m_data, m_len, key) >= 0)
        {
            /* if decryption went ok, do convertion */
            do_conv = 1;
        }

        /* pre-Incoming Message Event */
        YSM_Event( EVENT_PREINCOMINGMESSAGE,
            r_uin,
            r_nick,
            *m_len,
            *m_data,
            m_flags );

        if (do_conv)
        {
            /* Charset convertion time */
            YSM_Charset( CHARSET_INCOMING,
                *m_data,
                m_len,
                &data_conv,
                m_flags );

            if (data_conv != NULL)
            {
                /* m_data is freed in the callers function */
                *m_data = data_conv;
            }
        }

        /* filter unwanted characters */
        YSM_ParseMessageData(*m_data, *m_len);

        /* set again the data length after the convertion */
        (*m_len) = strlen(*m_data)+1;

        /* store last message */
        if (contact != NULL)
        {
            strncpy(YSM_LastMessage, *m_data, sizeof(YSM_LastMessage) - 1);
            YSM_LastMessage[sizeof(YSM_LastMessage)-1] = '\0';
        }
    }
}

/* We allow up to x amount of extra arguments to avoid creating
 * a va_list here and then converting all the other procedures
 * which arent va_list compatible. This function is mostly used
 * for executing action events.
 */

void YSM_ExecuteLine( char    *line,
        char    *extra1,
        char    *extra2,
        char    *extra3,
        char    *extra4 )
{
char    *exec_args[MAX_EXEC_ARGS], *aux = NULL, *auxb = line;
int    arg_index = 0, x = 0, y = 0;

    for (arg_index = 0; arg_index < MAX_EXEC_ARGS; arg_index++ )
        exec_args[arg_index] = NULL;

    arg_index = 0;

    y = strlen(line);

    for (;x < y && (arg_index+1 < MAX_EXEC_ARGS);) {
        aux = strchr(auxb, ' ');
        if (aux != NULL || ((aux = strchr(auxb, '\0')) != NULL)) {
            y--;    /* 0x20 */
            *aux = '\0';
            exec_args[arg_index] = auxb;
            x += strlen(auxb);
            auxb = aux;
            auxb++;

        } else break;

        arg_index++;
    }

    /* check if any extra arguments were specified */

    if (extra1 != NULL && arg_index+1 < MAX_EXEC_ARGS) {
        exec_args[arg_index] = extra1;
        arg_index++;
    }
    if (extra2 != NULL && arg_index+1 < MAX_EXEC_ARGS) {
        exec_args[arg_index] = extra2;
        arg_index++;
    }
    if (extra3 != NULL && arg_index+1 < MAX_EXEC_ARGS) {
        exec_args[arg_index] = extra3;
        arg_index++;
    }
    if (extra4 != NULL && arg_index+1 < MAX_EXEC_ARGS) {
        exec_args[arg_index] = extra4;
        arg_index++;
    }

    YSM_ExecuteCommand(arg_index, exec_args);
}

static void YSM_PostIncoming( slave_t    *victim,
        short    m_type,
        uin_t    r_uin,
        int    m_len,
        char    *m_data,
        char    *r_nick,
        int    log_flag )
{

    int8_t fl_inchat = 0, fl_vinchat = 0, fl_inafk = 0;

    if (m_type == YSM_MESSAGE_) {

        if (g_promptstatus.flags & FL_CHAT) fl_inchat = 1;
        if (victim != NULL && (victim->flags & FL_CHAT)) fl_vinchat = 1;
        if (g_promptstatus.flags & FL_AFKM) fl_inafk = 1;

        /* only log normal messages */
        if ((log_flag || g_cfg.logall || fl_inafk || fl_inchat) && !fl_vinchat)
            YSM_GenerateLogEntry(r_nick,
                    r_uin,
                    r_uin,
                    m_data,
                    m_len);

            if ((g_promptstatus.flags & FL_AFKM) ||
            (victim != NULL && !(victim->flags & FL_CHAT))) {
                YSM_AFKCount++;
            }

        /* do we have to send messages? either AFK, CHAT or FORWARD? */

        if (g_cfg.forward)
            YSM_ForwardMessage(
                    victim ? victim->uin : r_uin,
                    m_data);

        if (g_promptstatus.flags & FL_AFKM) {
            if (victim) {
                /* using send message here to send encrypted */
                if ((time(NULL)-victim->LastAFK)
                        > g_cfg.afkminimumwait) {

                YSM_SendMessage( victim->uin,
                    g_cfg.AFKMessage,
                    (int8_t)(victim->flags & FL_LOG),
                    victim,
                    1 );

                victim->LastAFK = time(NULL);

                }

            } else {
                YSM_SendMessage(r_uin,
                        g_cfg.AFKMessage,
                        0,
                        NULL,
                        1);
            }

        } else if (g_promptstatus.flags & FL_CHATM) {
            if (victim == NULL || !(victim->flags & FL_CHAT)) {
                /* only send the reply to those who dont
                 * belong to my chat session!.
                 */

                if (victim) {
                    YSM_SendMessage( victim->uin,
                        g_cfg.CHATMessage,
                        (int8_t)(victim->flags & FL_LOG),
                        victim,
                        0);
                } else {
                    YSM_SendMessage(r_uin,
                        g_cfg.CHATMessage,
                        0,
                        NULL,
                        0);
                }
            }
        }
    }

    g_promptstatus.flags |= FL_RAW;

    /* Incoming Messages Event */
    YSM_Event(EVENT_INCOMINGMESSAGE, r_uin, r_nick, m_len, m_data, 0);
}

void YSM_PreOutgoing( slave_t    *victim,
        uin_t        r_uin,
        int8_t        **m_data,
        int32_t        *m_len,
        int8_t        logflag )
{
    int8_t    *data_conv = NULL;
    u_int8_t m_flags = 0;

    if ((victim == NULL && (g_cfg.logall || logflag))
        || ((g_cfg.logall || logflag) && (victim && !(victim->flags & FL_CHAT))))
        YSM_GenerateLogEntry(
                "y0u",
                YSM_USER.Uin,
                r_uin,
                *m_data,
                *m_len);

    if (victim != NULL && (victim->caps & CAPFL_UTF8))
        m_flags |= MFLAGTYPE_UTF8;

    /* Charset Convertion Time! */
    YSM_Charset( CHARSET_OUTGOING, *m_data, m_len, &data_conv, m_flags);
    if (data_conv != NULL) *m_data = data_conv;

    *m_len = strlen(*m_data) + 1;
}

void YSM_PostOutgoing(
    uin_t    r_uin,
    int8_t  *r_nick,
    int32_t  m_len,
    int8_t  *m_data )
{
    /* Outgoing Messages Event */
    YSM_Event(EVENT_OUTGOINGMESSAGE, r_uin, r_nick, m_len, m_data, 0);
}

void YSM_DisplayMsg(
    int16_t      m_type,
    uin_t        r_uin,
    u_int16_t    r_status,
    int32_t      m_len,
    int8_t      *m_data,
    u_int8_t     m_flags,
    int8_t      *r_nick,
    int32_t      log_flag)
{
    char          status_string[MAX_STATUS_LEN];
    char          time_string[10], *aux = NULL, *auxb = NULL;
    keyInstance  *crypt_key = NULL;
    time_t        log_time;
    struct tm    *time_stamp;
    int           x = 0;
    slave_t      *contact = NULL;
    int8_t       *old_m_data = NULL;
    int           free_m_data = 0;

    YSM_WriteStatus(r_status, status_string);

    if (r_nick == NULL)
        r_nick = "NOT a SLAVE";
    else
        contact = YSM_QuerySlaves(SLAVE_NAME, r_nick, 0, 0);

#if defined(YSM_WITH_THREADS)
    /* If the display is busy, make the thread sleep for half a sec */
    while ((g_promptstatus.flags & FL_BUSYDISPLAY) || (g_promptstatus.flags & FL_COMFORTABLEM)) YSM_Thread_Sleep( 0, 500 );
#endif

    old_m_data = m_data;
    YSM_PreIncoming( contact,
            m_type,
            &m_len,
            r_uin,
            &m_data,
            r_nick,
            m_flags,
            &crypt_key );

    free_m_data = (m_data != old_m_data);
    log_time = time(NULL);

    /* are we in CHAT MODE ? we dont print messages which don't belong
     * to our chat session!. */
    if (g_promptstatus.flags & FL_CHATM) {
        if (contact == NULL || !(contact->flags & FL_CHAT))
            goto displaymsg_exit;
    }

    g_promptstatus.flags |= FL_OVERWRITTEN;

    switch (m_type) {

    case YSM_MESSAGE_:

        /* Check if its an Action! */
        if (contact != NULL
            && m_data[0] == '/'
            && strstr(m_data, "/me")) {

            for ( x = 0; x < m_len; x++) {

                if (m_data[x] == '\r' || m_data[x] == '\n')
                    m_data[x] = 0x20;
            }

            m_data = strstr(m_data, "/me");
            m_data += 3;
            if (m_data[0] == 0x20) m_data++;

            PRINTF( VERBOSE_BASE,
                "\r* " "%s %s" 
                " *\n" , contact->info.NickName, m_data);
        }
        else
        {
            time_stamp = localtime( &log_time );
            strftime( time_string, 9, "%H:%M:%S", time_stamp );

            if (contact != NULL)
            {
                PRINTF(VERBOSE_BASE,
                    "\r%s %s" 
                    "%s%s " 
                    "%s%s\n",
                    time_string,
                    (crypt_key != NULL) ? "<*" : "<",
                    r_nick,
                    (crypt_key != NULL) ? "*>" : ">",
                    (strchr(m_data,'\n') != NULL) ? "\n" : "",
                    m_data);
            }
            else
            {
                PRINTF(VERBOSE_BASE,
                    "\r%s %s" 
                    "%d%s " 
                    "%s%s\n" ,
                    time_string,
                    (crypt_key != NULL) ? "<*" : "<",
                    r_uin,
                    (crypt_key != NULL) ? "*>" : ">",
                    (strchr(m_data,'\n') != NULL) ? "\n" : "",
                    m_data);
            }
        }    /* else */
        break;

    case YSM_MESSAGE_PAGER:
        strtok(m_data, "IP: ");
        aux = strtok(NULL, "\n");
        auxb = strtok(NULL, "");
        PRINTF( VERBOSE_BASE, "\r%s - %s\n"
            "< Message: %s >\n", MSG_INCOMING_PAGER, aux, auxb);
        break;

    case YSM_MESSAGE_URL:
        auxb = strchr(m_data, 0xfe);
        if(auxb != NULL) {
            *auxb = '\0';
            aux = strtok(auxb+1, "");
            auxb = &m_data[0];

        } else {
            aux = &m_data[0];
            auxb = "No description Provided";
        }

        /* store the url as the last received url */
        if (aux != NULL) {
            strncpy( YSM_LastURL, aux,
                sizeof(YSM_LastURL) - 1 );

            YSM_LastURL[sizeof(YSM_LastURL)-1] = '\0';
        }

        PRINTF( VERBOSE_BASE ,"\r%s - from: %s - UIN: %d\n"
            "< url: %s >\n< description: %s >\n"
            "[TIP: you may now use the 'burl' command with a '!'\n"
            "[as parameter to trigger your browser with the last\n"
            "[received URL.\n",
            MSG_INCOMING_URL,
            r_nick,
            r_uin,
            aux,
            auxb);

        break;

    case YSM_MESSAGE_CONTACTS:
        for (x = 0; x < m_len; x++) {
            if ((unsigned char)m_data[x] == 0xfe)
                    m_data[x] = 0x20;
        }

        PRINTF( VERBOSE_BASE,
            "\n\rIncoming CONTACTS from: "
            "%s [ICQ# %d].\n",
            r_nick,
            r_uin );

        strtok(m_data, " ");    /* amount */
        x = 0;

        aux = strtok(NULL, " ");
        while (aux != NULL && x < atoi(m_data)) {
            PRINTF( VERBOSE_BASE,
                "\r" "UIN %-14.14s\t", aux );

            aux = strtok(NULL, " ");
            if (!aux) PRINTF( VERBOSE_BASE,
                    "Nick Unknown\n");

            else PRINTF( VERBOSE_BASE,
                    "Nick %s\n", aux);

            aux = strtok(NULL, " ");
            x++;
        }

        break;

    case YSM_MESSAGE_AUTH:

        PRINTF( VERBOSE_BASE, "\r%s UIN #%d "
            "(Slave: %s).\nThe following Message arrived with the"
            " request: %s\n",
            MSG_INCOMING_AUTHR,
            r_uin,
            r_nick,
            m_data);

        break;

    case YSM_MESSAGE_ADDED:
        PRINTF( VERBOSE_BASE,
            "\r%s ICQ #%d just added You to the list."
            " (Slave: %s).\n", MSG_WARN_ADDED, r_uin, r_nick);
        break;

    case YSM_MESSAGE_AUTHOK:
        PRINTF( VERBOSE_BASE, "\r%s #%d .\n",
            MSG_WARN_AUTHOK,
            r_uin);
        break;

    case YSM_MESSAGE_AUTHNOT:
        PRINTF( VERBOSE_BASE ,"\r%s ICQ #%d KILL HIM!\n",
            MSG_WARN_AUTHDENY,
            r_uin );
        break;

    default:
        YSM_ERROR(ERROR_CODE, 1);
        break;
    }


displaymsg_exit:
    /* log, etc */
    YSM_PostIncoming( contact,
            m_type,
            r_uin,
            m_len,
            m_data,
            r_nick,
            log_flag );

    if (free_m_data)
    {
        YSM_FREE(m_data);
    }
}

int32_t YSM_ParseMessageData(u_int8_t *data, u_int32_t length)
{
    u_int32_t x = 0;

    if (!length)
        return 0;

    for (x = 0; x <= length-1; x++)
    {
        /* lets make all < 32 ascii characters
         * be replaced by a space. but the ones we need :P
         */
        if (data[x] == 0x00) continue;    /* NUL */
        if (data[x] == 0x08) continue;    /* backspace */
        if (data[x] == 0x09) continue;    /* TAB */
        if (data[x] == 0x0a) continue;    /* LF */
        if (data[x] == 0x0d) continue;    /* CR */

        if (data[x] < 32) data[x] = 0x20;
    }

    /* by now we just keep the same length going */
    return length;
}
