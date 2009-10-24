#ifndef _YSMBOT_H_
#define _YSMBOT_H_

#define DEBUG_PRINT(fmt, args...) \
    fprintf(stderr, "DEBUG: [%s:%d] %s " fmt "\n", __FILE__, __LINE__, __FUNCTION__, ##args)

void sendOutput(const int8_t *str);

#endif /* _YSMBOT_H_ */
