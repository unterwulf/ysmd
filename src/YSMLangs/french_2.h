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
#define MSG_STARTING_TWO	"[ DÉMARRAGE YSM ][ " BLUE "UIN" NORMAL " A LOGUER :"
#define MSG_FOUND_SECPASS	"Trouvé mot de passe sécurisé, j'adore les utilisateus paranoïaques ;)."
#define MSG_FOUND_SECPASS2	"Retapez [verification]: "
#define MSG_READ_SLAVES		"Quantité d'esclaves YSM en db" BRIGHT_BLUE " ["
#define MSG_ASK_KEYMAP1		"Les raccourcis par default sont: "
#define MSG_ASK_KEYMAP2		"Voudriez vous reconfigurer votre key map? [Y/N]:"
#define MSG_NETWORK_INFO	"[Information Réseau: Port TCP local:"
#define MSG_NETWORK_INFO2	"Ok Nouvelles données du srv. reçues."
#define MSG_NETWORK_INFO3	"Connection au 2ème serveur BOS..."
#define MSG_LOGIN_OK		BLUE "Login OK!" NORMAL " << Bienvenu sur le réseau ICQ >>"
#define MSG_ASK_DOWNLOAD1	"Contacts DISPONIBLES trouvés sur les serveurs ICQ."
#define MSG_ASK_DOWNLOAD2	"Voudriez vous les télécharger? [Y/N]: "
#define MSG_DOWNLOADED_SLAVES	"downloaded SLAVES belong to your kingdom."

/* YSM SLAVES INTERACTION MESSAGES */

#define MSG_STATUS_CHANGE1	"[Un Esclave #"
#define MSG_STATUS_CHANGE2	", a appellé" BLUE
#define MSG_STATUS_CHANGE3	NORMAL "a changé son humeur à:"

#define MSG_STATUS_HIDE1	"[A Slave #"
#define MSG_STATUS_HIDE2	", called" BLUE
#define MSG_STATUS_HIDE3	NORMAL "hides from you in:"


#define MSG_MESSAGE_SENT1	MAGENTA "Msg YSM envoyé à l'Esclave appellé" WHITE
#define MSG_MESSAGE_SENT2	MAGENTA "Msg YSM envoyé à" WHITE

#define MSG_MESSAGE_SENT3	MAGENTA	"Encrypted Msg Sent to Slave called" WHITE
#define MSG_MESSAGE_SENT4	MAGENTA	"Encrypted Msg Sent to" WHITE

#define MSG_INCOMING_MESSAGE	"[Msg Entrant]"
#define MSG_INCOMING_URL	"[URL Entrant]"
#define MSG_INCOMING_AUTHR	"Demande d'Autorisation entrante de"

#define MSG_INCOMING_PAGER	"[Pager Message]"

#define MSG_SLAVES_LIST		"Vous POSSEDÉZ les suivants"
#define MSG_SLAVES_ONLINE	"Ceux ci sont les ESCLAVES ACTUELLEMENT EN LIGNE que vous possédez:"

#define MSG_SLAVES_BIRTHDAY	WHITE "BIRTHDAY" NORMAL

#define MSG_WARN_ADDED		"Devenez Paranoïaque! un utilisateur ICQ viens de vous ajouter a sa liste!"
#define MSG_WARN_AUTHOK		"Bien! Vous êtes autorisé à le rendre votre Esclave par ICQ"
#define MSG_WARN_AUTHDENY	"WTF! Le cinglé vous à -NIÉ- l'Autorisation!"

/* YSM AFK RELATED MESSAGES */

#define MSG_IDLE_TIME_REACHED	"Temps IDLE atteint. Passant à mode afk..\n"
#define MSG_AFK_MODE_ON		"Vous êtes " BRIGHT_BLUE "maintenant en mode AFK" NORMAL " (Away From Keyboard).\nles messages entrants seront enregistrés et une réponse autimatique sera envoyée\nà ceux qui vous envoient des messages.\nVous pouvez alors taper 'afk' pour sortir du mode AFK. Profitez de vôtre café ;)"

#define MSG_AFK_MODE_OFF1	"Bienvenu! vous étiez ailleurs"
#define MSG_AFK_MODE_OFF2	"minutes!.\n"
#define MSG_AFK_MODE_OFF3	"Nouveaux messages enregistrés dans votre\nrépértoire ysm dans le fichier"
#define MSG_AFK_MODE_OFF4	"pendant que vous étiez ailleurs.\nTapez 'readafk' pour lire ou effacer les messages enregistrés!.\nSortie du mode" BRIGHT_BLUE " AFK\n"


#define MSG_AFK_READ_MSG	GREEN "[[ Lecture des messages AFK enregistrés ]]" NORMAL

/* ERROR MESSAGES */

#define MSG_ERR_PASSMATCH	CYAN "<<NON!>>" NORMAL " Les mots de passe sont différents!."
#define MSG_ERR_DISCONNECTED	"Débranché par le serveur. Vérifiez votre mot de passe."
#define MSG_ERR_INVPASSWD	RED "Serveur a dit: Mauvais Mot de Passe." NORMAL
#define MSG_ERR_INVUIN		RED "Serveur a dit: UIN non valide." NORMAL
#define MSG_ERR_TOOMC		RED "Serveur a dit: Trop de clients depuis la même IP." NORMAL
#define MSG_ERR_RATE		RED "Serveur a dit: Taux de connection excédé. Réssayez un peu plus tard." NORMAL
#define MSG_ERR_SAMEUIN		CYAN "!PARANOID!" NORMAL " Someone else logged in using this ICQ UIN." 
#define MSG_ERR_CONNECT		"Connection échoe!"
#define MSG_ERR_PROXY		"Le Proxy a retourné une ERREUR!::"
#define MSG_ERR_PROXY2		"Proxy requires Authorization.\n"
#define MSG_ERR_PROXY3		"Method Failed. Unknown Reason.\n"

#define MSG_ERR_REGISTERING	"Réponse du serveur non valide pendant l'enregistrement. Sortie."

#define MSG_ERR_FEATUREDISABLED	"Fonction non disponible. Recompilez YSM avec support pour threads."

#define MSG_CONN_PROXYOK	"Connecté au PROXY!"
#define MSG_CONN_SERVEROK	"connection TCP reussie avec le SERVEUR ICQ"

#define MSG_AOL_WARNING		"Attention AOL! arretez d'envoyer des messages trop vite!"

#define MSG_REQ_SENT1		"Demande YSM Envoyée à"
#define MSG_REQ_SENT2		"Esclave appellé"

#define MSG_AUTH_SENT1		"Autorisation YSM Envoyée à"
#define MSG_AUTH_SENT2		"Slave called"

#define MSG_NEW_OFFLINE		"Message réçu pendant que vous étiez" BRIGHT_BLUE " offline"

#define MSG_BUDDY_GROUPCREATED	"Groupe YSM Crée."
#define MSG_BUDDY_SAVING1	BRIGHT_BLUE "Sauvegardant ["
#define MSG_BUDDY_SAVING2	"]" WHITE " ["
#define MSG_BUDDY_SAVING3	"]" NORMAL	

#define MSG_BUDDY_SAVINGOK	GREEN " [OK] ESCLAVE SAVEGARDÉ"
#define MSG_BUDDY_SAVINGERROR	RED " [ERREUR]"
#define MSG_BUDDY_SAVINGERROR2	"Peut-etre ajouter un compte non disponible?."


#define MSG_BUDDY_SAVINGAUTH	RED " [AUTH]"
#define MSG_BUDDY_SAVINGAUTH2	"Autorisation de l'utilisateur nécessaire."
#define MSG_BUDDY_SAVINGAUTH3	" [tapez la commande 'req']."

#define MSG_BUDDY_BLOCKWARN	"Recption de l'info de status de l'esclave."
#define MSG_BUDDY_BLOCKWARN2	"L'entrée peut rester bloquée pendant quelques secondes."

#define MSG_REG_SUCCESFULL	"Enregistrement Réussi. Votre noveau UIN est"
#define MSG_REG_FAILED		RED "UIN Registration failed. This may be due to too many registration requests from the same IP address. Please try again later." NORMAL

#define MSG_INFO_UPDATED	"Information Mise à Jour"

#define MSG_SEARCH_FAILED	"Recherche échouée. le UIN/Mail est valide?"

#define MSG_DIRECT_CONNECTING	"Tenez bon. Connection à "
#define MSG_DIRECT_ESTABLISHED	"Connection Réussie."
#define MSG_DIRECT_ERR1	"Impossible d'établir une connection directe derrière un proxy."
#define MSG_DIRECT_ERR2 "Impossible d'établir connection directe.\nSoit l'IP out le port remote d'écoute est inconnu."
#define MSG_DIRECT_ERR3 "Impossible établir connection directe.\nConnection Refusée."
