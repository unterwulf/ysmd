#ifndef _YSM_H_
#define _YSM_H_

#include "config.h"
#include "compat.h"
#include "misc.h"
#include "bytestream.h"

#ifdef DEBUG

#define DEBUG_PRINT(fmt, args...) \
    printf("DEBUG: [%s:%d] %s " fmt "\n", __FILE__, __LINE__, __FUNCTION__, ## args)

#else

#define DEBUG_PRINT(fmt, args...)

#endif

#define MAX_PATH               256
#define MAX_SEQS_FILED         50
#define MAX_DATA_LEN           2547
#define MAX_MSGDATA_LEN        MAX_DATA_LEN
#define MAX_PEER_DATA_SIZE     2050
#define MAX_PEER_FILENAMEL     50
#define MAX_CRYPT_PADDING      16
#define MAX_PWD_LEN            12
#define MAX_NICK_LEN           30
#define MAX_UIN_LEN            12
#define MAX_CMD_NAME           15
#define MAX_CMD_ARGS           MAX_DATA_LEN+2
#define MAX_EXEC_ARGS          50
#define MAX_CMD_LEN            MAX_DATA_LEN
#define MAX_CMD_HIST           15
#define MAX_LINE_LEN           80
#define MAX_LOGSEP_LEN         5
#define MAX_STATUS_LEN         50
#define MAX_CONTACTS_SEND      15
#define MAX_CHARSET            10
#define MAX_SAVE_COUNT         5
#define MAX_SCAN_COUNT         2
#define MAX_TIME_LEN           21    /* dd mmm yyyy hh:mm:ss + NUL */
#define MAX_SLAVELIST_LEN      8192

/* Required for log files parsing watch out for MAX_LOGSEP_LEN  */
#define YSM_LOG_SEPARATOR      "$YSM$"

#define YSM_CHAT_MESSAGE       "I'm busy in a conversation. Your message has been logged. I'll get back to you ASAP."

#define NOT_A_SLAVE            0
/* cast to uint8_t* is needed to avoid compiler warning */

/* Authorization Request definitions */

#define YSM_DEFAULT_AUTHREQ_MESSAGE    "Hi. I require your authorization in order to add you to my contacts list."


#define YSM_BUDDY_GROUPNAME    "YSM"
#define YSM_BUDDY_GROUPID      0x1337

#define YSM_BUDDY_SLAVE        0x0000
#define YSM_BUDDY_SLAVE_VIS    0x0002
#define YSM_BUDDY_SLAVE_INV    0x0003
#define YSM_BUDDY_SLAVE_IGN    0x000e

#define YSM_BUDDY_GROUP        0x0001


/* Debug Level Definitions! */
#define VERBOSE_BASE        0
#define VERBOSE_STCHANGE    1
#define VERBOSE_CONNINFO    2
#define VERBOSE_DCON        20
#define VERBOSE_MOATA       21
#define VERBOSE_PACKET      22
#define VERBOSE_SDOWNLOAD   23


#define FINGERPRINT_YSM_CLIENT          0xabffffff
#define FINGERPRINT_YSM_CLIENT_CRYPT    0xdeadbabe
#define FINGERPRINT_TRILLIAN_CLIENT     0x3b75ac09
#define FINGERPRINT_LIB2K_CLIENT        0x3aa773ee
#define FINGERPRINT_MICQ2003A_CLIENT_1  0x3CF02D22
#define FINGERPRINT_MICQ2003A_CLIENT_2  0x3E76502C
#define FINGERPRINT_LICQ_CLIENT         0x7d000000
#define FINGERPRINT_MICQ_CLIENT         0xffffff42
#define FINGERPRINT_MIRANDA_CLIENT      0xffffffff
#define FINGERPRINT_STRICQ_CLIENT       0xffffff8f

/* the following clients are identified by capabilities */
#define FINGERPRINT_SIMICQ_CLIENT       0x04040404
#define FINGERPRINT_M2000_CLIENT        0x06060606
#define FINGERPRINT_M20012_CLIENT       0x07070707
#define FINGERPRINT_M2002_CLIENT        0x08080808
#define FINGERPRINT_MICQLITE_CLIENT     0x09090909
#define FINGERPRINT_ICQ2GO_CLIENT       0x0a0a0a0a
#define FINGERPRINT_MISC_CLIENT         0x00

#define SLAVES_TAG    "SLAVES"

#define STATUS_ONLINE      0x0000
#define STATUS_OFFLINE     0xffff
#define STATUS_INVISIBLE   0x0100
#define STATUS_NA          0x0005
#define STATUS_NA2         0x0004
#define STATUS_DND         0x0013
#define STATUS_OCCUPIED    0x0011
#define STATUS_FREE_CHAT   0x0020
#define STATUS_AWAY        0x0001
#define STATUS_UNKNOWN     0xdead

#define STATUS_FLWEBAWARE  0x0001
#define STATUS_FLIPSHOW    0x0002
#define STATUS_FLBIRTHDAY  0x0008
#define STATUS_FLDC_AUTH   0x1000
#define STATUS_FLDC_CONT   0x2000

#define INFO_MAIN          0x01
#define INFO_HP            0x02
#define INFO_WORK          0x03
#define INFO_ABOUT         0x04

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

/* Definition of ERROR standards */
#define ERROR_CODE        0
#define ERROR_CODE_M      "There has been a code ERROR. Debug!.\n"
#define ERROR_NETWORK     1
#define ERROR_NETWORK_M   "There has been a Network Disconnection!.\n"
#define ERROR_CRITICAL    2
#define ERROR_CRITICAL_M  "There has been a CRITICAL ERROR. duck!\n"

typedef int32_t  uin_t;
typedef int8_t   bool_t;
typedef uint16_t sl_status_t;
typedef uint8_t  sl_flags_t;
typedef uint8_t  sl_caps_t;
typedef uint32_t sl_fprint_t;
typedef uint32_t req_id_t;


typedef struct
{
    time_t signOn;
    time_t lastMessage;
    time_t statusChange;
} buddy_timing_t;

typedef struct
{
    uint8_t nickName[MAX_NICK_LEN+1];
    uint8_t firstName[MAX_NICK_LEN+1];
    uint8_t lastName[MAX_NICK_LEN+1];
    uint8_t email[MAX_NICK_LEN+1];
    uint8_t city[MAX_NICK_LEN+1];
    uint8_t state[MAX_NICK_LEN+1];
    uint8_t gender;
    uint8_t age;
} buddy_main_info_t;

typedef struct
{
    int32_t  rSocket;
    int8_t   authHost[MAX_PATH];
    int8_t   cookieHost[MAX_PATH];
    uint16_t authPort;
    uint16_t cookiePort;
} network_connection_t;

struct YSM_DIRECT_CONNECTION_FILETINFO
{
    uint32_t size;
    uint32_t totsize;
    int8_t   name[MAX_PEER_FILENAMEL];
    uint16_t num;
    uint16_t totnum;
    uint16_t rPort;
    int32_t  speed;
    int32_t  kbs;
    time_t   statstime;
    int32_t  statsbytes;
    int32_t  rSocket;
    uint16_t lPort;
#define DC_TRANSFERTIMEOUT    30    /* seconds */
    FILE     *fd;
};

typedef struct
{
    uint16_t                seq_in;
    uint16_t                seq_out;
    int32_t                 rSocket;
    uint16_t                rPort;
    uint32_t                rIP_int;
    uint32_t                rIP_ext;
    uint16_t                version;
    int32_t                 rCookie;
#define DC_EXPECTDATA   0x01
#define DC_EXPECTNEG    0x02
#define DC_OUTGOINGNEG  0x04
#define DC_INCOMINGNEG  0x08
#define DC_CONNECTED    0x10
#define DC_TRANSFERING  0x20
#define DC_RECEIVING    0x40
#define DC_ACTIVITY     0x80

    uint8_t                 flags;
    struct YSM_DIRECT_CONNECTION_FILETINFO    finfo;
} direct_connection_t;

typedef struct
{
    int8_t  birthday;
    int32_t ignoreId;
    int32_t visibleId;
    int32_t invisibleId;
    int32_t budId;
    int32_t grpId;
} buddy_special_status_t;

#define YSM_PROXY_HTTPS      0x01
#define YSM_PROXY_AUTH       0x02
#define YSM_PROXY_RESOLVE    0x04

typedef struct
{
    uint8_t   username[MAX_PATH];
    uint8_t   password[MAX_PATH];
    uint8_t   host[MAX_PATH];
    uint16_t  port;
    uint8_t   flags;
} proxy_info_t;

#define FL_LOGGEDIN          0x01
#define FL_DOWNLOADEDSLAVES  0x02

typedef struct
{
    uint16_t  seqnum;

    /* queued slave scans */
    int32_t   scanqueue;

    /* buddy list information */
    int32_t   blentries;
    int32_t   blgroupsidentries;
    bsd_t     blgroupsid;
    int32_t   blusersidentries;
    bsd_t     blusersid;
    uint32_t  blysmgroupid;
    uint16_t  blprivacygroupid;
    uint8_t   flags;
} ysm_server_info_t;

typedef struct
{
#define MAX_KEY_LEN    64 /* 512 bits */
    int8_t       strkey[MAX_KEY_LEN+1];
    keyInstance  key_out;
    keyInstance  key_in;
} encryption_info_t;

typedef struct
{
    uin_t                uin;
    uint16_t             status;
    uint16_t             status_flags;
    int8_t               password[MAX_PWD_LEN];
    uint8_t              flags;
    time_t               delta;

    buddy_main_info_t    info;
    buddy_timing_t       timing;
    proxy_info_t         proxy;
    direct_connection_t  d_con;
    network_connection_t network;
} ysm_model_t;

/* list types declaration */

#define FL_SCANNED       0x01
#define FL_CHAT          0x08
#define FL_DOWNLOADED    0x10
#define FL_AUTHREQ       0x20

#define IS_DOWNLOADED(x) ((x) & FL_DOWNLOADED)

typedef struct {
    uint8_t   verbose;
    uint8_t   awaytime;
    uint8_t   spoof;
    uint8_t   antisocial;
    uint8_t   updateNicks;
    uint8_t   dcdisable;
    uint8_t   dclan;
    uint16_t  dcport1;
    uint16_t  dcport2;
    uin_t     forward;
    char      charsetTrans[MAX_CHARSET + 4];
    char      charsetLocal[MAX_CHARSET + 4];

    int8_t    CHATMessage[MAX_DATA_LEN+1];
    enum {OT_FIFO, OT_STDIN, OT_STDOUT} outputType;
    int8_t    outputPath[MAX_DATA_LEN+1];
} ysm_config_t;

typedef struct {
    uint8_t  reasonToSuicide;
    uint8_t  reconnecting;
    uin_t    lastRead;
    uin_t    lastSent;
    uint8_t  lastMessage[MAX_DATA_LEN + 1];
    uint8_t  lastUrl[MAX_DATA_LEN + 1];
    uint8_t  configFile[MAX_PATH];
    uint8_t  slavesFile[MAX_PATH];
    uint8_t  configDir[MAX_PATH];

#define FL_OVERWRITTEN  0x01
#define FL_RAW          0x02
#define FL_BUSYDISPLAY  0x04
#define FL_COMFORTABLEM 0x08
#define FL_CHATM        0x20
#define FL_AUTOAWAY     0x40

    uint8_t  promptFlags;
} ysm_state_t;

/* exports */
extern ysm_model_t           YSM_USER;
extern ysm_server_info_t     g_sinfo;
extern ysm_config_t          g_cfg;
extern ysm_state_t           g_state;
extern sem_t                 semOutput;
#ifdef YSM_TRACE_MEMLEAK
extern uint16_t              unfreed_blocks;
#endif

#endif /* _YSM_H_ */
