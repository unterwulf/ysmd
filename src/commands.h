/*    $Id: YSM_Commands.h,v 1.23 2005/07/17 23:35:33 rad2k Exp $    */
/*
-======================== ysmICQ client ============================-
        Having fun with a boring Protocol
-======================== YSM_Commands.h ===========================-

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

#ifndef _COMMANDS_H_
#define _COMMANDS_H_

/* Command Groups */
#define YSM_COMMAND_GROUP_USERS         0x00
#define YSM_COMMAND_GROUP_USERS_STR     MAGENTA "USERS/SLAVES" NORMAL":\n"
#define YSM_COMMAND_GROUP_SETTINGS      0x01
#define YSM_COMMAND_GROUP_SETTINGS_STR  MAGENTA "SETTINGS/CONFIGURATION"NORMAL":\n"
#define YSM_COMMAND_GROUP_ACCOUNT       0x02
#define YSM_COMMAND_GROUP_ACCOUNT_STR   MAGENTA "ACCOUNT/SESSION"NORMAL":\n"
#define YSM_COMMAND_GROUP_CLIENT        0x03
#define YSM_COMMAND_GROUP_CLIENT_STR    MAGENTA "CLIENT"NORMAL":\n"
#define YSM_COMMAND_GROUP_AMOUNT        0x04


#define YSM_COMMAND_QUIT_HELP        "Close the ysm client.\n"                            "usage: 'quit'\n\n"

#define YSM_COMMAND_HELP_HELP        "Query for help on a command\n"                            "usage: 'help <command>'\n\n"

#define YSM_COMMAND_INFO_HELP        "Show information about your ysm client and session.\n"                                                        "usage: 'info'\n\n"

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

#define YSM_COMMAND_LASTSENT_HELP    "Send a message to the last slave you messaged.\n(Also check the <tab> key function)\n"                                        "usage: 'a [message]'\n\n"

#define YSM_COMMAND_REPLY_HELP        "Send a message to the last slave who messaged you.\n(Also check the <tab> key function)\n"                                    "usage: 'r [message]'\n\n"

#define YSM_COMMAND_WHOIS_HELP        "Request information on a slave or icq#\n"                    "usage: 'whois <name>'\n"                            "usage: 'whois <uin>'\n\n"

#define YSM_COMMAND_BEEP_HELP        "Turn on/off message beeping!\n"                        "usage: 'beep [on|off]'\n\n"

#define YSM_COMMAND_SOUNDS_HELP        "Turn on/off wave sounds globally.\n"                        "usage: 'sounds [on|off]'\n\n"

#define YSM_COMMAND_AFK_HELP        "Type 'afk' to switch ON or OFF the Away from Keyboard mode. excellent, huh?\nType 'afk here_a_message' to set the afk message in runtime.\n"                                                    "usage: 'afk [auto_message]'\n\n"

#define YSM_COMMAND_READAFK_HELP    "Read the messages stored while you were on afk mode!\n"                                                    "usage: 'readafk'\n\n"

#define YSM_COMMAND_LOGALL_HELP        "Check/Configure global/specific Logging.\n"                    "usage: 'logall [on|off]'\n"                            "usage: 'logall <name>'\n\n"

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

#define YSM_COMMAND_TABKEY_HELP        "Special Key. Nick auto-complete. Reply/Resend a message to a slave.\n"        "usage: Press <TAB> after you send a message.\n"                "usage: Press <TAB> after you receive a message.\n"                "usage: Press <TAB> while writing a slave's name.\n\n"

#define YSM_COMMAND_HOTKEYS_HELP    "YSM Hotkeys provide you with quick command keys.\nPress any of the following keys at the start of line:\n"                            "'1' - help command.\n"                                "'2' - whos' online (wo)\n"                            "'3' - list all slaves (w)\n"                            "'4' - enable/disable AFK (afk)\n"                        "'5' - read afk messages (readafk)\n"                        "'6' - current file transfers (fstatus)"                    "\n\n"

#define YSM_COMMAND_LAST_HELP        "Show the last received message.\n\n"

#define YSM_COMMAND_HIST_HELP        "Read a slave's log file (message HISTORY).\n"                    "usage: 'hist <name>'\n"

#define YSM_COMMAND_LOADCONFIG_HELP    "Reload configuration file settings.\n"                        "usage: 'loadconfig'\n\n"

#define YSM_COMMAND_MINIMIZE_HELP    "Minimize YSM's console window [win32 only]\n"                    "usage: 'z'\n\n"

#define YSM_COMMAND_KEY_HELP        "Set an encryption key to use between two YSM clients.\nKeys are used to send encrypted messages and encrypted file transfers.\nThe keys are specified in hexadecimal and they must be max 32 bytes [32 * 2 hex]\nIn example: 'key rad2k 616161616161616161' sets a 9 bytes long key with rad2k.\nYou should then tell the slave to set the same key with you.\nIf used with no arguments, the slave's key will be cleared.\nIf used with a '?' a 32 bytes random key will be generated, set,  and shown in the screen.\nIn example: 'key rad2k ?'.\n"                                                    "usage: 'key [name] [hex_key|?]'\n\n"

#define YSM_COMMAND_BURL_HELP        "Launch a browser for a specified URL or the last received URL message.\nThe browser is configured inside the cfg file using the 'BROWSER>' setting.\n"                                                "usage: 'burl <url>' for a url\n"                        "usage: 'burl !' for last saved url\n"

#define YSM_COMMAND_RUN_HELP        "Run a command in your current shell.\n"                    "usage: '! ls -al'\n\n"

#define YSM_COMMAND_FORWARD_HELP    "Forward your incoming messages to a specified slave or icq#.\n"        "usage: 'forward [name]'\n"                            "usage: 'forward [uin]'\n"

#define YSM_COMMAND_SEEN_HELP    "Display signon, last status change and\nlast message received time of a specified slave.\n"                                        "usage: 'seen <name>'\n\n"

#define YSM_COMMAND_PASSWORD_HELP    "Change your ICQ password. Passwords must be between 4 and 8 characters long.\n"                            "usage: 'password <new_pasword>'\n\n"

#define YSM_COMMAND_RECONNECT_HELP    "Reconnect to the ICQ Network.\n"                        "usage: 'reconnect'\n\n"

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

command_t * YSM_AddCommandtoList(
	int8_t    *cmd_name,
    int8_t    *cmd_alias,
    int8_t    *cmd_help,
    int16_t    groupid,
    u_int16_t  cmd_margs,
    void      *pfunc);

void YSM_Init_Commands(void);

#ifdef HAVE_LIBREADLINE
    /* The following calls to ConsoleSetup and ConsoleRestore,
     * fix a weird bug on readline when calling a function that
     * makes use of the 'getkey()' function, after receiving
     * a command from the user. (This doesn't happen for hotkeys)
     */
#define PRE_GETKEY_FIX    YSM_ConsoleSetup();
#define POS_GETKEY_FIX    YSM_ConsoleRestore();
#else
#define PRE_GETKEY_FIX
#define POS_GETKEY_FIX
#endif

#endif
