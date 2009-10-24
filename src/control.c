#include "ysm.h"

int initCtl(void)
{
    switch (g_cfg.ctlType)
    {
        case CT_UNIX:
            break;

        case CT_TCP:
        {
            struct sockaddr_in addr;
            int retval;

            memset(&addr, '\0', sizeof(addr));

            addr.sin_family = AF_INET;
            addr.sin_port = htons(g_cfg.ctlPort);
            addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); //g_cfg.ctlAddress;

            fprintf(stderr, "%d\n", g_cfg.ctlPort);

            g_model.ctl = socket(PF_INET, SOCK_STREAM, 0);
            ioctl(g_model.ctl, F_SETFL, O_NONBLOCK);
            setsockopt(g_model.ctl, SOL_SOCKET,
                    SO_REUSEADDR, (void *)1, sizeof(int));

            retval = bind(g_model.ctl, (struct sockaddr *)&addr, sizeof(addr));
            fprintf(stderr, "%d %d %d\n", g_model.ctl, retval, errno);
            listen(g_model.ctl, 1);
            break;
        }
    }
}

int closeCtl(void)
{
    DEBUG_PRINT("");

    switch (g_cfg.ctlType)
    {
        case CT_UNIX:
            break;

        case CT_TCP:
            shutdown(g_model.ctl, SHUT_RDWR);
            close(g_model.ctl);
            break;
    }
}

#if 0
void YSM_ConsoleRead(int fd)
{
    int8_t *tmp = NULL;
    int8_t *retline = NULL;
    int8_t *pos = NULL;
    size_t  size = BUFSIZ;
    ssize_t readsize;

    /* update the idle keyboard timestamp */
    resetTimer(IDLE_TIMEOUT);

    retline = YSM_CALLOC(1, size);
    if (retline == NULL)
        return;
    pos = retline;

    DEBUG_PRINT("");

    while ((readsize = read(fd, pos, size - (pos - retline))) > 0)
    {
        if ((size_t)readsize == size)
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
    if (retline[0] == '\0')
        return;

    /* parse and process the command */
#endif

static void ctlTcpHandler(void)
{
    int     sock;
    int     retval;
    char    buf[8192];
    char   *ptr, *end;
    ssize_t size;

    sock = accept(g_model.ctl, NULL, NULL);

    if (sock < 0)
    {
        if (errno == EAGAIN)
        {
            threadSleep(0, 100);
            return;
        }
        else
           ysm_exit(1);
    }

    send(sock, "ysmd is listening to you\r\n", 26, 0);
    
    retval = ioctl(sock, F_SETFL, O_NONBLOCK);
    end = buf;
    
    while (!g_state.reasonToSuicide)
    {
        size = recv(sock, end, sizeof(buf) - (end - buf), 0);

        if (size == 0)
            break;
        else if (size < 0)
        {
            if (errno == EAGAIN)
            {
                threadSleep(0, 100);
                continue;
            }
            else if (errno == EINTR)
                continue;
            else
                break;
        }

        DEBUG_PRINT("size: %d", size);
        
        end += size;

        if (end == buf + sizeof(buf) - 1)
        {
            end = buf;
        }

        for (ptr = buf; ptr < end - 1; ptr++)
        {
            if (*ptr == '\r' && *(ptr+1) == '\n')
            {
                *ptr = '\0';
                DEBUG_PRINT("buf: %s", buf);
                YSM_DoCommand(buf);
                ptr += 2;
                memmove(buf, ptr, end - ptr);
                end -= ptr - buf;
                ptr = buf;
            }
        }
    }

//    shutdown(sock, SHUT_RDWR);
    close(sock);
}

void ctlHandler()
{
    switch (g_cfg.ctlType)
    {
        case CT_TCP: ctlTcpHandler(); break;
    }
}
