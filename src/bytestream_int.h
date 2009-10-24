#ifndef _BYTESTREAM_C_
#error This header should not be included to any module except bytestream.c.
#else

#include "bytestream.h"

#define MAX_BSD_NUM 255
#define BS_INIT_LEN 8192

typedef struct
{
    uint8_t   *data;
    bs_pos_t   ptr;
    bs_pos_t   end;
    uint32_t   len;
    uint8_t    flags;
} bs_t;

#endif /* _BYTESTREAM_C_ */
