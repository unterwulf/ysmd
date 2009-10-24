/*	$Id: french_2.h,v 1.10 2005/08/16 00:28:08 rad2k Exp $	*/

/*
 * YSM Language Specific header file
 *
 *	ENGLISH LANGUAGE
 *
 * by rad2k
 *
 */


/* YSM STARTING MESSAGES */

#define MSG_STARTING_ONE	"..une" WHITE " Experience" NORMAL " que vous n'oublierez jamais.."
#define MSG_STARTING_TWO	"[ D�MARRAGE YSM ][ " BLUE "UIN" NORMAL " A LOGUER :"
#define MSG_FOUND_SECPASS	"Trouv� mot de passe s�curis�, j'adore les utilisateus parano�aques ;)."
#define MSG_FOUND_SECPASS2	"Retapez [verification]: "
#define MSG_READ_SLAVES		"Quantit� d'esclaves YSM en db" BRIGHT_BLUE " ["
#define MSG_ASK_KEYMAP1		"Les raccourcis par default sont: "
#define MSG_ASK_KEYMAP2		"Voudriez vous reconfigurer votre key map? [Y/N]:"
#define MSG_NETWORK_INFO	"[Information R�seau: Port TCP local:"
#define MSG_NETWORK_INFO2	"Ok Nouvelles donn�es du srv. re�ues."
#define MSG_NETWORK_INFO3	"Connection au 2�me serveur BOS..."
#define MSG_LOGIN_OK		BLUE "Login OK!" NORMAL " << Bienvenu sur le r�seau ICQ >>"
#define MSG_ASK_DOWNLOAD1	"Contacts DISPONIBLES trouv�s sur les serveurs ICQ."
#define MSG_ASK_DOWNLOAD2	"Voudriez vous les t�l�charger? [Y/N]: "
#define MSG_DOWNLOADED_SLAVES	"downloaded SLAVES belong to your kingdom."

/* YSM SLAVES INTERACTION MESSAGES */

#define MSG_STATUS_CHANGE1	"[Un Esclave #"
#define MSG_STATUS_CHANGE2	", a appell�" BLUE
#define MSG_STATUS_CHANGE3	NORMAL "a chang� son humeur �:"

#define MSG_STATUS_HIDE1	"[A Slave #"
#define MSG_STATUS_HIDE2	", called" BLUE
#define MSG_STATUS_HIDE3	NORMAL "hides from you in:"


#define MSG_MESSAGE_SENT1	MAGENTA "Msg YSM envoy� � l'Esclave appell�" WHITE
#define MSG_MESSAGE_SENT2	MAGENTA "Msg YSM envoy� �" WHITE

#define MSG_MESSAGE_SENT3	MAGENTA	"Encrypted Msg Sent to Slave called" WHITE
#define MSG_MESSAGE_SENT4	MAGENTA	"Encrypted Msg Sent to" WHITE

#define MSG_INCOMING_MESSAGE	"[Msg Entrant]"
#define MSG_INCOMING_URL	"[URL Entrant]"
#define MSG_INCOMING_AUTHR	"Demande d'Autorisation entrante de"

#define MSG_INCOMING_PAGER	"[Pager Message]"

#define MSG_SLAVES_LIST		"Vous POSSED�Z les suivants"
#define MSG_SLAVES_ONLINE	"Ceux ci sont les ESCLAVES ACTUELLEMENT EN LIGNE que vous poss�dez:"

#define MSG_SLAVES_BIRTHDAY	WHITE "BIRTHDAY" NORMAL

#define MSG_WARN_ADDED		"Devenez Parano�aque! un utilisateur ICQ viens de vous ajouter a sa liste!"
#define MSG_WARN_AUTHOK		"Bien! Vous �tes autoris� � le rendre votre Esclave par ICQ"
#define MSG_WARN_AUTHDENY	"WTF! Le cingl� vous � -NI�- l'Autorisation!"

/* YSM AFK RELATED MESSAGES */

#define MSG_IDLE_TIME_REACHED	"Temps IDLE atteint. Passant � mode afk..\n"
#define MSG_AFK_MODE_ON		"Vous �tes " BRIGHT_BLUE "maintenant en mode AFK" NORMAL " (Away From Keyboard).\nles messages entrants seront enregistr�s et une r�ponse autimatique sera envoy�e\n� ceux qui vous envoient des messages.\nVous pouvez alors taper 'afk' pour sortir du mode AFK. Profitez de v�tre caf� ;)"

#define MSG_AFK_MODE_OFF1	"Bienvenu! vous �tiez ailleurs"
#define MSG_AFK_MODE_OFF2	"minutes!.\n"
#define MSG_AFK_MODE_OFF3	"Nouveaux messages enregistr�s dans votre\nr�p�rtoire ysm dans le fichier"
#define MSG_AFK_MODE_OFF4	"pendant que vous �tiez ailleurs.\nTapez 'readafk' pour lire ou effacer les messages enregistr�s!.\nSortie du mode" BRIGHT_BLUE " AFK\n"


#define MSG_AFK_READ_MSG	GREEN "[[ Lecture des messages AFK enregistr�s ]]" NORMAL

/* ERROR MESSAGES */

#define MSG_ERR_PASSMATCH	CYAN "<<NON!>>" NORMAL " Les mots de passe sont diff�rents!."
#define MSG_ERR_DISCONNECTED	"D�branch� par le serveur. V�rifiez votre mot de passe."
#define MSG_ERR_INVPASSWD	RED "Serveur a dit: Mauvais Mot de Passe." NORMAL
#define MSG_ERR_INVUIN		RED "Serveur a dit: UIN non valide." NORMAL
#define MSG_ERR_TOOMC		RED "Serveur a dit: Trop de clients depuis la m�me IP." NORMAL
#define MSG_ERR_RATE		RED "Serveur a dit: Taux de connection exc�d�. R�ssayez un peu plus tard." NORMAL
#define MSG_ERR_SAMEUIN		CYAN "!PARANOID!" NORMAL " Someone else logged in using this ICQ UIN." 
#define MSG_ERR_CONNECT		"Connection �choe!"
#define MSG_ERR_PROXY		"Le Proxy a retourn� une ERREUR!::"
#define MSG_ERR_PROXY2		"Proxy requires Authorization.\n"
#define MSG_ERR_PROXY3		"Method Failed. Unknown Reason.\n"

#define MSG_ERR_REGISTERING	"R�ponse du serveur non valide pendant l'enregistrement. Sortie."

#define MSG_ERR_FEATUREDISABLED	"Fonction non disponible. Recompilez YSM avec support pour threads."

#define MSG_CONN_PROXYOK	"Connect� au PROXY!"
#define MSG_CONN_SERVEROK	"connection TCP reussie avec le SERVEUR ICQ"

#define MSG_AOL_WARNING		"Attention AOL! arretez d'envoyer des messages trop vite!"

#define MSG_REQ_SENT1		"Demande YSM Envoy�e �"
#define MSG_REQ_SENT2		"Esclave appell�"

#define MSG_AUTH_SENT1		"Autorisation YSM Envoy�e �"
#define MSG_AUTH_SENT2		"Slave called"

#define MSG_NEW_OFFLINE		"Message r��u pendant que vous �tiez" BRIGHT_BLUE " offline"

#define MSG_BUDDY_GROUPCREATED	"Groupe YSM Cr�e."
#define MSG_BUDDY_SAVING1	BRIGHT_BLUE "Sauvegardant ["
#define MSG_BUDDY_SAVING2	"]" WHITE " ["
#define MSG_BUDDY_SAVING3	"]" NORMAL	

#define MSG_BUDDY_SAVINGOK	GREEN " [OK] ESCLAVE SAVEGARD�"
#define MSG_BUDDY_SAVINGERROR	RED " [ERREUR]"
#define MSG_BUDDY_SAVINGERROR2	"Peut-etre ajouter un compte non disponible?."


#define MSG_BUDDY_SAVINGAUTH	RED " [AUTH]"
#define MSG_BUDDY_SAVINGAUTH2	"Autorisation de l'utilisateur n�cessaire."
#define MSG_BUDDY_SAVINGAUTH3	" [tapez la commande 'req']."

#define MSG_BUDDY_BLOCKWARN	"Recption de l'info de status de l'esclave."
#define MSG_BUDDY_BLOCKWARN2	"L'entr�e peut rester bloqu�e pendant quelques secondes."

#define MSG_REG_SUCCESFULL	"Enregistrement R�ussi. Votre noveau UIN est"
#define MSG_REG_FAILED		RED "UIN Registration failed. This may be due to too many registration requests from the same IP address. Please try again later." NORMAL

#define MSG_INFO_UPDATED	"Information Mise � Jour"

#define MSG_SEARCH_FAILED	"Recherche �chou�e. le UIN/Mail est valide?"

#define MSG_DIRECT_CONNECTING	"Tenez bon. Connection � "
#define MSG_DIRECT_ESTABLISHED	"Connection R�ussie."
#define MSG_DIRECT_ERR1	"Impossible d'�tablir une connection directe derri�re un proxy."
#define MSG_DIRECT_ERR2 "Impossible d'�tablir connection directe.\nSoit l'IP out le port remote d'�coute est inconnu."
#define MSG_DIRECT_ERR3 "Impossible �tablir connection directe.\nConnection Refus�e."
