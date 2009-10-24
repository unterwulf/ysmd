#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include "ysmbot.h"
#include "parser.h"

static int8_t *g_fifoIn = NULL;
static int8_t *g_fifoOut = NULL;

void printUsage()
{
    printf("usage: ysmbot <inputFifo> <outputFifo>\n");
}

void sendOutput(const int8_t *str)
{
    int fd;

    DEBUG_PRINT("data: %s", str);

    fd = open(g_fifoOut, O_WRONLY | O_NONBLOCK);
    if (fd == -1)
    {
        DEBUG_PRINT("Unable to open output FIFO.");
        return;
    }
    write(fd, str, strlen(str));
    close(fd);
}

void mainLoop()
{
    int     fd;
    int8_t *buf;
    int8_t *newBuf;
    size_t  totalLen;
    size_t  availLen;
    size_t  offset;
    size_t  readed;

    totalLen = BUFSIZ;
    buf = (int8_t *)malloc(totalLen);
    if (buf == NULL)
        return;

    while (1)
    {
        fd = open(g_fifoIn, O_RDONLY);
        if (fd == -1)
            break;
        availLen = totalLen;
        offset = 0;
        while ((readed = read(fd, buf + offset, availLen)) > 0)
        {
            while (readed == availLen)
            {
                availLen = offset = totalLen;
                totalLen *= 2;
                newBuf = (int8_t *)realloc((void *)buf, totalLen);
                if (newBuf == NULL)
                {
                    DEBUG_PRINT("Unable to increase input buffer size from %d to %d.", availLen, totalLen);
                    buf[totalLen-1] = '\0';
                    availLen = 0; /* read in outside loop returns 0 if len is 0 */
                    break;
                }
                else
                {
                    DEBUG_PRINT("Increase input buffer size from %d to %d.", availLen, totalLen);
                    buf = newBuf;
                }
                readed = read(fd, buf + offset, availLen);
            }
            offset += readed;
            availLen -= readed;
        }
        buf[offset] = '\0';
        parseInput(buf);
        close(fd);
    }
    free(buf);
}

int main(int argc, char **argv)
{
    if (argc == 3)
    {
        g_fifoIn = argv[1];
        g_fifoOut = argv[2];
        DEBUG_PRINT("g_fifoIn: %s, g_fifoOut: %s.", g_fifoIn, g_fifoOut);
    }

    if (argc != 3 || g_fifoIn == NULL || g_fifoOut == NULL)
    {
        printUsage();
        return 1;
    }

    /* make sure that fifoIn is exists */
    unlink(g_fifoIn);
    if (mkfifo(g_fifoIn, 0660) == -1)
    {
        printf("Unable to create input FIFO.\n");
        return 1;
    }

    chdir("/");
    close(STDIN_FILENO);
    close(STDOUT_FILENO);

    if (fork())
        exit(0);

    setsid();
    mainLoop();

    return 0;
}
