#ifndef _NETWORK_H_
#define _NETWORK_H_

#include "slaves.h"
#include "bytestream.h"
#include "bs_oscar.h"
#include "ystring.h"

#define READ_SNAC(x, y)      y = readSnac((x), (y))
#define READ_TLV(x, y)       y = readTLV((x), (y))
#define READ_UINT8(x, y)     y = readUint8((x), (y))
#define READ_INT16(x, y)     y = readInt16((x), (y))
#define READ_UINT16LE(x, y)  y = readUint16LE((x), (y))
#define READ_UINT16(x, y)    y = readUint16((x), (y))
#define READ_UINT32LE(x, y)  y = readUint32LE((x), (y))
#define READ_UINT32(x, y)    y = readUint32((x), (y))
#define READ_STRING(x, y, l) y = readString((x), (y), (l))

int32_t initNetwork(void);

void sendUpdatePrivacy(uint8_t setting);

int YSM_LookupHN(char *Hostname, uint32_t *out);

int32_t YSM_LoginSequence(uin_t uin, int8_t *password); 
void YSM_Init_LoginA(uin_t uin, uint8_t *password);
void loginSendCookie(uint8_t *buff, uint16_t cookieLen);
void sendCapabilities(void); 
void sendCheckContactsReq(void);
void sendOfflineMsgsReq(void);
void YSM_RequestAutoMessage(slave_hnd_t *victim);
void sendDeleteOfflineMsgsReq(void);
void sendCliReady(void);
void sendICBMRightsReq(void);
void sendIcbmParams(void);
void sendBuddyRightsReq(void);
void YSM_RequestRates(void);

void YSM_BuddyIncomingList(bsd_t bsd);
void YSM_BuddyIncomingChange(snac_head_t *snac, bsd_t bsd);
void incomingFlapSnacData(flap_head_t *head, bsd_t bsd);
void incomingFlapCloseConnection(flap_head_t *head, bsd_t bsd);

int32_t YSM_ReceiveMessageType2Common(
    slave_hnd_t  *victim,
    int32_t   tsize,
    uint8_t  *data,
    uin_t     uin,
    uint16_t  status,
    int8_t   *m_id,
    uint8_t   m_flags,
    uint16_t *pseq,
    int8_t    dc_flag);

void sendAckType2(uin_t uin, uint16_t msgSeq, msg_type_t msgType, int8_t *msgId);
void YSM_SendContact(slave_hnd_t *victim, char *datalist, char *am);
void YSM_SendUrl(slave_hnd_t *victim, int8_t *url, int8_t *desc);
void forwardMessage(uin_t uin, char *msg);
void YSM_RequestPersonal(void);

void YSM_BuddyAck(void);
void YSM_BuddyRequestFinished(void);
void buddyReadSlave(char *buf); 
void incomingScanRsp(snac_head_t *snac);

int32_t sendMessage2Client(
    slave_hnd_t *victim,
    int16_t      msgFormat,
    msg_type_t   msgType,
    int8_t      *msgData,
    int32_t      msgLen,
    uint8_t      msgFlags,
    uint8_t      sendflags,
    req_id_t     reqId);

void YSM_SendContacts(void);
void YSM_BuddyDelSlave(slave_hnd_t *poorone);

int networkSignIn(void);
void serverResponseHandler(void);

int32_t YSM_ChangeStatus(uint16_t status);

int32_t YSM_BuddyAddItem(
    slave_hnd_t   *item,
    uint8_t   *grpname,
    uint16_t   grpid,
    uint16_t   bID,
    uint32_t   type,
    uint8_t    cmd,
    uint8_t    authawait,
    uint8_t    add_update);

int32_t YSM_Connect(int8_t *host, uint16_t port, int8_t verbose);
void    networkReconnect(void);

void sendKeepAlive(void);
void sendRemoveContactReq(uin_t uin);
void sendAuthReq(uin_t uin, uint8_t *nick, uint8_t *message);
void sendAuthRsp(slave_hnd_t *victim);
void sendSetBasicUserInfoReq(void);
int32_t sendMetaInfoReq(uin_t r_uin, int16_t subtype);
void sendFindByMailReq(uint8_t *contactMail);
void sendSetPasswordReq(uint8_t *newPassword);

void YSM_BuddyUploadList(slave_hnd_t *refugee);
void YSM_BuddyInvisible(slave_hnd_t *buddy, int flag);
void YSM_BuddyVisible(slave_hnd_t *buddy, int flag);
void YSM_BuddyIgnore(slave_hnd_t *buddy, int flag);

void YSM_War_Kill(slave_hnd_t *victim);
void YSM_War_Scan(slave_hnd_t *victim);
void YSM_SendRTF(slave_hnd_t *victim);

int8_t *readSnac(snac_head_t *tlv, const int8_t *buf);
int8_t *readTLV(tlv_t *tlv, const int8_t *buf);
int8_t *readUint8(uint8_t *uint8, const int8_t *buf);
int8_t *readInt16(int16_t *int16, const int8_t *buf);
int8_t *readUint16(uint16_t *uint16, const int8_t *buf);
int8_t *readUint16LE(uint16_t *uint16, const uint8_t *buf);
int8_t *readUint32(uint32_t *uint32, const int8_t *buf);
int8_t *readUint32LE(uint32_t *uint32, const uint8_t *buf);
int8_t *readString(int8_t *str, const int8_t *buf, int16_t len);

#endif /* _NETWORK_H_ */
