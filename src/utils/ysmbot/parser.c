#include <stdio.h>
#include <fcntl.h>
#include "ysmbot.h"
#include "parser.h"

void parseInput(const int8_t *str)
{
    int32_t uin;
    DEBUG_PRINT("data: %s", str);
    int32_t len = BUFSIZ;
    int8_t buf[BUFSIZ];

    if (sscanf(str, "IN MSG %ld", &uin) == 1)
    {
        snprintf(buf, len, "msg %ld pong\n", uin);
        sendOutput(buf);
    }
    else if (sscanf(str, "INFO STATUS %ld %*s %s", &uin, buf) == 2)
    {
        DEBUG_PRINT("status: %s", buf);
        if (strcasecmp(buf, "online") == 0)
        {
            snprintf(buf, len, "msg %d Hi!\n", uin);
            sendOutput(buf);
        }
    }
/*
    else if (strcmp(str, "MSG 149101003") == 0)
        sendOutput("unterwulf");
    else if (strcmp(str, "STATUS OFFLINE") == 0)
        sendOutput("offline");
    else
        sendOutput("What does this stuff mean?");
*/
}
