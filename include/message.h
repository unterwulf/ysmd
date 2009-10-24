#ifndef _MESSAGE_H_
#define _MESSAGE_H_

#include "ysm.h"
#include "slaves.h"
#include "ystring.h"

/* Definition of Message Types for Displaying functions */
typedef enum {
    YSM_MESSAGE_UNDEF    = 0x00,
    YSM_MESSAGE_NORMAL   = 0x01,
    YSM_MESSAGE_FILE     = 0x03,
    YSM_MESSAGE_URL      = 0x04,
    YSM_MESSAGE_AUTH     = 0x06,
    YSM_MESSAGE_AUTHNOT  = 0x07,
    YSM_MESSAGE_AUTHOK   = 0x08,
    YSM_MESSAGE_ADDED    = 0x0C,
    YSM_MESSAGE_PAGER    = 0x0D,
    YSM_MESSAGE_CONTACTS = 0x13,
    YSM_MESSAGE_GREET    = 0x1A,
    YSM_MESSAGE_GETAWAY  = 0xE8,
    YSM_MESSAGE_GETOCC   = 0xE9,
    YSM_MESSAGE_GETNA    = 0xEA,
    YSM_MESSAGE_GETDND   = 0xEB,
    YSM_MESSAGE_GETFFC   = 0xEC,
} msg_type_t;

typedef uint8_t msg_type_flags_t;
typedef uint8_t msg_flags_t;

typedef struct {
    slave_t            *sender;
    user_status_t       status;
    user_flags_t        statusFlags;
    msg_type_t          type;
    msg_type_flags_t    typeFlags;
    msg_flags_t         flags;
    uint8_t             id[8];
    uint16_t            seq;
    string_t           *data;
} msg_t;

#endif /* _MESSAGE_H_ */
