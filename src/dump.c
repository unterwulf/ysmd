#include "ysm.h"
#include "ystring.h"
#include "output.h"
#include "bytestream.h"
#include "bs_oscar.h"
#include "network.h" // flap_head_bit_t
#include "icqv7.h"   // SNAC_HEAD_SIZE, YSM_CHANNEL_SNACDATA
#include "wrappers.h"

static void dumpFlap(string_t *str, const flap_head_t *flap)
{
    printfString(str,
        "FLAP Information:\n"
        "channel id: 0x%.2X\n"
        "seq: 0x%.2X\n"
        "dlen: 0x%.2X\n",
        flap->channelId,
        flap->seq,
        flap->len);
}

static void dumpSnac(string_t *str, snac_head_t *snac)
{
    printfString(str,
        "SNAC Information:\n"
        "family: 0x%.2X\n"
        "subtype: 0x%.2X\n"
        "flags: 0x%.2X\n"
        "reqid: 0x%X\n",
        snac->familyId,
        snac->subTypeId,
        snac->flags,
        snac->reqId);
}

void dumpHex(string_t *str, const uint8_t *buf, uint32_t len)
{
    uint32_t        x = 0;
    uint8_t         line[17];
    uint8_t         ch;
    const uint8_t  *ptr = buf;

    if (len > 0)
    {
        printfString(str, "Rest of data dump:\n");
        for (x = 0; x < len; ptr++, x++)
        {
            ch = (uint8_t)*ptr; 
            printfString(str, "%.2X ", ch);
            line[x % 16] = ch < '0' ? '.' : ch;
            if (x != 0 && (x+1) % 16 == 0 || x == len-1)
            {
                line[x % 16 + 1] = '\0';
                for (; (x+1) % 16 != 0; x++)
                {
                    printfString(str, "   ");
                }
                printfString(str, " %s\n", line);
            }
        }
        printfString(str, "\n");
    }
}

void dumpHexOutput(const uint8_t *buf, uint32_t len)
{
    string_t *str;
    str = initString();
    dumpHex(str, buf, len);
    writeOutput(VERBOSE_BASE, getString(str));
    freeString(str);
}

void dumpPacket(string_t *str, bsd_t bsd)
{
    flap_head_t flap;
    snac_head_t snac;
    uint32_t    len;

    /* dump the flap */
    bsReadFlapHead(bsd, &flap);
    dumpFlap(str, &flap);
    len = flap.len;

    DEBUG_PRINT("flap.len: %d", flap.len);

    if (flap.len >= SIZEOF_SNAC_HEAD && flap.channelId == YSM_CHANNEL_SNACDATA)
    {
        /* dump a snac if we have one */
        bsReadSnacHead(bsd, &snac);
        dumpSnac(str, &snac);
        len -= SIZEOF_SNAC_HEAD;
    }

    /* dump the rest of the data */
    if (len)
    {
        uint8_t *buf = YSM_MALLOC(len);

        if (buf)
        {
            bsRead(bsd, buf, len);
            dumpHex(str, buf, len);
            YSM_FREE(buf);
        }
    }
}
