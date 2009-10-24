#include "ysm.h"
#include "commands.h"
#include "prompt.h"
#include "setup.h"
#include "toolbox.h"
#include "slaves.h"
#include "timers.h"
#include "wrappers.h"
#include "bytestream.h"
#include "network.h"
#include "direct.h"
#include "crypt.h"
#include "output.h"
#include "misc.h"
#include "fingerprint.h" /* FINGERPRINT_YSM_CLIENT_CRYPT */
#include "icqv7.h"       /* CLI_FULLINFO_REQ */

static command_t g_command_list[] = {
    { "quit",       "q",        YSM_COMMAND_QUIT_HELP,          CG_CLIENT,   0, &cmdQUIT },
    { "help",       "?",        YSM_COMMAND_HELP_HELP,          CG_CLIENT,   0, &cmdHELP },
    { "uptime",     NULL,       YSM_COMMAND_UPTIME_HELP,        CG_CLIENT,   0, &cmdUPTIME },
    { "backdoor",   NULL,       YSM_COMMAND_BACKDOOR_HELP,      CG_CLIENT,   0, &cmdBACKDOOR },
    { "connect",    NULL,       YSM_COMMAND_CONNECT_HELP,       CG_CLIENT,   0, &cmdCONNECT },
    { "disconnect", NULL,       YSM_COMMAND_DISCONNECT_HELP,    CG_CLIENT,   0, &cmdDISCONNECT },
    { "slaves",     "w",        YSM_COMMAND_SLAVES_HELP,        CG_USERS,    0, &cmdSLAVES },
    { "ls",         "l",        YSM_COMMAND_SLAVES_HELP,        CG_USERS,    0, &cmdSLAVES },
    { "slavesall",  "wa",       YSM_COMMAND_SLAVESALL_HELP,     CG_USERS,    0, &cmdSLAVESALL },
    { "slaveson",   "wo",       YSM_COMMAND_SLAVESON_HELP,      CG_USERS,    0, &cmdSLAVESON },
    { "addslave",   "add",      YSM_COMMAND_ADDSLAVE_HELP,      CG_USERS,    2, &cmdADDSLAVE },
    { "delslave",   "del",      YSM_COMMAND_DELSLAVE_HELP,      CG_USERS,    1, &cmdDELSLAVE },
    { "msg",        "m",        YSM_COMMAND_MSG_HELP,           CG_USERS,    1, &cmdMSG },
    { "mplain",     "mp",       YSM_COMMAND_MPLAIN_HELP,        CG_USERS,    1, &cmdMPLAIN },
    { "chat",       "ch",       YSM_COMMAND_CHAT_HELP,          CG_USERS,    0, &cmdCHAT },
    { "whois",      NULL,       YSM_COMMAND_WHOIS_HELP,         CG_USERS,    1, &cmdWHOIS },
    { "search",     NULL,       YSM_COMMAND_SEARCH_HELP,        CG_USERS,    1, &cmdSEARCH },
    { "save",       NULL,       YSM_COMMAND_SAVE_HELP,          CG_USERS,    0, &cmdSAVE },
    { "req",        NULL,       YSM_COMMAND_REQ_HELP,           CG_USERS,    1, &cmdREQ },
    { "auth",       NULL,       YSM_COMMAND_AUTH_HELP,          CG_USERS,    1, &cmdAUTH },
    { "rename",     "mv",       YSM_COMMAND_RENAME_HELP,        CG_USERS,    2, &cmdRENAME },
    { "ignore",     "ign",      YSM_COMMAND_IGNORE_HELP,        CG_USERS,    1, &cmdIGNORE },
    { "visible",    "vis",      YSM_COMMAND_VISIBLE_HELP,       CG_USERS,    1, &cmdVISIBLE },
    { "invisible",  "inv",      YSM_COMMAND_INVISIBLE_HELP,     CG_USERS,    1, &cmdINVISIBLE },
    { "key",        NULL,       YSM_COMMAND_KEY_HELP,           CG_USERS,    1, &cmdKEY },
#ifdef YSM_WAR_MODE
    { "scan",       NULL,       YSM_COMMAND_SCAN_HELP,          CG_USERS,    1, &cmdSCAN },
    { "kill",       NULL,       YSM_COMMAND_KILL_HELP,          CG_USERS,    1, &cmdKILL },
#endif
    { "rtf",        NULL,       YSM_COMMAND_KILL_HELP,          CG_USERS,    1, &cmdRTF },
    { "seen",       NULL,       YSM_COMMAND_SEEN_HELP,          CG_USERS,    1, &cmdSEEN },
    { "contacts",   "contact",  YSM_COMMAND_CONTACTS_HELP,      CG_USERS,    2, &cmdCONTACTS },
    { "url",        NULL,       YSM_COMMAND_URL_HELP,           CG_USERS,    1, &cmdURL },
    { "opendc",     NULL,       YSM_COMMAND_OPENDC_HELP,        CG_USERS,    1, &cmdOPENDC },
    { "closedc",    NULL,       YSM_COMMAND_CLOSEDC_HELP,       CG_USERS,    1, &cmdCLOSEDC },
    { "faccept",    NULL,       YSM_COMMAND_FILEACCEPT_HELP,    CG_USERS,    1, &cmdFILEACCEPT },
    { "fdecline",   NULL,       YSM_COMMAND_FILEDECLINE_HELP,   CG_USERS,    1, &cmdFILEDECLINE },
    { "send",       "file",     YSM_COMMAND_SEND_HELP,          CG_USERS,    3, &cmdSEND },
    { "fstatus",    NULL,       YSM_COMMAND_FILESTATUS_HELP,    CG_USERS,    0, &cmdFILESTATUS },
    { "fcancel",    NULL,       YSM_COMMAND_FILECANCEL_HELP,    CG_USERS,    1, &cmdFILECANCEL },
    { "info",       NULL,       YSM_COMMAND_INFO_HELP,          CG_ACCOUNT,  0, &cmdINFO },
    { "status",     "s",        YSM_COMMAND_STATUS_HELP,        CG_ACCOUNT,  0, &cmdSTATUS },
    { "nick",       NULL,       YSM_COMMAND_NICK_HELP,          CG_ACCOUNT,  0, &cmdNICK },
    { "email",      NULL,       YSM_COMMAND_EMAIL_HELP,         CG_ACCOUNT,  0, &cmdEMAIL },
    { "password",   NULL,       YSM_COMMAND_PASSWORD_HELP,      CG_ACCOUNT,  1, &cmdPASSWORD },
    { "loadconfig", NULL,       YSM_COMMAND_LOADCONFIG_HELP,    CG_SETTINGS, 0, &cmdLOADCONFIG },
#ifdef YSM_TRACE_MEMLEAK
    { "showleak",   NULL,       YSM_COMMAND_HELP_HELP,          CG_SETTINGS, 0, &cmdSHOWLEAK },
#endif
    { NULL,         NULL,       NULL,                           0,           0, NULL }
};

extern int8_t YSM_DefaultCHATMessage[MAX_DATA_LEN + 1];

static void cmdQUIT(uint16_t argc, int8_t **argv)
{
    g_state.reasonToSuicide = TRUE;
}

void cmdHELP(uint16_t argc, int8_t **argv)
{
    uint16_t        y = 0;
    command_group_t  group;
    string_t        *str;
    uint8_t         i;

    str = initString();

    if (argc == 0)
    {
        printfString(str,
            "List of commands: \n"
            "run help + command for detailed information.\n");

        for (group = CG_USERS; group < CG_EXTRA; group++)
        {
            switch (group)
            {
                case CG_CLIENT:
                    printfString(str, "\n\r"CG_CLIENT_STR );
                    break;

                case CG_ACCOUNT:
                    printfString(str, "\n\r"CG_ACCOUNT_STR );
                    break;

                case CG_SETTINGS:
                    printfString(str, "\n\r"CG_SETTINGS_STR );
                    break;

                case CG_USERS:
                    printfString(str, "\n\r"CG_USERS_STR );
                    break;
            }

            for (i = 0, y = 0; g_command_list[i].name != NULL; i++, y++)
            {
                if (g_command_list[i].group != group)
                {
                    if (y) y--;
                    continue;
                }

                if (g_command_list[i].help == NULL)
                {
                    if (y) y--;
                    continue;
                }

                if (y && !(y % 4))
                {
                    printfString(str, "\n" );
                    y = 0;
                }

                if (g_command_list[i].alias != NULL)
                {
                    printfString(str, " < %s or %s >",
                        g_command_list[i].name,
                        g_command_list[i].alias);
                }
                else
                {
                    printfString(str, " < %s > ", g_command_list[i].name);
                }
            }
        }
        printfString(str, "\n");
    }
    else
    {
        /* detailed help for a command */
        for (i = 0; g_command_list[i].name != NULL; i++)
        {
            if (g_command_list[i].name[0] != argv[1][0]
            || strcasecmp(g_command_list[i].name, argv[1]))
            {
                if (g_command_list[i].alias != NULL)
                {
                    if (g_command_list[i].alias[0] != argv[1][0]
                    || strcasecmp(g_command_list[i].alias, argv[1]))
                        continue;
                }
                else
                    continue;
            }

            if (g_command_list[i].help == NULL)
                break;

            printfString(str,
                "Detailed information for '%s'.\n"
                "%s\n",
                argv[1],
                g_command_list[i].help);
            break;
        }
    }

    writeOutput(VERBOSE_BASE, getString(str));
    freeString(str);
}

static void cmdINFO(uint16_t argc, int8_t **argv)
{
    struct    in_addr ysmintaddr, ysmextaddr;
    string_t *str;

    str = initString();

    printfString(str,        
        "INFO GENERAL\n"
        "* Program information\n" 
        "Release name: %s\n"
        "Release version: %s\n",
        YSM_INFORMATION,
        YSM_INFORMATION2);

    printfString(str, "* Session information\n"
        "UIN: %ld\n"
        "Slaves: %ld\n"
        "Sequence #: %d\n"
        "Nickname: %s\n"
        "Firstname: %s\n"
        "Lastname: %s\n"
        "Email: %s\n",
         g_model.uin,
         getSlavesListLen(),
         g_sinfo.seqnum,
         g_model.user.nickName,
         g_model.user.firstName,
         g_model.user.lastName,
         g_model.user.email);

    ysmextaddr.s_addr = g_model.dc.rIP_ext;
    ysmintaddr.s_addr = g_model.dc.rIP_int;

    /* we had to split the printfStrings in two due to the static buffer
     * inet_ntoa uses, think about it! I'm so angry!.
     */

    printfString(str, "* DC information\n");

    printfString(str,
        "Internal IP address: %s\n", inet_ntoa(ysmintaddr));

    printfString(str,
        "External IP address: %s\n"
        "Bound port: %d\n",
        inet_ntoa(ysmextaddr),
        ntohs(g_model.dc.rPort));

    writeOutput(VERBOSE_BASE, getString(str));
    freeString(str);
}

static void cmdLOADCONFIG(uint16_t argc, int8_t **argv)
{
    printfOutput(VERBOSE_BASE, "INFO RELOAD_CONFIG\n");
    initConfig();
}

static void cmdSLAVES(uint16_t argc, int8_t **argv)
{
    printOrganizedSlaves(STATUS_OFFLINE, argv[1], 0x0);
}

static void cmdADDSLAVE(uint16_t argc, int8_t **argv)
{
    uint32_t   x = 0;
    int8_t     *pnick = NULL, *puin = NULL;

    /* we allow 2 ways of adding a slave. either add nick uin
     * or add uin nick. Start by checking argv[1] and then argv[2].
     * we can't allow adding a slave name just made of numbers.
     */

    for (x = 0; x < strlen(argv[1]); x++)
    {
        if (!isdigit((int)argv[1][x]))
        {
            pnick = argv[1];
            puin = argv[2];
            break;
        }
    }

    /* then check argv[2] .. */

    for (x = 0; x < strlen(argv[2]); x++)
    {
        if (!isdigit((int)argv[2][x]))
        {
            if (pnick != NULL)
            {
                /* can't have 2 nicks! need a #! */
                printfOutput(VERBOSE_BASE,
                    "Syntax mistake. "
                    "Usage: add NAME UIN or add UIN NAME\n");
                return;
            }
            else
            {
                pnick = argv[2];
                puin = argv[1];
                break;
            }
        }
    }

    if (pnick == NULL)
    {
        /* the nick specified is a #! We can't distinguish
         * which one of them is the nick and which the UIN. */

        printfOutput(VERBOSE_BASE,
            "Syntax mistake. "
            "A nickname must contain non-numeric characters.\n");
        return;
    }

    addSlave(pnick, atoi(puin));
    YSM_SendContacts();
}

static void cmdDELSLAVE(uint16_t argc, int8_t **argv)
{
    slave_t *slave = NULL;

    lockSlaveList();

    slave = getSlaveByNick(argv[1]);
    
    if (slave)
    {
        /* check the slave is out of your ignore and invisible lists first
         * if done manually this saves us some packets ;) */

        if (slave->budType.ignoreId || slave->budType.invisibleId ||
                slave->budType.visibleId)
        {
            printfOutput(VERBOSE_BASE,
                    "Slave won't be removed until he/she is removed from "
                    "your IGNORE, VISIBLE and\n INVISIBLE lists. Please be "
                    "sure to remove him/her from such lists before.\n"
                    "deleting the slave.\n[read about the 'invisible', "
                    "'visible' and 'ignore' commands]\n");

            return;
        }

        sendRemoveContactReq(slave->uin);

        /* Now remove from the server too (only if it was stored up there) */
        if (IS_DOWNLOADED(slave->flags))
            YSM_BuddyDelSlave(slave);

        deleteSlave(slave);
    }
    else
    {
        if (!isdigit((int)argv[1][0]))
        {
            printfOutput(VERBOSE_BASE,
                "ERR Unknown slave name. Won't delete a ghost!\n");
        }
        else
        {
            printfOutput(VERBOSE_BASE,
                "ERR Incorrect use. Try del 'slavename'.\n");
        }
    }

    unlockSlaveList();
}

static void cmdAUTH(uint16_t argc, int8_t **argv)
{
    slave_t *slave = NULL;

    lockSlaveList();
    
    slave = getSlaveByNick(argv[1]);

    if (slave)
    {
        sendAuthRsp(slave->uin);
    }
    else
    {
        if (!isdigit((int)argv[1][0])) 
        {
            printfOutput(VERBOSE_BASE,
                "ERR SLAVE Unknown. Authorization cancelled.\n");
        }
        else
        {
            sendAuthRsp(atoi(argv[1]));
        }
    }

    unlockSlaveList();
}

/* Validates the Destination field    */
/* If its a multidest message or single    */

static int cmdMSG_ValidateDest(char *dst)
{
    char     *idx = NULL, *dest = NULL;
    int       amount = 0;
    bool_t    last = FALSE;

    DEBUG_PRINT("");

    idx = dest = dst;
    for (amount = 0; (idx = strchr(dest, ',')) || !last; amount++)
    {
        if (idx)
            *idx = '\0';

        if (!getSlaveByNick(dest) && !isdigit((int)dest[0]))
        {
            printfOutput(VERBOSE_BASE,
                    "(%s) - slave unknown. message cancelled.\n",
                    dest);

            return -1;
        }

        if (idx)
            dest = idx + 1;
        else
            last = TRUE;
    }

    return amount;
}

/* if plainflag == TRUE, send only plain messages.  */

static void cmdMSG_main(uint16_t argc, int8_t **argv, char plainflag)
{
    slave_t   *victim = NULL;
    slave_t    strg;
    char      *aux = NULL, *dest = NULL;
    int32_t    x = 0, amount = 0, amount_cpy = 0;
    uint32_t   fprint_bkp = 0;

    DEBUG_PRINT("");

    amount = cmdMSG_ValidateDest(argv[1]);
    /* amount has the amount of destinations */
    if (amount < 0) return;

    DEBUG_PRINT("");

    lockSlaveList();

    /* check we don't pretend to send encrypted messages when we
     * can't, hence avoiding surprises. */

    for (amount_cpy = amount, dest = argv[1]; amount_cpy > 0; amount_cpy--)
    {
        victim = getSlaveByNick(dest);

        if (victim && !plainflag)
        {
            /* it is a slave and plain mode is not forced */
            if (!isKeyEmpty(victim->crypto.strkey)
                    && (victim->fprint != FINGERPRINT_YSM_CLIENT_CRYPT))
            {
                /* oh oh, can't send encrypted */
                printfOutput(VERBOSE_BASE,
                        "ysm can't send an encrypted message to the "
                        "slave '%s' because he/she/it is\n"
                        "not online or is not using ysm with "
                        "encryption support.\n"
                        "Use the 'mp' or 'mplain' command to force a "
                        "plaintext message. Thank you.\n",
                        victim->info.nickName);
                goto exit;
            }
        }

        dest += strlen(dest) + 1;
    }

    DEBUG_PRINT("");

    /* Turn argv[x] into a long chain */
    for (x = 2; x < argc; x++)
    {
        aux = strchr(argv[x],'\0');
        if (aux != NULL) *aux = 0x20;
    }

    DEBUG_PRINT("");
 
    /* START CYCLE THROUGH DESTINATION LIST */
    for (dest = argv[1]; amount; amount--)
    {
        victim = getSlaveByNick(dest);

        if (!victim)
        {
            if (!isdigit((int)dest[0]))
            {
                printfOutput(VERBOSE_BASE, "(%s) - Slave unknown. "
                        "Message cancelled.\n", dest );
                goto exit;
            }
            else
            {
                initStranger(&strg, atoi(dest));
            }
        }
        else if (plainflag)
        {
            fprint_bkp = victim->fprint;
            victim->fprint = 0;
        }

        sendMessage(victim ? victim : &strg, argv[2], TRUE);

        /* skip the last \0 and get the next man! */
        dest += strlen(dest) + 1;

        /* Restore the fingerprint flag if neccesary */
        if (victim && plainflag)
        {
            victim->fprint = fprint_bkp;
            fprint_bkp = 0;
        }
    }

exit:
    DEBUG_PRINT("");
    unlockSlaveList();
}

static void cmdMSG(uint16_t argc, int8_t **argv)
{
    cmdMSG_main(argc, argv, 0);
}

static void cmdMPLAIN(uint16_t argc, int8_t **argv)
{
    cmdMSG_main(argc, argv, 1);
}

void cmdCHAT(uint16_t argc, int8_t **argv)
{
    slave_t *  slave = NULL;
    sl_flags_t   flags = 0;
    int8_t      *dest = NULL, *aux = NULL;
    int32_t      amount = 0, y = 0;

    if (argc < 1)
    {
        if (!(g_state.promptFlags & FL_CHATM))
            return;

        printfOutput( VERBOSE_BASE,
                "############### closing ysm CHAT session #############\n");

        /* we are now officialy out of ysm's chat mode! */
        g_state.promptFlags &= ~FL_CHATM;
        g_state.promptFlags &= ~FL_COMFORTABLEM;

        /* loop through the slaves list and unmark FL_CHAT */
        while (slave = getNextSlave(slave))
        {
            slave->flags &= ~FL_CHAT;
        }

        return;
    }

    printfOutput(VERBOSE_BASE,
            "############### activating ysm CHAT mode #############\n"
            "# slaves involved: %s\n"
            "#\n"
            "# all messages you receive which don't belong to this\n"
            "# chat session, will be replied with your CHAT message\n"
            "# and logged using AFK. Once you leave the CHAT session,\n"
            "# you will have to type 'readafk' to read them.\n"
            "# you can leave the session by using the 'chat' command.\n"
            "######################################################\n",
            argv[1]);

    amount = cmdMSG_ValidateDest(argv[1]);
    /* amount has the amount of destinations */
    if (amount < 0) {
        return;
    }

    /* START CYCLE THROUGH CHAT LIST */
    dest = argv[1];

    do {
        slave = getSlaveByNick(dest);

        if (!slave)
        {
            printfOutput(VERBOSE_BASE, "(%s) - Slave unknown. "
                    "skipped.\n", dest);
        }
        else
        {
            /* this is REALLY ugly and SLOW but I'm not in the mood
             * of creating a new list for chat slaves only. Instead,
             * we set the flag to FL_CHAT in our slaves. */

            slave->flags |= FL_CHAT;
        }

        amount--;

        /* skip the last \0 and get the next man! */
        dest += strlen(dest) + 1;
    } while (amount);

    /* get the CHAT message */

    if (argc > 2)
    {
        /* Turn argv[2] into a long chain */
        for (y = 2; y < argc; y++) {
            aux = strchr(argv[y],'\0');
            if (aux != NULL) *aux = 0x20;
        }

        strncpy(g_cfg.CHATMessage,
                argv[2],
                sizeof(g_cfg.CHATMessage) - 1);
        g_cfg.CHATMessage[sizeof(g_cfg.CHATMessage)-1] = '\0';
    }

    if (g_cfg.CHATMessage[0] == 0x00)
    {
        strncpy(g_cfg.CHATMessage,
                YSM_DefaultCHATMessage,
                sizeof(g_cfg.CHATMessage) - 1);
        g_cfg.CHATMessage[sizeof(g_cfg.CHATMessage)-1] = '\0';
    }

    /* we are now officialy in ysm's chat mode! */
    g_state.promptFlags |= FL_CHATM;
}

static void cmdSTATUS(uint16_t argc, int8_t **argv)
{
    user_status_t newStatus;

    if (!argc)
    {
        printfOutput(VERBOSE_BASE, "INFO STATUS %s\n",
                strStatus(g_model.status));
        return;
    }

    if (!convertStatus(FROM_STR, (const uint8_t **)&argv[1], &newStatus))
    {
        printfOutput(VERBOSE_BASE,
                "ERR Invalid status specified. Carlin!\n");
        return;
    }


    if (changeStatus(newStatus))
    {
        printfOutput(VERBOSE_BASE, "INFO STATUS %s\n", strStatus(newStatus));
    }
}

static void cmdWHOIS(uint16_t argc, int8_t **argv)
{
    slave_t        *victim = NULL;
    int8_t          buf[MAX_STATUS_LEN+1];
    struct in_addr  rintIP, rextIP;
    uint32_t        reqId;

    lockSlaveList();

    victim = getSlaveByNick(argv[1]);
    
    if (!victim)
    {
        victim = getSlaveByUin(atoi(argv[1]));

        if (!victim)
        {
            if (!isdigit((int)argv[1][0]))
            {
                printfOutput(VERBOSE_BASE,
                        "Unknown SLAVE Name. Request cancelled.\n");
            }
            else
                sendMetaInfoReq(atoi(argv[1]), CLI_FULLINFO_REQ);

            unlockSlaveList();
            return;
        }
    }

    memset(buf, 0, sizeof(buf));
    YSM_WriteFingerPrint(victim->fprint, buf);

    printfOutput(VERBOSE_BASE,

            "------------------------------------------------------------------\n"
            "Information on %s .. how interesting..\n" 
            "------------------------------------------------------------------\n"
            ,
            victim->info.nickName );

    rintIP.s_addr = victim->d_con.rIP_int;
    rextIP.s_addr = victim->d_con.rIP_ext;

    /* we had to split the printfOutputs in two due to the static buffer
     * inet_ntoa uses, think about it! I'm so angry!.
     */

    printfOutput(VERBOSE_BASE,
            "%-15.15s" " : %-12.d "
            "%-20.20s" " : %s\n",
            "UIN", victim->uin,
            "Internal IP address", inet_ntoa(rintIP));

    printfOutput(VERBOSE_BASE,
            "%-15.15s" " : %-12.12s "
            "%-20.20s" " : %s\n"
            "%-15.15s" "   %-7.s      " /* blank */
            "%-20.20s" " : %d\n\n"    /* 2 newlines */
            "%-15.15s" " : %s\n\n",
            "Current status", strStatus(victim->status),
            "External IP address", inet_ntoa(rextIP),
            "", "",
            "Bound Port", victim->d_con.rPort,
            "Fingerprint", buf);

    if (victim->budType.birthday)
    {
        printfOutput(VERBOSE_BASE,
                "\aToday is this slave's BIRTHDAY!\n"
                "[i~] Blow a candle! "
                "[~~] Eat a cake!.\n" );
    }

    /* request auto message if any */
    YSM_RequestAutoMessage(victim);

    /* update fields */
    victim->reqId = sendMetaInfoReq(victim->uin, CLI_FULLINFO_REQ);
    unlockSlaveList();
}

static void cmdSLAVESON(uint16_t argc, int8_t **argv)
{
    if (argc > 0)
        printOrganizedSlaves(STATUS_ONLINE, argv[1], 0x1);
    else
        printOrganizedSlaves(STATUS_ONLINE, NULL, 0x1);
}

static void cmdSEARCH(uint16_t argc, int8_t **argv)
{
    if (strchr(argv[1], '@'))
        sendFindByMailReq(argv[1]);
    else
        printfOutput(VERBOSE_BASE, "ERR Invalid e-mail address specified.\n");
}

static void cmdNICK(uint16_t argc, int8_t **argv)
{
    if (!argc)
    {
        printfOutput(VERBOSE_BASE, "INFO NICK ");
        if (strlen(g_model.user.nickName) < 2)
        {
            if (g_model.user.nickName[0] == '%')
                printfOutput(VERBOSE_BASE, "none specified\n");
            else
                printfOutput(VERBOSE_BASE, "Server hasn't replied yet.\n");
        }
        else
        {
            printfOutput(VERBOSE_BASE, "%s\n", g_model.user.nickName);
        }
    }
    else
    {
        snprintf(g_model.user.nickName, sizeof(g_model.user.nickName),
                 "%s", argv[1]);
        sendSetBasicUserInfoReq();
    }
}

static void cmdSAVE(uint16_t argc, int8_t **argv)
{
    slave_t *victim = NULL;

    printfOutput(VERBOSE_BASE,
            "INFO YSM POLITICAL ASYLUM FOR SLAVES\n");

    lockSlaveList();

    if (!argc)
        YSM_BuddyUploadList(NULL);
    else if (victim = getSlaveByNick(argv[1]))
        YSM_BuddyUploadList(victim);
    else
    {
        printfOutput(VERBOSE_BASE,
                "ERR SLAVE Unknown. Can't save a ghost.\n");
    }

    unlockSlaveList();
}

static void cmdREQ(uint16_t argc, int8_t **argv)
{
    slave_t *victim = NULL;
    uint8_t  flags = 0;
    char    *aux = NULL;
    int      x = 0;

    if (argc > 1)
    {
        /* Turn argv[x] into a long chain */
        for (x = 2; x < argc; x++)
        {
            aux = strchr(argv[x],'\0');
            if (aux != NULL) *aux = 0x20;
        }
    }

    lockSlaveList();

    victim = getSlaveByNick(argv[1]);

    if (victim)
    {
        sendAuthReq(victim->uin, victim->info.nickName, argv[2]);

        if (victim->flags & FL_AUTHREQ)
        {
            YSM_BuddyAddItem(victim,
                YSM_BUDDY_GROUPNAME,
                YSM_BUDDY_GROUP,
                0x0,
                YSM_BUDDY_SLAVE,
                0,
                1,
                0x08);

            victim->flags &= ~FL_AUTHREQ;
        }
    }
    else
    {
        if (!isdigit((int)argv[1][0]))
        {
            printfOutput(VERBOSE_BASE,
                "ERR SLAVE Unknown. Authorization Request cancelled.\n");
        }
        else
            sendAuthReq(atoi(argv[1]), NULL, argv[2]);
    }

    unlockSlaveList();
}

static void cmdRENAME(uint16_t argc, int8_t **argv)
{
    slave_t *victim = NULL;
    char    *newslavename = NULL;

    lockSlaveList();

    newslavename = argv[2];

    /* sanitize the slave name, trim? */
    while (*newslavename == 0x20) newslavename++;

    if (getSlaveByNick(newslavename))
    {
        /* The new name exists! Abort! */
        printfOutput(VERBOSE_BASE,
            "ERR Renaming to an existing name.\n");
    }
    else if (victim = getSlaveByNick(argv[1]))
    {
        printfOutput(VERBOSE_BASE,
            "ERR SLAVE Unknown. Can't rename a non existing slave!"
            "cancelled.\n");
    }
    /* The old_name exists, renaming a valid slave */
    else if (!strcasecmp(argv[1], newslavename))
    {
        /* Renaming to the same name? no way! */
        printfOutput(VERBOSE_BASE,
            "ERR Ahahaha..thats a joke, right? Renaming"
            " requires two DIFFERENT nicks.\n");
    }
    else
    {
        printfOutput(VERBOSE_BASE,
                "INFO Renaming %s to %s\n",
                argv[1], newslavename);

        strncpy(victim->info.nickName, newslavename,
                sizeof(victim->info.nickName) - 1);
    }

    unlockSlaveList();
}

static void cmdEMAIL(uint16_t argc, int8_t **argv)
{
    if (!argc)
    {
        printfOutput(VERBOSE_BASE, "INFO EMAIL ");

        if (strlen(g_model.user.email) < 2)
        {
            if (g_model.user.email[0] == '%')
                printfOutput(VERBOSE_BASE, "none specified\n");
            else
                printfOutput(VERBOSE_BASE, "Server hasn't replied yet.\n");
        }
        else
        {
            printfOutput(VERBOSE_BASE, "%s\n", g_model.user.email);
        }
    }
    else
    {
        snprintf(g_model.user.email, sizeof(g_model.user.email),
                 "%s", argv[1]);
        sendSetBasicUserInfoReq();
    }
}

static void cmdUPTIME(uint16_t argc, int8_t **argv)
{
    int days = 0, hours = 0, minutes = 0, seconds = 0;

    seconds = getTimer(UPTIME);
    minutes = seconds/60;
    hours = minutes/60;
    days = hours/24;

    seconds -= 60*minutes;
    minutes -= 60*hours;
    hours -= 24*days;

    printfOutput(VERBOSE_BASE,
        "INFO UPTIME %d days %d hours %d minutes %d seconds\n",
        days, hours, minutes, seconds);
}

static void cmdBACKDOOR(uint16_t argc, int8_t **argv)
{
    printfOutput(VERBOSE_BASE,
        "INFO ahaha. just kidding :) command is not implemented.\n");
}

#ifdef YSM_WAR_MODE

static void cmdSCAN(uint16_t argc, int8_t **argv)
{
    slave_t *victim = NULL;

    if (victim = getSlaveByNick(argv[1]) == 0)
    {
        printfOutput(VERBOSE_BASE,
            "INFO YSMD ULTRASECRET SCANNING OF HIDDEN ENEMIES\n");

        YSM_War_Scan(victim);
    }
    else 
    {
        printfOutput(VERBOSE_BASE,
            "SLAVE Unknown. Won't scan a non existing slave.\n");
    }
}

static void cmdKILL(uint16_t argc, int8_t **argv)
{
    slave_t *victim = NULL;

    printfOutput(VERBOSE_BASE,
        "..if this is what you want. It is what i'll do.\n");

    if (victim = getSlaveByNick(argv[1]) == 0)
        YSM_War_Kill(victim);
    else
        printfOutput(VERBOSE_BASE,
                "ERR Unknown SLAVE Name. Won't kill a Ghost.\n");
}

#endif

static void cmdRTF(uint16_t argc, int8_t **argv)
{
    slave_t *victim = NULL;

    printfOutput(VERBOSE_BASE, "rtfing...\n");

    lockSlaveList();

    victim = getSlaveByNick(argv[1]);

    if (victim)
        YSM_SendRTF(victim);
    else
        printfOutput(VERBOSE_BASE,
            "ERR Unknown SLAVE Name. Won't rtf a Ghost.\n");

    unlockSlaveList();
}

static void cmdIGNORE(uint16_t argc, int8_t **argv)
{
    slave_t *victim;

    lockSlaveList();

    victim = getSlaveByNick(argv[1]);

    if (!victim)
    {
        printfOutput(VERBOSE_BASE, "ERROR IGNORE UNKNOWN_SLAVE");
        goto exit;
    }

    if (!victim->budType.ignoreId)
    {
        if (!IS_DOWNLOADED(victim->flags))    /* Slave isn't saved */
        {
            printfOutput(VERBOSE_BASE,
                "Slave won't be added to your ignore list until "
                "you upload him\nto the icq servers using the 'save' "
                "command. (try 'save slave_name').\n");
            goto exit;
        }
        else
        {
            printfOutput(VERBOSE_BASE,
                "Adding the slave to your ignore list..\n"
                "..and to your invisible list.\n");
                YSM_BuddyIgnore(victim, 0x1);

            if (!victim->budType.invisibleId)
                YSM_BuddyInvisible(victim, 0x1);
        }
    }
    /* Unignore the user */
    else
    {
        printfOutput(VERBOSE_BASE,
            "Removing the slave from your Ignore and Invisible "
            "lists.\n");

        YSM_BuddyIgnore(victim, 0x0);

        if (victim->budType.invisibleId)
            YSM_BuddyInvisible(victim, 0x0);
    }

    printfOutput(VERBOSE_BASE,
        "INFO Switching Ignore to %s for Slave %s - UIN %d\n",
        victim->budType.ignoreId ? "ON" : "OFF",
        victim->info.nickName, victim->uin);

exit:
    unlockSlaveList();
}

static void cmdVISIBLE(uint16_t argc, int8_t **argv)
{
#if 0
    slave_t *             victim;
    buddy_special_status_t  bss;
    sl_flags_t              flags;

    if (victim = getSlaveByNick(argv[1]) != 0)
    {
        printfOutput(VERBOSE_BASE,
                "Unknown SLAVE Name. Won't add to Visible!\n");
        return;
    }

    getSlaveSpecialStatus(&victim, &bss);
    getSlaveFlags(&victim, &flags);

    if (!bss.visibleId)
    {
        if (!IS_DOWNLOADED(flags))    /* Slave isn't saved */
        {
            printfOutput(VERBOSE_BASE,
                    "Slave won't be added to your visible list until "
                    "you upload him\nto the icq servers using the 'save' "
                    "command. (try 'save slave_name').\n");

            return;
        }
        else
        {
            printfOutput(VERBOSE_BASE,
                    "Adding the slave to your visible list..\n");

            YSM_BuddyVisible(&victim, 0x1);
        }
    }
    /* Remove from the visible list */
    else
    {
        printfOutput(VERBOSE_BASE,
                "Removing slave from your visible list..\n");

        YSM_BuddyVisible(&victim, 0x0);
    }

    printfOutput(VERBOSE_BASE,
            "INFO Switching Visible to %s for Slave %s - UIN %d\n",
            bss.visibleId ? "ON" : "OFF",
            victim->info.nickName, victim.uin);
#endif
}

static void cmdINVISIBLE(uint16_t argc, int8_t **argv)
{
#if 0
    slave_t *             victim;
    buddy_special_status_t  bss;
    sl_flags_t              flags;

    if (victim = getSlaveByNick(argv[1]) != 0)
    {
        printfOutput(VERBOSE_BASE,
                "Unknown SLAVE Name. "
                "Won't add to the invisible list!\n");
        return;
    }

    getSlaveSpecialStatus(&victim, &bss);
    getSlaveFlags(&victim, &flags);

    /* Meaning the buddy IS in our invisible list */
    if (!bss.invisibleId)
    {
        if (!IS_DOWNLOADED(flags))    /* Slave isn't saved */
        {
            printfOutput(VERBOSE_BASE,
                    "Slave won't be added to your invisible list until "
                    "you upload him\nto the icq servers using the 'save' "
                    "command. (try 'save slave_name').\n");

            return;
        }
        else
        {
            printfOutput(VERBOSE_BASE,
                    "Adding the slave to your invisible list..\n");

            YSM_BuddyInvisible(&victim, 0x1);
        }
    }
    /* Remove from the invisible list */
    else
    {
        printfOutput(VERBOSE_BASE,
                "Removing slave from your Invisible list..\n");

        YSM_BuddyInvisible(&victim, 0x0);
    }

    printfOutput(VERBOSE_BASE,
            "INFO Switching Invisible to %s for Slave %s - UIN %d\n",
            bss.invisibleId ? "ON" : "OFF",
            victim->info.nickName, victim.uin);
#endif
}

static void cmdKEY(uint16_t argc, int8_t **argv)
{
    slave_t   *slave = NULL;
    uint32_t   x = 0, keylen = 0;
    int32_t    retval = 0;
    int8_t     goodKey[64];
    string_t  *str = NULL;

    slave = getSlaveByNick(argv[1]);

    if (!slave)
    {
        printfOutput(VERBOSE_BASE,
            "ERR Unknown SLAVE Name. Mommi told me not to "
            "do encryption on strangers.\n");
        return;
    }

    if (argc < 2)
    {
        /* Clear key for slave. No key was supplied, hence we understand
         * the user wants to clear the key with this slave. */

        printfOutput(VERBOSE_BASE,
            "INFO Clearing key for slave %s\n",
            slave->info.nickName);

        /* If there is an existing key, reset it.
         * Then update the configuration file to clear the key. */

        if (!isKeyEmpty(slave->crypto.strkey))
        {
            /* clear the key by memsetting it to zeroes */
            memset(slave->crypto.strkey, 0, sizeof(slave->crypto.strkey));
            /* update the configuration file for this slave */
            updateSlave(UPDATE_SLAVE, NULL, slave->uin);
        }

        /* Thats it. No more steps to take */
        return;
    }

    /* If we get this far, it means the user wants to change
     * the key with the slave. There are 2 possibilities:
     * a. a key was supplied
     * b. the '?' character was used to generate a new key randomly.
     * lets find out..
     */

    memset(slave->crypto.strkey, 0, sizeof(slave->crypto.strkey));

    if (argv[2][0] == '?')
    {
        /* Generate a random 64 bytes key for slave.
         * The '?' character used as a key determines that we have
         * to randomly generate a key for the specified slave. */

        for (x = 0; x < MAX_KEY_LEN; x++)
        {
            /* step 1:
             * choose a random base value between 0 and f
             * sum that value to '0' to get an ascii value.
             */

            slave->crypto.strkey[x] = rand() % 0xf + '0';

            /* step 2:
             * since the obtained ascii value can be erroneous
             * because after '9' comes '@' for example. We pad
             * the difference with the value of 'A', but only
             * if step 1 left us with a value higher than '9'.
             */

            if (slave->crypto.strkey[x] > '9')
                slave->crypto.strkey[x] += 'A' - '9' - 1;
        }
    }
    else
    {
        /* Assign the specified key to the slave.
         * The user supplied a key, we must first validate it.
         * If the key is valid, it is assigned to the slave.
         * Note validation is done afterwards. */

        for (x = 0; (x < strlen(argv[2])) && (x < MAX_KEY_LEN); x++)
            slave->crypto.strkey[x] = argv[2][x];
    }

    /* Since the user is allowed to enter shorter keys than
     * 64 bytes long (because otherwise it would turn hideous),
     * we make sure the final key is at least 64 bytes long by
     * repeating the key n amount of times as neccesary. */

    keylen = strlen(slave->crypto.strkey);

    for (x = 0; x < sizeof(goodKey); x++)
        goodKey[x] = slave->crypto.strkey[x % keylen];

    /* Try to make both in and out keys.
     * At this point either we have a (should be) valid randomly
     * generated 64 bytes key, or, a (maybe invalid) 64 bytes key
     * supplied by the user. */

    retval = makeKey(&slave->crypto.key_out, DIR_ENCRYPT, 256, goodKey);
    if (retval == TRUE)
    {
        /* OUT key instance created successfully.
         * Proceed to create the second key */

        retval = makeKey(&slave->crypto.key_in,
                    DIR_DECRYPT,
                    256,
                    goodKey);
    }

    /* ACTION:
     *    Check if any of the keys failed creating. We don't mind
     *    telling at this point which of them failed. The user
     *    doesn't really care.
     */

    if (TRUE != retval)
    {
        switch (retval)
        {
            case BAD_KEY_DIR:
                /* bad key direction */
            case BAD_KEY_MAT:
                /* key material length is incorrect */
            case BAD_KEY_INSTANCE:
                /* invalid supplied key */
            default:
                /* unknown */
                break;
        }

        printfOutput(VERBOSE_BASE,
            "ERR Error setting cipher key. Please check the key meets"
            "\nthe requirements by using the 'help key' command.\n");

        return;
    }

    /* print the key back to the user */

    str = initString();
    printfString(str, "INFO Slave encryption key is now:\n");

    for (x = 0; x < MAX_KEY_LEN; x++)
        printfString(str, "%c", slave->crypto.strkey[x]);

    printfString(str, "\n");
    writeOutput(VERBOSE_BASE, getString(str));
    freeString(str);

    /* update the configuration file with the new key for this slave */
    updateSlave(UPDATE_SLAVE, NULL, slave->uin);
}

static void cmdSEEN(uint16_t argc, int8_t **argv)
{
    slave_t *victim;
    char     buf[MAX_TIME_LEN];

    victim = getSlaveByNick(argv[1]);

    if (victim)
    {
        printfOutput(VERBOSE_BASE,
            "Signon time: %s\n",
            YSM_gettime(victim->timing.signOn, buf, sizeof(buf)));

        printfOutput(VERBOSE_BASE,
            "Last status change: %s\n",
            YSM_gettime(victim->timing.statusChange, buf, sizeof(buf)));

        printfOutput(VERBOSE_BASE,
            "Last message: %s\n",
            YSM_gettime(victim->timing.lastMessage, buf, sizeof(buf)));
    }
    else
    {
        printfOutput(VERBOSE_BASE,
            "ERR Unknown SLAVE Name. Only slaves are allowed.\n");
    }
}

static void cmdPASSWORD(uint16_t argc, int8_t **argv)
{
    if (strlen(argv[1]) < 4 || strlen(argv[1]) > 8)
    {
        // icq passwords can only be between 4 and 8 characters.
        printfOutput(VERBOSE_BASE,
                "Incorrect password length. Password must be between "
                "4 and 8 characters long.\n");
    }
    else
        sendSetPasswordReq(argv[1]);
}

static void cmdCONNECT(int argc, char **argv)
{
    if (argc == 0)
    {
        printfOutput(VERBOSE_BASE, g_state.connected
                ? "INFO connected as %lu (%s)\n"
                : "INFO disconnected\n",
                g_model.uin, g_model.user.nickName);
    }
    else if (!g_state.connected && argc == 2)
    {
        g_model.uin = atoi(argv[1]);
        strncpy(g_model.password, argv[2], sizeof(g_model.password));

        if (networkSignIn() < 0)
        {
            printfOutput(VERBOSE_BASE, "ERR connection failed\n");
        }
    }
    else if (g_state.connected)
    {
        printfOutput(VERBOSE_BASE, "ERR already connected as %lu (%s)\n",
                g_model.uin, g_model.user.nickName);
    }
}

static void cmdDISCONNECT(int argc, char **argv)
{
    g_state.connected = FALSE;
    close(g_model.network.socket);
}

static void cmdCONTACTS(uint16_t argc, int8_t **argv)
{
    int32_t  x, y = 0, buf_len = 0;
    slave_t *list[MAX_CONTACTS_SEND+1], *victim = NULL, *YSM_Query = NULL;
    int8_t  *data = NULL, tmp[MAX_UIN_LEN + MAX_NICK_LEN + 3], am[3];

    victim = getSlaveByNick(argv[1]);

    /* Check if the victim exists */
    if (!victim)
    {
        printfOutput(VERBOSE_BASE,
                "Sending cancelled. slave %s unknown.\n",
                argv[1]);
        return;
    }

    for (y = 0; y <= MAX_CONTACTS_SEND; y++)
        list[y] = NULL;

    y = 0;

    for (x = 2; x <= argc && (x-2 < MAX_CONTACTS_SEND); x++)
    {
        YSM_Query = getSlaveByNick(argv[x]);
        if (YSM_Query == NULL)
        {
            printfOutput( VERBOSE_BASE,
                    "(%s) - slave unknown. "
                    "Not sending him/her.\n", argv[x]);
        }
        else
        {
            list[y] = YSM_Query;
            memset( tmp, 0, MAX_UIN_LEN + MAX_NICK_LEN + 3 );

            snprintf( tmp,
                    MAX_UIN_LEN + MAX_NICK_LEN + 2,
                    "%d%c%s%c",
                    (int)YSM_Query->uin,
                    0xFE,
                    YSM_Query->info.nickName,
                    0xFE );

            tmp[MAX_UIN_LEN+MAX_NICK_LEN+2] = 0x00;

            buf_len += strlen(tmp);
            y++;
        }
    }

    /* Not enough contacts */
    if (!buf_len) return;

    buf_len++;    /* ending zero */

    data = YSM_CALLOC(1, buf_len);
    if (!data) return;

    for (y = 0; list[y] != NULL; y++)
    {
        memset(tmp, 0, MAX_UIN_LEN + MAX_NICK_LEN + 3);

        snprintf(tmp,
                MAX_UIN_LEN + MAX_NICK_LEN + 2,
                "%d%c%s%c",
                (int)list[y]->uin,
                0xFE,
                list[y]->info.nickName,
                0xFE);

        tmp[MAX_UIN_LEN+MAX_NICK_LEN+2] = 0x00;
        memcpy(data+strlen(data), tmp, strlen(tmp));
    }

    /* Make the amount ASCII */
    snprintf(am, sizeof(am), "%d", (int)y);
    am[sizeof(am) - 1] = 0x00;

    /* Send them out! */
    YSM_SendContact(victim, data, am);

    printfOutput(VERBOSE_BASE, "INFO CONTACTS SENT\n");

    YSM_FREE(data);
}

static void cmdURL(uint16_t argc, int8_t **argv)
{
    slave_t *victim;
    int8_t  *pdesc = NULL;
    int32_t  x = 0;

    victim = getSlaveByNick(argv[1]);

    /* Check if the victim exists */
    if (!victim)
    {
        printfOutput(VERBOSE_BASE,
            "ERR Sending cancelled. slave %s unknown.\n", argv[1]);

        return;
    }

    if (argc < 3)
        pdesc = "no description";
    else
    {
        /* Turn the description into a long chain */
        for (x = 3; x < argc; x++)
        {
            pdesc = strchr(argv[x],'\0');
            if (pdesc != NULL) *pdesc = 0x20;
        }

        pdesc = argv[3];
    }

    YSM_SendUrl(victim, argv[2], pdesc);
}

static void cmdFILECANCEL( uint16_t argc, int8_t **argv )
{
    slave_t *victim = NULL;

    victim = getSlaveByNick(argv[1]);

    /* Check if the victim exists */
    if (!victim)
    {
        printfOutput(VERBOSE_BASE,
                "File cancelling aborted. "
                "slave %s unknown.\n", argv[1]);
    }
    else
    {
        printfOutput(VERBOSE_BASE,
                "INFO Cancelling file transfer to/from %s..\n",
                argv[1]);

        YSM_CloseTransfer(victim);
    }
}

void cmdFILESTATUS(uint16_t argc, int8_t **argv)
{
    slave_t *slave = NULL;
    double   t = 0, p = 0, y = 0;

    printfOutput(VERBOSE_BASE, "INFO FILE_STATUS Active transfers:\n");

    /* cycle through our slaves list and find the ones who have an ongoing
     * transfer/receive and print its current percentage transfered. */

    while ((slave = getNextSlave(slave)) != NULL)
    {
        if (slave->d_con.flags & DC_ACTIVITY)
        {
            p = (slave->d_con.finfo.totsize - slave->d_con.finfo.size);

            t = p * 100;
            t = t / slave->d_con.finfo.totsize;

            printfOutput(VERBOSE_BASE, "%s [", slave->info.nickName);

            if (t < 10.00)
                printfOutput(VERBOSE_BASE, "0%.0f%][", t);
            else
                printfOutput(VERBOSE_BASE, "%.0f%][", t);

            for (y = 0; y <= 100; y += 10)
            {
                if (y <= t)
                    printfOutput(VERBOSE_BASE, ".");
                else
                    printfOutput(VERBOSE_BASE, " ");
            }

            printfOutput(VERBOSE_BASE, "] [%-13.13s - ",
                    slave->d_con.finfo.name);

            t = p/1024;
            printfOutput(VERBOSE_BASE,
                    "%.0f of ",
                    t);

            t = slave->d_con.finfo.totsize/1024;

            printfOutput(VERBOSE_BASE,
                    "%.0f kb at %d kb/s]\n",
                    t, slave->d_con.finfo.kbs);
        }
    }
}

static void cmdFILEACCEPT(uint16_t argc, int8_t **argv)
{
    slave_t *victim = NULL;

    lockSlaveList();

    victim = getSlaveByNick(argv[1]);

    /* Check if the victim exists */
    if (victim)
    {
        printfOutput(VERBOSE_BASE,
                "ERR File accept cancelled. slave %s unknown.\n", argv[1]);
    }
    else
    {

        printfOutput(VERBOSE_BASE, "Accepting file transfer request..\n"
                "You may cancel it by using the 'fcancel' command.\n"
                "You can check its status by using the 'fstatus' command.\n");

        if (YSM_DC_FileB(victim, victim->d_con.finfo.name, NULL) <= 0)
        {
            printfOutput(VERBOSE_BASE,
                    "Receiving cancelled. Errors showed up.\n");
        }
    }

    unlockSlaveList();
}

static void cmdFILEDECLINE(uint16_t argc, int8_t **argv)
{
    slave_t *victim = NULL;

    lockSlaveList();

    victim = getSlaveByNick(argv[1]);

    /* Check if the victim exists */
    if (!victim)
    {
        printfOutput(VERBOSE_BASE,
                "File decline cancelled. slave %s unknown.\n", argv[1]);
    }
    else
    {
        printfOutput(VERBOSE_BASE, "Denying file transfer request..\n");

        if (YSM_DC_FileDecline(victim, argv[2] ) <= 0)
        {
            printfOutput(VERBOSE_BASE,
                    "Receiving cancelled. Errors showed up.\n");
        }
    }

    unlockSlaveList();
}

static void cmdSEND(uint16_t argc, int8_t **argv)
{
    int8_t  *aux = NULL;
    int32_t  x = 0;
    slave_t *victim = NULL;

    lockSlaveList();

    victim = getSlaveByNick(argv[1]);

    /* Check if the victim exists */
    if (!victim)
    {
        printfOutput( VERBOSE_BASE,
                "Sending cancelled. slave %s unknown.\n",
                argv[1] );
        goto exit;
    }

    if (victim->d_con.finfo.fd != 0x0)
    {
        printfOutput(VERBOSE_BASE,
                "There is already an open file transfer with "
                "this slave.\nYou may cancel it by using the "
                "'fcancel' command.\n" );
        goto exit;
    }

    if (!isKeyEmpty(victim->crypto.strkey)
            && victim->fprint == FINGERPRINT_YSM_CLIENT_CRYPT )
    {
        printfOutput(VERBOSE_BASE,
                "Sending ENCRYPTED file transfer request to %s..\n",
                victim->info.nickName);
    }
    else
    {
        printfOutput(VERBOSE_BASE,
                "Sending file transfer request to %s..\n",
                victim->info.nickName);
    }

    /* Check if a long filename with spaces was specified
     * we notice this by the presence of '"' bytes.
     */

    if (argv[2][0] == '\"') {
        /* Turn argv[x] into a long chain */
        for (x = 2; x < argc; x++) {
            aux = strchr(argv[x],'\0');
            if (aux != NULL) *aux = 0x20;
        }

        argc -= (x - 2);
        argv[2]++;
        if ((aux = strchr(argv[2], '\"')) == NULL) {
            /* there was a starting '"' but no ending one */
            /* this is a mistake by the user, abort. */
            printfOutput( VERBOSE_BASE,
                    "Syntax mistake, no ending \" char found.\n" );
            goto exit;
        }

        *aux = 0x00;
        aux += 2;    /* zero + space */
        argv[3] = aux;
    }

    /* Turn the reason into a long chain */
    for (x = 3; x < argc; x++)
    {
        aux = strchr(argv[x],'\0');
        if (aux != NULL) *aux = 0x20;
    }

    if (YSM_DC_File(victim, argv[2], argv[3] ) < 0)
    {
        printfOutput(VERBOSE_BASE, "Sending cancelled. Errors showed up.\n");
    }

exit:
    unlockSlaveList();
}

static void cmdOPENDC(uint16_t argc, int8_t **argv)
{
    slave_t   *victim = NULL;
    pthread_t  tid;

    lockSlaveList();

    victim = getSlaveByNick(argv[1]);

    /* Check if the victim exists */
    if (!victim)
    {
        printfOutput(VERBOSE_BASE,
            "Negotiation cancelled. slave %s unknown.\n", argv[1]);
        goto exit;
    }

    if (victim->d_con.flags & DC_CONNECTED)
    {
        printfOutput(VERBOSE_BASE,
            "An existing session with this slave was found.\n"
            "Use the 'closedc' command to end it if required.\n");
        goto exit;
    }

    printfOutput(VERBOSE_BASE,
        "Initiating a DC session with %s..\n", victim->info.nickName);

    pthread_create(&tid, NULL, (void *)&YSM_OpenDC, (void *)victim->uin);

exit:
    unlockSlaveList();
}

static void cmdCLOSEDC(uint16_t argc, int8_t **argv)
{
    slave_t *victim;

    lockSlaveList();

    /* Check if the victim exists */
    if ((victim = getSlaveByNick(argv[1])) == NULL)
    {
        printfOutput(VERBOSE_BASE,
            "Negotiation cancelled. slave %s unknown.\n", argv[1]);
        unlockSlaveList();
        return;
    }

    if (!victim->d_con.rSocket)
    {
        printfOutput(VERBOSE_BASE,
            "No active DC session with this slave was found.\n"
            "Use the 'opendc' command to open a DC session.\n");
    }
    else
    {
        printfOutput(VERBOSE_BASE, "Closing DC Session with %s..\n",
                victim->info.nickName);
        YSM_CloseDC(victim);
    }
    
    unlockSlaveList();
}

static void cmdSLAVESALL(uint16_t argc, int8_t **argv)
{
    printOrganizedSlaves(STATUS_ONLINE, argv[1], 0x0);
}

#ifdef YSM_TRACE_MEMLEAK
static void cmdSHOWLEAK(uint16_t argc, int8_t **argv)
{
    printfOutput(VERBOSE_BASE, "INFO Unfreed blocks: %d\n", unfreed_blocks);
}
#endif

bool_t doCommand(uint16_t argc, int8_t **argv)
{
    int8_t i;
 
    for (i = 0; g_command_list[i].name != NULL; i++)
    {
        /* speed up with first checks */
        if (g_command_list[i].name[0] != (int8_t)tolower(argv[0][0])
        || strcasecmp(g_command_list[i].name, argv[0]))
        {
            if (g_command_list[i].alias != NULL)
            {
                if (g_command_list[i].alias[0] != (int8_t)tolower(argv[0][0])
                || strcasecmp(g_command_list[i].alias, argv[0])) {
                    continue;
                }
            }
            else
            {
                continue;
            }
        }

        if (argc < g_command_list[i].margs)
        {
            printfOutput(VERBOSE_BASE,
                "Missing parameters. Use the 'help'"
                " command for detailed information.\n");
        }
        else
        {
            /* use the low caps argv[0], just in case */
            /* who knows when batman may come 2 kill us (?!) */
            argv[0] = g_command_list[i].name;

            if (g_command_list[i].func != NULL)
            {
                g_command_list[i].func(argc, argv);
                return TRUE;
            }
        }

        break;
    }

    return FALSE;
}
