#include "ysm.h"
#include "icqv7.h"
#include "direct.h"
#include "network.h"
#include "toolbox.h"
#include "wrappers.h"
#include "setup.h"
#include "prompt.h"
#include "charset.h"
#include "crypt.h"
#include "slaves.h"
#include "output.h"

static void YSM_DC_InitA(slave_t *remote_slave, int32_t sock);
static void YSM_DC_InitB(slave_t *remote_slave, int32_t sock);

static const uint8_t id_str_chat[] = {
    0xbf,0xf7,0x20,0xb2,0x37,0x8e,0xd4,0x11,0xbd,0x28,0x00,0x04,
    0xac,0x96,0xd9,0x05 };

static const uint8_t id_str_file[] = {
    0xf0,0x2d,0x12,0xd9,0x30,0x91,0xd3,0x11,0x8d,0xd7,0x00,0x10,
    0x4b,0x06,0x46,0x2e };

static const uint8_t id_str_url[] = {
    0x37,0x1c,0x58,0x72,0xe9,0x87,0xd4,0x11,0xa4,0xc1,0x00,0xd0,
    0xb7,0x59,0xb1,0xd9 };

static const uint8_t id_str_contacts[] = {
    0x2a,0x0e,0x7d,0x46,0x76,0x76,0xd4,0x11,0xbc,0xe6,0x00,0x04,
    0xac,0x96,0x1e,0xa6 };

static int32_t YSM_DC_IncomingFile(
    slave_t    *victim,
    int32_t     len,
    int8_t     *data,
    msg_type_t  m_type,
    uint8_t    m_flags,
    int16_t     m_status);

static int32_t YSM_DC_FileC(slave_t *victim, uint16_t rport);

/* *port must point to an int16_t. If initialized to something
 * that isn't 0, that value is used for binding.
 * the binded port is returned in *port.
 */

static int32_t YSM_DC_BindPort(int32_t sock, uint16_t *port)
{
    struct sockaddr_in addr;
    socklen_t len;

    if (port == NULL || sock <= 0) return -1;

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;    /* all */
    addr.sin_port = *port;            /* random? */
    len = sizeof(addr);

    if (bind(sock, (struct sockaddr *)&addr, len) < 0)
    {
        printfOutput(VERBOSE_DCON, "YSM_DC_BindPort: Bind failed.\n");
        return -1;
    }

    getsockname(sock, (struct sockaddr *)&addr, &len);
    *port = addr.sin_port;
    listen(sock, 3);

    return 0;
}

int initDC(void)
{
    YSM_USER.d_con.seq_out = 0xffff;

    YSM_USER.d_con.rSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (YSM_USER.d_con.rSocket < 0) {
        printfOutput( VERBOSE_DCON, "YSM_DC_Init: socket failed.\n" );
        return -1;
    }

    /* fill our DC information. rIP was filled during connect */
    YSM_USER.d_con.rPort = htons(g_cfg.dcport1);
    if (YSM_DC_BindPort( YSM_USER.d_con.rSocket,
            &YSM_USER.d_con.rPort ) < 0) return -1;

    return 0;
}

uin_t YSM_DC_Wait4Client(void)
{
    int32_t     cli_sock, addrlen, r_len;
    uint32_t   r_ip1 = 0, r_ip2 = 0;
    uin_t       r_uin = 0;
    struct      sockaddr_in addr;
    int8_t      buf[MAX_PEER_DATA_SIZE], pver = 0;
    struct      timeval tv;
    fd_set      net_fd;
    slave_t    *YSM_Query = NULL;

    addrlen = sizeof(addr);
    cli_sock = accept(YSM_USER.d_con.rSocket,
                (struct sockaddr *)&addr,
                (int *)&addrlen);

    if (cli_sock < 0)
        return NOT_A_SLAVE;

    /* Read the Init Packet.
     * Set a timeout of 1 second to receive activity.
     * if the socket timeouts, the dc connection is closed.
     * we have to go back to accept as soon as possible.
     */

    FD_ZERO(&net_fd);
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    FD_SET(cli_sock, &net_fd);
    select(cli_sock + 1, &net_fd, NULL, NULL, &tv);

    /* drop connection if the first packet didn't arrive */
    if (!FD_ISSET(cli_sock, &net_fd)) {
        close(cli_sock);
        return NOT_A_SLAVE;
    }

    r_len = YSM_DC_ReadPacket( cli_sock, buf);
    if (r_len <= 0) {

        printfOutput( VERBOSE_DCON,
            "YSM_DC_Wait4Client: "
            "Error reading first PEER_INIT packet.\n" );

        close(cli_sock);
        return NOT_A_SLAVE;
    }

    /* INIT_PEER packet checks.
     * length must be 0x30.
     */

    if (r_len != 0x30) {

        printfOutput( VERBOSE_DCON,
            "YSM_DC_Wait4Client: "
            "PEER_INIT packet length isn't correct.\n" );

        close( cli_sock );
        return NOT_A_SLAVE;
    }

    /*
     * Protocol Version Check.
     */

    pver = buf[1];
    if (pver < YSM_PROTOCOL_VERSION) {
        printfOutput( VERBOSE_DCON,
            "YSM_DC_Wait4Client: "
            "Incompatible p2p protocol version (oldie).\n" );

        close( cli_sock );
        return NOT_A_SLAVE;
    }

    /* Get packet information. */
    r_uin = Chars_2_DW(&buf[15]);
    r_ip1 = Chars_2_DW(&buf[19]);    /* server-side address */
    r_ip2 = Chars_2_DW(&buf[23]);    /* internal address */

    /* check if the UIN exists and get a slave pointer */
    if (!r_uin
    || (YSM_Query = querySlave(SLAVE_UIN, NULL, r_uin, 0)) == NULL ) {
        printfOutput( VERBOSE_DCON,
            "YSM_DC_Wait4Client: "
            "Invalid/Not in list UIN specified.\n" );
        close( cli_sock );
        return NOT_A_SLAVE;
    }

    /* carlin attack checks ->
     * either external matches external and internal matches socket.
     * or external matches socket.
     */

    /* Trillian is breaking the rules, it sends the local address
     * in both external and internal fields.
     */
    if ((YSM_Query->d_con.rIP_ext == r_ip1
        && YSM_Query->d_con.rIP_int == addr.sin_addr.s_addr )
        || (YSM_Query->d_con.rIP_ext == addr.sin_addr.s_addr )
        /* below is just for the mess Trillian is doing */
        || (YSM_Query->d_con.rIP_int == r_ip1
        && YSM_Query->d_con.rIP_int == r_ip2
        && YSM_Query->fprint == FINGERPRINT_TRILLIAN_CLIENT)) {

        /* Fine, after all these tests we seem safe.
         * Mark the slave as expecting data.
         */

        /* Was there an open socket with this slave already?
         * if there was, close this session, its probably
         * an automatic reconnect from mirabilis clients.
         */
        if (YSM_Query->d_con.flags & DC_CONNECTED) {
            close( cli_sock );
            return NOT_A_SLAVE;

        } else {
            YSM_Query->d_con.rSocket = cli_sock;
            YSM_Query->d_con.flags |= DC_EXPECTDATA
                    | DC_INCOMINGNEG | DC_EXPECTNEG;
        }

    } else {
        /* Attack detected. AT0 call police, now NOW. */
        printfOutput( VERBOSE_DCON,
            "YSM_DC_Wait4Client: "
            "Possible spoofing attack detected.\n" );

        close( cli_sock );
        return NOT_A_SLAVE;
    }

    /* seq */
    YSM_Query->d_con.seq_in = 0xffff;

    /* Ack PEER_INIT and send PEER_INIT */
    YSM_DC_InitB( YSM_Query, YSM_Query->d_con.rSocket );
    YSM_DC_InitA( YSM_Query, YSM_Query->d_con.rSocket );

    return YSM_Query->uin;
}

void dcSelect(void)
{
    uint32_t x = 0;
    slave_t  *slave = NULL;

    while (slave = getNextSlave(slave))
    {
        if (slave->d_con.flags & DC_EXPECTDATA)
        {
            FD_Timeout(0, 0);
            FD_Init(FD_DIRECTCON);
            FD_Add(slave->d_con.rSocket, FD_DIRECTCON);

            if (FD_Select(FD_DIRECTCON) < 0)
            {
                YSM_CloseDC(slave);
                continue;
            }

            if (FD_IsSet(slave->d_con.rSocket, FD_DIRECTCON))
            {
                /* incoming DC data! */
                YSM_DC_CommonResponse(slave, slave->d_con.rSocket );
            }
        }
    }
}

int YSM_DC_ReadPacket( int cli_sock, char *buf )
{
int r_len = 0, p_len = 0, x = 0;

    if (buf == NULL || cli_sock <= 0)
        return -1;

    /* Read the command byte reply    */
    r_len = YSM_READ(cli_sock, buf, 2, 0);
    if (r_len <= 0) {
        if (r_len < 0) {
            printfOutput( VERBOSE_DCON,
                "YSM_DC_ReadPacket: "
                "Error reading size from DC Socket.\n" );
        }
        return -1;
    }

    p_len = Chars_2_Word(&buf[0]);
    r_len = 0; /* reset r_len */

    /* Exceeds MAX size */
    if (p_len >= MAX_PEER_DATA_SIZE) {
        printfOutput( VERBOSE_DCON,
            "YSM_DC_ReadPacket: Incoming size (%d) exceeds "
            "buf size.\n", p_len);
        close(cli_sock);
        return -1;
    }

    while (r_len < p_len) {

        x = YSM_READ( cli_sock,
            &buf[0+r_len],
            p_len,
            0);

        if (x < 0) {
            printfOutput( VERBOSE_DCON,
                "YSM_DC_ReadPacket: "
                "Error reading from DC TCP Socket.\n");
            return -1;
        }

        r_len += x;
    }

    return r_len;
}


/* PEER_INIT packet */

static void YSM_DC_InitA( slave_t *remote_slave, int32_t sock )
{
/* We are sure about this size, dun worry kid :P */
char    buf[50];
int    pos = 0;

    if (remote_slave == NULL || sock <= 0)
        return;

    memset(&buf[0], 0, sizeof(buf));

    /* len of the whole thing */
    buf[pos] = 0x30;
    pos += 2;

    buf[pos] = (char) PEER_INIT;
    pos += 1;

    buf[pos] = YSM_PROTOCOL_VERSION;
    pos += 2;

    buf[pos] = 0x2b;
    pos +=2;

    DW_2_Chars(buf+pos, remote_slave->uin);
    pos += 4;

    /* buf[7 && 8] are 0x0000 */
    pos += 2;

    /* buf[9 && 10 && 11 && 12] are local port. */
    pos += 2;
    Word_2_Chars(buf+pos, YSM_USER.d_con.rPort);
    pos += 2;

    DW_2_Chars(buf+pos, YSM_USER.uin);
    pos += 4;

    DW_2_Chars(buf+pos, YSM_USER.d_con.rIP_ext);
    pos += 4;    /* Our external IP, stalker! */
    DW_2_Chars(buf+pos, YSM_USER.d_con.rIP_int);
    pos += 4;    /* Our internal IP, Even this! god! */

    buf[pos] = 0x04;    /* TCP Capable flag.0x04 is YES(liE?)*/
    pos += 1;

    pos += 4;    /* Our local port again for Chat/Files..fuck out ! */

    /* DC Cookie... */
    DW_2_Chars(buf+pos, remote_slave->d_con.rCookie);
    pos += 4;

    buf[pos] = 0x50;        /* Extra crap, just in case */
    pos += 4;

    buf[pos] = 0x03;
    pos += 4;

    pos += 4;        /* Extra dword , just 0 */
    YSM_WRITE( sock, &buf[0], pos );
}

static int32_t YSM_DC_ReceiveInitA(slave_t *victim, int8_t *buf, int32_t r_len)
{
    int32_t ret = TRUE;

    /* This packet is parsed inside Wait4Client() */
    return ret;
}

/* PEER_INITACK */

static void YSM_DC_InitB( slave_t *remote_slave, int32_t sock )
{
/* We are sure about this size, dun worry kid :P */
char    buf[6];
int    pos = 0;

    if (remote_slave == NULL || sock <= 0)
        return;

    memset(&buf[0], 0, sizeof(buf));

    buf[pos] = 0x04;        /* whole len */
    pos += 2;

    buf[pos] = PEER_INITACK;    /* ACK */
    pos += 4;

    YSM_WRITE( sock, &buf[0], pos );
}

/* PEER_INIT2 */

static void YSM_DC_InitC( slave_t *remote_slave, int32_t sock )
{
/* We are sure about this size, dun worry kid :P */
char    buf[35];
int    pos = 0;

    if (remote_slave == NULL || sock <= 0)
        return;

    memset(&buf[0], 0, sizeof(buf));

    buf[pos] = 0x21;        /* whole len */
    pos += 2;

    buf[pos] = (char)PEER_INIT2;
    pos += 1;

    buf[pos] = 0x0a;
    pos += 4;

    buf[pos] = 0x01;
    pos += 4;            /* Let it be 0x00, part of len */

    pos += 20;

    buf[pos] = 0x01;
    pos += 2;
    buf[pos] = 0x04;
    pos += 2;

    YSM_WRITE( sock, &buf[0], pos );
}

static int32_t YSM_DC_ReceiveInitC( slave_t *victim, int8_t *buf, int32_t r_len )
{
    /* standard checks */
    if (buf == NULL || victim == NULL || r_len < 30)
        return 0;

    if (buf[1] != 0x0a) return -1;

    /* if this byte isnt 0x01 its an automatic connection    */
    /* probably from a mirabilis client at startup.        */
    /* since we hate these connections plus they attach a    */
    /* message with them, we return an error if it isn't 1.    */
    /* but! wait, we return -2 and close the socket manually */
    /* in order to leave the user alone and avoid printing     */
    /* the closed dc session message.            */
    if (buf[5] != 0x01) {
        /* nota, esto todavia no funcionar..uno de los frames
         * sigue imprimiendo el closed socket, rompe bolas! heh */
        printfOutput( VERBOSE_DCON,
            "Detected Auto DC Negotiation request from %s.\n",
            victim->info.nickName );
            close( victim->d_con.rSocket );
            return -2;
    }

    /* this byte is 0x01 for INCOMING connections.        */
    if (buf[29] == 0x00 && (victim->d_con.flags & DC_INCOMINGNEG))
        return -1;

    return 1;
}

/* Open a direct Connection to a Slave */
/* Incoming slave_t structure pointer */

void YSM_OpenDC( slave_t *victim )
{
struct in_addr    r_addr;
uint32_t    rIP;

    if (victim == NULL)
        return;

    victim->d_con.rSocket = 0;
    victim->d_con.flags = 0;

    /* Cant use DC if we don't have the users IP or port! */
    if ((victim->d_con.rIP_int == 0 && victim->d_con.rIP_ext == 0)
            ||  victim->d_con.rPort == 0) {
        printfOutput( VERBOSE_BASE, MSG_DIRECT_ERR2 "\n");
        return;
    }

    /* Can't communicate with old protocol Versions */
    if (victim->d_con.version < YSM_PROTOCOL_VERSION) {
        printfOutput( VERBOSE_BASE,
            "\nNo. The user uses an old protocol version.\n");
        return;
    }

    /* Start with the external IP address if we are in non-LAN mode */
    if(victim->d_con.rIP_ext != 0 && !g_cfg.dclan)
        rIP = victim->d_con.rIP_ext;
    else
        rIP = victim->d_con.rIP_int;


    do {
        r_addr.s_addr = rIP;

        printfOutput( VERBOSE_DCON,
            "\n" MSG_DIRECT_CONNECTING "%s:%d..\n",
            inet_ntoa(r_addr),
            victim->d_con.rPort);

        g_state.promptFlags |= FL_RAW;

        victim->d_con.rSocket = YSM_Connect(
                    inet_ntoa(r_addr),
                    victim->d_con.rPort,
                    0x0 );

        if (!g_cfg.dclan) {
            if (rIP == victim->d_con.rIP_int) break;
        } else {
            if (rIP == victim->d_con.rIP_ext) break;
        }

        if(victim->d_con.rSocket < 0) {
            if (g_cfg.dclan)
                rIP = victim->d_con.rIP_ext;
            else
                rIP = victim->d_con.rIP_int;
        }

    } while (victim->d_con.rSocket < 0);

    if (victim->d_con.rSocket < 0) {
        printfOutput(VERBOSE_BASE, "\n" MSG_DIRECT_ERR3 "\n");
        return;
    }

    printfOutput(VERBOSE_BASE, "\n" MSG_DIRECT_ESTABLISHED "\n");
    printfOutput(VERBOSE_BASE, "Initializing negotiation..\n" );

    victim->d_con.flags |= DC_EXPECTDATA | DC_OUTGOINGNEG | DC_EXPECTNEG;

    /* Send first Hello Packet */
    YSM_DC_InitA( victim, victim->d_con.rSocket );

    /* Seq num init */
    victim->d_con.seq_in = 0xffff;

    g_state.promptFlags |= FL_RAW;
}

/* Close a direct connection with a slave. */
/* close socket, reset flags, etc.         */

void YSM_CloseDC(uin_t uin)
{
    if (uin == NOT_A_SLAVE)
        return;

    YSM_CloseTransfer(uin);

    /* close the victim's socket */
    close(victim->d_con.rSocket);

    /* reset victim's flags */
    victim->d_con.flags = 0;

    printfOutput(VERBOSE_BASE,
        "DC session to %s closed.\n", victim->info.nickName);
}

void YSM_CloseTransfer(uin_t uin)
{
    if (uin == NOT_A_SLAVE)
        return;

    victim->d_con.finfo.statstime = victim->d_con.finfo.statsbytes = 0;

    if (victim->d_con.flags & DC_TRANSFERING)
        close( victim->d_con.finfo.rSocket );

    if (victim->d_con.finfo.fd != 0) {
        fclose( victim->d_con.finfo.fd );
        victim->d_con.finfo.fd = 0;
    }

    victim->d_con.flags &= ~DC_TRANSFERING;
    victim->d_con.flags &= ~DC_RECEIVING;
    victim->d_con.flags &= ~DC_ACTIVITY;
}

int32_t YSM_DC_IncomingMessageGREET( slave_t *victim,
    msg_type_t   m_type,
    uint8_t   m_flags,
    int8_t    *m_data,
    int16_t    m_len,
    int16_t    m_status )
{
int16_t    greet_type;
int32_t    pos = 0, id_pos = 0, len = 0, ret = TRUE;

    if (victim == NULL || m_data == NULL || m_len <= 0)
        return -1;

    /* skip empty data field */
    m_data += m_len;

    /* Now we are in the MSG_EXTRA_DATA field. Where GREET data is */
    greet_type = Chars_2_Word(m_data+pos);
    pos += 2;

    /* Later checks with greet_type require id_pos */
    id_pos = pos;

    pos += 16;    /* skip 16 bytes ID */
    pos += 2;    /* empty bytes */

    len = Chars_2_DW(m_data+pos);
    pos += 4;    /* skip this string */
    pos += len;

    pos += 15;    /* Unknown */

    pos += 4;    /* remaining len */

    len = Chars_2_DW(m_data+pos);    /* we have data len here */
    pos += 4;            /* now m_data+pos points to data */

    /* Now daddy, what did I get? Files? Chats? What?
     * We use the following 16 bytes ID field to recognize
     * the incoming type of packet since some of them share
     * the same m_type values (burn them down!)
     */

    switch (greet_type) {
        case 0x2d:    /* chat/contacts */

            /* CHAT ? */
            if (!memcmp( m_data + id_pos,
                    id_str_chat,
                    sizeof(id_str_chat) )) {

            /* CONTACTS ? */
            } else if (!memcmp( m_data + id_pos,
                    id_str_contacts,
                    sizeof(id_str_contacts) )) {

                YSM_DisplayMsg( YSM_MESSAGE_CONTACTS,
                    victim->uin,
                    victim->status,
                    len,
                    m_data+pos,
                    0x00);

                /* ACK the Message */
                ret = YSM_DC_MessageACK( victim, m_type );
            }

            break;

        case 0x29:    /* file request    */
        case 0x32:    /* file response */

            /* FILE ? */
            if (!memcmp( m_data + id_pos,
                    id_str_file,
                    sizeof(id_str_file) )) {

                ret = YSM_DC_IncomingFile( victim,
                            len,
                            m_data+pos,
                            m_type,
                            m_flags,
                            m_status );
            }


            break;

        case 0x40:    /* url */

            /* URL ? */
            if (!memcmp( m_data + id_pos,
                    id_str_url,
                    sizeof(id_str_url) )) {

                YSM_DisplayMsg( YSM_MESSAGE_URL,
                    victim->uin,
                    victim->status,
                    len,
                    m_data+pos,
                    0x00);

                /* ACK the Message */
                ret = YSM_DC_MessageACK( victim, m_type );
            }

        default:
            break;
    }

    /* YSM_DC_IncomingFile/MessageACK may change the ret value */
    return ret;
}

int32_t YSM_DC_IncomingMessageFILE( slave_t *victim,
    msg_type_t m_type,
    uint8_t    m_flags,
    int8_t    *m_data,
    int16_t    m_len,
    int16_t    m_status)
{
    int32_t        pos = 0, ret = TRUE, fsize = 0;
    uint16_t    rport = 0, fnamelen = 0;
    int8_t        *pfname = NULL;

    if (victim == NULL || m_data == NULL || m_len <= 0)
        return -1;

    /* empty data field */
    m_data += m_len;

    /* sending an ACK, user accepted */

    if (m_flags & MFLAGTYPE_ACK)
    {
        /* Remote file port, empty on requests */
        memcpy(&rport, m_data+pos, 2);
        pos += 2;

        /* Incoming File Transfer Ack, start transfering */
        ret = YSM_DC_FileC(victim, rport);

        if (ret) {

        printfOutput( VERBOSE_BASE,
            "\nStarting %sFile Transfer.. "
            "(the slave accepted the request).\n",
            (m_flags & MFLAGTYPE_CRYPTACK) ? "ENCRYPTED " : "");
        }

    /* sending a File request */

    } else if (m_flags & MFLAGTYPE_NORM) {

        pos += 2;    /* port empty */
        pos += 2;    /* probably padding */

        fnamelen = Chars_2_Word(m_data+pos); /* Filename length */
        pos += 2;

        if (fnamelen) {        /* Filename buffer */
            pfname = ysm_calloc(1, fnamelen+1, __FILE__, __LINE__ );
            if (pfname == NULL) return 0;
            memcpy(pfname, m_data+pos, fnamelen);
        }

        pos += fnamelen;
        fsize = Chars_2_DW(m_data+pos);

        printfOutput( VERBOSE_BASE,
            "\nIncoming file transfer request from: %s.\n",
            victim->info.nickName );

        printfOutput( VERBOSE_BASE,
            "(use the faccept/fdecline command "
            "to accept/decline the request)\n"
            "Filename: '%s' [%d kb].\n",
            pfname, fsize/1024 );

        if (pfname != NULL) {
            YSM_FREE(pfname);
        }

    }
    else ret = 0;

    return ret;
}


int32_t YSM_DC_IncomingMessage( slave_t *victim, int8_t *data, int32_t data_len )
{
int16_t    cmd = 0;
int32_t    pos = 0, ret = TRUE;
int8_t    m_flags = 0;

    if (victim == NULL || data == NULL || data_len <= 0)
        return -1;

    if (!(victim->d_con.flags & DC_CONNECTED)) {
        printfOutput( VERBOSE_DCON,
            "Incoming PEER_MSG before Negotiation, ignoring.\n" );
        return -1;
    }

    if (YSM_DecryptDCPacket(data, data_len) <= 0) {
        printfOutput( VERBOSE_DCON, "Decryption Failed.\n" );
        return -1;
    }

    pos ++;        /* cmd */
    pos += 4;    /* checksum */

    cmd = Chars_2_Word(&data[pos]);
    pos += 2;    /* command */

    switch (cmd) {
        case 0x07d0:
            m_flags |= MFLAGTYPE_END;
            break;

        case 0x6666:    /* used between ysm's */
            m_flags |= MFLAGTYPE_CRYPTACK;
        case 0x07da:
            m_flags |= MFLAGTYPE_ACK;
            break;

        case 0x6766:    /* used between ysm's */
            m_flags |= MFLAGTYPE_CRYPTNORM;
        case 0x07ee:
            m_flags |= MFLAGTYPE_NORM;
            break;

        default:
            printfOutput( VERBOSE_DCON, "YSM_DC_IncomingMessage: "
                "unknown cmd(%d)\n",
                cmd );

            return -1;
    }

    ret = YSM_ReceiveMessageType2Common(victim,
                    pos,
                    data,
                    0,
                    0,
                    NULL,
                    m_flags,
                    &victim->d_con.seq_in,
                    0x1);

    return ret;
}


int32_t YSM_DC_CommonResponse( slave_t *victim, int32_t sock )
{
uint8_t    buf[MAX_PEER_DATA_SIZE];
int        r_len = 0, ret = 0;

    if (victim == NULL || sock <= 0)
        return -1;

    r_len = YSM_DC_ReadPacket( sock, &buf[0] );
    if (r_len < 0) {
        if (victim->d_con.flags & DC_RECEIVING) {

            printfOutput( VERBOSE_BASE,
                "File receiving aborted. "
                "%s closed the connection.\n",
                victim->info.nickName);

        } else if (victim->d_con.flags & DC_TRANSFERING) {

            printfOutput( VERBOSE_BASE,
                "File sending aborted. "
                "%s closed the connection.\n",
                victim->info.nickName );
        }

        YSM_CloseDC( victim );
        return 0;
    }

    /* Since I made this a common function, i need DC_EXPECTNEG to be ON
     * on the slave's flags if we are waiting for negotiation packets.
     * (this is because PEER_FILE and PEER_NEGOT packets share same ids
     * duh!) kill miracrap, kill em all!
     */

    if (victim->d_con.flags & DC_EXPECTNEG) {
        /* Incoming p2p Negotiation Command List */
        ret = YSM_DC_CommonResponseNeg( victim, sock, buf, r_len );

    } else if (victim->d_con.flags & DC_TRANSFERING
        || victim->d_con.flags & DC_RECEIVING) {
        /* Incoming p2p Files Command List */
        ret = YSM_DC_CommonResponseFile( victim, sock, buf, r_len );

    } else {
        /* Incoming p2p Misc Command List */
        ret = YSM_DC_CommonResponseMisc( victim, sock, buf, r_len );
    }

    if (ret <= 0) {
        /* An error ocured during DC. that requires
         * to close the current DC session, probably carlin attacks.
         */

        /* ret 0 just close file transfer.
         * ret < 0 close the whole DC session.
         */

        YSM_CloseTransfer( victim );
        if (ret < 0) {
            YSM_CloseDC( victim );
        }
    }

    return ret;
}


/* Incoming File related Packets */

int32_t YSM_DC_CommonResponseFile( slave_t    *victim,
            int32_t        sock,
            uint8_t    *buf,
            int32_t        len )
{
int32_t ret = TRUE;

    if (victim == NULL || sock <= 0 || buf == NULL || len <= 0)
        return -1;

    printfOutput( VERBOSE_DCON, "Incoming p2p File command: %x\n", buf[0]);

    switch (buf[0]) {

        case PEER_FILE_INIT:
            /* has to be for incoming files */
            if (victim->d_con.flags & DC_RECEIVING) {
                ret = YSM_DC_FileInitAck( victim,
                            sock,
                            buf,
                            len );
            } else
                ret = -1;    /* abort DC! */
            break;

        case PEER_FILE_INIT_ACK:
            /* has to be for outgoing files */
            if (victim->d_con.flags & DC_TRANSFERING) {
                ret = YSM_DC_FileStart( victim );
            } else
                ret = -1;    /* abort DC! */

            break;

        case PEER_FILE_START:
            if (!(victim->d_con.flags & DC_ACTIVITY)) {

                victim->d_con.flags |= DC_ACTIVITY;

                /* has to be for incoming files */
                if (victim->d_con.flags & DC_RECEIVING) {
                    ret = YSM_DC_FileStartAck( victim,
                                sock,
                                buf,
                                len );
                } else
                    ret = -1;

            } else {
            /* This can also be a PEER_MSG sent DURING a transfer
             * I would suggest calling Reponse Misc from here on
             */

                ret = YSM_DC_CommonResponseMisc( victim,
                                sock,
                                buf,
                                len );
            }
            break;

        case PEER_FILE_START_ACK:
            /* has to be for outgoing files */
            if (victim->d_con.flags & DC_TRANSFERING) {
                victim->d_con.flags |= DC_ACTIVITY;
                /* okie, done, start the transfer! */
                ret = YSM_DC_FileTransfer( victim, buf, len );
            } else
                ret = -1;

            break;

        case PEER_FILE_STOP:
            break;

        case PEER_FILE_SPEED:
            break;

        case PEER_FILE_DATA:
            /* has to be for incoming files */
            if (victim->d_con.flags & DC_RECEIVING) {
                ret = YSM_DC_FileReceive( victim, buf, len );
            } else
                ret = -1;    /* abort DC! */

            break;

        default:
            ret = -1;        /* abort DC, shouldnt happend */
            break;
    }

    return ret;
}


/* Incoming Negotiation Packets */

int32_t YSM_DC_CommonResponseNeg( slave_t    *victim,
            int32_t        sock,
            uint8_t    *buf,
            int32_t        len )
{
int32_t ret = TRUE;

    if (victim == NULL || sock <= 0 || buf == NULL || len <= 0)
        return -1;

    printfOutput( VERBOSE_DCON, "Incoming p2p Negotiation command: %x\n", buf[0]);

    switch (buf[0]) {

        case PEER_INIT:

            /* If this was the second negotiation
             * (starting a file transfer)
             * then end it here, its done. */

            if (victim->d_con.flags & DC_TRANSFERING) {

                printfOutput( VERBOSE_DCON,
                    "Pre-transfer Negotation Completed.\n");

                victim->d_con.flags &= ~DC_EXPECTNEG;
                /* PEER_INITACK */
                YSM_DC_InitB( victim, sock );

                /* PEER_FILE_INIT */
                YSM_DC_FileInit( victim,
                        1,
                        victim->d_con.finfo.size );

            } else if (victim->d_con.flags & DC_RECEIVING) {

                /* PEER_INITACK */
                YSM_DC_InitB( victim, sock );

                /* PEER_INIT */
                YSM_DC_InitA( victim, sock );

            } else {

                printfOutput( VERBOSE_DCON,
                    "Received remote DC information.\n"
                    "Remote protocol version: %x\n"
                    "Sending PEER INIT ACK and "
                    "PEER INIT 2\n", buf[1]);

                ret = YSM_DC_ReceiveInitA( victim, buf, len );

                if (ret) {
                    /* PEER_INITACK */
                    YSM_DC_InitB( victim, sock );

                    /* PEER_INIT2 */
                    YSM_DC_InitC( victim, sock );
                }

            }

            break;

        case PEER_INITACK:
            printfOutput(VERBOSE_DCON, "First PEER INIT ACK Received.\n");

            if (victim->d_con.flags & DC_RECEIVING) {

                printfOutput( VERBOSE_DCON,
                    "Pre-transfer Negotation Completed.\n");

                victim->d_con.flags &= ~DC_EXPECTNEG;
            }

            break;

        case PEER_INIT2:

            ret = YSM_DC_ReceiveInitC( victim, buf, len );
            if (ret > 0) {

                if ( victim->d_con.flags & DC_INCOMINGNEG )
                    YSM_DC_InitC( victim, sock );

                printfOutput( VERBOSE_DCON,
                    "v8 Negotation Completed.\n");

#ifndef COMPACT_DISPLAY
                printfOutput( VERBOSE_BASE,
                    "\nDC session to %s established.\n",
                    victim->info.nickName );
#endif

                victim->d_con.flags |= DC_CONNECTED;
                victim->d_con.flags &= ~DC_EXPECTNEG;

                g_state.promptFlags |= FL_RAW;
            }

            break;

        default:
            printfOutput( VERBOSE_DCON,
                "Unknown Neg command received. Closing!\n");
            ret = -1;
            break;


    }

    return ret;
}



/* Incoming Misc(MSGS,etc) Packets */

int32_t YSM_DC_CommonResponseMisc( slave_t    *victim,
            int32_t        sock,
            uint8_t    *buf,
            int32_t        len )
{
int32_t ret = TRUE;

    if (victim == NULL || sock <= 0 || buf == NULL || len <= 0)
        return -1;

    printfOutput( VERBOSE_DCON, "Incoming p2p Misc command: %x\n", buf[0]);

    switch (buf[0]) {

        case PEER_MSG:
            printfOutput( VERBOSE_DCON, "PEER_MSG received.\n");
            ret = YSM_DC_IncomingMessage( victim, buf, len );
            break;

        default:
            printfOutput( VERBOSE_DCON,
                "Unexpected p2p command: %x\n", buf[0]);
            printfOutput( VERBOSE_DCON, "Closing down connection..\n");
            ret = -1;
            break;

    }

    return ret;
}


static uint8_t dc_string_table[] = {
  "As part of this software beta version Mirabilis is "
  "granting a limited access to the ICQ network, "
  "servers, directories, listings, information and databases (\""
  "ICQ Services and Information\"). The "
  "ICQ Service and Information may databases (\""
  "ICQ Services and Information\"). The "
  "ICQ Service and Information may\0"
};


int YSM_EncryptDCPacket ( unsigned char *buf, int buf_len )
{
unsigned int check_code, x_key, pos;
unsigned int B1, M1;
unsigned char    X1, X2, X3;
int        i;

    if (buf == NULL || buf_len <= 0)
        return -1;

    /* skip the command */
    /* NOTE, len must be skipped before calling this function. */
    /* len is a WORD. command is a BYTE */
    buf += 1;
    buf_len -= 1;

    M1 = (rand() % ((buf_len < 255 ? buf_len : 255) - 10)) + 10;
    X1 = buf[M1] ^ 0xFF;
    X2 = rand() % 220;
    X3 = dc_string_table[X2] ^ 0xFF;
    B1 = (buf[4] << 24) | (buf[6] << 16) | (buf[4] << 8 ) | (buf[6]);

    check_code = (M1 << 24) | (X1 << 16) | (X2 << 8) | X3;
    check_code ^= B1;

    /* Build the XOR Key */
    x_key = 0x67657268 * buf_len + check_code;

    /* Run through buf */
    for (i = 0; i < (buf_len+3) / 4;) {
        pos = x_key + dc_string_table[ i & 0xFF];
        buf[i++] ^= pos & 0xFF;
        buf[i++] ^= (pos >> 8) & 0xFF;
        buf[i++] ^= (pos >> 16) & 0xFF;
        buf[i++] ^= (pos >> 24) & 0xFF;
    }

    DW_2_Chars(&buf[0], check_code);

    return 1;
}

int YSM_DecryptDCPacket ( unsigned char *buf, unsigned int buf_len)
{

unsigned int    check_code;
unsigned long    B1, M1, x_key, pos;
unsigned char    X1, X2, X3;
unsigned int    i = 0;

    if (buf == NULL || buf_len <= 0)
        return -1;

    /* skip the command */
    /* NOTE, len must be skipped before calling this function. */
    /* len is a WORD. command is a BYTE */
    buf += 1;
    buf_len -= 1;

    check_code = Chars_2_DW(buf);

    /* Build the XOR Key */
    x_key = 0x67657268 * buf_len + check_code;

    /* Run through buf */
    for (i = 4; i < (buf_len + 3) / 4;)
    {
        pos = x_key + dc_string_table[i&0xff];
        buf[i++] ^= pos & 0xff;
        buf[i++] ^= (pos >> 8) & 0xff;
        buf[i++] ^= (pos >> 16) & 0xff;
        buf[i++] ^= (pos >> 24) & 0xff;
    }


    B1 = (buf[4]<<24) | (buf[6]<<16) | (buf[4]<<8) | (buf[6]<<0);
    B1 ^= check_code;

    M1 = (B1 >> 24) & 0xff;

    if (M1 < 10 || M1 >= buf_len)
        return 0;

    X1 = buf[M1] ^ 0xff;

    if (((B1 >> 16) & 0xff) != X1)
        return 0;

    X2 = (unsigned char)((B1 >> 8) & 0xff);

    if (X2 < 220) {
        X3 = dc_string_table[X2] ^ 0xff;
        if ((B1 & 0xff) != X3) return 0;
    }

    return 1;
}


static int32_t YSM_DC_MessageBase(
    slave_t *victim,
    char        *_msg,
    int        dlen,
    msg_type_t     msgType,
    int16_t        m_status,
    int16_t        _priority,
    char        *extra,
    int        extra_len,
    int8_t        m_flags,
    uint16_t    seq)
{

char    *buf = NULL;
int    buf_size = 0, pos = 0;
int32_t    ret = TRUE;

    if (victim == NULL || dlen < 0)
        return -1;

    buf_size = 33 + dlen + extra_len + 1;

    buf = ysm_calloc(1, buf_size, __FILE__, __LINE__ );
    pos += 2;

    buf[pos] = (char) PEER_MSG;
    pos += 1;

    pos += 4;    /* check_code, filled in crypt */

    if (m_flags & MFLAGTYPE_NORM) {
        buf[pos] = (char)0xEE;
        buf[pos+1] = 0x07;    /* CMD MESSAGE */
    } else if (m_flags & MFLAGTYPE_CRYPTNORM) {
        buf[pos] = 0x66;
        buf[pos+1] = 0x67;    /* CMD MESSAGE */
    } else if (m_flags & MFLAGTYPE_ACK) {
        buf[pos] = (char)0xDA;
        buf[pos+1] = 0x07;    /* CMD MESSAGE */
    } else if (m_flags & MFLAGTYPE_CRYPTACK) {
        buf[pos] = 0x66;
        buf[pos+1] = 0x66;    /* CMD MESSAGE */
    } else {
        buf[pos] = (char)0xD0;
        buf[pos+1] = 0x07;    /* CMD MESSAGE */
    }

    pos += 2;

    buf[pos] = 0x0E;    /* 2nd UNK CMD */
    pos += 2;

    Word_2_Chars(buf+pos, seq);
    pos += 2;        /* Seq (0xffff) start? */

    pos += 4;
    pos += 4;
    pos += 4;

    buf[pos] = msgType;
    pos += 2;

    Word_2_Chars(buf+pos, m_status);
    pos += 2;        /* status */

    Word_2_Chars(buf+pos, _priority);
    pos += 2;        /* Mesage Priority? */

    buf[pos] = dlen+1;    /* len of msg WORD */
    pos += 2;

    if (_msg != NULL)
        memcpy(&buf[pos], _msg, dlen);    /* zero ended msg */

    pos += dlen + 1;

    /* Now attach any extra data */
    if (extra) {
        memcpy(&buf[pos], extra, extra_len);
    }
    pos += extra_len;

    /* len of the whole thing */
    buf[0] = (33 + dlen + extra_len + 1) - 2;

    YSM_EncryptDCPacket( buf+2, pos-2 );

    ret = YSM_WRITE_DC( victim, victim->d_con.rSocket, buf, pos );

    YSM_FREE(buf);
    return ret;
}

int32_t YSM_DC_MessageGreet( slave_t    *victim,
        char        cmd,
        char        *textstring,
        char        *reason,
        char        *filename,
        int        file_size,
        uint16_t    port,
        char        **out_ptr )    /* bytes */
{
int32_t        buf_len = 0, pos = 0, rem_data = 0;
int8_t        *buf = NULL;
uint8_t    portb[4];

    if (victim == NULL)
        return -1;

    buf_len += 2;    /* Command */
    buf_len += 16;    /* ID */
    buf_len += 2;    /* Unknown */

    /* TEXTSTRING -> LENGTH[4BYTES] + TEXTSTRING */
    buf_len += 4;
    if (textstring != NULL) buf_len += strlen(textstring);

    buf_len += 15;    /* Unknown */
    buf_len += 4;    /* Len of remaining Data */

    /* REASON -> LENGTH[4BYTES] + REASON */
    buf_len += 4;
    if (reason != NULL) buf_len += strlen(reason);

    buf_len += 2;    /* Port */
    buf_len += 2;    /* Padding? */

    /* FILENAME -> LENGTH[2BYTES] + FILENAME */
    buf_len += 2;
    if (filename != NULL) buf_len += strlen(filename);

    buf_len += 4;    /* File size */


    /* IN CHAT REQUESTS THE NEXT FIELD IS MISSING */
    if (cmd != 0x2d && cmd != 0x3a) {
        buf_len += 4;    /* Port Again */
    }

    buf = ysm_calloc( 1, buf_len, __FILE__, __LINE__ );

    buf[pos] = cmd;    /* COMMAND */
    pos += 2;

    switch(cmd)         /* ID */
    {
        case 0x2d:
        default:
            memcpy(&buf[pos], id_str_chat, sizeof(id_str_chat));
            break;

        case 0x29:
        case 0x32:
            memcpy(&buf[pos], id_str_file, sizeof(id_str_file));
            break;
    }

    /* Both have the same size, no problem at all */
    pos += sizeof(id_str_file);

    /* Unknown 2 bytes */
    pos += 2;

    /* TEXTSTRING -> LENGTH[4BYTES] + TEXTSTRING */
    if (textstring != NULL) buf[pos] = strlen(textstring);
    pos += 4;
    if (textstring) {
        memcpy(&buf[pos], textstring, strlen(textstring));
        pos += strlen(textstring);
    }

    pos += 2;    /* unknown */
    buf[pos] = 0x01;
    pos ++;        /* even more unknown */

    switch(cmd)     /* Unknown ID */
    {
        case 0x2d:
        case 0x29:
            pos += 2;
            buf[pos] = 0x01;
            pos += 2;
            break;

        case 0x32:
            buf[pos] = 0x01;
        default:
            pos += 4;
            break;
    }

    pos += 8;    /* Unknown */

    rem_data = pos;
    pos += 4; /* Remaining Data, filled at the end. */

    /* REASON -> LENGTH[4BYTES] + REASON */
    if (reason != NULL) {
        buf[pos] = strlen(reason);
    }
    pos += 4;
    if (reason) {
        memcpy(&buf[pos], reason, strlen(reason));
        pos += strlen(reason);
    }

    memcpy(buf+pos, &port, 2);
    pos += 2;    /* Port , 0 on first outgoing packet */
    pos += 2;    /* Padding? */

    /* FILENAME -> LENGTH[2BYTES] + FILENAME + 0 */
    if (filename != NULL) buf[pos] = strlen(filename) + 1;
    else buf[pos] = 0x01;
    pos += 2;

    if (filename) {
        memcpy(&buf[pos], filename, strlen(filename));
        pos += strlen(filename);
    }
    pos ++;    /* ending 0 */

    DW_2_Chars(&buf[pos], file_size);    /* File Size - Bytes */
    pos += 4;

    /* IN CHAT REQUESTS THE NEXT FIELD IS MISSING */
    if (cmd == 0x29 || cmd == 0x32) {
        Word_2_Charsb( portb, port );
        portb[2] = portb[3] = 0x00;
        memcpy(&buf[pos], &portb[0], 4);
        pos += 4;
    }

    buf[rem_data] = pos - (rem_data+4);

    *out_ptr = buf;
    return pos;
}


int32_t YSM_DC_Message(
    slave_t  *victim,
    char     *_msg,
    int       dlen,
    int8_t    msgType)
{
    /* PEER_MSG_MSG has FG/BG Colors as extra data */
    char    xtra[8], m_flags = 0;

    if (victim == NULL)
        return -1;

    /* FOREGROUND */
    xtra[0] = (char)0x00;
    xtra[1] = (char)0x00;
    xtra[2] = (char)0x00;
    xtra[3] = (char)0x00;
    /* BACKGROUND */
    xtra[4] = (char)0xff;
    xtra[5] = (char)0xff;
    xtra[6] = (char)0xff;
    xtra[7] = (char)0x00;

    m_flags |= MFLAGTYPE_NORM;

    return YSM_DC_MessageBase( victim,
                _msg,
                dlen,
                msgType,
                0x0000,
                0x0000,
                xtra,
                sizeof(xtra),
                m_flags,
                YSM_USER.d_con.seq_out-- );
}

int32_t YSM_DC_MessageACK(slave_t *victim, msg_type_t msgType)
{
/* PEER_MSG_MSG has FG/BG Colors as extra data */
char    xtra[8], m_flags = 0, *_msg = "";
int32_t    dlen = 0x00;

    if (victim == NULL)
        return -1;

    /* FOREGROUND */
    xtra[0] = (char)0x00;
    xtra[1] = (char)0x00;
    xtra[2] = (char)0x00;
    xtra[3] = (char)0x00;
    /* BACKGROUND */
    xtra[4] = (char)0xff;
    xtra[5] = (char)0xff;
    xtra[6] = (char)0xff;
    xtra[7] = (char)0x00;

    m_flags |= MFLAGTYPE_ACK;

    return YSM_DC_MessageBase( victim,
                _msg,
                dlen,
                msgType,
                0x0000,
                0x0000,
                xtra,
                sizeof(xtra),
                m_flags,
                victim->d_con.seq_in );
}



/* Takes care of receiving the MSG_GREET_FILE packet */

static int32_t YSM_DC_IncomingFile(
    slave_t  *victim,
    int32_t   len,        /* reason length */
    int8_t   *data,       /* reason buffer */
    msg_type_t m_type,     /* message type */
    uint8_t    m_flags,    /* message flags */
    int16_t    m_status )  /* accepted/denied */
{
    int32_t    pos = 0, fsize = 0, ret = TRUE;
    uint16_t  rport = 0, fnamelen = 0;
    int8_t    *pfname = NULL, *preason = NULL;

    if ((len + 1) > 1) {    /* data points to reason */
        preason = ysm_calloc(1, len+1, __FILE__, __LINE__ );
        if (preason == NULL) return -1;
        memcpy( preason, data+pos, len );
    }
    pos += len;

    memcpy(&rport, data+pos, 2);
    pos += 2;
    pos += 2;    /* padding? */

    fnamelen = Chars_2_Word(data+pos);  /* Filename length */
    pos += 2;

    if (fnamelen) {        /* Filename buffer */
        pfname = ysm_calloc(1, fnamelen+1, __FILE__, __LINE__ );
        if (!pfname) {
            if (preason != NULL) {
                YSM_FREE(preason);
            }
            return -1;
        }
        memcpy( pfname, data+pos, fnamelen );
    }

    pos += fnamelen;

    fsize = Chars_2_DW(data+pos);

    if (m_flags & MFLAGTYPE_ACK)
    {
        switch (m_status)
        {
        case 0x00:
            break;

        case 0x01:
            printfOutput( VERBOSE_BASE, "\n"
            "File transfer to %s aborted. (denied by the slave).\n",
            victim->info.nickName);

            if (pfname != NULL) {
                YSM_FREE(pfname);
            }

            if (preason != NULL) {
                YSM_FREE(preason);
            }
            return 0;
        case 0x04:
            printfOutput( VERBOSE_BASE, "\n"
            "File transfer to %s aborted. (the slave was away).\n",
            victim->info.nickName);

            if (pfname != NULL) {
                YSM_FREE(pfname);
            }

            if (preason != NULL) {
                YSM_FREE(preason);
            }

            return 0;
        case 0x09:
            printfOutput( VERBOSE_BASE, "\n"
            "File transfer to %s aborted. "
            "(the slave was occupied).\n",
            victim->info.nickName);

            if (pfname != NULL) {
                YSM_FREE(pfname);
            }

            if (preason != NULL) {
                YSM_FREE(preason);
            }

            return 0;
        case 0x0a:
            printfOutput( VERBOSE_BASE, "\n"
            "File transfer to %s aborted. "
            "(the slave was in DND).\n",
            victim->info.nickName);

            if (pfname != NULL) {
                YSM_FREE(pfname);
            }

            if (preason != NULL) {
                YSM_FREE(preason);
            }

            return 0;

        case 0x0e:
            printfOutput( VERBOSE_BASE, "\n"
            "File transfer to %s aborted. (the slave was in NA).\n",
            victim->info.nickName);

            if (pfname != NULL) {
                YSM_FREE(pfname);
            }

            if (preason != NULL) {
                YSM_FREE(preason);
            }

            return 0;

        default:
            printfOutput( VERBOSE_BASE, "\n"
            "File transfer to %s aborted. (m_status is unknown).\n",
            victim->info.nickName);

            if (pfname != NULL) {
                YSM_FREE(pfname);
            }

            if (preason != NULL) {
                YSM_FREE(preason);
            }

            return 0;

        }

        /* Incoming File Transfer Ack, start transfering */
        ret = YSM_DC_FileC( victim, rport);

        if (ret)
        {
            printfOutput( VERBOSE_BASE,
            "\nStarting %sFile Transfer.. "
            "(the slave accepted the request).\n",
            (m_flags & MFLAGTYPE_CRYPTACK) ? "ENCRYPTED " : "");
        }

        g_state.promptFlags |= FL_RAW;
    }
    else if (m_flags & MFLAGTYPE_NORM)
    {
        if ((m_flags & MFLAGTYPE_CRYPTNORM)
        && (isKeyEmpty(victim->crypto.strkey)
        || victim->fprint != FINGERPRINT_YSM_CLIENT_CRYPT ))
        {
            printfOutput(VERBOSE_BASE,
                "\nIncoming ENCRYPTED file transfer from: %s "
                "was IGNO.\n"
                "The key used to encrypt the transfer isn't "
                "known by\n"
                "your ysm client. read about the 'key' command."
                "\n",
                victim->info.nickName);

            YSM_CloseTransfer(victim);
        }
        else
        {
            printfOutput(VERBOSE_BASE,
                "\nIncoming %sfile transfer request from: %s.\n",
                (m_flags & MFLAGTYPE_CRYPTNORM) ? "ENCRYPTED " : "",
                victim->info.nickName);

            printfOutput(VERBOSE_BASE,
                "(use the faccept/fdecline command "
                "to accept/decline the request)\n"
                "Filename: '%s' [%d kb].\n"
                "Reason: %s\n",
                YSM_CharsetConvertOutputString(&pfname, 1),
                fsize/1024,
                YSM_CharsetConvertOutputString(&preason, 1));
        }

        g_state.promptFlags |= FL_RAW;
    }
    else
    {
        /* File transfer cancelled */
    }

    if (pfname != NULL)
        YSM_FREE(pfname);

    if (preason != NULL)
        YSM_FREE(preason);

    return ret;
}

/* Handles RECEIVING of files.
 * binds a port and waits.
 */

static int32_t YSM_DC_FileTIncoming( slave_t *victim )
{
struct    sockaddr_in addr;
int32_t    addrlen, cli_sock;
struct    timeval tv;
fd_set    net_fd;

    if (victim == NULL)
        return -1;

    /* Wait for the client to connect */
    addrlen = sizeof(addr);
    cli_sock = accept( victim->d_con.finfo.rSocket,
            (struct sockaddr *)&addr,
            (int *)&addrlen );

    if (cli_sock < 0) return -1;

    /* required for both switches inside YSM_DC_CommonResponse() */
    victim->d_con.flags |= DC_EXPECTNEG;

    /* Loop through this socket */
    while(1) {

         /* Set a timeout of DC_TRANSFERTIMEOUT seconds to receive
         * activity. if the socket timeouts, the transfer connection is
         * closed.
          */

        FD_ZERO(&net_fd);
        tv.tv_sec = DC_TRANSFERTIMEOUT;
        tv.tv_usec = 0;
        FD_SET(cli_sock, &net_fd);

        if (select( cli_sock + 1, &net_fd, NULL, NULL, &tv) < 0) {
            printfOutput( VERBOSE_BASE,
            "File receiving aborted. "
            "The other side closed the connection.\n" );
            close( cli_sock );
            return -1;
        }

        /* drop connection if we reached connection timeout */
        if (!FD_ISSET( cli_sock, &net_fd )) {

            printfOutput( VERBOSE_BASE,
            "File receiving aborted. The connection timed out.\n");
            close( cli_sock );
            return -1;
        }

        /* else, generate a response */
        if (YSM_DC_CommonResponse( victim, cli_sock ) <= 0) {
            /* Abort transfer normally */
            close( cli_sock );
            return 0;
        }

        threadSleep( 0, 2 );
    }

    victim->d_con.flags &= ~DC_EXPECTNEG;

    return 0;
}


/* Handles TRANSFERING of files.
 * connects to the victim's specified port for transfering a file.
 * (called from YSM_DC_FileT())
 */

static int32_t YSM_DC_FileTOutgoing( slave_t *victim )
{
struct in_addr    r_addr;
uint32_t    rIP;
struct timeval    tv;
fd_set        net_fd;

    if (victim == NULL)
        return -1;

    victim->d_con.finfo.rSocket = 0;

    /* Can't connect if we dont know the port/addr where to transfer to */
    if (victim->d_con.finfo.rPort == 0
        || victim->d_con.rIP_ext == 0 || victim->d_con.rIP_int == 0) {
        printfOutput( VERBOSE_DCON,
            "FileTOutgoing(): victim's port to transfer is 0, "
            "aborting..\n");
        return -1;
    }

    /* Start with the external address! */
    if(victim->d_con.rIP_ext != 0)
        rIP = victim->d_con.rIP_ext;
    else
        rIP = victim->d_con.rIP_int;

    do {

        r_addr.s_addr = rIP;

        printfOutput( VERBOSE_DCON,
            "\n" MSG_DIRECT_CONNECTING "%s:%d..\n",
            inet_ntoa(r_addr),
            victim->d_con.finfo.rPort);


        victim->d_con.finfo.rSocket = YSM_Connect(
                    inet_ntoa(r_addr),
                    victim->d_con.finfo.rPort,
                    0x0 );

        if (rIP == victim->d_con.rIP_int) break;

        if (victim->d_con.finfo.rSocket < 0)
            rIP = victim->d_con.rIP_int;

    } while (victim->d_con.finfo.rSocket < 0);


    if (victim->d_con.finfo.rSocket < 0) {
        printfOutput(VERBOSE_BASE, MSG_DIRECT_ERR3 "\n");
        return -1;
    }

    printfOutput(VERBOSE_BASE, "\n" MSG_DIRECT_ESTABLISHED "\n");
    printfOutput(VERBOSE_BASE, "Started Transfering..\n"
        "You may cancel it by using the 'fcancel' command.\n"
        "You can check its status by using the 'fstatus' command.\n" );

    g_state.promptFlags |= FL_RAW;

    /* Send File Init! */
    YSM_DC_InitA( victim, victim->d_con.finfo.rSocket );

    /* required for both switches inside YSM_DC_CommonResponse() */
    victim->d_con.flags |= DC_EXPECTNEG;

    /* Loop through this socket */
    while(1) {

         /* Set a timeout of DC_TRANSFERTIMEOUT seconds to receive
         * activity. if the socket timeouts, the transfer connection is
         * closed.
          */

        FD_ZERO(&net_fd);
        tv.tv_sec = DC_TRANSFERTIMEOUT;
        tv.tv_usec = 0;
        FD_SET(victim->d_con.finfo.rSocket, &net_fd);
        select( victim->d_con.finfo.rSocket + 1,
            &net_fd,
            NULL,
            NULL,
            &tv);

        /* drop connection if we reached connection timeout */
        if (!FD_ISSET( victim->d_con.finfo.rSocket, &net_fd )) {
            close( victim->d_con.finfo.rSocket );
            return -1;
        }

        /* else, generate a response */
        if (YSM_DC_CommonResponse( victim,
                victim->d_con.finfo.rSocket ) <= 0) {
            /* Abort transfer normally */
            close( victim->d_con.finfo.rSocket );
            return 0;
        }

        threadSleep(0, 100);

    }

    victim->d_con.flags &= ~DC_EXPECTNEG;

    return 0;
}


/* Called for TRANSFERING and RECEIVING files */

static void YSM_DC_FileT( slave_t    *victim )
{

    if (victim->d_con.flags & DC_TRANSFERING) {
        YSM_DC_FileTOutgoing( victim );
    } else if (victim->d_con.flags & DC_RECEIVING) {
        YSM_DC_FileTIncoming( victim );
    }

    /* exiting thread */

}

/* INCOMING file transfers, decline a request!.
 * Takes care of sending a DECLINE ack for a transfer request.
 */

int32_t YSM_DC_FileDecline( slave_t *victim, int8_t *reason )
{
/* PEER_MSG_GREET has Greet Msg as extra data */
char    *file_greet = NULL, m_flags = 0;
int32_t    file_greet_len = 0;

    file_greet_len = YSM_DC_MessageGreet( victim,
                    0x29,
                    "File",
                    reason,
                    NULL,
                    0,
                    0,
                    &file_greet );

    if (file_greet == NULL || file_greet_len == 0)
        return -1;

    /* check if this is an outgoing ENCRYPT ack */
    if (!isKeyEmpty( victim->crypto.strkey )
        && victim->fprint == FINGERPRINT_YSM_CLIENT_CRYPT ) {
        m_flags |= MFLAGTYPE_CRYPTACK;
    } else {
        m_flags |= MFLAGTYPE_ACK;
    }

    return YSM_DC_MessageBase( victim,
                NULL,
                0,
                0x1a,
                0x0001,
                0x0000,
                file_greet,
                file_greet_len,
                m_flags,
                victim->d_con.seq_in );
}

/* OUTGOING file transfers.
 * Takes care of starting the file transfers TO a client.
 */

static int32_t YSM_DC_FileC( slave_t    *victim, uint16_t rport )
{
#ifdef WIN32
HANDLE        th;
DWORD        tid;
#elif OS2
int        tid;
#else
pthread_t    tid;
#endif

    /* Save the remote port */
    victim->d_con.finfo.rPort = ntohs(rport);

    /* Mark the slave as transfering a file */
    victim->d_con.flags |= DC_TRANSFERING;

#ifdef WIN32
    th = CreateThread(
        NULL,
        0,
        (LPTHREAD_START_ROUTINE)&YSM_DC_FileT,
        victim,
        0,
        (LPDWORD)&tid );
#elif OS2
    tid = _beginthread( (void *)&YSM_DC_FileT,
                    NULL,
                    THREADSTACKSIZE,
                    victim
                    );
#else
    pthread_create( &tid, NULL, (void *)&YSM_DC_FileT, victim );
#endif

    return 1;
}


/* INCOMING file transfers.
 * Takes care of sending the MSG_GREET_FILE packet
 * for ACKING a transfer and binding a port, etc..
 */

int32_t YSM_DC_FileB( slave_t    *victim,
        char        *filename,
        char        *reason )
{
/* PEER_MSG_GREET has Greet Msg as extra data */
char    *file_greet = NULL, m_flags = 0;
int32_t    file_greet_len = 0;
#ifdef WIN32
HANDLE        th;
DWORD        tid;
#elif OS2
int        tid;
#else
pthread_t    tid;
#endif


    /* Mark the slave as receiving a file */
    victim->d_con.flags |= DC_RECEIVING;

    /* Alloc a new port and send the port so the other side
     * can initiate the file transfer.
     */

    victim->d_con.finfo.rSocket = socket( AF_INET, SOCK_STREAM, 0 );
    if (victim->d_con.finfo.rSocket < 0) {
        printfOutput(VERBOSE_DCON, "YSM_DC_FileB: socket failed.\n" );
        return -1;
    }

    /* Fill our local port for the transfer, FileB() is waiting for it */
    victim->d_con.finfo.lPort = htons(g_cfg.dcport2);
    if (YSM_DC_BindPort( victim->d_con.finfo.rSocket,
            &victim->d_con.finfo.lPort ) < 0) return -1;
#ifdef WIN32
    th = CreateThread(
        NULL,
        0,
        (LPTHREAD_START_ROUTINE)&YSM_DC_FileT,
        victim,
        0,
        (LPDWORD)&tid );
#elif OS2
    tid = _beginthread( (void *)&YSM_DC_FileT,
                NULL,
                THREADSTACKSIZE,
                victim
                );
#else
    pthread_create( &tid, NULL, (void *)&YSM_DC_FileT, victim );
#endif

    file_greet_len = YSM_DC_MessageGreet( victim,
                    0x29,
                    "File",
                    reason,
                    filename,
                    victim->d_con.finfo.size,
                    victim->d_con.finfo.lPort,
                    &file_greet );

    if (file_greet == NULL || file_greet_len == 0)
        return -1;

    /* check if this is an outgoing ENCRYPT ack */
    if (!isKeyEmpty( victim->crypto.strkey )
    && victim->fprint == FINGERPRINT_YSM_CLIENT_CRYPT )
    {
        m_flags |= MFLAGTYPE_CRYPTACK;
    }
    else
    {
        m_flags |= MFLAGTYPE_ACK;
    }

    return YSM_DC_MessageBase( victim,
                NULL,
                0,
                0x1a,
                0x0000,
                0x0000,
                file_greet,
                file_greet_len,
                m_flags,
                victim->d_con.seq_in );
}


/* Takes care of sending the MSG_GREET_FILE packet
 * for initiating a transfer.
 */

static int32_t YSM_DC_FileA( slave_t    *victim, char *reason )
{
/* PEER_MSG_GREET has Greet Msg as extra data */
char    *file_greet = NULL, m_flags = 0;
int32_t    file_greet_len = 0;

    file_greet_len = YSM_DC_MessageGreet( victim,
                    0x29,
                    "File",
                    reason,
                    victim->d_con.finfo.name,
                    victim->d_con.finfo.size,
                    0x0000,
                    &file_greet );

    if (file_greet == NULL || file_greet_len == 0)
        return -1;

    /* check if we should encrypt the outgoing file transfer. */
    if (!isKeyEmpty( victim->crypto.strkey )
    && victim->fprint == FINGERPRINT_YSM_CLIENT_CRYPT )
    {
        m_flags |= MFLAGTYPE_CRYPTNORM;
    }
    else
    {
        m_flags |= MFLAGTYPE_NORM;
    }

    YSM_DC_MessageBase( victim,
            NULL,
            0,
            0x1a,
            0x0000,
            0x0000,
            file_greet,
            file_greet_len,
            m_flags,
            YSM_USER.d_con.seq_out-- );

    return 0;
}

/*
 * should take care of filtering unwanted characters on file names.
 * either on sending or receiving files.
 */

void YSM_DC_FileParseFilename( int8_t *fname )
{
int8_t        *paux = NULL;
int32_t        newsize = 0;

#ifdef WIN32
    paux = strrchr(fname, '\\');
#else
    paux = strrchr(fname, '/');
#ifdef OS2    /* OS2 supports both '/' and '\' */
    if (paux == NULL) paux = strrchr(fname, '\\');
#endif
#endif
    if (paux != NULL) {
        newsize = strlen(paux) - 1;
        memcpy( fname, paux+1, newsize );
        fname[newsize] = 0x00;
    }
}

/* First step is sending a MSG_GREET with a File request.
 * We then get a MSG_GREET with a File request ACK if it was ok.
 * We then open a new socket against the remote client and send
 * a FILE_INIT packet. We get a FILE_INIT_ACK.
 * We have to send a FILE_START before we send each file. for
 * what we get a FILE_START_ACK right before we start sending
 * the data.
 */

int32_t YSM_DC_File( slave_t    *victim, int8_t *fname, int8_t *desc )
{
struct stat    filestat;

    /* Check if the file exists */
    if (stat(fname, &filestat) < 0) {
        printfOutput( VERBOSE_BASE, "File not found. Try the full path!.\n");
        return -1;
    }

    if (desc == NULL) desc = "File sent with ysmICQ";

    /* Open the file and save its FD */
    victim->d_con.finfo.fd = fopen(fname, "rb");
    if (victim->d_con.finfo.fd == NULL) {
        printfOutput( VERBOSE_BASE, "Unable to open the file for reading.\n");
        return -1;
    }

    victim->d_con.finfo.size = filestat.st_size;
    /* totsize allowing 1 file transfer */
    victim->d_con.finfo.totsize = victim->d_con.finfo.size;

    YSM_DC_FileParseFilename(fname);

    if (strlen(fname) >= MAX_PEER_FILENAMEL) {
        printfOutput( VERBOSE_BASE, "Filename too long. Don't get mad..\n");
        return -1;
    }

    memset(victim->d_con.finfo.name, 0, MAX_PEER_FILENAMEL);
    memcpy(victim->d_con.finfo.name, fname, strlen(fname));

    YSM_DC_FileA( victim, desc );
    return 0;
}

int32_t YSM_DC_FileInit( slave_t *victim, int32_t numfiles, int32_t numbytes )
{
int8_t    *buf = NULL;
int32_t    pos = 0, ret = TRUE;

    pos += 2;            /* cmd len */
    pos ++;                /* byte command */
    pos += 4;            /* empty dword */
    pos += 4;            /* total num of files dword */
    pos += 4;            /* total num of bytes dword */

    /* speed: 0 == PAUSE, 64 == NODELAY, (0<n<64) == (64-n)*0.05s delay */
    pos += 4;

    /* Sender's nick, word + zero */
    pos += strlen(YSM_USER.info.nickName) + 3;

    buf = ysm_calloc( 1, pos, __FILE__, __LINE__ );
    if (buf == NULL) return -1;

    pos = 0;

    buf[pos] = 19 + strlen(YSM_USER.info.nickName) + 1;
    pos += 2;

    buf[pos] = PEER_FILE_INIT;
    pos ++;

    pos += 4;

    DW_2_Chars( &buf[pos], numfiles );
    pos += 4;

    DW_2_Chars( &buf[pos], numbytes );
    pos += 4;

    /* set NO DELAY speed by default */
    victim->d_con.finfo.speed = 0x64;

    DW_2_Chars( &buf[pos], victim->d_con.finfo.speed );
    pos += 4;

    buf[pos] = strlen(YSM_USER.info.nickName) + 1;
    pos += 2;

    memcpy( &buf[pos],
        YSM_USER.info.nickName,
        strlen(YSM_USER.info.nickName) );
    pos += strlen(YSM_USER.info.nickName) + 1;

    ret = YSM_WRITE_DC( victim, victim->d_con.finfo.rSocket, buf, pos );

    YSM_FREE(buf);
    return ret;
}

int32_t YSM_DC_FileInitAck( slave_t *victim, int32_t sock, int8_t *buf, int32_t len )
{
int8_t    *data = NULL;
int32_t    pos = 0, num_files = 0, num_bytes = 0, speed = 0, ret = TRUE;

    /* first parse incoming data and store required data */
    pos ++;                /* byte command */
    pos += 4;            /* empty dword */
    num_files = Chars_2_DW(buf+pos);
    pos += 4;            /* total num of files dword */
    num_bytes = Chars_2_DW(buf+pos);
    pos += 4;            /* total num of bytes dword */
    speed = Chars_2_DW(buf+pos);
    /* speed: 0 == PAUSE, 64 == NODELAY, (0<n<64) == (64-n)*0.05s delay */
    pos += 4;

    /* carlin checks */
    if (num_files <= 0 || num_bytes <= 0 || speed <= 0) return -1;

    victim->d_con.finfo.totsize    = num_bytes;
    victim->d_con.finfo.totnum    = num_files;
    victim->d_con.finfo.num        = 1;
    victim->d_con.finfo.speed    = speed;

    pos = 0;
    /* finished parsing required data */

    pos += 2;            /* cmd len */
    pos ++;                /* byte command */
    pos += 4;            /* other end's speed */

    /* other end's nick, word + zero */
    pos += strlen(victim->info.nickName) + 3;

    data = ysm_calloc(1, pos, __FILE__, __LINE__ );
    if (buf == NULL) return -1;

    pos = 0;
    data[pos] = 7 + strlen(victim->info.nickName) + 1;
    pos += 2;

    data[pos] = PEER_FILE_INIT_ACK;
    pos ++;

    DW_2_Chars( &data[pos], victim->d_con.finfo.speed );
    pos += 4;

    data[pos] = strlen(victim->info.nickName) + 1;
    pos += 2;

    memcpy( &data[pos],
        victim->info.nickName,
        strlen(victim->info.nickName) );

    pos += strlen(victim->info.nickName) + 1;

    ret = YSM_WRITE_DC( victim, sock, data, pos );

    YSM_FREE(data);
    return ret;
}

int32_t YSM_DC_FileStart( slave_t *victim )
{
int8_t    *buf = NULL;
int32_t    pos = 0, ret = TRUE;

    pos += 2;                /* cmd len */
    pos ++;                    /* byte command */
    pos ++;                    /* Attribute (File(0)/Dir(1)) */

    /* filename of the file to transfer */
    pos += strlen(victim->d_con.finfo.name) + 3;
    /* unknown empty stringz */
    pos += 3;

    pos += 4;                /* length of file to transfer */
    pos += 4;                /* unknown */
    pos += 4;                /* speed of transfer */

    buf = ysm_calloc( 1, pos, __FILE__, __LINE__ );
    if (buf == NULL) return -1;

    pos = 0;

    buf[pos] = 19 + strlen(victim->d_con.finfo.name) + 1;
    pos += 2;

    buf[pos] = PEER_FILE_START;
    pos ++;

    buf[pos] = 0x00;    /* File? When should this be dir anyway? */
    pos ++;

    buf[pos] = strlen(victim->d_con.finfo.name) + 1;
    pos += 2;

    memcpy( &buf[pos],
        victim->d_con.finfo.name,
        strlen(victim->d_con.finfo.name) );
    pos += strlen(victim->d_con.finfo.name) + 1;

    buf[pos] = 0x01;
    pos += 3;

    DW_2_Chars( &buf[pos], victim->d_con.finfo.size );
    pos += 4;

    pos += 4;
    DW_2_Chars( &buf[pos], victim->d_con.finfo.speed );
    pos += 4;

    ret = YSM_WRITE_DC( victim, victim->d_con.finfo.rSocket, buf, pos );

    YSM_FREE(buf);
    return ret;
}


int32_t YSM_DC_FileStartAck( slave_t    *victim,
        int32_t        sock,
        int8_t        *data,
        int32_t        len )
{
int8_t    *buf = NULL;
int32_t    pos = 0, num_bytes = 0, speed = 0, ret = TRUE;


    /* first parse incoming data and store required data */
    pos ++;                /* byte command */
    pos ++;                /* Attribute (File(0)/Dir(1)) */
    /* use speed as a temp variable */
    speed = Chars_2_Word(data+pos);
    pos += 2;
    if ((speed + pos) >= len) return -1;

    memset( victim->d_con.finfo.name, 0, MAX_PEER_FILENAMEL );

    memcpy( &victim->d_con.finfo.name[0],
        &data[pos],
        (speed >= MAX_PEER_FILENAMEL) ? MAX_PEER_FILENAMEL-1 : speed );

    pos += speed;
    pos += 3;

    num_bytes = Chars_2_DW(data+pos);
    pos += 4;                /* length of file to transfer */
    pos += 4;                /* unknown */
    speed = Chars_2_DW(data+pos);
    pos += 4;                /* speed of transfer */

    /* carlin checks */
    if (num_bytes <= 0 || speed <= 0) return -1;

    victim->d_con.finfo.size = num_bytes;
    victim->d_con.finfo.speed = speed;

    /* finished parsing required data */
    pos = 0;
    pos += 2;        /* cmd len */
    pos ++;            /* byte command */
    pos += 4;        /* offset from file from where to transfer */
    pos += 4;        /* empty */
    pos += 4;        /* other end's speed */
    pos += 4;        /* number of the file being acked */

    buf = ysm_calloc( 1, pos, __FILE__, __LINE__ );
    if (buf == NULL) return -1;

    pos = 0;
    buf[pos] = 17;
    pos += 2;

    buf[pos] = PEER_FILE_START_ACK;
    pos ++;

    pos += 4;    /* what offset? this could allow 'resuming' */

    pos += 4;
    DW_2_Chars(buf+pos, victim->d_con.finfo.speed);
    pos += 4;

    /* file num being acked */
    DW_2_Chars(buf+pos, victim->d_con.finfo.num);
    pos += 4;

    ret = YSM_WRITE_DC( victim, sock, buf, pos );

    YSM_FREE(buf);
    return ret;
}

int32_t YSM_DC_FileStop( slave_t *victim, int32_t numfile )
{
int8_t    *buf = NULL;
int32_t    pos = 0, ret = TRUE;

    pos += 2;            /* cmd len */
    pos ++;                /* byte command */
    pos += 4;            /* file number being stopped */

    buf = ysm_calloc(1, pos, __FILE__, __LINE__ );
    if (buf == NULL) return -1;

    pos = 0;
    buf[pos] = 5;
    pos += 2;

    buf[pos] = PEER_FILE_STOP;
    pos ++;

    DW_2_Chars(buf+pos, numfile);      /* file num being stopped! */
    pos += 4;

    ret = YSM_WRITE_DC( victim, victim->d_con.finfo.rSocket, buf, pos );

    YSM_FREE(buf);
    return ret;
}


int32_t YSM_DC_FileSpeed( slave_t *victim, int32_t newspeed )
{
int8_t    *buf = NULL;
int32_t    pos = 0, ret = TRUE;

    pos += 2;            /* cmd len */
    pos ++;                /* byte command */
    pos += 4;            /* new speed */

    buf = ysm_calloc( 1, pos, __FILE__, __LINE__ );
    if (buf == NULL) return -1;

    pos = 0;
    buf[pos] = 0x05;
    pos += 2;

    buf[pos] = PEER_FILE_SPEED;
    pos ++;

    DW_2_Chars(&buf[pos], newspeed);
    pos += 4;

    ret = YSM_WRITE_DC( victim, victim->d_con.finfo.rSocket, buf, pos );

    YSM_FREE(buf);
    return ret;
}

/* doesn't really ACK, it stores the new speed.
 * btw, we arent updating the speed nowadays, it breaks
 * our sending.
 */

int32_t YSM_DC_FileSpeedAck( slave_t *victim, int8_t *buf, int32_t len )
{
int32_t    pos = 0, ret = TRUE, newspeed = 0;


    pos ++;                /* byte command */

    newspeed = Chars_2_DW(buf+pos);
    pos += 4;

    victim->d_con.finfo.speed = newspeed;
    return ret;
}

int32_t YSM_DC_FileData( slave_t *victim, int8_t *data, int32_t dlen )
{
int8_t    *buf = NULL;
int32_t    pos = 0, ret = TRUE;

    pos += 2;        /* cmd len */
    pos ++;            /* byte command */
    pos += dlen;        /* data size */

    buf = ysm_calloc( 1, pos, __FILE__, __LINE__ );
    if (buf == NULL) return -1;

    pos = 0;

    Word_2_Chars(&buf[pos], dlen+1 );
    pos += 2;

    buf[pos] = PEER_FILE_DATA;
    pos ++;

    memcpy( &buf[pos], data, dlen );
    pos += dlen;

    ret = YSM_WRITE_DC( victim, victim->d_con.finfo.rSocket, buf, pos );

    YSM_FREE(buf);
    return ret;
}

int32_t YSM_DC_FileTransfer( slave_t *victim, int8_t *buf, int32_t len )
{
int8_t    *read_buf, *pread_buf = NULL;
int32_t    read_len = 0, read_len2 = 0, transbytes = 0, foffset = 0;
double    sleepspeed = 0, transkb = 0;
keyInstance    *crypt_key = NULL;

    read_buf = ysm_malloc( MAX_PEER_DATA_SIZE, __FILE__, __LINE__ );
    if (read_buf == NULL) return 0;

    /* we maintain two pointers because after a successfull encryption
     * the used pointer is changed to a new allocated area. so when we
     * free our data we free it using the original pointer + the new.
     */
    pread_buf = read_buf;

    victim->d_con.finfo.statstime = time(NULL);

    /* get the offset from where to start sending */
    memcpy(&foffset, buf+1, 4);
    if (fseek(victim->d_con.finfo.fd, foffset, SEEK_SET) != 0) {
        YSM_FREE(pread_buf);
        return 0;
    }

    if (foffset) {
        printfOutput( VERBOSE_BASE, "\nThe slave requested to resume..\n" );
        victim->d_con.finfo.size -= foffset;
    }

    sleepspeed = (0x64 - victim->d_con.finfo.speed);
    if (sleepspeed > 0) sleepspeed = sleepspeed * 0.05;
    else sleepspeed = 0;

    while (victim->d_con.finfo.fd != NULL
    && !feof(victim->d_con.finfo.fd)) {

        /* we read 2048 minus the minimum space we would need if this
         * was an encrypted file transfer. */
        read_len = fread( read_buf,
                    1,
                    2048 -
                    sizeof(struct YSMCryptH) -
                    MAX_CRYPT_PADDING,
                    victim->d_con.finfo.fd );

        if (read_len) {

            /* read_len2 is used because its size might be changed
             * through the encryption routines */
            read_len2 = read_len;

            /* encrypt the outgoing file data if neccesary */
            crypt_key = YSM_EncryptAnyData( victim,
                            &read_buf,
                            &read_len2,
                            MAX_PEER_DATA_SIZE );

            if (YSM_DC_FileData( victim, read_buf, read_len2 ) <= 0)
            {
                printfOutput( VERBOSE_BASE,
                    "\nAn error showed up during the "
                    "transfer, aborting..\n" );

                YSM_FREE(pread_buf);

                if (crypt_key != NULL)
                    YSM_FREE(read_buf);

                return 0;
            }
        }

        victim->d_con.finfo.size -= read_len;
        transbytes += read_len;

        /* update our kb/s trace */
        if ((time(NULL) - victim->d_con.finfo.statstime) > 1) {
            victim->d_con.finfo.kbs =
            (transbytes - victim->d_con.finfo.statsbytes)/1024;
            victim->d_con.finfo.statsbytes = transbytes;
            victim->d_con.finfo.statstime = time(NULL);
        }

        threadSleep(0, (int32_t)sleepspeed);
    }

    if (victim->d_con.finfo.size) {
        printfOutput( VERBOSE_BASE,
            "\nFile named '%s' transfered incompletely.\n",
            victim->d_con.finfo.name );
    } else {
        printfOutput( VERBOSE_BASE,
            "\nFile named '%s' transfered successfully.\n",
            victim->d_con.finfo.name );
    }

    transkb = transbytes/1024;

    printfOutput( VERBOSE_BASE,
        "%sFile transfer finished [%d bytes (%.0f kb) transfered].\n",
        (crypt_key != NULL) ? "Encrypted " : "",
        transbytes,
        transkb );

    g_state.promptFlags |= FL_RAW;

    YSM_FREE(pread_buf);

    if (crypt_key != NULL) {
        YSM_FREE(read_buf);
    }

    return 0;
}


int32_t YSM_DC_FileReceiveOpenHandle( slave_t *victim )
{
int32_t        size;
int8_t        *path = NULL;
struct stat    filestat;

    if (victim->d_con.finfo.fd != NULL) {
        /* descriptor is already open */
        return 0;
    }

    size = strlen(g_state.configDir) + 1;
    size += strlen(YSM_INCOMINGDIRECTORY) + 1;
    size += strlen(victim->d_con.finfo.name) + 1;
    path = ysm_calloc(1, size, __FILE__, __LINE__ );

    /* does our incoming directory exist? */
    snprintf( path,
        size,
        "%s/%s",
        g_state.configDir,
        YSM_INCOMINGDIRECTORY );

    path[size - 1] = 0x00;

    if (stat(path, &filestat)) {
        printfOutput( VERBOSE_BASE,
            "incoming directory doesn't exist.\n"
            "Creating %s\n", path );

        /* mkdir returns 0 if success */
        if (mkdir(path, 0700)) {
            printfOutput( VERBOSE_BASE, "Couldn't create directory.\n" );
            YSM_FREE(path);
            return -1;
        }
    }

    /* last minute file checks, we shouldn't reach this code though */
    if (strstr(victim->d_con.finfo.name, "\\..")
    || strstr(victim->d_con.finfo.name,"..\\")
    || strstr(victim->d_con.finfo.name,"../")
    || strstr(victim->d_con.finfo.name,"/..")) {
            printfOutput( VERBOSE_BASE,
            "Suspicious filename detected. Aborting..\n");
            return -1;
    }

    snprintf( path,
        size,
        "%s/%s/%s",
        g_state.configDir,
        YSM_INCOMINGDIRECTORY,
        victim->d_con.finfo.name );

    path[size - 1] = 0x00;

    victim->d_con.finfo.fd = fopen( path, "ab" );
    if (victim->d_con.finfo.fd == NULL) {
        printfOutput( VERBOSE_BASE, "Unable to open %s for write.\n", path);
        YSM_FREE(path);
        return -1;
    }

    YSM_FREE(path);

    return 0;
}

int32_t YSM_DC_FileReceive( slave_t *victim, int8_t *buf, int32_t len )
{
size_t        ret = 0;
int8_t        *writedata = NULL;
int32_t        writelen = 0;
double        transkb = 0;
keyInstance    *key = NULL;


    if (YSM_DC_FileReceiveOpenHandle( victim ) < 0)
        return 0;

    writedata = buf;
    writedata++;    /* byte command */
    writelen = len - 1;

    /* decrypt the incoming file data if neccesary */
    if (YSM_DecryptFileData( victim,
                &writedata,
                &writelen,
                &key ) < 0) {

        /* this is a very critical error.. */
        return 0;
    }

    /* since this function is called several times to receive a file
     * we do the following: */
    if (victim->d_con.finfo.statstime == 0)
        victim->d_con.finfo.statstime = time(NULL);

    ret = fwrite( writedata, 1, writelen, victim->d_con.finfo.fd );
    if (ret != (size_t)writelen) {
        printfOutput( VERBOSE_BASE, "fwrite() returned an invalid len.\n");
        return 0;
    }

    victim->d_con.finfo.size -= ret;

    /* update our kb/s trace */
    if ((time(NULL) - victim->d_con.finfo.statstime) > 1) {
        victim->d_con.finfo.kbs =
            (victim->d_con.finfo.statsbytes -
             victim->d_con.finfo.size)/1024;

            victim->d_con.finfo.statsbytes =
                victim->d_con.finfo.size;

            victim->d_con.finfo.statstime = time(NULL);
    }

    /* Successful file transfered */
    if (!victim->d_con.finfo.size) {
        victim->d_con.finfo.totnum--;
        victim->d_con.finfo.num++;

        printfOutput( VERBOSE_BASE,
            "\nFile named '%s' received successfully.\n",
            victim->d_con.finfo.name );

        transkb = victim->d_con.finfo.totsize/1024;
        if (!victim->d_con.finfo.totnum) {
            printfOutput( VERBOSE_BASE,
                "%sFile transfer finished "
                "[%d files - %d bytes (%.0f kb) received].\n",
                (key != NULL) ? "Encrypted " : "",
                victim->d_con.finfo.num-1,
                victim->d_con.finfo.totsize,
                transkb );
        }

        g_state.promptFlags |= FL_RAW;
        return 0;
    }

    return 1;
}
