#include "ysm.h"
#include "toolbox.h"
#include "wrappers.h"
#include "prompt.h"
#include "output.h"
#include "misc.h"

void sigHandler(int32_t sig)
{
    /* don't do a thing. depending on the OS we get
     * called by process or thread, and unless we
     * spend more time on this function, its better
     * to do nothing on ctrl+c
     */
    if (sig == SIGCHLD)
    {
        while (waitpid (-1, NULL, WNOHANG) > 0)
            ;   /* don't hang, if no kids are dead yet */
    }
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

    printfOutput(VERBOSE_BASE, "%s", text);

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
    printfOutput(VERBOSE_BASE, "\r\n", text);

    if (restore)
    {
        t.c_lflag |= ECHO;
        if (tcsetattr(STDIN_FILENO, TCSANOW, &t)) {
            /* if it fails..we have to proceed */
        }
    }

    return (int8_t *) &buf;
}
