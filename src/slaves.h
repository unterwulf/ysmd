#ifndef _SLAVES_H_
#define _SLAVES_H_

#include "ysm.h"
#include "handle.h"

#define SLAVE_HND_FIND        -2
#define SLAVE_HND_START       -3
#define SLAVE_HND_END         -4
#define SLAVE_HND_NOT_A_SLAVE -10

typedef struct {
    hnd_t hnd;
    uin_t uin;
} slave_hnd_t;

int   initSlaveList(void);
void  freeSlaveList(void);
int   getNextSlave(slave_hnd_t *hnd);

int   addSlaveToList(
    uint8_t      *nick,
    uin_t         uin,
    sl_flags_t    flags,
    uint8_t      *c_key,
    uint32_t      budId,
    uint32_t      grpId,
    uint16_t      budType,
    slave_hnd_t  *newHnd);

void     deleteSlaveFromList(slave_hnd_t *hnd);
uint32_t getSlavesListLen(void);
int      querySlaveByUin(uin_t uin, slave_hnd_t *hnd);
int      querySlaveByNick(const uint8_t *nick, slave_hnd_t *hnd);
int      querySlaveByReqId(req_id_t reqId, slave_hnd_t *hnd);
int      getSlaveStatus(slave_hnd_t *hnd, sl_status_t *status);
int      setSlaveStatus(slave_hnd_t *hnd, sl_status_t status);
int      getSlaveCapabilities(slave_hnd_t *hnd, sl_caps_t *caps);
int      setSlaveCapabilities(slave_hnd_t *hnd, sl_caps_t caps);
int      getSlaveReqId(slave_hnd_t *hnd, req_id_t *reqId);
int      setSlaveReqId(slave_hnd_t *hnd, req_id_t reqId);
int      getSlaveFingerprint(slave_hnd_t *hnd, sl_fprint_t *fprint);
int      setSlaveFingerprint(slave_hnd_t *hnd, sl_fprint_t fprint);
int      getSlaveFlags(slave_hnd_t *hnd, sl_flags_t *flags);
int      setSlaveFlags(slave_hnd_t *hnd, sl_flags_t flags);
int      getSlaveSpecialStatus(slave_hnd_t *hnd, buddy_special_status_t *bss);
int      setSlaveSpecialStatus(slave_hnd_t *hnd, const buddy_special_status_t *bss);
int      getSlaveTiming(slave_hnd_t *hnd, buddy_timing_t *timing);
int      setSlaveTiming(slave_hnd_t *hnd, const buddy_timing_t *timing);
int      getSlaveDC(slave_hnd_t *hnd, direct_connection_t *dc);
int      setSlaveDC(slave_hnd_t *hnd, const direct_connection_t *dc);
int      getSlaveNick(slave_hnd_t *hnd, uint8_t *nick, uint8_t size);
int      setSlaveNick(slave_hnd_t *hnd, const uint8_t *nick);

#endif /* _SLAVES_H_ */
