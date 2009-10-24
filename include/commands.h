#ifndef _COMMANDS_H_
#define _COMMANDS_H_

typedef enum
{
    CG_USERS,
    CG_SETTINGS,
    CG_ACCOUNT,
    CG_CLIENT,
    CG_EXTRA 
} command_group_t;

typedef struct
{
    int8_t          *name;
    int8_t          *alias;
    int8_t          *help;
    command_group_t  group;
    uint16_t        margs;
    void           (*func)(uint16_t argc, int8_t **argv);
} command_t;

/* Command Groups */
#define CG_USERS_STR     "USERS/SLAVES" ":\n"
#define CG_SETTINGS_STR  "SETTINGS/CONFIGURATION"":\n"
#define CG_ACCOUNT_STR   "ACCOUNT/SESSION"":\n"
#define CG_CLIENT_STR    "CLIENT"":\n"
#define CG_AMOUNT        0x04


#define YSM_COMMAND_QUIT_HELP \
    "Close the ysm client.\n" \
    "usage: 'quit'\n\n"

#define YSM_COMMAND_HELP_HELP \
    "Query for help on a command\n" \
    "usage: 'help <command>'\n\n"

#define YSM_COMMAND_INFO_HELP \
    "Show information about your ysm client and session.\n" \
    "usage: 'info'\n\n"

#define YSM_COMMAND_SLAVES_HELP        "You may check on your Slave list anytime by using this command. The slaves are organized in <slave_name> <status> <flags>. Flags are modified by the 'ignore', 'visible' and 'invisible' commands. Slaves in ignore are only shown in the 'wa' command.\n"                                    "usage: 'w [pattern]'\n\n"

#define YSM_COMMAND_SLAVESON_HELP    "Check your Slaves list just for the online ones.\n(more help on the 'slaves or w' commands.)\n"                                    "usage: 'wo [pattern]'\n\n"

#define YSM_COMMAND_SLAVESALL_HELP    "You may check on your Slave list anytime by using this command. The slaves are organized in <slave_name> <status> <flags>. Flags are modified by the 'ignore', 'visible' and 'invisible' commands.\n\n"                    "usage: 'wa [pattern]'\n\n"

#define YSM_COMMAND_ADDSLAVE_HELP    "Add a slave to your list.\n"                            "usage: 'add <name> <uin>'\n\n"

#define YSM_COMMAND_DELSLAVE_HELP    "Delete a slave from list.\n"                            "usage: 'del <name>'\n\n"

#define YSM_COMMAND_AUTH_HELP        "Send an authorization OK reply to a slave or icq.\n"                                                        "usage: 'auth <name>'\n"                            "usage: 'auth <uin>'\n\n"

#define YSM_COMMAND_MSG_HELP        "Send a message to a slave or icq.\n"                        "usage: 'msg <name> [message]'\n"        "usage: 'msg <name>,<name2>,.. [message]'\n"                "usage: 'msg <uin> [message]'\n"                        "usage: 'msg <uin1>,<uin2>,.. [message]'\n"            "If a message isn't specified, Comfortable mode is activated.\n\n"

#define YSM_COMMAND_MPLAIN_HELP        "Send a textplain message to a slave or icq. (Ignoring any encryption settings).\n"                        "usage: 'mplain <name> [message]'\n"        "usage: 'mplain <name>,<name2>,.. [message]'\n"                "usage: 'mplain <uin> [message]'\n"                        "usage: 'mplain <uin1>,<uin2>,.. [message]'\n"            "If a message isn't specified, Comfortable mode is activated.\n\n"


#define YSM_COMMAND_CHAT_HELP        "Turn ysm into CHAT mode. Just specify the slaves to chat with.\n"        "usage: 'chat <slave1>,<slave2>,..'\n"                        "usage: 'ch <slave1>,<slave2>,..'\n"                        "\n\n"

#define YSM_COMMAND_STATUS_HELP        "Change/Check your current status.\n"                        "usage: 'status [new_status]'\n\n"

#define YSM_COMMAND_WHOIS_HELP        "Request information on a slave or icq#\n"                    "usage: 'whois <name>'\n"                            "usage: 'whois <uin>'\n\n"

#define YSM_COMMAND_SEARCH_HELP        "Search icq users by their e-mail!.\n"                        "usage: 'search example@email.com'\n\n"

#define YSM_COMMAND_NICK_HELP        "Check/Change your icq nickname.\n"                        "usage: 'nick [new_nick]'\n\n"

#define YSM_COMMAND_EMAIL_HELP        "Check/Change your e-mail!.\n"                            "usage: 'email [new@address]'\n\n"

#define YSM_COMMAND_SAVE_HELP        "Upload your slaves to the ICQ servers.\n"                    "usage: 'save [name]'\n\n"

#define YSM_COMMAND_REQ_HELP        "Send an Authorization Request to a slave or icq #.\n"                "usage: 'req <name>'\n"                                "usage: 'req <uin>'\n\n"

#define YSM_COMMAND_RENAME_HELP        "Rename a slave.\n"                                "usage: 'rename <old_name> <new_name>'\n\n"

#define YSM_COMMAND_UPTIME_HELP        "Check the amount of days, hours, minutes and seconds since you started YSM.\n"    "usage: 'uptime'\n\n"

#define YSM_COMMAND_BACKDOOR_HELP    "Hehe just a joke :).\n\n"

#define YSM_COMMAND_CLEAR_HELP        "Clear the screen.\n"                                "usage: 'clear'\n\n"

#define YSM_COMMAND_IGNORE_HELP        "Add/Remove a slave to/from your ignore list.\n"                "usage: 'ignore <name>'\n\n"

#define YSM_COMMAND_INVISIBLE_HELP    "Add/Remove a slave to/from your invisible list.\n"                "usage: 'invisible <name>'\n\n"

#define YSM_COMMAND_VISIBLE_HELP    "Add/Remove a slave to/from your visible list.\n"                "usage: 'visible <name>'\n\n"

#define YSM_COMMAND_ALERT_HELP        "Add/Remove a slave to/from your alert list.\n"                    "usage: 'alert <name>'\n\n"

#define YSM_COMMAND_HIST_HELP        "Read a slave's log file (message HISTORY).\n"                    "usage: 'hist <name>'\n"

#define YSM_COMMAND_LOADCONFIG_HELP    "Reload configuration file settings.\n"                        "usage: 'loadconfig'\n\n"

#define YSM_COMMAND_KEY_HELP        "Set an encryption key to use between two YSM clients.\nKeys are used to send encrypted messages and encrypted file transfers.\nThe keys are specified in hexadecimal and they must be max 32 bytes [32 * 2 hex]\nIn example: 'key rad2k 616161616161616161' sets a 9 bytes long key with rad2k.\nYou should then tell the slave to set the same key with you.\nIf used with no arguments, the slave's key will be cleared.\nIf used with a '?' a 32 bytes random key will be generated, set,  and shown in the screen.\nIn example: 'key rad2k ?'.\n"                                                    "usage: 'key [name] [hex_key|?]'\n\n"

#define YSM_COMMAND_RUN_HELP        "Run a command in your current shell.\n"                    "usage: '! ls -al'\n\n"

#define YSM_COMMAND_SEEN_HELP    "Display signon, last status change and\nlast message received time of a specified slave.\n"                                        "usage: 'seen <name>'\n\n"

#define YSM_COMMAND_PASSWORD_HELP    "Change your ICQ password. Passwords must be between 4 and 8 characters long.\n"                            "usage: 'password <new_pasword>'\n\n"

#define YSM_COMMAND_CONNECT_HELP    "Connect to the ICQ Network.\n"                        "usage: 'reconnect'\n\n"

#define YSM_COMMAND_DISCONNECT_HELP    "Reconnect to the ICQ Network.\n"                        "usage: 'reconnect'\n\n"

#define YSM_COMMAND_CONTACTS_HELP    "Send contacts to a slave or icq #.\n"                        "usage: 'contacts <dest_slave> <slave1> [slave2] [slave3] ..'\n\n"

#define YSM_COMMAND_URL_HELP    "Send a url to a slave or icq #.\n"                        "usage: 'url <dest_slave> <url> [description]'\n\n"

#define YSM_COMMAND_FILEACCEPT_HELP    "Accept a file transfer request from a slave.\n"                "usage: 'faccept <dest_slave>'\n\n"

#define YSM_COMMAND_FILEDECLINE_HELP    "Decline a file transfer request from a slave.\n"                "usage: 'fdecline <dest_slave> [reason]'\n\n"

#define YSM_COMMAND_FILESTATUS_HELP    "Show the status of active transfers.\n"                    "usage: 'fstatus'\n\n"

#define YSM_COMMAND_FILECANCEL_HELP    "Cancel an ongoing transfer to/from a slave.\n"                    "usage: 'fcancel <slave>'\n\n"

#define YSM_COMMAND_SEND_HELP        "Send a file to a slave.\n"                            "usage: 'send <dest_slave> <filepath> <reason>'\n"                "You may specify a filename with spaces by enclosing it in '\"' chars.\n\n"

#define YSM_COMMAND_OPENDC_HELP        "Open a DC session to a slave.\n"                        "usage: 'opendc <slave>'\n\n"

#define YSM_COMMAND_CLOSEDC_HELP    "Close a DC session to a slave.\n"                        "usage: 'closedc <slave>'\n\n"

/* War function descriptions */

#define YSM_COMMAND_KILL_HELP    "can't provide you with many details. I'm sorry :). Lawyers, OFF MY BACK!\n\n"

#define YSM_COMMAND_SPOOF_HELP    "sorry, won't help you on stuff that is obvious.\n\n"

#define YSM_COMMAND_SCAN_HELP    "can't provide you with details, let's just say...it let's you see dead people.\n[or those who play dead].\n\n"

/* End of Help System definition */

static void cmdQUIT(uint16_t argc, int8_t **argv);
void cmdHELP(uint16_t argc, int8_t **argv);
static void cmdINFO(uint16_t argc, int8_t **argv);
static void cmdLOADCONFIG(uint16_t argc, int8_t **argv);
static void cmdSLAVES(uint16_t argc, int8_t **argv);
static void cmdADDSLAVE(uint16_t argc, int8_t **argv);
static void cmdDELSLAVE( uint16_t argc, int8_t **argv );
static void cmdAUTH(uint16_t argc, int8_t **argv);
static void cmdMSG_main(uint16_t argc, int8_t **argv, char plainflag);
static void cmdMSG(uint16_t argc, int8_t **argv);
static void cmdMPLAIN(uint16_t argc, int8_t **argv);
void cmdCHAT(uint16_t argc, int8_t **argv);
static void cmdSTATUS(uint16_t argc, int8_t **argv);
static void cmdWHOIS(uint16_t argc, int8_t **argv);
static void cmdSLAVESON(uint16_t argc, int8_t **argv);
static void cmdSEARCH(uint16_t argc, int8_t **argv);
static void cmdNICK(uint16_t argc, int8_t **argv);
static void cmdSAVE(uint16_t argc, int8_t **argv);
static void cmdREQ(uint16_t argc, int8_t **argv);
static void cmdRENAME(uint16_t argc, int8_t **argv);
static void cmdEMAIL(uint16_t argc, int8_t **argv);
static void cmdUPTIME(uint16_t argc, int8_t **argv);
static void cmdBACKDOOR(uint16_t argc, int8_t **argv);
static void cmdSCAN(uint16_t argc, int8_t **argv);
static void cmdKILL(uint16_t argc, int8_t **argv);
static void cmdRTF(uint16_t argc, int8_t **argv);
static void cmdIGNORE(uint16_t argc, int8_t **argv);
static void cmdVISIBLE(uint16_t argc, int8_t **argv);
static void cmdINVISIBLE(uint16_t argc, int8_t **argv);
static void cmdALERT(uint16_t argc, int8_t **argv);
static void cmdKEY(uint16_t argc, int8_t **argv);
static void cmdSEEN(uint16_t argc, int8_t **argv);
static void cmdPASSWORD(uint16_t argc, int8_t **argv);
static void cmdCONNECT(int argc, char **argv);
static void cmdDISCONNECT(int argc, char **argv);
static void cmdCONTACTS(uint16_t argc, int8_t **argv);
static void cmdURL(uint16_t argc, int8_t **argv);
static void cmdFILECANCEL(uint16_t argc, int8_t **argv);
void cmdFILESTATUS(uint16_t argc, int8_t **argv);
static void cmdFILEACCEPT(uint16_t argc, int8_t **argv);
static void cmdFILEDECLINE(uint16_t argc, int8_t **argv);
static void cmdSEND(uint16_t argc, int8_t **argv);
static void cmdOPENDC(uint16_t argc, int8_t **argv);
static void cmdCLOSEDC(uint16_t argc, int8_t **argv);
static void cmdSLAVESALL(uint16_t argc, int8_t **argv);
static void cmdSHOWLEAK(uint16_t argc, int8_t **argv);

bool_t doCommand(uint16_t argc, int8_t **argv);

#endif /* _COMMANDS_H_ */
