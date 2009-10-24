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
#include "wrappers.h"
#include "toolbox.h"
#include "slaves.h"
#include "main.h"

#ifdef YSM_TRACE_MEMLEAK
/* Debugging purposes */
int unfreed_blocks = 0;
#endif

void YSM_Reconnect(void)
{
    slave_t    *slave;

    /* Starting time is 10 seconds */
    u_int32_t   x = 10;
    int32_t     y = 0;

    g_sinfo.flags &= ~FL_LOGGEDIN;
    g_state.reconnecting = TRUE;

    /* put them all offline */
    g_sinfo.onlineslaves = 0;

    /* Reset slaves status */
    for (slave = (slave_t *) g_slave_list.start;
         slave != NULL;
         slave = (slave_t *) slave->suc)
    {
        slave->status = STATUS_OFFLINE;
    }

    while (y <= 0)
    {
        PRINTF(VERBOSE_BASE,
        "\nDisconnection detected. "
        "Reconnecting in %d seconds.\n" , x );

        YSM_Thread_Sleep(x, 0);

        close(YSM_USER.network.rSocket);

        if ((y = YSM_SignIn()) < 0)
        {
            if (x < 300)
                x += 5;
            else
            {
                PRINTF(VERBOSE_BASE,
                "\nMaximum reconnects reached. "
                "Network must be down..\n" );
                YSM_ERROR(ERROR_NETWORK, 0);
            }
        }
    }
}

int YSM_READ( int32_t    sock,
    void        *buf,
    int        read_len,
    char        priority )
{
int32_t    r = 0, x = 0, rlen = 0;

    rlen = read_len;

    while(read_len > r && ((g_state.reconnecting && priority)
    || !g_state.reconnecting)) {

        x = SOCK_READ(sock, (char *)buf+r, rlen);

        if(x >= 0) {
            if (!x) break;
            r += x;
            rlen -= x;
        } else {
            return -1;
        }
    }


    /* Only negotiation functions take precedence */
    /* since we have multiple threads, we stop them this way */
    if (!priority && g_state.reconnecting) {
        /* make the thread sleep a little dont consume 100% ! */
        YSM_Thread_Sleep(0, 100);
        return -1;
    }

    return r;
}


int YSM_WRITE(int32_t sock, void *data, int32_t data_len)
{
    int32_t r = 0, y = 0;

    do {
        y = SOCK_WRITE(sock, data, data_len);
        if (y) r += y;
    } while (y >= 0 && r != data_len);

    if (y < 0)
        YSM_Reconnect();

    return r;
}

int32_t YSM_WRITE_DC(slave_t *victim, int32_t sock, void *data, int32_t data_len)
{
    /* checks on DC, open a DC if it doesn't exist! */
    if (victim->d_con.flags & DC_CONNECTED)
        return SOCK_WRITE(sock , data, data_len);
    else {
        PRINTF( VERBOSE_BASE,
            "No open DC session found with slave.\n"
            "Use the 'opendc' command to open a DC session.\n ");

        return -1;
    }
}

/* reads a single line until \r\n is met.
 * returns the read amount of bytes in a size_t (without \r\n)
 */

size_t YSM_READ_LN(int32_t sock, int8_t *obuf, size_t maxsize)
{
    int8_t ch = 0;
    size_t bread = 0;

    while (ch != '\n' && bread < maxsize) {
        if (SOCK_READ(sock, &ch, 1) <= 0)
            break;

        obuf[bread++] = ch;
    }

    /* we make sure we dont return \r\n in our read size */
    if (bread >= 2 && obuf[bread-2] == '\r' && obuf[bread-1] == '\n')
        return bread - 2;

    /* we might have had a single \n */
    if (bread >= 1 && obuf[bread-1] == '\n')
        return bread - 1;

    return bread;
}

void * ysm_malloc(size_t size, char *file, int line)
{
    char *memory;

    memory = malloc(size);

    if ((size <= 0) || (memory == NULL))
    {
        PRINTF(VERBOSE_BASE,
            "\rYSM_Malloc: Error in block. Probably size error.\n");

        PRINTF(VERBOSE_BASE,
            "Inform the author! File: %s Line: %d\n", file, line);

        ysm_exit(-1, 1);
    }

#ifdef YSM_TRACE_MEMLEAK
    unfreed_blocks++;
#endif

    return memory;
}

void * ysm_calloc(size_t nmemb, size_t size, char *file, int line)
{
    char *memory;

    memory = ysm_malloc(nmemb * size, file, line);
    memset(memory, 0, nmemb * size);

    return memory;
}

void ysm_free(void *what, char *file, int line)
{
    if (what == NULL)
    {
        PRINTF(VERBOSE_BASE,
            "\rysm_free: NULL Block . Probably double free?.\n");

        PRINTF(VERBOSE_BASE,
            "Inform the author! File: %s Line: %d\n", file, line);

        ysm_exit(-1, 1);
    }

#ifdef YSM_TRACE_MEMLEAK
    unfreed_blocks--;
#endif

    free(what);
    what = NULL;
}

/* This is the function that should be called instead of directly */
/* using the exit() syscall. It does some garbage collection and  */
/* allows the addition of pre-leaving procedures.                 */

void ysm_exit(int32_t status, int8_t ask)
{
    if (g_sinfo.blgroupsid != NULL)
        YSM_FREE(g_sinfo.blgroupsid);

    if (g_sinfo.blusersid != NULL)
        YSM_FREE(g_sinfo.blusersid);

    /* Logging off event */
    YSM_Event(EVENT_LOGOFF,
        YSM_USER.Uin,
        YSM_USER.info.NickName,
        0,
        NULL,
        0);

    /* close the network socket */
    close(YSM_USER.network.rSocket);

    /* Free all those nodes! */
    freelist(&g_command_list);
    freelist(&g_slave_list);

    /* the following iteration through child processes could be
     * done in a tidy manner by waiting for the SIGCHLD signal
     * in order to call waitpid. meanwhile..this does the job.
     */
    while (waitpid(-1, NULL, WNOHANG) > 0);

    /* now exit without zombies */
    exit(status);
}
