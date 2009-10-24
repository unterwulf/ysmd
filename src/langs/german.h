/*	$Id: german.h,v 1.10 2005/08/16 00:28:08 rad2k Exp $	*/

/*
 * YSM Language Specific header file
 *
 *	GERMAN LANGUAGE
 *
 * by sMiLe
 *
 */


/* YSM STARTING MESSAGES */

#define MSG_STARTING_ONE	"..eine" WHITE " Erfahrung" NORMAL " die Du nie vergessen wirst.."
#define MSG_STARTING_TWO	"[ INITIALISIERE YSM ][ " BLUE "UIN" NORMAL ", die benutzt wird:"
#define MSG_FOUND_SECPASS	"Habe ein geschütztes Password gefunden. Ich liebe paranoide Benutzer ;)."
#define MSG_FOUND_SECPASS2	"Bitte noch einmal [zur Sicherheit]: "
#define MSG_READ_SLAVES		"YSM Anzahl der Sklaven in der db" BRIGHT_BLUE " ["
#define MSG_ASK_KEYMAP1		"Standard Tastenkürzel sind:: "
#define MSG_ASK_KEYMAP2		"Möchtest Du deine Tastenbelegung neu konfigurieren? [Y/N]:"
#define MSG_NETWORK_INFO	"[Netzwerk Information: TCP lokales port:"
#define MSG_NETWORK_INFO2	"Ok neue srv. Informationen eingetrudelt."
#define MSG_NETWORK_INFO3	"Verbinde mit dem 2. BOS Server.."
#define MSG_LOGIN_OK		BLUE "Login OK!" NORMAL " << Wilkommen im ICQ Netzwerk >>"
#define MSG_ASK_DOWNLOAD1	"VERFÜGBARE Contacts wurden auf dem ICQ-Server gefunden."
#define MSG_ASK_DOWNLOAD2	"Möchtest Du sie runterladen? [Y/N]: "
#define MSG_DOWNLOADED_SLAVES	"downloaded SLAVES belong to your kingdom."

/* YSM SLAVES INTERACTION MESSAGES */

#define MSG_STATUS_CHANGE1	"[Ein Sklave #"
#define MSG_STATUS_CHANGE2	", genannt" BLUE
#define MSG_STATUS_CHANGE3	NORMAL "änderte seine Stimmung zu:"

#define MSG_STATUS_HIDE1	"[A Slave #"
#define MSG_STATUS_HIDE2	", called" BLUE
#define MSG_STATUS_HIDE3	NORMAL "hides from you in:"


#define MSG_MESSAGE_SENT1	MAGENTA "YSM Nachricht gesendet an den Sklaven, genannt" WHITE
#define MSG_MESSAGE_SENT2	MAGENTA "YSM Nachricht gesendet an" WHITE

#define MSG_MESSAGE_SENT3	MAGENTA	"Encrypted Msg Sent to Slave called" WHITE
#define MSG_MESSAGE_SENT4	MAGENTA	"Encrypted Msg Sent to" WHITE

#define MSG_INCOMING_MESSAGE	"[Hereinkommende Nachricht]"
#define MSG_INCOMING_URL	"[Hereinkommende URL]"
#define MSG_INCOMING_AUTHR	"Hereinkommende Bitte um Authorisation von"

#define MSG_INCOMING_PAGER	"[Pager Message]"

#define MSG_SLAVES_LIST		"Dir GEHÖREN die folgenden"
#define MSG_SLAVES_ONLINE	"Dies sind die MOMENTAN VERFÜGBAREN SKLAVEN, die Dir gehören:"

#define MSG_SLAVES_BIRTHDAY	WHITE "BIRTHDAY" NORMAL

#define MSG_WARN_ADDED		"Werde paranoid! Ein ICQ-User hat Dich zu seiner/ihrer Contac-List hinzugefügt!"
#define MSG_WARN_AUTHOK		"Sehr schön! Du wurdest authorisiert, ihn/sie zu Deinem ICQ-Sklaven zu machen"
#define MSG_WARN_AUTHDENY	"WTF! Dieser Heckenpenner hat Deine Authorisations-Anfrage abgelehnt"

/* YSM AFK RELATED MESSAGES */

#define MSG_IDLE_TIME_REACHED	"IDLE Time erreicht. Gehe in afk..\n"
#define MSG_AFK_MODE_ON		"Du bist " BRIGHT_BLUE "jetzt im AFK" NORMAL "-Modus (Away From Keyboard).\nEingehende Nachrichten werden gelogged und eine automatische Antwort wird an\njeden gesendet, der Dir eine Nachricht schickt.\nDu kannst dann 'afk' noch einmal eingeben um den AFK-Modus zu verlassen.\nGeniess Deinen Kaffee ;)"

#define MSG_AFK_MODE_OFF1	"Willkommen ZURÜCK! Du warst"
#define MSG_AFK_MODE_OFF2	"Minuten weg!\n"
#define MSG_AFK_MODE_OFF3	"neue Nachrichten wurden in Deinem ysm-Verzeichnis\ngespeichert in der Datei"
#define MSG_AFK_MODE_OFF4	"während Du weg warst.\nTippe 'readafk', um die gespeicherten Nachrichten zu lesen und zu löschen!\nVerlasse" BRIGHT_BLUE " AFK" NORMAL "-Modus..\n"

#define MSG_AFK_READ_MSG	GREEN "[[ Lese gespeicherte AFK-Nachrichten ]]" NORMAL

/* ERROR MESSAGES */

#define MSG_ERR_PASSMATCH	CYAN "<<NEIN!>>" NORMAL " Die Passwörter stimmen nicht überein."
#define MSG_ERR_DISCONNECTED	"Verbindung zum Server verloren. Überprüfe Dein Passwort."

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
