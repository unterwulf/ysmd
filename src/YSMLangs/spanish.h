/*	$Id: spanish.h,v 1.10 2005/08/16 00:28:08 rad2k Exp $	*/

/*
 * YSM Language Specific header file
 *
 *	SPANISH LANGUAGE
 *
 * by rad2k
 *
 */

#define MSG_STARTING_ONE	"..una" WHITE " experiencia" NORMAL " que nunca olvidaras.."
#define MSG_STARTING_TWO	"[ INICIANDO YSM ][ " BLUE "UIN" NORMAL " TO USE :"
#define MSG_FOUND_SECPASS	"Encontre una Password SECURE!.\nMe caen bien los usuarios paranoicos ;)."
#define MSG_FOUND_SECPASS2	"Otra vez [verificacion]: "

#define MSG_READ_SLAVES		"YSM Cantidad de Slaves en la base" BRIGHT_BLUE " ["

#define MSG_ASK_KEYMAP1		"Los accesos rapidos al teclado son: "
#define MSG_ASK_KEYMAP2		"Queres re-definirlos? [Y/N]:"

#define MSG_NETWORK_INFO	"[Informacion de red: TCP puerto local:"

#define MSG_NETWORK_INFO2	"Ok llego nueva Informacion del srv."
#define MSG_NETWORK_INFO3	"Conectando al BOS, el segundo servidor.."

#define MSG_LOGIN_OK		BLUE "Login OK!" NORMAL " << Bienvenido a la red de ICQ >>"

#define MSG_ASK_DOWNLOAD1	"ENCONTRE contactos guardados en los servidores de ICQ."
#define MSG_ASK_DOWNLOAD2	"Queres bajarlos? [Y/N]: "

#define MSG_DOWNLOADED_SLAVES	"downloaded SLAVES belong to your kingdom."


#define MSG_STATUS_CHANGE1	"[Un Slave #"
#define MSG_STATUS_CHANGE2	", llamado" BLUE
#define MSG_STATUS_CHANGE3	NORMAL "cambio su status a:"

#define MSG_STATUS_HIDE1	"[A Slave #"
#define MSG_STATUS_HIDE2	", called" BLUE
#define MSG_STATUS_HIDE3	NORMAL "hides from you in:"


#define MSG_MESSAGE_SENT1	MAGENTA "YSM Mensaje enviado al Slave llamado" WHITE
#define MSG_MESSAGE_SENT2	MAGENTA "YSM Mensaje enviado a" WHITE

#define MSG_MESSAGE_SENT3	MAGENTA	"Encrypted Msg Sent to Slave called" WHITE
#define MSG_MESSAGE_SENT4	MAGENTA	"Encrypted Msg Sent to" WHITE

#define MSG_INCOMING_MESSAGE	"[Mensaje Nuevo]"
#define MSG_INCOMING_URL	"[URL Nueva]"
#define MSG_INCOMING_AUTHR	"Pedido de Autorizacion desde el"	

#define MSG_INCOMING_PAGER	"[Pager Message]"

#define MSG_SLAVES_LIST		"Te PERTENECEN los siguientes"
#define MSG_SLAVES_ONLINE	"Estos son los SLAVES ACTUALMENTE CONECTADOS:"

#define MSG_SLAVES_BIRTHDAY	WHITE "CUMPLEA~OS" NORMAL

#define MSG_WARN_ADDED		"Atencion! un usuario de ICQ te agrego a su lista!"
#define MSG_WARN_AUTHOK		"Ok! Fuiste autorizado para esclavizar al ICQ"
#define MSG_WARN_AUTHDENY	"Caraho! un infeliz te -DENEGO- tu pedido de Autorizacion"


#define MSG_IDLE_TIME_REACHED	"Tiempo IDLE superado. entrando en modo afk..\n"
#define MSG_AFK_MODE_ON		"Ahora estas en " BRIGHT_BLUE "AFK mode" NORMAL " (Away From Keyboard).\nLos mensajes que lleguen seran logueados y un auto-mensaje sera enviado\na quien te mande un mensaje.\nUsando el comando 'afk' podras salir del AFK mode. disfruta del recreo ;)"

#define MSG_AFK_MODE_OFF1	"Bienvenido nuevamente! Estuviste fuera"
#define MSG_AFK_MODE_OFF2	"minutos!.\n"
#define MSG_AFK_MODE_OFF3	"mensajes nuevos fueron guardados en tu\ndirectorio de YSM, en el archivo"
#define MSG_AFK_MODE_OFF4	"mientras no estabas.\nUsa el comando 'readafk' para leer/borrar los mensajes!.\nSaliendo del modo" BRIGHT_BLUE " AFK " NORMAL "..\n"

#define MSG_AFK_READ_MSG	GREEN "[[ Mensajes AFK guardados ]]" NORMAL


#define MSG_ERR_PASSMATCH	CYAN "<<NO!>>" NORMAL " Las Passwords no coinciden!."

#define MSG_ERR_DISCONNECTED	"El servidor nos desconecto!. Revisa tu password."


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
