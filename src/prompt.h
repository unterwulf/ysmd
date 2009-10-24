#ifndef _PROMPT_H_
#define _PROMPT_H_

#include "slaves.h"

void sendMessage(uin_t uin, int8_t *data, bool_t verbose);

void YSM_PasswdCheck(void);
void YSM_ConsoleRead(int fd);
void YSM_ParseCommand(int8_t *_input, int32_t *argc, int8_t *argv[]);
void YSM_DoCommand(char *cmd);
void YSM_DoChatCommand(int8_t *cmd);
void displayMessage(msg_t *msg);
int32_t YSM_ParseMessageData(uint8_t *data, uint32_t length);

#endif
