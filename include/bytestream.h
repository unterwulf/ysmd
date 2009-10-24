#ifndef _BYTESTREAM_H_
#define _BYTESTREAM_H_

#define BS_FL_BADBSD   0x01
#define BS_FL_INVALID  0x02

typedef int16_t bsd_t;
typedef int32_t bs_pos_t;
typedef enum {BS_SEEK_SET, BS_SEEK_CUR, BS_SEEK_END} bs_seek_t;

bsd_t         initBs(void);
void          freeBs(bsd_t);
uint8_t       bsGetFlags(bsd_t);
const int8_t *bsGetBuf(bsd_t bsd);
uint32_t      bsGetLen(bsd_t bsd);
bs_pos_t      bsTell(bsd_t bsd);
bs_pos_t      bsSeek(bsd_t bsd, uint32_t offset, bs_seek_t whence);
void          bsRewind(bsd_t bsd);
bs_pos_t      bsAppend(bsd_t bsd, const uint8_t *buf, uint32_t len);
uint32_t      bsRead(bsd_t bsd, uint8_t *buf, uint32_t len);
bs_pos_t      bsUpdate(bsd_t bsd, bs_pos_t pos, const uint8_t *buf, uint32_t len);
uint32_t      bsAppendFromSocket(bsd_t bsd, int fd, size_t count);

#endif /* _BYTESTREAM_H_ */
