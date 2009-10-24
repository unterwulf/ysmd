#ifndef _NETWORK_H_
#define _NETWORK_H_

#include "slaves.h"
#include "bytestream.h"
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

typedef struct
{
    uint16_t familyId;
    uint16_t subTypeId;
    uint16_t flags;
    uint32_t reqId;
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

int32_t initNetwork(void);

void sendUpdatePrivacy(uint8_t setting);

int YSM_LookupHN(char *Hostname, uint32_t *out);

int32_t YSM_LoginSequence(uin_t uin, int8_t *password); 
void YSM_Init_LoginA(uin_t uin, uint8_t *password);
void loginSendCookie(uint8_t *buff, uint16_t cookieLen);
void sendCapabilities(void); 
void sendCheckContactsReq(void);
void sendOfflineMsgsReq(void);
void YSM_RequestAutoMessage(slave_t *victim);
void sendDeleteOfflineMsgsReq(void);
void sendCliReady(void);
void sendICBMRightsReq(void);
void sendIcbmParams(void);
void sendBuddyRightsReq(void);
void YSM_RequestRates(void);

void YSM_IncomingMultiUse(flap_head_bit_t *head, snac_head_t *snac, char *buf);
void YSM_BuddyIncomingList(bsd_t bsd);
void YSM_BuddyIncomingChange(snac_head_t *snac, bsd_t bsd);
void YSM_Incoming_ClientAck(flap_head_bit_t *flap, int8_t *buf);
void incomingFlapSnacData(flap_head_t *head, bsd_t bsd);
void incomingFlapCloseConnection(flap_head_t *head, bsd_t bsd);
void YSM_ReceiveMessage(flap_head_bit_t *head, int8_t *data);

int32_t YSM_ReceiveMessageType2Common(
    slave_t  *victim,
    int32_t   tsize,
    uint8_t  *data,
    uin_t     uin,
    uint16_t  status,
    int8_t   *m_id,
    uint8_t   m_flags,
    uint16_t *pseq,
    int8_t    dc_flag);

void sendAckType2(uin_t uin, uint16_t msgSeq, msg_type_t msgType, int8_t *msgId);
void YSM_SendContact(slave_t *victim, char *datalist, char *am);
void YSM_SendUrl(slave_t *victim, int8_t *url, int8_t *desc);
void forwardMessage(uin_t uin, char *msg);
void YSM_IncomingPersonal(flap_head_bit_t *head, int8_t *buf);
void YSM_RequestPersonal(void);
void YSM_BuddyChangeStatus(flap_head_bit_t *flap, snac_head_t *snac, int8_t *data);

void YSM_BuddyParseStatus(
    slave_t    *victim, /* IN */
    flap_head_bit_t  *flap,
    int8_t     *data,
    int32_t     pos,
    direct_connection_t *dcinfo,
    uint32_t  *fprint,
    uint16_t  *status,
    uint16_t  *flags,
    time_t     *onsince,
    int8_t    **statusStr);

void YSM_BuddyUpdateStatus(slave_t *victim,
    direct_connection_t *dcinfo,
    uint16_t  status,
    uint16_t  flags,
    uint32_t  fprint,
    time_t     onsince,
    int8_t    *statusStr);

void YSM_IncomingInfo(
    char type,
    char *buf,
    int tsize,
    unsigned int reqid);

void YSM_IncomingMainInfo(
    int8_t *buf,
    int32_t tsize,
    int8_t *pnick,
    int8_t *pfirst,
    int8_t *plast,
    int8_t *pemail,
    uint32_t reqid,
    uin_t *puin);

void YSM_IncomingHPInfo(int8_t *buf, int32_t tsize);
void YSM_IncomingWorkInfo(int8_t *buf, int32_t tsize);
void YSM_IncomingAboutInfo(int8_t *buf, int32_t tsize);
void YSM_IncomingSearch(char *buf, int tsize);
void YSM_BuddyAck(void);
void YSM_BuddyRequestModify(void);
void YSM_BuddyRequestFinished(void);
void buddyReadSlave(char *buf); 
void incomingScanRsp(snac_head_t *snac);

int32_t sendMessage2Client(
    slave_t    *victim,
    uin_t       uin,
    int16_t     msgFormat,
    msg_type_t  msgType,
    int8_t     *msgData,
    int32_t     msgLen,
    uint8_t     msgFlags,
    uint8_t     sendflags,
    uint32_t    reqId);

void bsAppendMessageHead(
    bsd_t      bsd,
    uin_t      uin,
    int16_t    msgFormat,
    uint32_t   msgTime,
    uint32_t   msgId);

void bsAppendMessageBodyType1(
    bsd_t      bsd,
    slave_t   *victim,
    int8_t    *msgData,
    int32_t    msgLen);

void bsAppendMessageBodyType2(
    bsd_t       bsd,
    slave_t    *victim,
    int8_t     *msgData,
    int32_t     msgLen,
    msg_type_t  msgType,
    uint32_t    msgTime,
    uint32_t    msgId,
    uint8_t     msgFlags,
    uint8_t     sendflags);

void bsAppendMessageBodyType4(
    bsd_t       bsd,
    int8_t     *msgData,
    int32_t     msgLen,
    msg_type_t  msgType,
    uint8_t     msgFlags);

void YSM_SendContacts(void);
void YSM_BuddyDelSlave(slave_t *poorone);

int networkSignIn(void);
void serverResponseHandler(void);

int32_t YSM_ChangeStatus(uint16_t status);

int32_t YSM_BuddyAddItem(
    slave_t   *item,
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
void sendAuthRsp(uin_t uin, uint8_t *nick);
void sendSetBasicUserInfoReq(void);
int32_t sendMetaInfoReq(uin_t r_uin, int16_t subtype);
void sendFindByMailReq(uint8_t *contactMail);
void sendSetPasswordReq(uint8_t *newPassword);

void YSM_BuddyUploadList(slave_t *refugee);
void YSM_BuddyInvisible(slave_t *buddy, int flag);
void YSM_BuddyVisible(slave_t *buddy, int flag);
void YSM_BuddyIgnore(slave_t *buddy, int flag);

void YSM_War_Kill(slave_t *victim);
void YSM_War_Scan(slave_t *victim);
void YSM_SendRTF(slave_t *victim);

int8_t *readSnac(snac_head_t *tlv, const int8_t *buf);
int8_t *readTLV(tlv_t *tlv, const int8_t *buf);
int8_t *readUint8(uint8_t *uint8, const int8_t *buf);
int8_t *readInt16(int16_t *int16, const int8_t *buf);
int8_t *readUint16(uint16_t *uint16, const int8_t *buf);
int8_t *readUint16LE(uint16_t *uint16, const uint8_t *buf);
int8_t *readUint32(uint32_t *uint32, const int8_t *buf);
int8_t *readUint32LE(uint32_t *uint32, const uint8_t *buf);
int8_t *readString(int8_t *str, const int8_t *buf, int16_t len);

#endif
