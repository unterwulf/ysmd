/*	$Id: italian.h,v 1.10 2005/08/16 00:28:08 rad2k Exp $	*/

/*
 * YSM Language Specific header file
 *
 *	ITALIAN LANGUAGE
 *
 * by Marco
 *
 */


/* YSM MESSAGGI DI AVVIO */

#define MSG_STARTING_ONE	"..una" WHITE " Esperienza" NORMAL " che non dimenticherai.."
#define MSG_STARTING_TWO	"[ INIZIALIZZO YSM ][ " BLUE "UIN" NORMAL "UTENTE :"
#define MSG_FOUND_SECPASS	"Trovata una password di sicurezza. Amo gli utenti paranoici ;)."
#define MSG_FOUND_SECPASS2	"Digita ancora [verifica]: "
#define MSG_READ_SLAVES		"YSM quantit� di utenti nel db" BRIGHT_BLUE " ["
#define MSG_ASK_KEYMAP1		"I collegamenti predefiniti sono: "
#define MSG_ASK_KEYMAP2		"Vuoi riconfigurare la tua tastiera? [Y/N]:"
#define MSG_NETWORK_INFO	"[Informazione di rete: porta locale TCP:"
#define MSG_NETWORK_INFO2	"Ok Nuovo server. Arrivata l'informazione."
#define MSG_NETWORK_INFO3	"Sto connettendomi al secondo BOS server.."
#define MSG_LOGIN_OK		BLUE "Login OK!" NORMAL " << Benvenuto nel network ICQ >>"
#define MSG_ASK_DOWNLOAD1	"Trovati contatti DISPONIBILI memorizzati nel server ICQ."
#define MSG_ASK_DOWNLOAD2	"Vuoi scaricarli? [Y/N]: "
#define MSG_DOWNLOADED_SLAVES	"downloaded SLAVES belong to your kingdom."

/* YSM SLAVES INTERACTION MESSAGES */

#define MSG_STATUS_CHANGE1	"[L'utente #"
#define MSG_STATUS_CHANGE2	", chiamato" BLUE
#define MSG_STATUS_CHANGE3	NORMAL "ha cambiato il suo stato in:"

#define MSG_STATUS_HIDE1	"[A Slave #"
#define MSG_STATUS_HIDE2	", called" BLUE
#define MSG_STATUS_HIDE3	NORMAL "hides from you in:"

#define MSG_MESSAGE_SENT1	MAGENTA "YSM Messaggio inviato all'utente" WHITE
#define MSG_MESSAGE_SENT2	MAGENTA "YSM Messaggio inviato a" WHITE

#define MSG_MESSAGE_SENT3	MAGENTA	"Encrypted Msg Sent to Slave called" WHITE
#define MSG_MESSAGE_SENT4	MAGENTA	"Encrypted Msg Sent to" WHITE

#define MSG_INCOMING_MESSAGE	"[Messaggio in arrivo]"
#define MSG_INCOMING_URL	"[URL in arrivo]"
#define MSG_INCOMING_AUTHR	"Richiesta autorizzazione da"

#define MSG_INCOMING_PAGER	"[Pager Message]"

#define MSG_SLAVES_LIST		"Tu conosci i seguenti"
#define MSG_SLAVES_ONLINE	"Lista degli utenti ATTUALMENTE CONNESSI da te conosciuti:"

#define MSG_SLAVES_BIRTHDAY	WHITE "BIRTHDAY" NORMAL

#define MSG_WARN_ADDED		"Che Paranoia! Un utente ICQ ti ha aggiunto alla sua lista!"
#define MSG_WARN_AUTHOK		"Bene! Sei stato autorizzato da ICQ"
#define MSG_WARN_AUTHDENY	"WTF! il bastardo ha  -NEGATO- la tua richiesta di Autorizzazione"

/* YSM AFK RELATED MESSAGES */

#define MSG_IDLE_TIME_REACHED	"Raggiunto il tempo massimo di inattivit�. Vado in modalit� afk..\n"
#define MSG_AFK_MODE_ON		"Tu sei " BRIGHT_BLUE "in modo AFK" NORMAL " (Lontano dalla tastiera).\nI messaggi in entrata saranno registrati e l'autorisposta sar� inviata\na chi ti scriver�.\ntu potrai premere 'afk' ancora per lasciare il tuo stato afk. buon caff� ;)"

#define MSG_AFK_MODE_OFF1	"BENTORANTO! Sei stato via"
#define MSG_AFK_MODE_OFF2	"minuti!.\n"
#define MSG_AFK_MODE_OFF3	"nuovi messaggi sono stati memorizzati nel file nella tua ysm directory"
#define MSG_AFK_MODE_OFF4	"mentre tu eri lontano.\nUsa 'readafk' per leggere o cancellare i messaggi memorizzati!.\nLascia" BRIGHT_BLUE " lo stato AFK " NORMAL "o..\n"

#define MSG_AFK_READ_MSG	GREEN "[[ Sto leggendo i messaggi AFK]]" NORMAL

/* ERROR MESSAGES */

#define MSG_ERR_PASSMATCH	CYAN "<<NO!>>" NORMAL " La Password non corrisponde."
#define MSG_ERR_DISCONNECTED	"Disconnessione da parte del server. Controlla la tua password."
#define MSG_ERR_INVPASSWD	RED "Server said: Bad Password." NORMAL
#define MSG_ERR_INVUIN		RED "Server said: Invalid UIN." NORMAL
#define MSG_ERR_TOOMC		RED "Server said: Too many clients from the same IP." NORMAL
#define MSG_ERR_RATE		RED "Server said: Rate limit exceeded. Try again later." NORMAL
#define MSG_ERR_SAMEUIN		CYAN "!PARANOID!" NORMAL " Someone else logged in using this ICQ UIN." 
#define MSG_ERR_CONNECT		"Connect failed!"
#define MSG_ERR_PROXY		"Proxy returned an ERROR!::"
#define MSG_ERR_PROXY2		"Proxy requires Authorization.\n"
#define MSG_ERR_PROXY3		"Method Failed. Unknown Reason.\n"

#define MSG_ERR_REGISTERING	"Invalid Server Response While Registering. Leaving."

#define MSG_ERR_FEATUREDISABLED	"Feature disabled. Recompile YSM with Threads support."

#define MSG_CONN_PROXYOK	"Connected to the PROXY!"
#define MSG_CONN_SERVEROK	"TCP connection established with ICQ SERVER"

#define MSG_AOL_WARNING		"AOL Warning! stop sending messages too fast!"

#define MSG_REQ_SENT1		"YSM Request Sent to"
#define MSG_REQ_SENT2		"Slave called"

#define MSG_AUTH_SENT1		"YSM Authorization Sent to"
#define MSG_AUTH_SENT2		"Slave called"

#define MSG_NEW_OFFLINE		"Message received when you were" BRIGHT_BLUE " offline"

#define MSG_BUDDY_GROUPCREATED	"YSM Group Created."
#define MSG_BUDDY_SAVING1	BRIGHT_BLUE "Saving ["
#define MSG_BUDDY_SAVING2	"]" WHITE " ["
#define MSG_BUDDY_SAVING3	"]" NORMAL	

#define MSG_BUDDY_SAVINGOK	GREEN " [OK] SLAVE SAVED"
#define MSG_BUDDY_SAVINGERROR	RED " [ERROR]"
#define MSG_BUDDY_SAVINGERROR2	"Maybe adding a disabled account?."


#define MSG_BUDDY_SAVINGAUTH	RED " [AUTH]"
#define MSG_BUDDY_SAVINGAUTH2	"Authorization from the user is required."
#define MSG_BUDDY_SAVINGAUTH3	" [use the 'req' command]."

#define MSG_BUDDY_BLOCKWARN	"Receiving Slave status information."
#define MSG_BUDDY_BLOCKWARN2	"Your input may get blocked for a few seconds."

#define MSG_REG_SUCCESFULL	"Registration Succesfull. Your new UIN is "
#define MSG_REG_FAILED		RED "UIN Registration failed. This may be due to too many registration requests from the same IP address. Please try again later." NORMAL

#define MSG_INFO_UPDATED	"Information Updated!"

#define MSG_SEARCH_FAILED	"Search Failed. valid UIN/Mail?"

#define MSG_DIRECT_CONNECTING	"Hold on. Connecting to "
#define MSG_DIRECT_ESTABLISHED	"Connection Established."
#define MSG_DIRECT_ERR1	"Can't establish a Direct Connection from behind a proxy."
#define MSG_DIRECT_ERR2	"Can't establish a Direct Connection.\nEither the remote IP or listening Port are unknown."
#define MSG_DIRECT_ERR3 "Can't establish a Direct Connection.\nConnection Refused"