#include "ysm.h"
#include "wrappers.h"
#include "toolbox.h"
#include "slaves.h"
#include "output.h"
#include "network.h"
#include "ystring.h"
#include "bytestream.h"
#include "icqv7.h"
#include "dump.h"

#ifdef YSM_TRACE_MEMLEAK
/* Debugging purposes */
int unfreed_blocks = 0;
#endif

int YSM_READ(int32_t sock, void *buf, int read_len, char priority)
{
    int32_t r = 0, x = 0, rlen = 0;

    rlen = read_len;

    while (read_len > r && ((!g_state.connected && priority)
    || g_state.connected))
    {
        x = SOCK_READ(sock, (char *)buf+r, rlen);

        if (x >= 0)
        {
            if (!x) break;
            r += x;
            rlen -= x;
        }
        else
        {
            return -1;
        }
    }

    /* Only negotiation functions take precedence */
    /* since we have multiple threads, we stop them this way */
    if (!priority && !g_state.connected)
    {
        /* make the thread sleep a little dont consume 100% ! */
        threadSleep(0, 100);
        return -1;
    }

    return r;
}

int bsAppendReadLoop(bsd_t bsd, int sock, size_t count, uint8_t priority)
{
    int32_t r = 0, x = 0, rlen = 0;

    rlen = count;

    while (count > r && ((!g_state.connected && priority)
    || g_state.connected))
    {
        x = bsAppendFromSocket(bsd, sock, rlen);

        if (x >= 0)
        {
            if (!x) break;
            r += x;
            rlen -= x;
        }
        else
        {
            return -1;
        }
    }

    /* Only negotiation functions take precedence */
    /* since we have multiple threads, we stop them this way */
    if (!priority && !g_state.connected)
    {
        /* make the thread sleep a little dont consume 100% ! */
        threadSleep(0, 100);
        return -1;
    }

    return r;
}

int writeBs(int sock, bsd_t bsd)
{
    int32_t     r = 0;
    int32_t     y = 0;
    int32_t     dataLen = 0;
    const void *data;
    string_t   *str;

    if (bsGetFlags(bsd) & (BS_FL_INVALID | BS_FL_BADBSD))
        return -1;

    data = (const void *)bsGetBuf(bsd);
    dataLen = bsGetLen(bsd);


    bsRewind(bsd);
    str = initString();
    printfString(str, "OUT PACKET\n");
    dumpPacket(str, bsd);
    writeOutput(VERBOSE_PACKET, getString(str));
    freeString(str);
    bsRewind(bsd);

    do {
        y = SOCK_WRITE(sock, data, dataLen);
        if (y) r += y;
    } while (y >= 0 && r != dataLen);

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
        networkReconnect();

    return r;
}

int YSM_WRITE_DC(slave_t *victim, int32_t sock, void *data, int32_t data_len)
{
    /* checks on DC, open a DC if it doesn't exist! */
    if (victim->d_con.flags & DC_CONNECTED)
        return SOCK_WRITE(sock, data, data_len);
    else
    {
        printfOutput(VERBOSE_BASE,
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

void *ysm_malloc(size_t size, char *file, int line)
{
    void *memory;

    memory = malloc(size);

    if ((size <= 0) || (memory == NULL))
    {
        printfOutput(VERBOSE_BASE,
            "ysm_malloc: Error in block. Probably size error.\n");

        printfOutput(VERBOSE_BASE,
            "Inform the author! File: %s Line: %d\n", file, line);

        ysm_exit(-1, 1);
    }

#ifdef YSM_TRACE_MEMLEAK
    unfreed_blocks++;
#endif

    return memory;
}

void *ysm_calloc(size_t nmemb, size_t size, char *file, int line)
{
    void *memory = ysm_malloc(nmemb * size, file, line);
    memset(memory, '\0', nmemb * size);
    return memory;
}

void *ysm_realloc(void *mem, size_t size, char *file, int line)
{
    void *newMem = realloc(mem, size);
    return newMem;
}

void ysm_free(void *what, char *file, int line)
{
    if (what == NULL)
    {
        printfOutput(VERBOSE_BASE,
            "ysm_free: NULL Block . Probably double free?.\n");

        printfOutput(VERBOSE_BASE,
            "Inform the author! File: %s Line: %d\n", file, line);

        ysm_exit(-1, 1);
    }

#ifdef YSM_TRACE_MEMLEAK
    unfreed_blocks--;
#endif

    free(what);
}

/* This is the function that should be called instead of directly */
/* using the exit() syscall. It does some garbage collection and  */
/* allows the addition of pre-leaving procedures.                 */

void ysm_exit(int32_t status, int8_t ask)
{
    printfOutput(VERBOSE_BASE, "INFO QUIT\n");

    freeBs(g_sinfo.blgroupsid);
    freeBs(g_sinfo.blusersid);

    /* close the network socket */
    close(g_model.network.socket);

    /* free slaves list allocated memory */
    freeSlaveList();

    /* the following iteration through child processes could be
     * done in a tidy manner by waiting for the SIGCHLD signal
     * in order to call waitpid. meanwhile..this does the job.
     */
    while (waitpid(-1, NULL, WNOHANG) > 0);

    /* now exit without zombies */
    exit(status);
}
