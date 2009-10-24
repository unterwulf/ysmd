#ifndef _MISC_H_
#define _MISC_H_

int getkey(void);
void do_backspace(void);
char * YSM_fgets(char *str, int size, char hide);

void sigHandler(int32_t sig);
int putch(int c);

int8_t * YSM_getpass(int8_t *text);

#endif
