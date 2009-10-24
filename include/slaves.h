#ifndef _SLAVES_H_
#define _SLAVES_H_

#include "ysm.h"
#include "ylist.h"

typedef struct
{
    COMMON_LIST

    uin_t                  uin;
    sl_fprint_t            fprint;
    sl_caps_t              caps;
    req_id_t               reqId;
    sl_status_t            status;
    sl_status_t            status_flags;
    char                  *statusStr;
    sl_flags_t             flags;
    buddy_special_status_t budType;
    buddy_timing_t         timing;
    direct_connection_t    d_con;
    buddy_main_info_t      info;
    encryption_info_t      crypto;
} slave_t;

typedef enum
{
    UPDATE_NICK,
    UPDATE_SLAVE
} slave_update_t;

int   initSlaveList(void);
void  freeSlaveList(void);
void  lockSlaveList(void);
void  unlockSlaveList(void);

slave_t *addSlaveToList(
    char         *nick,
    uin_t         uin,
    sl_flags_t    flags,
    uint8_t      *c_key,
    uint32_t      budId,
    uint32_t      grpId,
    uint16_t      budType);

void     deleteSlaveFromList(slave_t *victim);
long     getSlavesListLen(void);
int      updateSlave(slave_update_t type, char *nick, uin_t uin);

slave_t *getSlaveByUin(uin_t uin);
slave_t *getSlaveByNick(const char *nick);
slave_t *getSlaveByReqId(req_id_t reqId);
slave_t *getNextSlave(const slave_t *slave);

#endif /* _SLAVES_H_ */
