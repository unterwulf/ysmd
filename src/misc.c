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

void YSM_WindowAlert(void)
{
    switch (g_cfg.winalert)
    {
        case 0x1:
        case 0x3:
            PRINTF(VERBOSE_BASE, DECONIFY);
            break;
        case 0x2:
        default:
            break;
    }

    return;
}


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
        YSM_Error(ERROR_CRITICAL, __FILE__, __LINE__, 1);
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

int gettxy(int *x, int *y)
{
    printf("\x1B[6n");

    /* Trash unrequired characters */
    getchar(); getchar();

    /* row */
    *y = 10 * (getchar() - '0');
    *y = *y + (getchar() - '0');

    /* Trash unrequired characters */
    getchar();

    *x = 10 * (getchar() - '0');
    *x = *x + (getchar() - '0');

    /* Trash unrequired characters */
    getchar();
    getchar();

    return (1);
}


int getxy(int32_t *x, int32_t *y)
{
    char buf[20], *p, *q;
    ssize_t r = 0, i = 0;

    fprintf(stdout, "\x1B[6n");
    fflush(stdout);

    usleep(50000);

    buf[r] = '\0';

    for (--r, i = 0; i < r && buf[i] != '\x1B' && buf[i+1] != '['; i++);
    if (i >= r)
        return (0);

    for (p = &buf[i]; *p != '\0' && *p != 'R'; p++);
    for (q = &buf[i]; *q != '\0' && *q != ';'; q++);
    if (*p == '\0' || *q == '\0')
        return (0);

    *p = '\0';
    *q = '\0';
    *x = atoi(q + 1);
    *y = atoi(&buf[i+2]);

    return (1);
}

void gotoxy( int8_t X, int8_t Y )
{
    fprintf( stdout, "\33[%d;%dH", Y, X );
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
struct termios t;
static int8_t    buf[MAX_PWD_LEN+1], *aux = NULL;
int8_t    restore = 0;

    PRINTF(VERBOSE_BASE, "%s", text);

    if (tcgetattr(STDIN_FILENO, &t) == 0) {

        t.c_lflag &= ~ECHO;

        if (tcsetattr(STDIN_FILENO, TCSANOW, &t)) {
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

    if (restore) {
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

    if ( verbose_level > g_cfg.verbose )
        return -1;

    while (g_promptstatus.flags & FL_BUSYDISPLAY)
        YSM_Thread_Sleep( 0, 100 );

    va_start(argptr, fmt);
    st = vprintf(fmt, argptr);
    va_end(argptr);

    fflush(stdout);

    return (st);
}

int32_t OutputCharacter( void *handle,
        int8_t    *ch,
        int32_t    len,
        COORD    coord,
        int32_t    *out,
        int8_t    *color )
{
    gotoxy( coord.X, coord.Y );
    (*out) = 0;

    if (color != NULL) printf( color );
    while (len--) {
        putch(*(ch+(*out)));
        (*out)++;
    }
    if (color != NULL) printf( NORMAL );

    return 0;
}

void DrawBox( void *hOut, SMALL_RECT rect, int8_t *color )
{
COORD    pt;
int8_t    chBox[6];
int32_t    x = 0, out;

    chBox[0] = (char)0xda; chBox[1] = (char)0xbf;
    chBox[2] = (char)0xc0; chBox[3] = (char)0xd9;
    chBox[4] = (char)0xc4; chBox[5] = (char)0xb3;

    pt.Y = rect.Top;

    /* Top Left char */
    pt.X = rect.Left;
    OutputCharacter(hOut, &chBox[0], 1, pt, &out, color);
    pt.X++;

    /* Draw Top Horizontal line */
    for (x = 0; x < (rect.Right - rect.Left - 1); x++) {
        OutputCharacter(hOut, &chBox[4], 1, pt, &out, color);
        pt.X++;
    }

    /* Top Right char */
    pt.X = rect.Right;
    OutputCharacter(hOut, &chBox[1], 1, pt, &out, color);

    /* Draw vertical Left and Right line */
    pt.Y = rect.Top;
    pt.Y ++;

    for (x = 0; x < (rect.Bottom - rect.Top - 1); x++) {
        pt.X = rect.Left;
        OutputCharacter(hOut, &chBox[5], 1, pt, &out, color);
        pt.X = rect.Right;
        OutputCharacter(hOut, &chBox[5], 1, pt, &out, color);
        pt.Y++;
    }

    pt.Y = rect.Bottom;

    /* Bottom left bottom char */
    pt.X = rect.Left;
    OutputCharacter(hOut, &chBox[2], 1, pt, &out, color);
    pt.X++;

    /* Draw Bottom Horizontal line */
    for (x = 0; x < (rect.Right - rect.Left - 1); x++) {
        OutputCharacter(hOut, &chBox[4], 1, pt, &out, color);
        pt.X++;
    }

    /* Bottom Right char */
    pt.X = rect.Right;
    OutputCharacter(hOut, &chBox[3], 1, pt, &out, color);
}

void BoxString( int8_t *string,
    int16_t    color,
    int32_t    top,
    int32_t    bottom,
    int8_t    *ansicol1,
    int8_t    *ansicol2 )
{
COORD                postext;
SMALL_RECT            rc;
void                *hOut = 0;
int32_t                i = 0, out = 0, x, y = 0, x1, x2, y1, y2;

    if (!getxy(&x, &y)) return;
    x = 80;            /* use a default value for x */

    x1 = (x - strlen(string))/2 - 2;
    y1 = y + top;
    x2 = x1 + strlen(string) + 4;
    y2 = y1 + bottom;

    postext.X = x1;
    postext.Y = y1;

    for (i = 0; i < 5; i++) {
        postext.Y++;
    }

    postext.X = x1 + 2;
    postext.Y = y1 + 2 + top;

    OutputCharacter(hOut, string, strlen(string), postext, &out, ansicol1);

    rc.Left    = x1;
    rc.Top    = y1;
    rc.Right = x2 - 1;
    rc.Bottom = y2 - 1;

    DrawBox( hOut, rc, ansicol2 );
}

void YSM_PrintWizardBox( int8_t *string )
{
    PRINTF( VERBOSE_BASE, CLRSCR );
    gotoxy( 0, 0 );

    BoxString( string,
            0,
            0,
            5,
            BRIGHT_BLUE,
            BRIGHT_GREEN);

    fprintf( stdout, "\n\n" );
}
