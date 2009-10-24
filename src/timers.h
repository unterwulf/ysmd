#ifndef _TIMERS_H_
#define _TIMERS_H_

typedef enum {KEEP_ALIVE_TIMEOUT, IDLE_TIMEOUT, UPTIME} ysm_timer_t;

int  get_timer(ysm_timer_t timer);
void reset_timer(ysm_timer_t timer);

#endif /* _TIMERS_H_ */
