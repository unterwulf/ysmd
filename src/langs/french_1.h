/*	$Id: french_1.h,v 1.10 2005/08/16 00:28:08 rad2k Exp $	*/

/*
 * YSM Language Specific header file
 *
 *	French 
 *
 * by zombie
 *
 */


/* YSM STARTING MESSAGES */

#define MSG_STARTING_ONE	"..une" WHITE " Experience" NORMAL " tu n'oblieras jamais.."
#define MSG_STARTING_TWO	"[ INITIALIZING YSM ][ " BLUE "UIN" NORMAL " TO USE :"
#define MSG_FOUND_SECPASS	"A trouve une mot de passe assurance.j'aime des users avec peur ;)."
#define MSG_FOUND_SECPASS2	"Tapes une nouvelle fois [verification]: "
#define MSG_READ_SLAVES		"YSM Quantite des escalves dans db" BRIGHT_BLUE " ["
#define MSG_ASK_KEYMAP1		"les d�faut raccourcis sont: "
#define MSG_ASK_KEYMAP2		"tu vais  re-configurer ton clavier? [Y/N]:"
#define MSG_NETWORK_INFO	"[R�seau ressignements : TCP porte local:"
#define MSG_NETWORK_INFO2	"Ok la Information de Nouveau srv. est arriv�."
#define MSG_NETWORK_INFO3	"an faisent conection avec le BOS 2 serveur.."
#define MSG_LOGIN_OK		BLUE "Login OK!" NORMAL " << bienvenu � ICQ R�sseau >>"
#define MSG_ASK_DOWNLOAD1	"DISPONIBLE. les Contacts stock� ont trouv� dans les ICQ serveurs ."
#define MSG_ASK_DOWNLOAD2	"Tu vais leur telecharcher? [Y/N]: "
#define MSG_DOWNLOADED_SLAVES	"downloaded SLAVES belong to your kingdom."

/* YSM SLAVES INTERACTION MESSAGES */

#define MSG_STATUS_CHANGE1	"[un escalve #"
#define MSG_STATUS_CHANGE2	", qui s' appelle" BLUE
#define MSG_STATUS_CHANGE3	NORMAL "a chang� sa humeur a:"

#define MSG_STATUS_HIDE1	"[A Slave #"
#define MSG_STATUS_HIDE2	", called" BLUE
#define MSG_STATUS_HIDE3	NORMAL "hides from you in:"


#define MSG_MESSAGE_SENT1	MAGENTA "YSM Msg Envoy� a escalve appel�" WHITE
#define MSG_MESSAGE_SENT2	MAGENTA "YSM Msg Envoy� a" WHITE

#define MSG_MESSAGE_SENT3	MAGENTA	"Encrypted Msg Sent to Slave called" WHITE
#define MSG_MESSAGE_SENT4	MAGENTA	"Encrypted Msg Sent to" WHITE

#define MSG_INCOMING_MESSAGE	"[Msg entrant]"
#define MSG_INCOMING_URL	"[URL entrant]"
#define MSG_INCOMING_AUTHR	"Demande d'autorisation de "

#define MSG_INCOMING_PAGER	"[Pager Message]"

#define MSG_SLAVES_LIST		"tu as le prochaine"
#define MSG_SLAVES_ONLINE	"Ceux sont les ESCALVES EN LIGNE qui tu as:"

#define MSG_SLAVES_BIRTHDAY	WHITE "BIRTHDAY" NORMAL

#define MSG_WARN_ADDED		"Obtenez Parano�de! un utilisateur d' ICQ t' a ajout� a sa liste!"
#define MSG_WARN_AUTHOK		"Tr�s bien! tu a �t� autoris� pour il devien un/e escalve"
#define MSG_WARN_AUTHDENY	"mERDE!! le fiz de putaine a accept� pas ta demande d' autorisation"

/* YSM AFK RELATED MESSAGES */

#define MSG_IDLE_TIME_REACHED	"IDLE Temp Complet. en allant afk..\n"
#define MSG_AFK_MODE_ON		"You are " BRIGHT_BLUE "a Ce moment je suis  AFK" NORMAL "  (Loin Du Clavier).\nLes msg qui arrive soyent not� et une autoreponse sera envoy� a qui te appel.\ntu peux taper 'afk' pour sortir d' afk mode. appr�cie ton caf� ;)"

#define MSG_AFK_MODE_OFF1	"Bienvenue! tu as �t� loin"
#define MSG_AFK_MODE_OFF2	"minutes!.\n"
#define MSG_AFK_MODE_OFF3	"les nouveau messages ont �t� enregistre dans ton directoire d' ysm "
#define MSG_AFK_MODE_OFF4	"pendant tu as �t� loin.\nUtilise 'readafk' pour lire les msg!.\nen partant" BRIGHT_BLUE " AFK " NORMAL "mode..\n"

#define MSG_AFK_READ_MSG	GREEN "[[ en lisant enregist� Messages ]]" NORMAL

/* ERROR MESSAGES */

#define MSG_ERR_PASSMATCH	CYAN "<<NON!>>" NORMAL " la mot de passe est diferent."
#define MSG_ERR_DISCONNECTED	"d�branch� par le serveur. contr�le ta mot de passe."
#define MSG_ERR_INVPASSWD	RED "serveur a dis: mouvais mot de passe." NORMAL
#define MSG_ERR_INVUIN		RED "serveur a dis: mouvias UIN." NORMAL
#define MSG_ERR_TOOMC		RED "serveur a dis: Beaucoup IPs egals ." NORMAL
#define MSG_ERR_RATE		RED "serveur a dis: Limite de taux d�pass�e.  Essai encore plus tard." NORMAL
#define MSG_ERR_SAMEUIN		CYAN "!PARANOID!" NORMAL " Someone else logged in using this ICQ UIN." 
#define MSG_ERR_CONNECT		"Reliez �chou�!"
#define MSG_ERR_PROXY		"Proxy donnes un ERROR!::"
#define MSG_ERR_PROXY2		"Proxy requires Authorization.\n"
#define MSG_ERR_PROXY3		"Method Failed. Unknown Reason.\n"

#define MSG_ERR_REGISTERING	"serveur invalide pendant Enregistrement. en partant."

#define MSG_ERR_FEATUREDISABLED	"Function  handicap�. Recompile YSM avec Appui De Fils."

#define MSG_CONN_PROXYOK	"Reli� � le PROXY!"
#define MSG_CONN_SERVEROK	"TCP relie avec ICQ serveur"

#define MSG_AOL_WARNING		"AOL Advertance! n' envoyes pas msg tr�s vite!"

#define MSG_REQ_SENT1		"YSM demande a envoy� �"
#define MSG_REQ_SENT2		"escalve appel�"

#define MSG_AUTH_SENT1		"YSM Autorisation a envoy� �"
#define MSG_AUTH_SENT2		"Slave called"

#define MSG_NEW_OFFLINE		"Message re�u quand tu a �t� " BRIGHT_BLUE " off line"

#define MSG_BUDDY_GROUPCREATED	"YSM Groupe a �t� Cr��."
#define MSG_BUDDY_SAVING1	BRIGHT_BLUE "en registrant ["
#define MSG_BUDDY_SAVING2	"]" WHITE " ["
#define MSG_BUDDY_SAVING3	"]" NORMAL	

#define MSG_BUDDY_SAVINGOK	GREEN " [OK] a enregistr� un escalve"
#define MSG_BUDDY_SAVINGERROR	RED " [ERROR]"
#define MSG_BUDDY_SAVINGERROR2	"a ajout� une compte handicap� Peut-�tre?."


#define MSG_BUDDY_SAVINGAUTH	RED " [AUTH]"
#define MSG_BUDDY_SAVINGAUTH2	"Autorisation de le utilizateur est necesaire."
#define MSG_BUDDY_SAVINGAUTH3	" [user le 'req' commande]."

#define MSG_BUDDY_BLOCKWARN	"information de l' escalve a ete re�u."
#define MSG_BUDDY_BLOCKWARN2	"Your input may get blocked for a few seconds."

#define MSG_REG_SUCCESFULL	"Registration Complet. ton nouveau UIN es "
#define MSG_REG_FAILED		RED "UIN Registration failed. This may be due to too many registration requests from the same IP address. Please try again later." NORMAL

#define MSG_INFO_UPDATED	"Information Mis � jour!"

#define MSG_SEARCH_FAILED	"Search Failed. valid UIN/Mail?"

#define MSG_DIRECT_CONNECTING	"Attendes!. en reliant a"
#define MSG_DIRECT_ESTABLISHED	"Raccordement Complet."
#define MSG_DIRECT_ERR1	"li ne peux pas faire un raccordement derri�re le PROXY ."
#define MSG_DIRECT_ERR2	"li ne peux pas faire un raccordement\il ne connu pas l'ip."
#define MSG_DIRECT_ERR3 "li ne peux pas faire un raccordement.\nRaccordement Refus�"
