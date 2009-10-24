/*    $Id: swedish.h,v 1.10 2005/08/16 00:28:08 rad2k Exp $    */

/*
 * YSM Language Specific header file
 *
 *    SWEDISH LANGUAGE
 *
 * by ydo
 *
 */


/* YSM STARTING MESSAGES */

#define MSG_STARTING_ONE    "..en" " upplevelse " " du aldrig kommer att glömma.."
#define MSG_STARTING_TWO    "[ INITIALIZING YSM ][ " "UIN" " A USAR :"
#define MSG_FOUND_SECPASS    "Hittade ett säkert lösenord. Jag älskar paranoida användare ;)."
#define MSG_FOUND_SECPASS2    "Skriv igen [verifiering]: "
#define MSG_READ_SLAVES        "YSM Antal slavar i db" " ["
#define MSG_ASK_KEYMAP1        "Förinställda genvägar är: "
#define MSG_ASK_KEYMAP2        "Vill du omkonfigurera dina tangentgenvägar? [Y/N]:"
#define MSG_NETWORK_INFO    "[Nätverksinformation: TCP lokal port:"
#define MSG_NETWORK_INFO2    "Ok ny srv. Information togs emot."
#define MSG_NETWORK_INFO3    "Ansluter till BOS andra server.."
#define MSG_LOGIN_OK        "Inloggning OK!" " << Välkommen till ICQnätverket >>"
#define MSG_ASK_DOWNLOAD1    "Tillgängliga kontakter lagrade i ICQ servrarna funna."
#define MSG_ASK_DOWNLOAD2    "Vill du ladda ner dem? [Y/N]: "
#define MSG_DOWNLOADED_SLAVES    "downloaded SLAVES belong to your kingdom."

/* YSM SLAVES INTERACTION MESSAGES */

#define MSG_STATUS_CHANGE1    "[En slav #"
#define MSG_STATUS_CHANGE2    ", kallad" 
#define MSG_STATUS_CHANGE3    "bytte humör till:"

#define MSG_STATUS_HIDE1    "[A Slave #"
#define MSG_STATUS_HIDE2    ", called" 
#define MSG_STATUS_HIDE3    "hides from you in:"

#define MSG_MESSAGE_SENT1    "YSM Msg Skickat till slaven kallad " 
#define MSG_MESSAGE_SENT2    "YSM Msg Skickat till" 

#define MSG_MESSAGE_SENT3       "Encrypted Msg Sent to Slave called" 
#define MSG_MESSAGE_SENT4       "Encrypted Msg Sent to" 

#define MSG_INCOMING_MESSAGE    "[Ankommande Msg]"
#define MSG_INCOMING_URL    "[Ankommande URL]"
#define MSG_INCOMING_AUTHR    "Ankommande Authorization förfrågan från"

#define MSG_INCOMING_PAGER    "[Pager Message]"

#define MSG_SLAVES_LIST        "Du ÄGER följande"
#define MSG_SLAVES_ONLINE    "Dessa är de av dina SLAVAR som är ONLINE:"

#define MSG_SLAVES_BIRTHDAY    "CUMPLEA~OS" 

#define MSG_WARN_ADDED        "Börja nojja! En ICQ-användare la till dig till sin lista!"
#define MSG_WARN_AUTHOK        "Gött! Du fick befogenhet att göra honom/henne till din slav via ICQ"
#define MSG_WARN_AUTHDENY    "Vafan! Det jävla ålarenset -NEKADE- din auth förfrågan"

/* YSM AFK RELATED MESSAGES */

#define MSG_IDLE_TIME_REACHED    "Inaktivitetstid uppnåd, blir afk..\n"
#define MSG_AFK_MODE_ON        "Du är " "nu i AFK-" " läge (Away From Keyboard).\nAnkommande meddelanden kommer att loggas och ett automatiskt svar kommer att skickas\ntill den som meddelar dig.\nDu kan sen skriva 'afk' igen för att lämna AFK-läge. Njut av ditt kaffe ;)"

#define MSG_AFK_MODE_OFF1    "Välkommen tillbaks! Du var borta"
#define MSG_AFK_MODE_OFF2    "minuter!.\n"
#define MSG_AFK_MODE_OFF3    "nya meddelanden har lagrats i din ysm-\nkatalog i filen"
#define MSG_AFK_MODE_OFF4    "meddans du var borta.\nGlöm inte rensa denna filen efter att ha läst den!.\nLämnar" " AFK " "läge..\n"


/* ERROR MESSAGES */

#define MSG_ERR_PASSMATCH    "<<NEJ!>>" " Lösenorden matchar INTE."
#define MSG_ERR_DISCONNECTED    "Anslutning till server bröts. Kolla ditt lösenord."

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

#define MSG_BUDDY_SAVINGOK    " [OK] SLAVE SAVED"
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
