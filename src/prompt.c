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

#ifndef YSM_WITH_THREADS
#include "cmdline/ysmline.h"
#else
#ifdef HAVE_LIBREADLINE
#  if defined(HAVE_READLINE_READLINE_H)
#    include <readline/readline.h>
#  elif defined(HAVE_READLINE_H)
#    include <readline.h>
#  else /* !defined(HAVE_READLINE_H) */
extern char *readline ();
#  endif /* !defined(HAVE_READLINE_H) */
char *cmdline = NULL;

#ifdef HAVE_READLINE_HISTORY
#  if defined(HAVE_READLINE_HISTORY_H)
#    include <readline/history.h>
#  elif defined(HAVE_HISTORY_H)
#    include <history.h>
#  else /* !defined(HAVE_HISTORY_H) */
extern void add_history();
extern int write_history();
extern int read_history();
#  endif /* defined(HAVE_READLINE_HISTORY_H) */
 /* no history */
#endif /* HAVE_READLINE_HISTORY */

#else /* HAVE_LIBREADLINE */
#include "cmdline/getline.h"
#endif
#endif

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <termios.h>

struct YSM_PROMPTSTATUS g_promptstatus;

int16_t   YSM_AFKCount = 0;
time_t    YSM_AFK_Time = 0;
u_int16_t YSM_TabCount = 1;

static struct termios console_attrs;
static int8_t fl_setattr = 0;

extern slave_t    *YSMSlaves_LastSent, *YSMSlaves_LastRead;
extern slave_t    *YSMSlaves_TabSlave;
extern int8_t      YSM_AFKMessage[MAX_DATA_LEN + 1];
extern int8_t      YSM_CHATMessage[MAX_DATA_LEN + 1];
extern int8_t      YSM_LastMessage[MAX_DATA_LEN + 1];
extern int8_t      YSM_LastURL[MAX_DATA_LEN + 1];
extern char        YSM_cfgdir[MAX_PATH];

int8_t * YSM_ConsoleGetPrompt(void)
{
    static int8_t g_prompt[80];
    int8_t status[MAX_STATUS_LEN];

    /* get our status string */
    YSM_WriteStatus(YSM_USER.status, status);
    memset(g_prompt, 0, sizeof(g_prompt));

    if (g_promptstatus.flags & FL_AFKM) {
        /* we are in AFK MODE */
        snprintf( g_prompt,
            sizeof(g_prompt),
            "\r%s%-2.2s" NORMAL
            " " BRIGHT_BLUE "AFK[%d]" NORMAL "> ",
            YSM_GetColorStatus(status, NULL),
            status,
            YSM_AFKCount
            );

        g_prompt[sizeof(g_prompt)-1] = 0x00;

    } else if (g_promptstatus.flags & FL_CHATM) {
        /* we are in CHAT MODE */
        snprintf( g_prompt,
            sizeof(g_prompt),
            "\r%s[%d]" NORMAL "> ",
            YSM_GetColorStatus(status, NULL),
            YSM_AFKCount);
        g_prompt[sizeof(g_prompt)-1] = 0x00;
    } else {
        /* we are in NORMAL MODE */
        snprintf( g_prompt,
            sizeof(g_prompt),
            "\r%s%-2.2s" NORMAL "> ",
            YSM_GetColorStatus(status, NULL),
            status
            );
        g_prompt[sizeof(g_prompt)-1] = 0x00;
    }
    return g_prompt;
}

void YSM_ConsoleRedrawPrompt(int8_t redrawbuf)
{
    if (redrawbuf) {
#ifndef YSM_WITH_THREADS
        PRINTF(VERBOSE_BASE,
            "\r%s%s",
            YSM_ConsoleGetPrompt(),
            g_cmdstring);
#else
#ifdef HAVE_LIBREADLINE
        PRINTF(VERBOSE_BASE,
            "\r%s%s",
            YSM_ConsoleGetPrompt(),
            rl_line_buffer);
#else
        gl_fixup(YSM_ConsoleGetPrompt(), -2, GL_BUF_SIZE);
#endif
#endif
    } else
        PRINTF(VERBOSE_BASE, "\r%s", YSM_ConsoleGetPrompt());

    fflush(stdout);
}

void YSM_ConsoleClearLine(int8_t redrawprompt, int32_t cmdlen)
{
    int32_t x = 0, y = 0;

    fprintf(stdout, "\r");
    if (cmdlen > 0)
        y = cmdlen + 9; /* +9 in case it was AFK prompt */
    else
        y = 11;        /* no command, but afk prompt */

    for (x = 0; x < y; x++)
        fprintf(stdout, " ");

    fprintf(stdout, "\r");

    if (redrawprompt)
        YSM_ConsoleRedrawPrompt(0);

    fflush(stdout);
}

void YSM_ConsoleRestore(void)
{
    if (fl_setattr)
        tcsetattr(STDIN_FILENO, TCSANOW, &console_attrs);
}

void YSM_ConsoleSetup(void)
{
    struct termios t;

    if (tcgetattr(STDIN_FILENO, &t) != 0) {
        /* dont panic if we dont have a console */
        return;
    }

    console_attrs = t;
    t.c_lflag &= ~(ICANON | ECHO);
    t.c_cc[VTIME] = 0;
    t.c_cc[VMIN] = 1;

    if (tcsetattr(STDIN_FILENO, TCSANOW, &t)) {
        return;
    }

    fl_setattr = 1;
    atexit(YSM_ConsoleRestore);
}

/* only ansi stringz messages for this function.
 * encryption, if neccesary, is done inside.
 * if verbous, print any messages.
 */

void YSM_SendMessage(
    uin_t      r_uin,
    int8_t    *data,
    int8_t     logflag,
    slave_t   *slave,
    u_int8_t   verbous)
{
    int32_t      data_len = 0;
    int8_t       time_string[10], status_string[MAX_STATUS_LEN];
    int8_t      *oldmark1 = NULL, *oldmark2 = NULL, *r_nick = NULL;
    u_int8_t     flags = 0;
    time_t       log_time;
    struct       tm *time_stamp;
    keyInstance *crypt_key = NULL;

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
                YSM_MESSAGE_NORMAL,
                data,
                data_len,
                0x00,
                flags,
                rand() & 0xffffff7f );
    else
        YSM_SendMessage2Client( slave,
                r_uin,
                0x01,
                YSM_MESSAGE_NORMAL,
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
                    "%s %s %s"NORMAL" [%s]. (%d).\n",
                    time_string,
                    MSG_MESSAGE_SENT3,
                    slave->info.NickName,
                    status_string,
                    slave->uin );
            }
        } else {
            if (verbous) {
                PRINTF( VERBOSE_BASE,
                    "%s %s %s"NORMAL" [%s]. (%d).\n",
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
    if (data != oldmark1)
    {
        if (data != oldmark2 && oldmark2 != oldmark1) {
            ysm_free( oldmark2, __FILE__, __LINE__ );
            oldmark2 = NULL;
        }

        ysm_free( data, __FILE__, __LINE__ );
        data = NULL;
    }

    if (slave != NULL)
        r_nick = slave->info.NickName;
    else
        r_nick = "NOT a SLAVE";

    YSM_PostOutgoing(r_uin, r_nick, strlen(oldmark1), oldmark1);
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

/* YSM_ConsoleIsHotKey ************************************
 * Must be called sometime for every first character in a
 * line by all of our console handling libraries. If ret
 * is >= 1, then its a hotkey, libraries should ignore the
 * pressed key. Else, proceed as usual.
 */

#ifdef HAVE_LIBREADLINE
int32_t YSM_ConsoleIsHotKey(int count, int key)
#else
int32_t YSM_ConsoleIsHotKey(int32_t key)
#endif
{

#ifdef HAVE_LIBREADLINE
    /* we have to check HERE that the typed char is the first one */
    if (rl_point || (g_promptstatus.flags & FL_COMFORTABLEM)) {
        /* and..we have to insert it manually if its not :( */
        rl_insert_text((char *)&key);
        return 0;

    }
#else
    /* don't trigger hotkeys during comfortable messages */
    if (g_promptstatus.flags & FL_COMFORTABLEM)
        return 0;
#endif

    switch (key) {
        case '1':
            PRINTF(VERBOSE_BASE, "\n");
            YSM_Command_HELP(0x00, 0x00);
            break;

        case '2':
#ifndef COMPACT_DISPLAY
            PRINTF(VERBOSE_BASE, "\n");
#else
            PRINTF(VERBOSE_BASE, "\r");
#endif
            YSM_PrintOrganizedSlaves(STATUS_ONLINE, NULL, 0x01);
            break;

        case '3':
#ifndef COMPACT_DISPLAY
            PRINTF(VERBOSE_BASE, "\n");
#else
            PRINTF(VERBOSE_BASE, "\r");
#endif
            YSM_PrintOrganizedSlaves(STATUS_OFFLINE, NULL, 0x00);

            break;

        case '4':
#ifndef COMPACT_DISPLAY
            PRINTF(VERBOSE_BASE, "\n");
#else
            #if defined(YSM_WITH_THREADS)
            #if defined(HAVE_LIBREADLINE)
                YSM_ConsoleClearLine(0, strlen(rl_line_buffer));
            #else
                YSM_ConsoleClearLine(0, strlen(gl_buf));
            #endif
            #else
                YSM_ConsoleClearLine(0, strlen(g_cmdstring));
            #endif
#endif
            if (!(g_promptstatus.flags & FL_AFKM)) {
                strncpy(YSM_AFKMessage,
                    YSM_AFK_MESSAGE,
                    sizeof(YSM_AFKMessage) - 1);

                YSM_AFKMessage[sizeof(YSM_AFKMessage)-1] = 0x00;
            }

            YSM_AFKMode((u_int8_t)!(g_promptstatus.flags & FL_AFKM));
            break;

        case '5':
            PRINTF(VERBOSE_BASE, "\n");
            PRINTF(VERBOSE_BASE, "%s\n", MSG_AFK_READ_MSG);
            YSM_ReadLog(YSM_AFKFILENAME, 0);
            break;

#ifdef YSM_WITH_THREADS
        case '6':
            PRINTF(VERBOSE_BASE, "\n");
            YSM_Command_FILESTATUS(0x00, 0x00);
            break;
#endif
        default:
            return 0;
    }

    g_promptstatus.flags |= FL_REDRAW;

    return 1;
}

/* YSM_ConsoleTabHook *************************************
 *
 * what about it:
 * This is a wrapper for the 3 <TAB> key hooks in order
 * to use a common code for readline, getline and ysmline.
 * These hooks DONT use any completion functionality from
 * any of those libraries, they implement it themselves.
 ***********************************************************/

#ifndef YSM_WITH_THREADS
int YSM_ConsoleTabHook(void)
#else
#ifdef HAVE_LIBREADLINE
int YSM_ConsoleTabHook(int count, int key)
#else
int YSM_ConsoleTabHook(char *buf, int offset, int *loc, size_t bufsize)
#endif
#endif
{
    int8_t  *pbuf = NULL;
    int32_t *pos = 0, maxsize = 0;

    /* don't trigger TAB during comfortable messages */
    if (g_promptstatus.flags & FL_COMFORTABLEM)
        return 0;

#ifndef YSM_WITH_THREADS
    pbuf    = &g_cmdstring[0];
    pos     = &g_cmdlen;
    maxsize = sizeof(g_cmdstring);
#else
#ifdef HAVE_LIBREADLINE
    pbuf    = rl_line_buffer;
    pos     = &rl_point;
    maxsize = rl_end;
#else
    pbuf    = buf;
    pos     = loc;
    maxsize = sizeof(gl_buf);
#endif
#endif

    YSM_ConsoleTab(pbuf, pos, maxsize);

#ifndef YSM_WITH_THREADS
    /* redisplay everything */
    YSM_ConsoleRedrawPrompt(TRUE);
#else
#ifdef HAVE_LIBREADLINE
    /* when rl_point is at the end of the line, rl_end and rl_point
     * are equal */
    rl_point = rl_end;

    /* refresh the screen */
    rl_redisplay();
#else
    /* getline is so wonderful it needs nothing afterwards */
#endif
#endif

    return 0;
}


void YSM_ConsoleReadInit(void)
{
#ifdef HAVE_LIBREADLINE
    int8_t inputrc[MAX_PATH];
#endif

    memset(&g_promptstatus, 0, sizeof(g_promptstatus));

#ifndef YSM_WITH_THREADS
    /* set out tab hook */
    ysm_tab_hook = &YSM_ConsoleTabHook;
#else
#ifdef HAVE_LIBREADLINE
    /* readline seems to have trouble with the way we set up
     * our console to read from the keyboard. hence disable
     * our settings -here- before we start reading. We don't
     * do this at the very beginning because some stuff does
     * require our console configuration changed.
     */
    YSM_ConsoleRestore();

    /* Set default readline values */
    rl_variable_bind("editing-mode", "emacs");
    rl_bind_key('\t', YSM_ConsoleTabHook);    /* Tab hook for emacs */
    /* ysm hot keys */
    rl_bind_key('1', YSM_ConsoleIsHotKey);
    rl_bind_key('2', YSM_ConsoleIsHotKey);
    rl_bind_key('3', YSM_ConsoleIsHotKey);
    rl_bind_key('4', YSM_ConsoleIsHotKey);
    rl_bind_key('5', YSM_ConsoleIsHotKey);
    rl_bind_key('6', YSM_ConsoleIsHotKey);

    rl_variable_bind("editing-mode", "vi");
    rl_variable_bind("keymap", "vi-insert");
    rl_bind_key('\t', YSM_ConsoleTabHook);    /* Tab hook for vi-insert */
    /* ysm hot keys */
    rl_bind_key('1', YSM_ConsoleIsHotKey);
    rl_bind_key('2', YSM_ConsoleIsHotKey);
    rl_bind_key('3', YSM_ConsoleIsHotKey);
    rl_bind_key('4', YSM_ConsoleIsHotKey);
    rl_bind_key('5', YSM_ConsoleIsHotKey);
    rl_bind_key('6', YSM_ConsoleIsHotKey);


    rl_variable_bind("horizontal-scroll-mode", "on");

    /* Read user readline configuration */
    snprintf(inputrc, sizeof(inputrc), "%s/inputrc", YSM_cfgdir);
    inputrc[sizeof(inputrc) - 1] = 0x00;
    if (rl_read_init_file(inputrc) != 0) {
        /* there isn't an inputrc in ysm's directory.
         * go for the default locations.
         */
        rl_read_init_file(NULL);
    }

#else    /* getline */

    /* initialize tab completion and set our tab hook */
    gl_tab_hook = &YSM_ConsoleTabHook;
#endif
#endif
}

void YSM_ConsoleRead(void)
{
    int8_t *retline = NULL;

#ifndef DAEMON
#ifndef YSM_WITH_THREADS
    retline = ysmreadline(YSM_ConsoleGetPrompt());
#else
#ifdef HAVE_LIBREADLINE
    retline = readline(YSM_ConsoleGetPrompt());

    /* Add it to the readline history */
    if (retline && *retline)
        add_history(retline);
#else
int8_t    *aux = NULL;
    retline = getline(YSM_ConsoleGetPrompt());

    /* getline leaves a \n at the end.. */
    if (retline != NULL) {
        aux = strchr(retline, '\n');
        if (aux != NULL) *aux = 0x00;
    }

    if (retline && *retline)
        gl_histadd(retline);
#endif
#endif
    /* update the idle keyboard timestamp */
    reset_timer(IDLE_TIMEOUT);

#else /* ifdef DAEMON */

    retline = ysm_malloc(BUFSIZ, __FILE__, __LINE__);

    if (freopen(YSM_FIFO, "r", stdin) == NULL)
    {
        printf("Error stdin");
        exit(1);
    }

    fgets(&retline, BUFSIZ, stdin);

#endif

    /* did we get a command or not? */
    if (retline == NULL)
        return;

    /***************************** ysm chat mode **************
     * if we are in chat mode, we treat new commands directly as messages
     * and send them to the slaves who have the FL_CHAT flag in them
     ****************************************************************/
    if (g_promptstatus.flags & FL_CHATM)
    {
        YSM_DoChatCommand(retline);
    }
    else
    {
        /* parse and process the command */
        YSM_DoCommand(retline);
    }

#if defined(HAVE_LIBREADLINE) || defined(DAEMON)
    /* dont you dare to leak on me! */
    ysm_free(retline, __FILE__, __LINE__);
    retline = NULL;
#endif
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
    int8_t      *argv[MAX_CMD_ARGS], found = FALSE;
    int32_t      argc = 0;
    u_int32_t    x = 0;
    command_t   *node = (command_t *) g_command_list.start;

    if (!strlen(cmd)) return;

    for (x = 0; x < MAX_CMD_ARGS; x++)
        argv[x] = NULL;

    YSM_ParseCommand(cmd, &argc, argv);

    for (x = 0; x < g_command_list.length; x++)
    {
        if (!node) break;

        /* speed up with first checks */
        if (node->cmd_name[0] != (int8_t)tolower(argv[0][0])
        || strcasecmp(node->cmd_name, argv[0]))
        {
            if (node->cmd_alias != NULL)
            {
                if (node->cmd_alias[0] != (int8_t)tolower(argv[0][0])
                || strcasecmp(node->cmd_alias, argv[0])) {
                    node = (command_t *) node->suc;
                    continue;
                }
            }
            else
            {
                node = (command_t *) node->suc;
                continue;
            }
        }

        found = TRUE;

        if (argc < node->cmd_margs) {
            PRINTF( VERBOSE_BASE,
                "Missing parameters. Use the 'help'"
                " command for detailed information.\n"
                );
        } else {

            /* use the low caps argv[0], just in case */
            /* who knows when batman may come 2 kill us (?!) */
            argv[0] = node->cmd_name;

            if (node->cmd_func != NULL)
                node->cmd_func( argc, argv );

            g_promptstatus.flags |= FL_REDRAW;
        }

        break;
    }

    if (!found)
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

int8_t * YSM_ReadLongMessage(void)
{
static    int8_t msgdata[MAX_DATA_LEN + 1];
int8_t    *retline = NULL, *tmp = NULL;
size_t    x = 0, y = 0;
#ifndef HAVE_LIBREADLINE
int8_t    *backup = NULL;
#endif
    tmp = &msgdata[0];
    memset(msgdata, '\0', sizeof(msgdata));

    /* Once we are in here, don't let the command line autogenerate
     * if we are in the middle of a message.
     */
    g_promptstatus.flags |= FL_BUSYDISPLAY;

    /* disable hotkeys */
    g_promptstatus.flags |= FL_COMFORTABLEM;

    /* Since getline and ysmreadline use the same buffer for command line
     * as they will use in here. We have an aweful (really) fix for not
     * overwriting useful data (we will need it outside this function).
     * We make a backup of the command line buffers and restore them before
     * leaving.
     */

#ifndef YSM_WITH_THREADS
    backup = ysm_calloc(1, sizeof(g_cmdstring), __FILE__, __LINE__);
    memcpy(backup, g_cmdstring, sizeof(g_cmdstring));
#else
#ifndef HAVE_LIBREADLINE
    backup = ysm_calloc(1, sizeof(gl_buf), __FILE__, __LINE__);
    memcpy(backup, gl_buf, sizeof(gl_buf));
#endif
#endif
    do {
#ifndef YSM_WITH_THREADS
        retline = ysmreadline(NULL);
#else
#ifdef HAVE_LIBREADLINE
        retline = readline(NULL);
#else
int8_t    *aux = NULL;
        retline = getline(NULL);
        if (retline) {
            /* getline leaves a \n at the end.. */
            aux = strchr(retline, '\n');
            if (aux != NULL) *aux = 0x00;
        }
#endif
#endif
        if (retline != NULL) {

            y = strlen(retline) + 1;    /* +1 of the \n */

            /* if the input exceeds the maximum length, resize it
             * to the maximum. no exploitable integer overflow here.
             */

            if ((x + y) > (sizeof(msgdata) - 1))
                y = MAX_DATA_LEN - x;

            x += y;

            /* if !y then we have no more space left.
             * Warn and return what we read so far
             */
            if (!y) {
                PRINTF( VERBOSE_BASE,
                    "\nMaximum Data Reached. \a!\n");

                g_promptstatus.flags &= ~FL_BUSYDISPLAY;
                g_promptstatus.flags &= ~FL_COMFORTABLEM;

            /* restore the original input buffers on ysmline and
             * on getline. read above for further details. */
#ifndef YSM_WITH_THREADS
                memcpy(g_cmdstring,
                    backup,
                    sizeof(g_cmdstring));
                ysm_free(backup, __FILE__, __LINE__);
                backup = NULL;
#else
#ifdef HAVE_LIBREADLINE
                /* dont you dare to leak on me! */
                ysm_free(retline, __FILE__, __LINE__);
                retline = NULL;
#else
                memcpy(gl_buf, backup, sizeof(gl_buf));
                ysm_free(backup, __FILE__, __LINE__);
                backup = NULL;
#endif
#endif
                return (tmp);
            }

            /* did the user supply a control character? */
            if (strlen(retline) == 1) {

                /* end of message? */
                if (retline[0] == '.') {
                    g_promptstatus.flags &= ~FL_BUSYDISPLAY;
                    g_promptstatus.flags &= ~FL_COMFORTABLEM;
            /* restore the original input buffers on ysmline and
             * on getline. read above for further details. */
#ifndef YSM_WITH_THREADS
                memcpy(g_cmdstring,
                    backup,
                    sizeof(g_cmdstring));
                ysm_free(backup, __FILE__, __LINE__);
                backup = NULL;
#else
#ifdef HAVE_LIBREADLINE
                /* dont you dare to leak on me! */
                ysm_free(retline, __FILE__, __LINE__);
                retline = NULL;
#else
                memcpy(gl_buf, backup, sizeof(gl_buf));
                ysm_free(backup, __FILE__, __LINE__);
                backup = NULL;
#endif
#endif
                    return (tmp);

                /* cancel message? */
                } else if (retline[0] == '#') {
                    g_promptstatus.flags &= ~FL_BUSYDISPLAY;
                    g_promptstatus.flags &= ~FL_COMFORTABLEM;
            /* restore the original input buffers on ysmline and
             * on getline. read above for further details. */
#ifndef YSM_WITH_THREADS
                memcpy(g_cmdstring,
                    backup,
                    sizeof(g_cmdstring));
                ysm_free(backup, __FILE__, __LINE__);
                backup = NULL;
#else
#ifdef HAVE_LIBREADLINE
                /* dont you dare to leak on me! */
                ysm_free(retline, __FILE__, __LINE__);
                retline = NULL;
#else
                memcpy(gl_buf, backup, sizeof(gl_buf));
                ysm_free(backup, __FILE__, __LINE__);
                backup = NULL;
#endif
#endif
                    return (NULL);
                }
            }

            /* ok then, plug the new data at the end */
            strncat( msgdata, retline,
                sizeof(msgdata) - strlen(msgdata) - 1);

            /* add a newline to separate lines */
            strncat( msgdata, "\n",
                sizeof(msgdata) - strlen(msgdata) - 1);

#ifdef HAVE_LIBREADLINE
            /* dont you dare to leak on me! */
            ysm_free(retline, __FILE__, __LINE__);
#endif
        }

    } while (retline != NULL);

    g_promptstatus.flags &= ~FL_BUSYDISPLAY;
    g_promptstatus.flags &= ~FL_COMFORTABLEM;

    return tmp;
}

/* YSM_AlertUser()
 *    - Beeps
 *    - configurable deconify & flashing window (win32/OS2)
 */

static void YSM_AlertUser(void)
{
    u_int32_t x;

    if (g_cfg.beep)
    {
        for(x = 0; x < (unsigned)g_cfg.beep; x++)
        {
            PRINTF(VERBOSE_BASE, "\a");
            YSM_Thread_Sleep(0, 200);
        }
    }

    YSM_WindowAlert();
}

static void YSM_PreIncoming( slave_t    *contact,
        int16_t        m_type,
        int32_t        *m_len,
        uin_t        r_uin,
        int8_t        **m_data,
        int8_t        *r_nick,
        u_int8_t    m_flags,
        keyInstance    **key )
{
int8_t *data_conv = NULL, do_conv = 0;

    /* alert the user of an incoming Message */
    YSM_AlertUser();

    /* procedures applied to normal messages only */
    if (m_type == YSM_MESSAGE_NORMAL) {

        /* decrypt the incoming message if neccesary.
         * the function will return < 0 if decryption failed.
         * though it might be because we don't have a key with
         * this contact (or we don't have her on our list).
         */

        if (YSM_DecryptMessage( contact,
                    m_data,
                    m_len,
                    key ) >= 0) {

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

        if (do_conv) {
            /* Charset convertion time */
            YSM_Charset( CHARSET_INCOMING,
                *m_data,
                m_len,
                &data_conv,
                m_flags );

            if (data_conv != NULL) {
                /* m_data is freed in the callers function */
                *m_data = data_conv;
            }
        }

        /* filter unwanted characters */
        YSM_ParseMessageData(*m_data, *m_len);

        /* set again the data length after the convertion */
        (*m_len) = strlen(*m_data)+1;

        /* store last message */
        if ( contact != NULL ) {
            strncpy( YSM_LastMessage,
                *m_data,
                sizeof(YSM_LastMessage) - 1 );

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

    YSM_ExecuteCommand( arg_index, exec_args );
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

    if (m_type == YSM_MESSAGE_NORMAL) {

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
                    YSM_AFKMessage,
                    (int8_t)(victim->flags & FL_LOG),
                    victim,
                    1 );

                victim->LastAFK = time(NULL);

                }

            } else {
                YSM_SendMessage(r_uin,
                        YSM_AFKMessage,
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
                        YSM_CHATMessage,
                        (int8_t)(victim->flags & FL_LOG),
                        victim,
                        0);
                } else {
                    YSM_SendMessage(r_uin,
                        YSM_CHATMessage,
                        0,
                        NULL,
                        0);
                }
            }
        }
    }

    g_promptstatus.flags |= FL_REDRAW;

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

void YSM_DisplayMsg( int16_t        m_type,
        uin_t        r_uin,
        u_int16_t    r_status,
        int32_t        m_len,
        int8_t        *m_data,
        u_int8_t    m_flags,
        int8_t        *r_nick,
        int32_t        log_flag )
{

char        status_string[MAX_STATUS_LEN];
char        time_string[10], *aux = NULL, *auxb = NULL;
keyInstance    *crypt_key = NULL;
time_t        log_time;
struct tm    *time_stamp;
int        x = 0;
int8_t        *msgCol = g_cfg.color_message;
slave_t    *contact = NULL;
int8_t        *old_m_data = NULL;
int        free_m_data = 0;


    YSM_WriteStatus(r_status, status_string);

    if (r_nick == NULL) {
        r_nick = "NOT a SLAVE";
    } else {
        contact = YSM_QuerySlaves(SLAVE_NAME, r_nick, 0, 0);
         if (contact->color!=NULL) msgCol = contact->color;
    }

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

    /* this is ugly but we need to clear the prompt on incoming messages */
#if defined(YSM_WITH_THREADS)
#if defined(HAVE_LIBREADLINE)
    YSM_ConsoleClearLine(0, strlen(rl_line_buffer));
#else
    YSM_ConsoleClearLine(0, strlen(gl_buf));
#endif
#else
    YSM_ConsoleClearLine(0, strlen(g_cmdstring));
#endif
    switch (m_type) {

    case YSM_MESSAGE_NORMAL:

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
                "\r" BRIGHT_BLUE "* " NORMAL "%s %s" BRIGHT_BLUE
                " *\n" NORMAL, contact->info.NickName, m_data);


        } else {

            time_stamp = localtime( &log_time );
            strftime( time_string, 9, "%H:%M:%S", time_stamp );

            if (contact != NULL) {

                PRINTF( VERBOSE_BASE,
                    NORMAL "\r%s " BRIGHT_BLUE "%s" NORMAL
                    "%s" BRIGHT_BLUE "%s " NORMAL
                    "%s%s%s\n" NORMAL,
                    time_string,
                    (crypt_key != NULL) ? "<*" : "<",
                    r_nick,
                    (crypt_key != NULL) ? "*>" : ">",
                    msgCol,
                    (strchr(m_data,'\n') != NULL)
                    ? "\n" : "",
                    m_data );

            } else {

                PRINTF( VERBOSE_BASE,
                     NORMAL "\r%s " BRIGHT_BLUE "%s" NORMAL
                    "%d" BRIGHT_BLUE "%s " NORMAL
                    "%s%s%s\n" NORMAL,
                    time_string,
                    (crypt_key != NULL) ? "<*" : "<",
                    r_uin,
                    (crypt_key != NULL) ? "*>" : ">",
                    msgCol,
                    (strchr(m_data,'\n') != NULL)
                    ? "\n" : "",
                    m_data );
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
            "\n\r" MAGENTA "Incoming CONTACTS from: "
            NORMAL "%s [ICQ# %d].\n",
            r_nick,
            r_uin );

        strtok(m_data, " ");    /* amount */
        x = 0;

        aux = strtok(NULL, " ");
        while (aux != NULL && x < atoi(m_data)) {
            PRINTF( VERBOSE_BASE,
                "\r" WHITE "UIN" NORMAL " %-14.14s\t", aux );

            aux = strtok(NULL, " ");
            if (!aux) PRINTF( VERBOSE_BASE,
                    WHITE "Nick" NORMAL " Unknown\n");

            else PRINTF( VERBOSE_BASE,
                    WHITE "Nick" NORMAL " %s\n", aux);

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
        YSM_Error(ERROR_CODE, __FILE__, __LINE__, 1);
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

    if (free_m_data) {
        ysm_free(m_data, __FILE__, __LINE__);
        m_data = NULL;
    }
}

int32_t YSM_ParseMessageData( u_int8_t *data, u_int32_t length )
{
u_int32_t x = 0;

    if (!length) return 0;

    for ( x = 0; x <= length-1; x++) {
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


void YSM_ConsoleTabCommand(int32_t argc, int8_t **argv, int32_t *pos, size_t msize)
{
int8_t      *incomplete = argv[0], *match = NULL;
u_int32_t    x = 0;
command_t    *node = (command_t *) g_command_list.start;

    for (x = 0; x < strlen(incomplete); x++)
        incomplete[x] = tolower(incomplete[x]);

    /* Find Matching bytes of the command */
    for (x = 0; x < g_command_list.length; x++)
    {
        if (!node) break;

        if (!memcmp(node->cmd_name, incomplete, strlen(incomplete)))
        {
            match = node->cmd_name;
            break;
        }
        else if (node->cmd_alias != NULL)
        {
            if (!memcmp(node->cmd_alias, incomplete, strlen(incomplete)))
            {
                match = node->cmd_alias;
                break;
            }
        }

        node = (command_t *) node->suc;
    }

    if (match != NULL)
    {
        char tmpbuf[100];

        memset(tmpbuf, 0, sizeof(tmpbuf));
        snprintf( tmpbuf,
            sizeof(tmpbuf),
            "%s ",
            match );

        tmpbuf[sizeof(tmpbuf) - 1] = 0x00;

        /* go back to where the command starts. */
#if defined(YSM_WITH_THREADS) && defined(HAVE_LIBREADLINE)
        (*pos) = 0;
#else
        (*pos) -= strlen(incomplete);
#endif

#if defined(YSM_WITH_THREADS) && defined(HAVE_LIBREADLINE)
    /* for readline its different because the buffer is allocated
     * and this means its got nothing in it. We cant overwrite past
     * its end. This is why we use rl_insert_text()
     */

        /* first delete the current incomplete string. Then insert
         * the new complete one */
        rl_delete_text(*pos, msize);

        /* now insert the new name */
        rl_insert_text(tmpbuf);
#else
        if (*pos + strlen(tmpbuf) <= msize) {
            memcpy(incomplete, tmpbuf, strlen(tmpbuf));
            /* ending space included already */
            (*pos) += strlen(incomplete);

            /* end the string */
            incomplete[strlen(tmpbuf)] = 0x00;

        /* else set pos again to what it was */
        } else (*pos) += strlen(incomplete);
#endif
    }
}

void YSM_ConsoleTabSlave(int32_t argc, int8_t **argv, int32_t *pos, size_t msize)
{
slave_t    *node = NULL;
int8_t        *incomplete = argv[argc], cycle_flag = FALSE;
#if defined(YSM_WITH_THREADS) && defined(HAVE_LIBREADLINE)
#else
int32_t        x = 0;
#endif

    /* Only do this if at least 1 user is online
     * else its just a waste of time.
     */

    if (!g_sinfo.onlineslaves) return;

    /* find matching bytes of the incomplete slave name */
    node = (slave_t *) g_slave_list.start;
    while (node != NULL)
    {
        if (!strncasecmp( node->info.NickName,
                incomplete,
                strlen(incomplete) )) break;

        node = (slave_t *) node->suc;
    }

    /* if the incomplete pointer is exactly what the found
     * node pointer is, then it means the user provided the
     * whole slave name. This means it's not calling TAB to
     * complete the rest of the name but instead to cycle
     * through the next slaves (they are sorted alphabetically).
     */

    if (node != NULL) {

        if (!strcasecmp(node->info.NickName, incomplete)) {
            /* user chose to use TAB for cycling. */
            cycle_flag = TRUE;

            /* we care only about connected slaves */
            if (node->suc != NULL)
                node = (slave_t *) node->suc;
            else
                node = (slave_t *) g_slave_list.start;

            while (node->status == STATUS_OFFLINE) {
                if (node->suc != NULL)
                    node = (slave_t *) node->suc;
                else
                    node = (slave_t *) g_slave_list.start;
            }

        } else {

            /* the cycle_flag helps us prevent a problem
             * afterwards. There is an extra space we need
             * to add if the TAB key wasn't used for cycling
             */
            cycle_flag = FALSE;
        }

    }

    if (node != NULL) {
        int8_t tmpbuf[100];

        memset(tmpbuf, 0, sizeof(tmpbuf));
        snprintf( tmpbuf,
            sizeof(tmpbuf),
            "%s ",
            node->info.NickName );
        tmpbuf[sizeof(tmpbuf) - 1] = 0x00;

        /* go back to the beginning of the slave name */
        *pos -= strlen(incomplete);

        /* note one thing, if we are cycling, it means theres
         * one more space we have to handle at the end.
         */

        if (cycle_flag) (*pos)--;

#if defined(YSM_WITH_THREADS) && defined(HAVE_LIBREADLINE)
    /* for readline its different because the buffer is allocated
     * and this means its got nothing in it. We cant overwrite past
     * its end. This is why we use rl_insert_text()
     */

        /* first delete the current incomplete string. Then insert
         * the new complete one */
        rl_delete_text(*pos, msize);

        /* now insert the new name */
        rl_insert_text(tmpbuf);
#else
        if (*pos + strlen(tmpbuf) <= msize) {
            if (strlen(node->info.NickName)
                < strlen(incomplete) ) {
                    memset( incomplete,
                        0, strlen(incomplete) );
            }

            memcpy(incomplete, tmpbuf, strlen(tmpbuf));

            /* end the string */
            incomplete[strlen(tmpbuf)] = 0x00;
        }

        *pos = 0;
        for( x = 0; x <= argc; x++ ) {
            (*pos) += strlen(argv[x]);
            if (x < argc) (*pos)++;
        }
#endif
    }
}

void YSM_ConsoleTab(int8_t *string, int32_t *size, int32_t maxsize)
{
    int8_t   *aux = NULL;
    int8_t   *argv[MAX_CMD_ARGS];
    int32_t   argc = 0;
    int32_t   x = 0;

    if (*size && string[0] != ' ')
    {
        /* Parse the current input */
        for (x = 0; x < MAX_CMD_ARGS; x++)
            argv[x] = NULL;

        YSM_ParseCommand(string, &argc, argv);

        if (argc < 1)    /* COMMAND COMPLETE */
            YSM_ConsoleTabCommand(argc, argv, size, maxsize);
        else             /* SLAVE NAME COMPLETE */
            YSM_ConsoleTabSlave(argc, argv, size, maxsize);

        /* Restore parsed spaces */
        for (x = 0; x < argc; x++)
        {
            aux = strchr(argv[x],'\0');
            if (aux != NULL)
                *aux = 0x20;
        }
    }
    /* Nothing was typed and <TAB> was pressed. We have
    a default 'msg' command policy. Fill the pleasure. */
    else
    {
        if (YSMSlaves_TabSlave)
        {
            int8_t tmpbuf[100];

            memset(tmpbuf, 0, sizeof(tmpbuf));
            snprintf(tmpbuf, sizeof(tmpbuf), "msg %s ",
                YSMSlaves_TabSlave->info.NickName);
            tmpbuf[sizeof(tmpbuf) - 1] = 0x00;

#if defined(YSM_WITH_THREADS) && defined(HAVE_LIBREADLINE)
    /* for readline its different because the buffer is allocated
     * and this means its got nothing in it. We cant overwrite past
     * its end. This is why we use rl_insert_text()
     */
            rl_insert_text(tmpbuf);
#else
            memset(string, '\0', *size);
            strncpy(string, tmpbuf, maxsize-1);
#endif
        }

        *size = strlen(string);
    }
}
