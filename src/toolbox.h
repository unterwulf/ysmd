#ifndef _TOOLBOX_H_
#define _TOOLBOX_H_

#define YSM_ERROR(x,y) ysm_error((x), (y), __FILE__, __LINE__)

#define LETOH16(x) 
#define BETOH16(x) ntohs(*((uint16_t *)(x)))
#define LETOH32(x)
#define BETOH32(x) ntohl(*((uint32_t *)(x)))

typedef enum {
    FROM_STR,
    TO_STR
} conv_dir_t;

bool_t         convertStatus(conv_dir_t direction, uint8_t const **str, uint16_t *val);
const uint8_t *strStatus(uint16_t status);
bool_t         isStatusValid(uint16_t status);
void           YSM_WriteFingerPrint(int client, char *buf);


void    ysm_error(int32_t level, int8_t verbose, uint8_t *file, int32_t line);

int8_t *YSM_trim(int8_t *str);
ssize_t YSM_tokenize(char *str, const char *sep, char **arr, ssize_t count);
int32_t parseSlave(uint8_t *name);
void printOrganizedSlaves(uint16_t FilterStatus, char *Fstring, int8_t FilterIgnore);

uint32_t Chars_2_DW(const uint8_t *buf);
uint32_t Chars_2_DWb(const uint8_t *buf);
uint16_t Chars_2_Word(const uint8_t *buf);
uint16_t Chars_2_Wordb(const uint8_t *buf);

void DW_2_Chars(uint8_t *buf, uint32_t num);
void DW_2_Charsb(uint8_t *buf, uint32_t num);
void Word_2_Chars(uint8_t *buf, const int num);
void Word_2_Charsb(uint8_t *buf, const int num);

void EncryptPassword(char *Password, char *output);

#define FD_KEYBOARD   0
#define FD_DIRECTCON  1
#define FD_NETWORK    2

void FD_Init(int8_t whichfd);
void FD_Timeout(uint32_t sec, uint32_t usec);
void FD_Add(int32_t sock, int8_t whichfd);
void FD_Del(int32_t sock, int8_t whichfd);
int FD_IsSet(int32_t sock, int8_t whichfd);
int FD_Select(int8_t whichfd);

void threadSleep(unsigned long seconds, unsigned long ms);

char *YSM_gettime(time_t timestamp, char *buf, size_t len);

#endif
