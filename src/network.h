#ifndef _NETWORK_H_
#define _NETWORK_H_

#include "slaves.h"
#include "bytestream.h"
#include "bs_oscar.h"
#include "ystring.h"
#include "message.h"

int32_t initNetwork(void);

void sendUpdatePrivacy(uint8_t setting);

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

void YSM_BuddyIncomingList(bsd_t bsd);
void YSM_BuddyIncomingChange(snac_head_t *snac, bsd_t bsd);
void incomingFlapSnacData(oscar_msg_t *msg);
void incomingFlapCloseConnection(oscar_msg_t *msg);

int32_t YSM_ReceiveMessageType2Common(bsd_t bsd, msg_t *msg);

void YSM_SendContact(slave_t *victim, char *datalist, char *am);
void YSM_SendUrl(slave_t *victim, int8_t *url, int8_t *desc);
void YSM_RequestPersonal(void);

void YSM_BuddyRequestFinished(void);
void incomingScanRsp(snac_head_t *snac);

int32_t sendMessage2Client(
    slave_t *victim,
    int16_t      msgFormat,
    msg_type_t   msgType,
    int8_t      *msgData,
    int32_t      msgLen,
    uint8_t      msgFlags,
    uint8_t      sendflags,
    req_id_t     reqId);

void YSM_SendContacts(void);
void YSM_BuddyDelSlave(slave_t *poorone);

int networkSignIn(void);
void serverResponseHandler(void);

int changeStatus(user_status_t status);

int32_t YSM_BuddyAddItem(
    slave_t   *item,
    uint8_t   *grpname,
    uint16_t   grpid,
    uint16_t   bID,
    uint32_t   type,
    uint8_t    cmd,
    uint8_t    authawait,
    uint8_t    add_update);

int  YSM_Connect(int8_t *host, uint16_t port, int8_t verbose);
void networkReconnect(void);

void sendKeepAlive(void);
void sendRemoveContactReq(uin_t uin);
void sendAuthReq(uin_t uin, uint8_t *nick, uint8_t *message);
void sendAuthRsp(uin_t uint);
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

#endif /* _NETWORK_H_ */
