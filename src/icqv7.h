#ifndef _ICQV7_H_
#define _ICQV7_H_

#define FLAP_HEAD_SIZE 6     /* unaligned FLAP header size */
#define SNAC_HEAD_SIZE 10    /* unaligned SNAC header size */

#define YSM_PROTOCOL_VERSION    0x08

/* Definicion de los tipos de Mensaje */

#define AUTH_MESSAGE            0x0008
#define USER_ADDED_MESS         0x000C
#define AUTH_REQ_MESS           0x0006
#define URL_MESS                0x0004
#define WEB_MESS                0x000d
#define EMAIL_MESS              0x000e
#define MASS_MESS_MASK          0x8000
#define MRURL_MESS              0x8004
#define NORM_MESS               0x0001
#define MRNORM_MESS             0x8001
#define CONTACT_MESS            0x0013
#define MRCONTACT_MESS          0x8013


/* Definicion de los Canales de habla (?) JA! */

#define YSM_CHANNEL_NEWCON      0x01
#define YSM_CHANNEL_SNACDATA    0x02
#define YSM_CHANNEL_FLAPERR     0x03
#define YSM_CHANNEL_CLOSECONN   0x04
#define YSM_CHANNEL_CONNALIVE   0x05


/* Capabilities */

#define CAP_PRECAP    "\x09\x46\x13"
#define CAP_PRERTF    "\x97\xb1\x27"
#define CAP_POSCAP    "\x4c\x7f\x11\xd1\x82\x22\x44\x45\x53\x54\x00\x00"
#define CAP_POSRTF    "\x24\x3c\x43\x34\xad\x22\xd6\xab\xf7\x3f\x14\x92"

#define CAP_SRVRELAY  "\x49"
#define CAP_ISICQ     "\x44"
#define CAP_UTF8      "\x4e"
#define CAP_RTF       "\x51"

#define CAP_UTF8_GUID "{0946134E-4C7F-11D1-8222-444553540000}"
#define CAP_RTF_GUID  "{97B12751-243C-4334-AD22-D6ABF73F1492}"

/* Fingerprinting Capabilities */

#define CAP_M2001    "\x2e\x7a\x64"                                    "\x75"                                        "\xfa\xdf\x4d\xc8\x88\x6f\xea\x35\x95\xfd\xb6\xdf"

#define CAP_M2001_2    "\xa0\xe9\x3f"                                    "\x37"                                        "\x4f\xe9\xd3\x11\xbc\xd2\x00\x04\xac\x96\xdd\x96"

#define CAP_M2002    "\x10\xcf\x40"                                    "\xd1"                                        "\x4f\xe9\xd3\x11\xbc\xd2\x00\x04\xac\x96\xdd\x96"

#define CAP_MLITE    "\x56\x3f\xc8"                                    "\x09"                                        "\x0b\x6f\x41\xbd\x9f\x79\x42\x26\x09\xdf\xa2\xf3"

#define CAP_SIMICQ    "\x97\xb1\x27"                                    "\x51"                                        "\x24\x3c\x43\x34\xad\x22\xd6\xab\xf7\x3f\x14\x48"

#define CAP_MICQ    "mICQ \xa9 R.K. \x00\x00\x00\x00"

#define CAP_TRILL_NORM    "\x97\xb1\x27"                                    "\x51"                                        "\x24\x3c\x43\x34\xad\x22\xd6\xab\xf7\x3f\x14\x09"

#define CAP_TRILL_CRYPT    "\xf2\xe7\xc7"                                    "\xf4"                                        "\xfe\xad\x4d\xfb\xb2\x35\x36\x79\x8b\xdf\x00\x00"

#define CAP_LICQ    "\x09\x49\x13"                                    "\x49" CAP_POSCAP


enum CAP_FLAGS
{
    CAPFL_SRVRELAY = 1,
    CAPFL_ISICQ    = 2,
    CAPFL_UTF8     = 4,
    CAPFL_RTF      = 8
};

/* Definicion de las familias de SNACs y Sub IDs en un arbol */

#define YSM_BASIC_SERVICE_SNAC      0x01
    #define YSM_CLI_SRV_ERROR       0x01
    #define CLI_READY               0x02
    #define YSM_SERVER_IS_READY     0x03
    #define YSM_REQUEST_SERVICE     0x04
    #define YSM_SRV_IRECT           0x05
    #define YSM_RATE_INFO_REQ       0x06
    #define YSM_RATE_INFO_RESP      0x07
    #define YSM_RATE_INFO_ACK       0x08
    #define YSM_RATE_INFO_CHANGE    0x0a
    #define YSM_SERVER_PAUSE        0x0b
    #define YSM_SERVER_RESUME       0x0c
    #define YSM_SCREEN_INFO_REQ     0x0e
    #define YSM_SCREEN_INFO_RESP    0x0f
    #define YSM_STATUS_CHANGE_ACK   0x0f
    #define YSM_SERVER_EVIL_NOT     0x10
    #define YSM_SERVER_MIGRATE      0x12
    #define YSM_SERVER_MOTD         0x13
    #define YSM_SET_PRIVACY_FLAGS   0x14
    #define YSM_SERVER_KNOWN_URLS   0x15
    #define YSM_SERVER_NOT_OPER     0x16
    #define YSM_ICQ_CLIENT_NOTICE   0x17
    #define YSM_ACK_ICQ_CLIENT      0x18


#define YSM_LOCATION_SRVC_SNAC      0x02
#define YSM_BUDDY_LIST_SNAC         0x03
    #define YSM_CLI_SRV_ERRMESG     0x01
    #define YSM_CLI_REQ_RIGHTSINF   0x02
    #define YSM_SRV_RIGHTS_INFO     0x03
    #define YSM_CLI_ADD_BUDDY       0x04
    #define YSM_CLI_REMOVE_BUDDY    0x05
    #define YSM_CLI_WATCHER_QUERY   0x06
    #define YSM_SRV_WATCHER_RESP    0x07
    #define YSM_CLI_WATCHER_SUBREQ  0x08
    #define YSM_SRV_WATCHER_NOTICE  0x09
    #define YSM_SRV_REJECT_NOTICE   0x0a
    #define YSM_SRV_ONCOMING_BUD    0x0b
    #define YSM_SRV_OFFGOING_BUD    0x0c

#define YSM_MESSAGING_SNAC          0x04
    #define YSM_CLI_SRV_ERRORMSG    0x01
    #define SET_ICBM_PARAMS         0x02
    #define RESET_ICBM_PARAM        0x03
    #define YSM_REQUEST_PARAM_INFO  0x04
    #define YSM_PARAMETER_INFO      0x05
    #define YSM_MESSAGE_FROM_CLIENT 0x06
    #define YSM_MESSAGE_TO_CLIENT   0x07
    #define YSM_EVIL_REQUEST        0x08
    #define YSM_EVIL_REQ_REPLY      0x09
    #define YSM_SRV_MISSED_CALLS    0x0a
    #define YSM_CLIENT_ACK          0x0b
    #define YSM_HOST_ACK            0x0c

#define YSM_ICQV8FUNC_SNAC          0x13
    #define YSM_CLI_REQ_ROSTER      0x05
    #define YSM_SRV_SEND_ROSTER     0x06
    #define CLI_SSI_ACTIVATE        0x07
    #define CLI_SSI_ADD             0x08
    #define CLI_SSI_UPDATE          0x09
    #define CLI_SSI_REMOVE          0x0A
    #define YSM_SRV_CHANGE_ACK      0x0e
    #define    YSM_SRV_ROSTER_OK    0x0f
    #define CLI_SSI_EDIT_BEGIN      0x11
    #define CLI_SSI_EDIT_END        0x12
    #define CLI_SSI_AUTH_REQ        0x18
    #define SERVER_SSI_AUTH_REQ     0x19
    #define CLI_SSI_AUTH_RSP        0x1A
    #define SERVER_SSI_AUTH_RSP     0x1B

/*     These are some extra definitions for uploading
    the contacts to the servers.    */

    #define YSM_SRV_BUDDY_AUTH      0x0e
    #define YSM_SRV_BUDDY_NOAUTH    0x00
    #define YSM_SRV_BUDDY_ERRADD    0x0a

#define YSM_MULTIUSE_SNAC           0x15
    #define YSM_CLI_SEND_REQ        0x02
        #define CLI_FULLINFO_REQ    0x04B2
        #define CLI_FULLINFO2_REQ   0x04D0
    #define YSM_SRV_SEND_RSP        0x03

#define YSM_REGISTRATION_SNAC       0x17
    #define YSM_CLI_SEND_REG        0x04
    #define YSM_SRV_REPLY_REG       0x05
    #define YSM_SRV_REFUSED_REG     0x01

#define YSM_ADVERTISEMENT_SNAC      0x05
#define YSM_INVITATION_C2C_SNAC     0x06
#define YSM_ADMINISTRATIVE_SNAC     0x07
#define YSM_POPUP_NOTICES_SNAC      0x08
#define YSM_BOS_SPECIFIC_SNAC       0x09
#define YSM_USER_LOOKUP_SNAC        0x0a
#define YSM_STATS_SNAC              0x0b
#define YSM_TRANSLATE_SNAC          0x0c
#define YSM_CHAT_NAVIGAT_SNAC       0x0d
#define YSM_CHAT_SNAC               0x0e
#define YSM_UNKNOWN_SNAC            0x45

/* DC Packet Types */
#define PEER_INIT                   0xff
#define PEER_INITACK                0x01
#define PEER_MSG                    0x02
#define PEER_INIT2                  0x03

#define PEER_FILE_INIT              0x00
#define PEER_FILE_INIT_ACK          0x01
#define PEER_FILE_START             0x02
#define PEER_FILE_START_ACK         0x03
#define PEER_FILE_STOP              0x04
#define PEER_FILE_SPEED             0x05
#define PEER_FILE_DATA              0x06

/* Class: ICBM__IM_SECTION_ENCODINGS
 * An IM can be encoded in the following different forms: */

#define ICBM__IM_SECTION_ENCODINGS_ASCII    0x00  /* ANSI ASCII -- ISO 646 */
#define ICBM__IM_SECTION_ENCODINGS_UNICODE  0x02  /* ISO 10646.USC-2 Unicode */
#define ICBM__IM_SECTION_ENCODINGS_LATIN_1  0x03  /* ISO 8859-1 */

#endif
