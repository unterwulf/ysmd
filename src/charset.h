#ifndef _CHARSET_H_
#define _CHARSET_H_

#define CHARSET_INCOMING    0x00
#define CHARSET_OUTGOING    0x01

#ifdef YSM_USE_CHARCONV
#include <iconv.h>
#define YSM_ICONV_MAXLEN MAX_DATA_LEN * 4
#endif

void initCharset(void);

int32_t YSM_Charset(
    int8_t     direction,
    int8_t    *buf_from,
    int32_t   *buf_fromlen,
    int8_t   **buf_to,
    uint8_t    m_flags);

int8_t * YSM_CharsetConvertOutputString(
    int8_t **stringp,
    int8_t   fl_dofree);

int32_t YSM_CharsetConvertString(
    int8_t **stringp,
    int8_t   direction,
    uint8_t  flags,
    int8_t   fl_dofree);

uint8_t *encode64(const uint8_t *str);

#endif
