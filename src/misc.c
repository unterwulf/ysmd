/*    $Id: YSM_Win32.c,v 1.66 2005/09/04 01:36:48 rad2k Exp $    */
/*
-======================== ysmICQ client ============================-
        Having fun with a boring Protocol
-========================= YSM_Win32.c =============================-


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
#include "toolbox.h"
#include "wrappers.h"
#include "prompt.h"
#include "misc.h"

void CtrlHandler(int32_t sig)
{
    /* don't do a thing. depending on the OS we get
     * called by process or thread, and unless we
     * spend more time on this function, its better
     * to do nothing on ctrl+c
     */
}

int getkey()
{
    unsigned char c;

    if (read(STDIN_FILENO, &c, sizeof(unsigned char)) <= 0) {
        fprintf(stderr, "Can't read: %s.\n", strerror(errno));
        YSM_ERROR(ERROR_CRITICAL, 1);
        /* NOTREACHED */
    }

    return (int)c;
}

int putch(int c)
{
    putchar(c);
    fflush(stdout);
    return (0);
}

void do_backspace()
{
    putchar('\b');
    putchar(0x20);
    putchar('\b');

    fflush(stdout);
}

char * YSM_fgets(char *str, int size, char hide)
{
    unsigned char c;
    int i;

    if (size <= 0) {
        errno = EINVAL;
        return (NULL);
    }

    i = 0;
    str[i] = '\0';
    while (i < size) {
        c = getkey();
        switch (c) {
        case 0x08:
        case 0x7F:
            if (i) {
                str[--i] = '\0';
                do_backspace();
            }
            break;

        case '\n':
            putch(c);
            str[i++] = '\n';

        case '\0':
            str[i] = '\0';
            return (str);

        default:
            str[i++] = c;
            str[i] = '\0';
            if (!hide)
                putch(c);
        }
    }

    return (str);
}

int8_t * YSM_getpass(int8_t *text)
{
    struct         termios t;
    static int8_t  buf[MAX_PWD_LEN+1], *aux = NULL;
    int8_t         restore = 0;

    PRINTF(VERBOSE_BASE, "%s", text);

    if (tcgetattr(STDIN_FILENO, &t) == 0)
    {
        t.c_lflag &= ~ECHO;

        if (tcsetattr(STDIN_FILENO, TCSANOW, &t))
        {
            /* if it fails..we have to proceed */
        }

        restore = 1;
    }

    memset(buf, 0, sizeof(buf));
    YSM_fgets(buf, sizeof(buf)-1, 1);
    buf[sizeof(buf)-1] = 0x00;
    aux = strchr(buf, '\n');
    if (aux != NULL) *aux = 0x00;
    PRINTF(VERBOSE_BASE, "\r\n", text);

    if (restore)
    {
        t.c_lflag |= ECHO;
        if (tcsetattr(STDIN_FILENO, TCSANOW, &t)) {
            /* if it fails..we have to proceed */
        }
    }

    return (int8_t *) &buf;
}

int PRINTF(int verbose_level, char *fmt, ...)
{
    va_list argptr;
    int st;

    if (verbose_level > g_cfg.verbose)
        return -1;

    while (g_promptstatus.flags & FL_BUSYDISPLAY)
        YSM_Thread_Sleep(0, 100);

    va_start(argptr, fmt);
    st = vprintf(fmt, argptr);
    va_end(argptr);

    fflush(stdout);

    return st;
}
