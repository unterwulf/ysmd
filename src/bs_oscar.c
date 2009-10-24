#include "ysm.h"
#include "toolbox.h"
#include "wrappers.h"
#include "bytestream.h"
#include "bs_oscar.h"

bs_pos_t bsAppendByte(bsd_t bsd, uint8_t byte)
{
    return bsAppend(bsd, &byte, sizeof(byte));
}

bs_pos_t bsAppendWord(bsd_t bsd, uint16_t word)
{
    uint8_t buf[SIZEOF_WORD];

    Word_2_Charsb(buf, word);
    return bsAppend(bsd, buf, sizeof(buf));
}

bs_pos_t bsAppendWordLE(bsd_t bsd, uint16_t word)
{
    uint8_t buf[SIZEOF_WORD];

    Word_2_Chars(buf, word);
    return bsAppend(bsd, buf, sizeof(buf));
}

bs_pos_t bsAppendDword(bsd_t bsd, uint32_t dword)
{
    uint8_t buf[SIZEOF_DWORD];

    DW_2_Charsb(buf, dword);
    return bsAppend(bsd, buf, sizeof(buf));
}

bs_pos_t bsAppendDwordLE(bsd_t bsd, uint32_t word)
{
    int8_t buf[SIZEOF_DWORD];

    DW_2_Chars(buf, word);
    return bsAppend(bsd, buf, sizeof(buf));
}

bs_pos_t bsAppendFlapHead(bsd_t bsd, uint8_t channel, uint16_t seq, uint16_t len)
{
    uint8_t head[SIZEOF_FLAP_HEAD];

    head[0] = '\x2A';
    head[1] = channel;
    Word_2_Charsb(head + FLAP_SEQ_OFFSET, seq);
    Word_2_Charsb(head + FLAP_LEN_OFFSET, len);

    return bsAppend(bsd, head, sizeof(head));
}

bs_pos_t bsAppendTlv(bsd_t bsd, uint16_t type, uint16_t len, const int8_t *value)
{
    int8_t   tlv[SIZEOF_TLV_HEAD];
    bs_pos_t retVal;

    Word_2_Charsb(tlv, type);
    Word_2_Charsb(tlv+TLV_LEN_OFFSET, len);

    retVal = bsAppend(bsd, tlv, sizeof(tlv));

    if (retVal > 0 && value)
    {
        retVal = bsAppend(bsd, value, len);
    }

    return retVal;
}

void bsUpdateWord(bsd_t bsd, bs_pos_t pos, uint16_t word)
{
    uint8_t buf[SIZEOF_WORD];

    Word_2_Charsb(word, buf);
    bsUpdate(bsd, pos, buf, SIZEOF_WORD);
}

void bsUpdateWordLE(bsd_t bsd, bs_pos_t pos, uint16_t word)
{
    uint8_t buf[SIZEOF_WORD];

    Word_2_Chars(word, buf);
    bsUpdate(bsd, pos, buf, SIZEOF_WORD);
}

void bsUpdateFlapHeadLen(bsd_t bsd, bs_pos_t flapPos)
{
    bs_pos_t len;

    len = bsGetLen(bsd) - flapPos - SIZEOF_FLAP_HEAD;
    bsUpdateWord(bsd, flapPos + FLAP_LEN_OFFSET, len);
}

void bsUpdateTlvLen(bsd_t bsd, bs_pos_t tlvPos)
{
    bs_pos_t len;

    len = bsGetLen(bsd) - tlvPos - SIZEOF_TLV;
    bsUpdateWord(bsd, tlvPos + TLV_LEN_OFFSET, len);
}

void bsUpdateWordLen(bsd_t bsd, bs_pos_t wordPos)
{
    bs_pos_t len;

    len = bsGetLen(bsd) - wordPos - SIZEOF_WORD;
    bsUpdateWord(bsd, wordPos, len);
}

void bsUpdateWordLELen(bsd_t bsd, bs_pos_t wordle)
{
    bs_pos_t len;

    len = bsGetLen(bsd) - wordPos - SIZEOF_WORD;
    bsUpdateWordLE(bsd, wordPos, len);
}

uint32_t bsReadByte(bsd_t bsd, uint8_t *byte)
{
    return bsRead(bsd, byte, SIZEOF_BYTE);
}

uint32_t bsReadWord(bsd_t bsd, uint16_t *word)
{
    uint8_t buf[SIZEOF_WORD];
    bs_pos_t retVal;

    retVal = bsRead(bsd, buf, SIZEOF_WORD);
    *word = Chars_2_Wordb(buf);

    return retVal;
}

uint32_t bsReadWordLE(bsd_t bsd, uint16_t *word)
{
    uint8_t buf[SIZEOF_WORD];
    bs_pos_t retVal;

    retVal = bsRead(bsd, buf, SIZEOF_WORD);
    *word = Chars_2_Word(buf);

    return retVal;
}

uint32_t bsReadDword(bsd_t bsd, uint32_t *dword)
{
    uint8_t buf[SIZEOF_DWORD];
    bs_pos_t retVal;

    retVal = bsRead(bsd, buf, SIZEOF_DWORD);
    *dword = Chars_2_DWb(buf);

    return retVal;
}

uint32_t bsReadDwordLE(bsd_t bsd, uint32_t *dword)
{
    uint8_t buf[SIZEOF_DWORD];
    bs_pos_t retVal;

    retVal = bsRead(bsd, buf, SIZEOF_DWORD);
    *dword = Chars_2_DW(buf);

    return retVal;
}

uint32_t bsReadFlapHead(bsd_t bsd, flap_head_t *flap)
{
    uint8_t cmd;

    if (flap == NULL)
        return -1;

    bsReadByte(bsd, &cmd);

    if (cmd != '\x2A')
    {
        bsSeek(bsd, -SIZEOF_BYTE, BS_CUR_POS);
        return -1;
    }

    bsReadByte(bsd, &(flap->channelId));
    bsReadWord(bsd, &(flap->seq));
    bsReadWord(bsd, &(flap->len));

    return SIZEOF_FLAP_HEAD;
}

int32_t bsReadSnacHead(bsd_t bsd, snac_head_t *snac)
{
    if (snac == NULL)
        return -1;

    bsReadWord(bsd, &(snac->familyId));
    bsReadWord(bsd, &(snac->subTypeId));
    bsReadWord(bsd, &(snac->flags));
    bsReadDword(bsd, &(snac->reqId));

    return SIZEOF_SNAC_HEAD;
}

int32_t bsReadTlv(bsd_t bsd, tlv_t *tlv)
{
    if (tlv == NULL)
        return -1;

    bsReadWord(bsd, tlv->type);
    bsReadWord(bsd, tlv->len);

    return SIZEOF_TLV;
}

bs_pos_t bsAppendVprintfType(
    bsd_t         bsd,
    str_type_t    type,
    const int8_t *fmt,
    va_list       ap)
{
    uint32_t  len;
    uint8_t  *buf;
    bs_pos_t  pos = bsTell(bsd);

    len = vsnprintf(NULL, 0, fmt, ap) + 1; /* including null-terminator */
    buf = (uint8_t *)YSM_MALLOC(len);
    if (buf == NULL)
        return -1;

    switch (type)
    {
        case ST_STRING08: bsAppendByte(bsd, (uint8_t)len - 1); break;
        case ST_STRING16: bsAppendWord(bsd, (uint16_t)len - 1); break;
    }

    vsnprintf(buf, len, fmt, ap);
    switch (type)
    {
        case ST_STRING08:
        case ST_STRING16:
        case ST_NORMAL:
            len--; /* exclude null-terminator */
            break;

        default:
        case ST_ASCIIZ:
            break;
    }

    bsAppend(bsd, buf, len);
    YSM_FREE(buf);

    return pos;
}

bs_pos_t bsAppendPrintfType(bsd_t bsd, str_type_t type, const int8_t *fmt, ...)
{
    bs_pos_t retVal;
    va_list ap;

    va_start(ap, fmt);
    retVal = bsAppendVprintfType(bsd, type, fmt, ap);
    va_end(ap);

    return retVal;
}
