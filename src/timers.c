#include <time.h>
#include "timers.h"

static time_t t_keep_alive;   /* last keep alive */
static time_t t_idle;         /* last keyboard activity */
static time_t t_uptime;       /* uptime */

static time_t *timers[] = {&t_keep_alive, &t_idle, &t_uptime};

int getTimer(ysm_timer_t timer)
{
    return time(NULL) - *timers[timer];
}

void resetTimer(ysm_timer_t timer)
{
    *timers[timer] = time(NULL);
}
