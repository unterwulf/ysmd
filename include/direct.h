#ifndef _DIRECT_H_
#define _DIRECT_H_

#include "slaves.h"
#include "message.h"

#define MFLAGTYPE_NORM        0x01
#define MFLAGTYPE_ACK         0x02
#define MFLAGTYPE_END         0x04
#define MFLAGTYPE_CRYPTNORM   0x08
#define MFLAGTYPE_CRYPTACK    0x10
#define MFLAGTYPE_UTF8        0x20
#define MFLAGTYPE_UCS2BE      0x40
#define MFLAGTYPE_RTF         0x80

int     YSM_EncryptDCPacket(unsigned char *buf, int buf_len);
int     YSM_DecryptDCPacket(unsigned char *buf, unsigned int buf_len);
void    YSM_OpenDC(slave_t *victim);
void    YSM_CloseDC(slave_t *victim);
void    YSM_CloseTransfer(slave_t *victim);
void    dcSelect(void);
int     YSM_DC_ReadPacket(int cli_sock, char *buf);
uin_t   YSM_DC_Wait4Client(void);
int32_t YSM_DC_CommonResponse(slave_t *victim, int32_t sock );
int32_t YSM_DC_CommonResponseFile(slave_t *victim, int32_t sock, uint8_t *buf, int32_t rlen);
int32_t YSM_DC_CommonResponseMisc(slave_t *victim, int32_t sock, uint8_t *buf, int32_t rlen);
int32_t YSM_DC_CommonResponseNeg(slave_t *victim, int32_t sock, uint8_t *buf, int32_t len);
int32_t YSM_DC_Message(slave_t *victim, char *_msg, int dlen, int8_t type);
int32_t YSM_DC_MessageACK(slave_t *victim, uint8_t type);
int32_t YSM_DC_IncomingMessageFILE(msg_t *msg);
int32_t YSM_DC_IncomingMessageGREET(msg_t *msg);
int32_t YSM_DC_FileB(slave_t *victim, char *filename, char *reason);
int32_t YSM_DC_FileDecline(slave_t *victim, int8_t *reason);
int32_t YSM_DC_File(slave_t *victim, int8_t *fname, int8_t *desc);
int32_t YSM_DC_FileInit(slave_t *victim, int32_t numfiles, int32_t numbytes);
int32_t YSM_DC_FileInitAck(slave_t *victim, int32_t sock, int8_t *buf, int32_t len);
int32_t YSM_DC_FileStart(slave_t *victim );
int32_t YSM_DC_FileStartAck(slave_t *victim, int32_t sock, int8_t *buf, int32_t len);
int32_t YSM_DC_FileSpeedAck(slave_t *victim, int8_t *buf, int32_t len);
int32_t YSM_DC_FileTransfer(slave_t *victim, int8_t *buf, int32_t len);
int32_t YSM_DC_FileReceive(slave_t *victim, int8_t *buf, int32_t len);

int     initDC(void);

#endif /* _DIRECT_H_ */
