#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#define FIFO_OUT ".ys.fifo"

void printUsage()
{
    printf("usage: ys <command_line>\n");
}

int sendOutput(const int8_t *fifo, const int8_t *str)
{
    int fd;

    //printf("fifo: %s\n", fifo);
    //printf("data: %s\n", str);

    fd = open(fifo, O_WRONLY | O_NONBLOCK);
    if (fd == -1)
    {
        printf("Unable to open output FIFO. ysmd is really started?\n");
        return 1;
    }
    write(fd, str, strlen(str));
    close(fd);

    return 0;
}

int main(int argc, char **argv)
{
    int8_t *buf;
    int8_t *home;
    int8_t *fifoOut = NULL;
    int8_t  res = 1;
    int16_t i;
    int32_t len;
    int32_t offset = 0;

    if (argc < 2)
    {
        printUsage();
        return 1;
    }

    /* get output fifo name */
    home = getenv("HOME");
    len = snprintf(fifoOut, 0, "%s/%s", home, FIFO_OUT) + 1;
    if ((fifoOut = malloc(len)) == NULL)
        return 1;

    snprintf(fifoOut, len, "%s/%s", home, FIFO_OUT);

    /* compute a length of string to put arguments to */
    len = argc + 1;
    for (i = 1; i < argc; i++)
        len += strlen(argv[i]);

    if ((buf = malloc(len)) == NULL)
        return 1;

    {
        buf[0] = '\0';
        for (i = 1; i < argc && offset < len; i++)
        {
            offset += snprintf(buf + offset, len - offset, "%s ", argv[i]);
        }

        res = sendOutput(fifoOut, buf);
        free(buf);
    }

    free(fifoOut);
    return res;
}
