#include "ysm.h"
#include "toolbox.h"
#include "wrappers.h"
#include "prompt.h"
#include "setup.h"
#include "output.h"

static struct timeval tv;
static fd_set read_fds, dc_fds, net_fds;
static int max_read_fd, max_dc_fd, max_net_fd;

void ysm_error(int32_t level, int8_t verbose, uint8_t *file, int32_t line)
{
    switch (level)
    {
        case ERROR_CODE:
            printfOutput(VERBOSE_BASE, "%s", ERROR_CODE_M);
            break;

        case ERROR_NETWORK:
            printfOutput(VERBOSE_BASE, "%s", ERROR_NETWORK_M);
            break;

        case ERROR_CRITICAL:
            printfOutput(VERBOSE_BASE, "%s", ERROR_CRITICAL_M);
            break;
    }

    if (verbose)
        printfOutput(VERBOSE_BASE, "File: %s:%d\n", file, line);

    exit(-1);
}

bool_t convertStatus(conv_dir_t direction, uint8_t const **str, uint16_t *val)
{
    static struct
    {
        const uint8_t *str;
        const uint16_t val; 
    } table[] = {
        {"ONLINE",    STATUS_ONLINE},
        {"OFFLINE",   STATUS_OFFLINE},
        {"AWAY",      STATUS_AWAY},
        {"NA",        STATUS_NA},
        {"NA2",       STATUS_NA2},
        {"DND",       STATUS_DND},
        {"OCCUPIED",  STATUS_OCCUPIED},
        {"FREE4CHAT", STATUS_FREE_CHAT},
        {"INVISIBLE", STATUS_INVISIBLE},
        {NULL,        0}
    };
    uint16_t x;

    switch (direction)
    {
        case TO_STR:
            for (x = 0; table[x].str != NULL; x++)
            {
                if (table[x].val == *val)
                {
                    *str = table[x].str;
                    return TRUE;
                }
            }
            break;

        case FROM_STR:
            for (x = 0; table[x].str != NULL; x++)
            {
                if (!strcasecmp(*str, table[x].str))
                {
                    *val = table[x].val;
                    return TRUE;
                }
            }
            if (sscanf(*str, "0X%x", val))
                return TRUE;
            break;
    }

    return FALSE;
}

const uint8_t *strStatus(uint16_t status)
{
    static const uint8_t unknown[] = "UNKNOWN";
    const uint8_t *str;

    if (!convertStatus(TO_STR, &str, &status))
    {
        DEBUG_PRINT("unknown status: 0x%X", status);
        str = unknown;
    }

    return str;
}

bool_t isStatusValid(uint16_t status)
{
    const uint8_t *str;

    return convertStatus(TO_STR, &str, &status);
}

void YSM_WriteFingerPrint(int client, char *buf)
{
    switch (client)
    {
        case FINGERPRINT_YSM_CLIENT:
            strncpy(buf," YSM client.", MAX_STATUS_LEN - 1);
            break;

        case FINGERPRINT_YSM_CLIENT_CRYPT:
            strncpy(buf," YSM client w/Encryption.",
                MAX_STATUS_LEN - 1);
            break;

        case FINGERPRINT_MIRANDA_CLIENT:
            strncpy(buf," Miranda client.",
                MAX_STATUS_LEN - 1);
            break;

        case FINGERPRINT_TRILLIAN_CLIENT:
            strncpy(buf," Trillian client.",
                MAX_STATUS_LEN - 1);
            break;

        case FINGERPRINT_SIMICQ_CLIENT:
            strncpy(buf," SIM client.",
                MAX_STATUS_LEN - 1);
            break;

        case FINGERPRINT_LIB2K_CLIENT:
            strncpy(buf," centerICQ/Ickle client. (libicq2000)",
                MAX_STATUS_LEN - 1);
            break;

        case FINGERPRINT_M2000_CLIENT:
            strncpy(buf," Mirabilis ICQ 2000 client.",
                MAX_STATUS_LEN - 1);
            break;

        case FINGERPRINT_M20012_CLIENT:
            strncpy(buf," Mirabilis ICQ 2001/2002 client.",
                MAX_STATUS_LEN - 1);
            break;

        case FINGERPRINT_M2002_CLIENT:
            strncpy(buf," Mirabilis ICQ 2002 client.",
                 MAX_STATUS_LEN - 1);
            break;

        case FINGERPRINT_MICQLITE_CLIENT:
            strncpy(buf," Mirabilis ICQ LITE client.",
                 MAX_STATUS_LEN - 1);
            break;

        case FINGERPRINT_MICQ2003A_CLIENT_1:
            strncpy(buf," Mirabilis ICQ Pro 2003 client.",
                 MAX_STATUS_LEN - 1);
            break;

        case FINGERPRINT_MICQ_CLIENT:
            strncpy(buf," mICQ client.", MAX_STATUS_LEN - 1);
            break;

        case FINGERPRINT_STRICQ_CLIENT:
            strncpy(buf," StrICQ client.", MAX_STATUS_LEN - 1);
            break;

        case FINGERPRINT_LICQ_CLIENT:
            strncpy(buf," Licq client.", MAX_STATUS_LEN - 1);
            break;

        case FINGERPRINT_ICQ2GO_CLIENT:
            strncpy(buf," ICQ2Go! client.", MAX_STATUS_LEN - 1);
            break;

        case FINGERPRINT_MISC_CLIENT:
        default:
            strncpy(buf, " Client unmatched. (Mirabilis?)",
                MAX_STATUS_LEN - 1);
            break;
    }

    buf[MAX_STATUS_LEN - 1] = '\0';
}

/*
  Removes leading and trailing whitespace from str if str is non-NULL.
  Returns: str, if str != NULL
           NULL, if str == NULL
*/
int8_t * YSM_trim(int8_t *str)
{
    int8_t *str_begin = NULL;
    int8_t *str_end = NULL;
    int8_t *str_tmp = str;

    if (str == NULL)
        return NULL;

    for (str_begin = str; isspace(*str_begin); str_begin++)
        ;

    for (str_end = str_begin + strlen(str_begin) - 1;
         str_end != str_begin && isspace(*str_end); str_end--)
        ;

    if (*str_begin == '\0')
        *str = '\0';
    else
    {
        str_tmp = str;
        ++str_end;
        while (str_begin != str_end)
            *str_tmp++ = *str_begin++;
        *str_tmp = '\0';
    }

    return str;
}

/* Thanks a lot to mICQ for these convertion Functions.
   I made some Big Endian ones out of them */

uint32_t Chars_2_DW(const uint8_t *buf)
{
    uint32_t i;

    i = buf[3];
    i <<= 8;
    i += buf[2];
    i <<= 8;
    i += buf[1];
    i <<= 8;
    i += buf[0];

    return i;
}

uint32_t Chars_2_DWb(const uint8_t *buf)
{
    uint32_t i;

    i = buf[0];
    i <<= 8;
    i += buf[1];
    i <<= 8;
    i += buf[2];
    i <<= 8;
    i += buf[3];

    return i;
}

uint16_t Chars_2_Word(const uint8_t *buf)
{
    uint16_t i;

    i = buf[1];
    i <<= 8;
    i += buf[0];

    return i;
}

/* nuevo big endian */
uint16_t Chars_2_Wordb (const uint8_t *buf)
{
    uint16_t i;

    i = buf[0];
    i <<= 8;
    i += buf[1];

    return i;
}

void DW_2_Chars (uint8_t * buf, uint32_t num)
{
    buf[3] = (uint8_t) ((num) >> 24) & 0x000000FF;
    buf[2] = (uint8_t) ((num) >> 16) & 0x000000FF;
    buf[1] = (uint8_t) ((num) >> 8) & 0x000000FF;
    buf[0] = (uint8_t) (num) & 0x000000FF;
}

void DW_2_Charsb(uint8_t * buf, uint32_t num)
{
    buf[0] = (uint8_t) ((num) >> 24) & 0x000000FF;
    buf[1] = (uint8_t) ((num) >> 16) & 0x000000FF;
    buf[2] = (uint8_t) ((num) >> 8) & 0x000000FF;
    buf[3] = (uint8_t) (num) & 0x000000FF;
}

/* intel little endian */
void Word_2_Chars (uint8_t * buf, const int num)
{
    buf[1] = (uint8_t) (((unsigned) num) >> 8) & 0x00FF;
    buf[0] = (uint8_t) ((unsigned) num) & 0x00FF;
}

/* big endian code */
void Word_2_Charsb (uint8_t * buf, const int num)
{
    buf[0] = (uint8_t) (((unsigned) num) >> 8) & 0x00FF;
    buf[1] = (uint8_t) ((unsigned) num) & 0x00FF;
}

void EncryptPassword(char *Password, char *output)
{
    uint8_t x;
    static const uint8_t tablilla[] =
    {
        0xF3, 0x26, 0x81, 0xC4, 0x39, 0x86, 0xDB, 0x92,
        0x71, 0xA3, 0xB9, 0xE6, 0x53, 0x7A, 0x95, 0x7C,
    };

    for (x = 0; x < strlen(Password); x++)
    {
        output[x] = Password[x] ^ tablilla[x];
    }
}


/* In the following FD functions we handle n fd_sets:
 * one for p2p sockets one for network and one for keyboard
 * why? Because in threaded versions we want thread safe functions.
 * the p2p functions operate in a separate thread and we don't want
 * our FDs being cleared in the middle of a procedure.
 */

void FD_Init(int8_t fd)
{
    switch (fd)
    {
        case FD_KEYBOARD:
            FD_ZERO(&read_fds);
            max_read_fd = 0;
            break;

        case FD_NETWORK:
            FD_ZERO(&net_fds);
            max_net_fd = 0;
            break;

        case FD_DIRECTCON:
            FD_ZERO(&dc_fds);
            max_dc_fd = 0;
            break;

        default:
            break;
    }
}

void FD_Timeout(uint32_t sec, uint32_t usec)
{
    tv.tv_sec = sec;
    tv.tv_usec = usec;
}

void FD_Add(int32_t sock, int8_t fd)
{
    if (sock < 0) return;

    switch (fd)
    {
        case FD_KEYBOARD:
            FD_SET(sock, &read_fds);
            if (sock > max_read_fd)
                max_read_fd = sock;
            break;

        case FD_NETWORK:
            FD_SET(sock, &net_fds);
            if (sock > max_net_fd)
                max_net_fd = sock;
            break;

        case FD_DIRECTCON:
            FD_SET(sock, &dc_fds);
            if (sock > max_dc_fd)
                max_dc_fd = sock;
            break;

        default:
            break;
    }
}

void FD_Del(int32_t sock, int8_t whichfd)
{
    if (sock < 0) return;

    switch (whichfd)
    {
        case FD_KEYBOARD:
            FD_CLR( sock, &read_fds );
            if (sock == max_read_fd) max_read_fd = 0;
            break;

        case FD_NETWORK:
            FD_CLR( sock, &net_fds );
            if (sock == max_net_fd) max_net_fd = 0;
            break;

        case FD_DIRECTCON:
            FD_CLR( sock, &dc_fds );
            if (sock == max_dc_fd) max_dc_fd = 0;
            break;

        default:
            break;
    }
}

int FD_IsSet(int32_t sock, int8_t whichfd)
{
    if (sock < 0) return 0;

    switch (whichfd)
    {
        case FD_KEYBOARD:
            return (FD_ISSET(sock, &read_fds));

        case FD_NETWORK:
            return (FD_ISSET(sock, &net_fds));

        case FD_DIRECTCON:
            return (FD_ISSET(sock, &dc_fds));

        default:
            return -1;
    }
}

int FD_Select(int8_t fd)
{
    int res, max_fd = 0;
    fd_set *fds = NULL;

    switch (fd)
    {
        case FD_KEYBOARD:
            max_fd = max_read_fd;
            fds = &read_fds;
            break;

        case FD_NETWORK:
            max_fd = max_net_fd;
            fds = &net_fds;
            break;

        case FD_DIRECTCON:
            max_fd = max_dc_fd;
            fds = &dc_fds;
            break;

        default:
            break;
    }

    /* don't care about writefds and exceptfds: */
    res = select(max_fd + 1, fds, NULL, NULL, &tv);
    if (res == -1)
        FD_ZERO(fds);

    return res;
}

void threadSleep(unsigned long seconds, unsigned long ms)
{
    struct timeval  now;
    struct timespec expected;
    pthread_mutex_t condition_mutex;
    pthread_cond_t  condition_cond;

    /* Unix function */
    gettimeofday(&now, NULL);

    expected.tv_sec = now.tv_sec + seconds;
    expected.tv_nsec = (now.tv_usec * 1000) + (ms * 1000000);

    /* don't let nsec become seconds */
    if (expected.tv_nsec >= 1000000000)
    {
        expected.tv_sec += 1;
        expected.tv_nsec -= 1000000000;
    }

    pthread_mutex_init(&condition_mutex, NULL);
    pthread_mutex_lock(&condition_mutex);

    /* Now! Go to sleep for a second, y0 arent paid for nuthn boy */
    pthread_cond_init(&condition_cond, NULL);
    pthread_cond_timedwait(&condition_cond, &condition_mutex, &expected);
    pthread_cond_destroy(&condition_cond);

    pthread_mutex_unlock(&condition_mutex);
    pthread_mutex_destroy(&condition_mutex);
}

char *YSM_gettime(time_t timestamp, char *buf, size_t len)
{
    struct tm *tp;

    if (timestamp)
    {
        tp = localtime(&timestamp);    /* FIXME: not thread safe */
        strftime(buf, len, "%d %b %Y %H:%M:%S", tp);
    }
    else
        strcpy(buf, "Unknown");

    return buf;
}

/*
  The string str is split into tokens stored in array arr of maximum length
  count.
  Tokens are delimited by sep.
  Occurrences of sep are not treated as part of any token.
  The number of tokens in str is the number of occurrences of sep PLUS 1.
  If str starts with sep, the first token is an empty string. If str ends with
  sep, the last token is an empty string.

  All token string pointers point directly into str which is destroyed
  while tokenizing.

  Returns: -1 if str, sep or arr is NULL or if count<0 or if strlen(sep)==0
           the number of tokens written to arr (never more than count)
*/

ssize_t YSM_tokenize(char* str, const char* sep, char** arr, ssize_t count)
{
    ssize_t ret = 0, len = -1;

    if (sep != NULL)
        len = strlen(sep);

    if (str == NULL || sep == NULL || arr == NULL || len <= 0 || count < 0)
        return -1;

    while (count > 0)
    {
        *arr = str;
        --count;
        ++ret;
        ++arr;

        str = strstr(str, sep);
        if (str == NULL) break;
        *str='\0';
        str += len;
    }

    return ret;
}
