#ifndef _SLAVES_H_
#define _SLAVES_H_

#include "ysm.h"
#include "ylist.h"

typedef struct
{
    COMMON_LIST

    uin_t                  uin;
    uint32_t               fprint;
    uint8_t                caps;
    uint32_t               reqId;
    uint16_t               status;
    uint16_t               status_flags;
    uint8_t               *statusStr;
    uint8_t                flags;
    buddy_special_status_t budType;
    buddy_timing_t         timing;
    direct_connection_t    d_con;
    buddy_main_info_t      info;
    encryption_info_t      crypto;
} slave_t;
 
typedef enum {
    SLAVE_NICK,
    SLAVE_UIN,
    SLAVE_REQID
} slave_query_t;

typedef enum {
    UPDATE_SLAVE,
    UPDATE_NICK
} slave_update_t;

void YSM_PrintOrganizedSlaves(
    uint16_t  FilterStatus,
    int8_t   *Fstring,
    int8_t    FilterIgnore);

slave_t *getNextSlave(slave_t *slave);

slave_t *addSlaveToList(
    uint8_t   *nick,
    uin_t      uin,
    uint8_t    flags,
    uint8_t   *c_key,
    uint32_t   budId,
    uint32_t   grpId,
    uint16_t   budType);

void      deleteSlaveFromList(uin_t uin);
uint32_t  getSlavesListLen(void);
slave_t  *querySlave(slave_query_t type, int8_t *extra, uin_t uin, uint32_t reqId);
int32_t   YSM_ParseSlave(uint8_t *name);
int       updateSlave(slave_update_t type, uint8_t *data, uin_t r_uin);
void      freeSlaveList(void);

#endif /* _SLAVES_H_ */
