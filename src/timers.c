#include <time.h>
#include "timers.h"

static time_t t_keep_alive;   /* last keep alive */
static time_t t_idle;         /* last keyboard activity */
static time_t t_uptime;       /* uptime */

static time_t *timers[] = {&t_keep_alive, &t_idle, &t_uptime};

int get_timer(ysm_timer_t timer)
{
    return time(NULL) - *timers[timer];
}

void reset_timer(ysm_timer_t timer)
{
    *timers[timer] = time(NULL);
}
