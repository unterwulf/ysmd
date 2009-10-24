#include "ysm.h"
#include "ystring.h"
#include "toolbox.h"

static int writeFifo(int8_t *str)
{
    int fd;

    fd = open(g_cfg.outputPath, O_NONBLOCK | O_WRONLY);
    if (fd != -1)
    {
        write(fd, str, strlen(str));
        close(fd);
    }
}

static int writePipe(int8_t *str)
{
    int   fd[2];
    pid_t cpid;

    if (pipe(fd) == -1)
        return;

    cpid = fork();

    if (cpid == -1)
        return;

    if (cpid == 0)
    {
        close(fd[1]);
        if (dup2(fd[0], STDIN_FILENO) == -1)
            exit(1);
        execvp(g_cfg.outputPath, NULL);
        DEBUG_PRINT("execvp failed");
        exit(1);
    }
    else
    {
        close(fd[0]);
        write(fd[1], str, strlen(str));
        close(fd[1]);
    }
}

int writeOutput(int verboseLevel, const int8_t *str)
{
    int retVal = 0;
//    DEBUG_PRINT("str: %s", str);

    if (verboseLevel <= g_cfg.verbose)
    {
        sem_wait(&semOutput);

        switch (g_cfg.outputType)
        {
            case OT_STDIN:
                retVal = writePipe(str);
                break;

            case OT_FIFO:
                retVal = writeFifo(str);
                break;

            case OT_STDOUT:
                printf("%s", str);
                fflush(stdout);
                break;

            default:
                YSM_ERROR(ERROR_CRITICAL, 1);
        }

        sem_post(&semOutput);
    }

    return retVal;
}

int printfOutput(int verboseLevel, char *fmt, ...)
{
    va_list ap;
    int8_t *buf;
    string_t *str;
    int retVal;

    str = initString();

    va_start(ap, fmt);
    retVal = vprintfString(str, fmt, ap);
    va_end(ap);

    if (retVal > 0)
        retVal = writeOutput(verboseLevel, getString(str));

    freeString(str);

    return retVal;
}
