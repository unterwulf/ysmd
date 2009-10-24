#define _BYTESTREAM_C_

#include "ysm.h"
#include "wrappers.h"
#include "bytestream.h"
#include "bytestream_int.h"

static bs_t *g_bsd_pool[MAX_BSD_NUM];

static bs_t *getBs(bsd_t bsd)
{
    static bool_t initialized = FALSE;

    if (!initialized)
    {
        for (bsd = 0; bsd < MAX_BSD_NUM; bsd++)
            g_bsd_pool[bsd] = NULL;
        initialized = TRUE;
        return NULL;
    }

    return g_bsd_pool[bsd];
}

bsd_t initBs()
{
    bsd_t  bsd;
    bs_t  *bs;

    for (bsd = 0; bsd < MAX_BSD_NUM; bsd++)
    {
        if (!(bs = getBs(bsd)))
        {
            bs = (bs_t *)YSM_MALLOC(sizeof(bs_t));
            if (bs != NULL)
            {
                bs->data = NULL;
                bs->ptr = 0;
                bs->end = 0;
                bs->len = 0;
                bs->flags = 0;
                g_bsd_pool[bsd] = bs;
                return bsd;
            }
            else
                return -1;
        }
    }

    return -1;
}

void freeBs(bsd_t bsd)
{
    if (!getBs(bsd))
        return;

    if (g_bsd_pool[bsd]->data != NULL)
        YSM_FREE(g_bsd_pool[bsd]->data);

    YSM_FREE(g_bsd_pool[bsd]);
    g_bsd_pool[bsd] = NULL;
}

uint8_t bsGetFlags(bsd_t bsd)
{
    bs_t *bs;

    if (!(bs = getBs(bsd)))
        return BS_FL_BADBSD;

    return bs->flags;
}

const int8_t *bsGetBuf(bsd_t bsd)
{
    bs_t *bs;

    if (!(bs = getBs(bsd)))
        return NULL;

//    DEBUG_PRINT("bsd: %d, addr: %X", bsd, bs->data);

    return bs->data;
}

uint32_t bsGetLen(bsd_t bsd)
{
    bs_t *bs;

    if (!(bs = getBs(bsd)))
        return 0;

//    DEBUG_PRINT("bsd: %d, len: %d (allocated: %d)",
//        bsd, bs->end, bs->len);

    return bs->end;
}

bs_pos_t bsTell(bsd_t bsd)
{
    bs_t *bs;

    if (!(bs = getBs(bsd)))
        return 0;

    return bs->ptr;
}

bs_pos_t bsSeek(bsd_t bsd, uint32_t offset, bs_seek_t whence)
{
    bs_t *bs;

    if (!(bs = getBs(bsd)))
        return 0;

    switch (whence)
    {
        case BS_SEEK_SET:
            if (offset <= bs->len)
                bs->ptr = offset;
            break;

        case BS_SEEK_CUR:
            if (bs->ptr + offset <= bs->len)
                bs->ptr += offset;
            else
                ; // TODO: bs should be extended
            break;

        case BS_SEEK_END:
            break;
    }

    return bs->ptr;
}

void bsRewind(bsd_t bsd)
{
    bs_t *bs;

    if (!(bs = getBs(bsd)))
        return;

    bs->ptr = 0;
}

static bool_t reallocBsData(bs_t *bs, size_t newLen)
{
    int8_t *newData;
    size_t  offset;

//    DEBUG_PRINT("newLen: %d", newLen);

    if (bs == NULL)
    {
        return FALSE;
    }
    else if ((newData = YSM_REALLOC(bs->data, newLen)) != NULL)
    {
        if (bs->end > newLen - 1)
            bs->end = newLen - 1;
        if (bs->ptr > newLen - 1)
            bs->ptr = newLen - 1;
        bs->data = newData;
        bs->len = newLen;
        return TRUE;
    }

    bs->flags |= BS_FL_INVALID;

    return FALSE;
}

static bool_t reallocBsDataIfNeeded(bs_t *bs, size_t extraLen)
{
    uint32_t reqLen;
    uint32_t newLen;

    reqLen = bs->end + extraLen;

//    DEBUG_PRINT("bsd: %d, len: %d, allocated: %d, used: %d, required: %d",
//        bsd, len, bs->len, bs->end, reqLen);

    if (reqLen > bs->len)
    {
        if (bs->len == 0)
            bs->len = BS_INIT_LEN;
        newLen = ((reqLen / bs->len) + 1)*bs->len;
        if (!reallocBsData(bs, newLen))
        {
            return FALSE;
        }
    }

    return TRUE;
}

bs_pos_t bsAppend(bsd_t bsd, const uint8_t *buf, uint32_t len)
{
    bs_t *bs;

    if (!(bs = getBs(bsd)) || !buf)
        return -1;

    if (!reallocBsDataIfNeeded(bs, len))
        return -1;

    memcpy(bs->data + bs->end, buf, len);
    bs->end += len;

    return bs->end - len; /* offset where buf was written to */
}

uint32_t bsAppendFromSocket(bsd_t bsd, int fd, size_t count)
{
    bs_t *bs;
    size_t bytesRead;

    if (!(bs = getBs(bsd)))
        return -1;

    if (!reallocBsDataIfNeeded(bs, count))
        return -1;

    bytesRead = SOCK_READ(fd, (void *)(bs->data + bs->end), count);

    if (bytesRead > 0)
        bs->end += bytesRead;

    return bytesRead; /* size of data was read */
}

uint32_t bsRead(bsd_t bsd, uint8_t *buf, uint32_t len)
{
    bs_t *bs;

    if (!(bs = getBs(bsd)) || buf == NULL)
        return -1;

    if (bs->ptr + len > bs->end)
        len = bs->end - bs->ptr;

    memcpy(buf, bs->data + bs->ptr, len);
    bs->ptr += len;

    return len; /* new offset */
}

bs_pos_t bsUpdate(bsd_t bsd, bs_pos_t pos, const uint8_t *buf, uint32_t len)
{
    bs_t *bs;
        DEBUG_PRINT("");

    if (!(bs = getBs(bsd)) || buf == NULL)
        return -1;

    if (pos + len > bs->end)
        return -1;

    memcpy(bs->data + pos, buf, len);

    return pos + len;
}
