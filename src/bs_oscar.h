#ifndef _BS_OSCAR_H_
#define _BS_OSCAR_H_

#include "ysm.h"

#define SIZEOF_BYTE      1
#define SIZEOF_WORD      2
#define SIZEOF_DWORD     4

#define SIZEOF_FLAP_HEAD 6  /* unaligned FLAP header size */
#define FLAP_SEQ_OFFSET  2
#define FLAP_LEN_OFFSET  4

#define SIZEOF_SNAC_HEAD 10 /* unaligned SNAC header size */

#define SIZEOF_TLV_HEAD  4
#define TLV_LEN_OFFSET   2

typedef enum {
    ST_STRING08,
    ST_STRING16,
    ST_ASCIIZ,
    ST_NORMAL
} str_type_t;

typedef struct
{
    uint16_t familyId;
    uint16_t subTypeId;
    uint16_t flags;
    req_id_t reqId;
} snac_head_t;

typedef struct
{
    uint8_t  channelId;
    uint16_t seq;
    uint16_t len;
} flap_head_t;

typedef struct
{
    int8_t cmd;
    int8_t channelID;
    int8_t seq[2];
    int8_t dlen[2];
} flap_head_bit_t;

typedef struct
{
    int8_t type[2];
    int8_t len[2];
} tlv_bit_t;

typedef struct
{
    int16_t type;
    int16_t len;
} tlv_t;

#define bsAppendPrintfString08(x, y, args...) \
    bsAppendPrintfType((x), ST_STRING08, (y), ##args);
#define bsAppendPrintfString16(x, y, args...) \
    bsAppendPrintfType((x), ST_STRING16, (y), ##args);
#define bsAppendPrintfStringZ(x, y, args...) \
    bsAppendPrintfType((x), ST_ASCIIZ, (y), ##args);
#define bsAppendPrintfString(x, y, args...) \
    bsAppendPrintfType((x), ST_NORMAL, (y), ##args);

bs_pos_t bsAppendByte(bsd_t bsd, uint8_t byte);
bs_pos_t bsAppendWord(bsd_t bsd, uint16_t word);
bs_pos_t bsAppendWordLE(bsd_t bsd, uint16_t word);
bs_pos_t bsAppendDword(bsd_t bsd, uint32_t dword);
bs_pos_t bsAppendDwordLE(bsd_t bsd, uint32_t word);
bs_pos_t bsAppendFlapHead(bsd_t bsd, uint8_t channel, uint16_t seq, uint16_t len);
bs_pos_t bsAppendTlv(bsd_t bsd, uint16_t type, uint16_t len, const int8_t *value);
void bsUpdateWord(bsd_t bsd, bs_pos_t pos, uint16_t word);
void bsUpdateWordLE(bsd_t bsd, bs_pos_t pos, uint16_t word);
void bsUpdateFlapHeadLen(bsd_t bsd, bs_pos_t flapPos);
void bsUpdateTlvLen(bsd_t bsd, bs_pos_t tlvPos);
void bsUpdateWordLen(bsd_t bsd, bs_pos_t wordPos);
void bsUpdateWordLELen(bsd_t bsd, bs_pos_t wordPos);
uint32_t bsReadByte(bsd_t bsd, uint8_t *byte);
uint32_t bsReadWord(bsd_t bsd, uint16_t *word);
uint32_t bsReadWordLE(bsd_t bsd, uint16_t *word);
uint32_t bsReadDword(bsd_t bsd, uint32_t *dword);
uint32_t bsReadDwordLE(bsd_t bsd, uint32_t *dword);
uint32_t bsReadFlapHead(bsd_t bsd, flap_head_t *flap);
int32_t  bsReadSnacHead(bsd_t bsd, snac_head_t *snac);
int32_t  bsReadTlv(bsd_t bsd, tlv_t *tlv);

bs_pos_t bsAppendPrintfType(bsd_t bsd, str_type_t type, const int8_t *fmt, ...);
bs_pos_t bsAppendVprintfType(bsd_t bsd, str_type_t type, const int8_t *fmt, va_list ap);

#endif /* _BS_OSCAR_H_ */
