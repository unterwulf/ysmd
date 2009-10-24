/*    $Id: english.h,v 1.10 2005/08/16 00:28:08 rad2k Exp $    */

/*
 * YSM Language Specific header file
 *
 *    ENGLISH LANGUAGE
 *
 * by rad2k
 *
 */


/* YSM STARTING MESSAGES */

#define MSG_STARTING_ONE    "..an" " Experience" " you will never forget.."
#define MSG_STARTING_TWO    "[ INITIALIZING YSM ][ " "UIN" " TO USE :"
#define MSG_FOUND_SECPASS    "Found a Secured Password. I love Paranoid users ;)."
#define MSG_FOUND_SECPASS2    "Type again [verification]: "
#define MSG_READ_SLAVES        "YSM Amount of Slaves in db" " ["
#define MSG_ASK_KEYMAP1        "Default shortcuts are: "
#define MSG_ASK_KEYMAP2        "Would you like to re-configure your key map? [Y/N]:"
#define MSG_NETWORK_INFO    "[Network Information: TCP local port:"
#define MSG_NETWORK_INFO2    "Ok New srv. Information arrived."
#define MSG_NETWORK_INFO3    "Connecting to the BOS 2nd Server.."
#define MSG_LOGIN_OK        "Login OK!" " << Welcome to the ICQ Network >>"
#define MSG_ASK_DOWNLOAD1    "AVAILABLE Contacts stored in the ICQ servers were found."
#define MSG_ASK_DOWNLOAD2    "Do you wish to download them? [Y/N]: "
#define MSG_DOWNLOADED_SLAVES    "downloaded SLAVES belong to your kingdom."

/* YSM SLAVES INTERACTION MESSAGES */

#define MSG_STATUS_CHANGE1    "[A Slave #"
#define MSG_STATUS_CHANGE2    ", called" 
#define MSG_STATUS_CHANGE3    "changed his mood to:"

#define MSG_STATUS_HIDE1    "[A Slave #"
#define MSG_STATUS_HIDE2    ", called" 
#define MSG_STATUS_HIDE3    "hides from you in:"

#define MSG_MESSAGE_SENT1    "YSM Msg Sent to Slave called" 
#define MSG_MESSAGE_SENT2    "YSM Msg Sent to" 

#define MSG_MESSAGE_SENT3       "Encrypted Msg Sent to Slave called" 
#define MSG_MESSAGE_SENT4       "Encrypted Msg Sent to" 

#define MSG_INCOMING_MESSAGE    "[Incoming Msg]"
#define MSG_INCOMING_URL    "[Incoming URL]"
#define MSG_INCOMING_AUTHR    "Incoming Authorization Request from"

#define MSG_INCOMING_PAGER    "[Pager Message]"

#define MSG_SLAVES_LIST        "You OWN the Following"
#define MSG_SLAVES_ONLINE    "These are the CURRENTLY ONLINE SLAVES you own:"

#define MSG_SLAVES_BIRTHDAY    "BIRTHDAY" 

#define MSG_WARN_ADDED        "Get Paranoid! an ICQ user added you to his/her list!"
#define MSG_WARN_AUTHOK        "Fine! You were authorized to make him/her your Slave by the ICQ"
#define MSG_WARN_AUTHDENY    "WTF! the redneck bastard -DENIED- your Auth request"

/* YSM AFK RELATED MESSAGES */

#define MSG_IDLE_TIME_REACHED    "IDLE Time reached. going afk..\n"
#define MSG_AFK_MODE_ON        "You are " "now in AFK" " mode (Away From Keyboard).\nIncoming Messages will be logged and an autoreply will be sent\nto anyone who messages you.\nyou can then type 'afk' again to leave AFK mode. enjoy your coffee ;)"

#define MSG_AFK_MODE_OFF1    "Welcome BACK! You were away"
#define MSG_AFK_MODE_OFF2    "minutes!.\n"
#define MSG_AFK_MODE_OFF3    "new messages were stored in your ysm\ndirectory in the file"
#define MSG_AFK_MODE_OFF4    "while you were away.\nUse 'readafk' to read or clean the stored messages!.\nLeaving" " AFK " "mode..\n"

#define MSG_AFK_READ_MSG    "[[ Reading stored AFK Messages ]]" 

/* ERROR MESSAGES */

#define MSG_ERR_PASSMATCH    "<<NO!>>" " Passwords do NOT match."
#define MSG_ERR_DISCONNECTED    "Disconnected by the Server. Check your password."
#define MSG_ERR_INVPASSWD    "Server said: Bad Password." 
#define MSG_ERR_INVUIN        "Server said: Invalid UIN." 
#define MSG_ERR_TOOMC        "Server said: Too many clients from the same IP." 
#define MSG_ERR_RATE        "Server said: Rate limit exceeded. Try again later." 
#define MSG_ERR_SAMEUIN        "!PARANOID!" " Someone else logged in using this ICQ UIN." 

#define MSG_ERR_CONNECT        "Connect failed!"
#define MSG_ERR_PROXY        "Proxy returned an ERROR!::"
#define MSG_ERR_PROXY2        "Proxy requires Authorization.\n"
#define MSG_ERR_PROXY3        "Method Failed. Unknown Reason.\n"
#define MSG_ERR_REGISTERING    "Invalid Server Response While Registering. Leaving."

#define MSG_ERR_FEATUISABLED    "Feature disabled. Recompile YSM with Threads support."

#define MSG_CONN_PROXYOK    "Connected to the PROXY!"
#define MSG_CONN_SERVEROK    "TCP connection established with ICQ SERVER"

#define MSG_AOL_WARNING        "AOL Warning! stop sending messages too fast!"

#define MSG_REQ_SENT1        "YSM Request Sent to"
#define MSG_REQ_SENT2        "Slave called"

#define MSG_AUTH_SENT1        "YSM Authorization Sent to"
#define MSG_AUTH_SENT2        "Slave called"

#define MSG_NEW_OFFLINE        "Message received when you were" " offline"

#define MSG_BUDDY_GROUPCREATED    "YSM Group Created."
#define MSG_BUDDY_SAVING1    "Saving ["
#define MSG_BUDDY_SAVING2    "]" " ["
#define MSG_BUDDY_SAVING3    "]"    

#define MSG_BUDDY_SAVINGERROR    " [ERROR]"
#define MSG_BUDDY_SAVINGERROR2    "Maybe adding a disabled account?."


#define MSG_BUDDY_SAVINGAUTH    " [AUTH]"
#define MSG_BUDDY_SAVINGAUTH2    "Authorization from the user is required."
#define MSG_BUDDY_SAVINGAUTH3    " [use the 'req' command]."

#define MSG_BUDDY_BLOCKWARN    "Receiving Slave status information."
#define MSG_BUDDY_BLOCKWARN2    "Your input may get blocked for a few seconds."

#define MSG_REG_SUCCESFULL    "Registration Succesfull. Your new UIN is "
#define MSG_REG_FAILED        "UIN Registration failed. This may be due to too many registration requests from the same IP address. Please try again later." 

#define MSG_INFO_UPDATED    "Information Updated!"

#define MSG_SEARCH_FAILED    "Search Failed. valid UIN/Mail?"

#define MSG_DIRECT_CONNECTING    "Hold on. Connecting to "
#define MSG_DIRECT_ESTABLISHED    "Connection Established."
#define MSG_DIRECT_ERR1    "Can't establish a Direct Connection from behind a proxy."
#define MSG_DIRECT_ERR2    "Can't establish a Direct Connection.\nEither the remote IP or listening Port are unknown."
#define MSG_DIRECT_ERR3 "Can't establish a Direct Connection.\nConnection Refused"
