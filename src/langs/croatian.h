/*	$Id	*/

/*
 * YSM Language Specific header file
 *
 *	CROATIAN LANGUAGE
 *
 * by Fritz [fritz@hush.com]
 *
 */


/* YSM STARTING MESSAGES */

#define MSG_STARTING_ONE	"..." WHITE " iskustvo" NORMAL " koje nikada necete zaboraviti.."
#define MSG_STARTING_TWO	"[ POKRECEM YSM ][ " BLUE "UIN" NORMAL " ZA KORISTITI :"
#define MSG_FOUND_SECPASS	"Pronadena sigurna lozinka. Volim paranoicne korisnike ;)."
#define MSG_FOUND_SECPASS2	"Unesi ponovno [verifikacija]: "
#define MSG_READ_SLAVES		"YSM kolicina korisnika u bazi" BRIGHT_BLUE " ["
#define MSG_ASK_KEYMAP1		"Uobicajne kratice su: "
#define MSG_ASK_KEYMAP2		"Zelite li rekonfigurirat vas key map? [Y/N]:"
#define MSG_NETWORK_INFO	"[Mrezna informacija: TCP lokalni port:"
#define MSG_NETWORK_INFO2	"U redu novi server. Informacija stigla."
#define MSG_NETWORK_INFO3	"Spajam se na 2. BOS server.."
#define MSG_LOGIN_OK		BLUE "Prijavljivanje u redu!" NORMAL " << Dobro dosli u ICQ mrezu! >>"
#define MSG_ASK_DOWNLOAD1	"Kontakti pohranjeni na ICQ serveru su pronadeni."
#define MSG_ASK_DOWNLOAD2	"Zelite li ih ucitati? [Y/N]: "
#define MSG_DOWNLOADED_SLAVES	"ucitanih korisnika koji pripadaju vasoj listi."

/* YSM SLAVES INTERACTION MESSAGES */

#define MSG_STATUS_CHANGE1	"[Korisnik #"
#define MSG_STATUS_CHANGE2	", zvan" BLUE
#define MSG_STATUS_CHANGE3	NORMAL "promjenio raspolozenje u:"

#define MSG_STATUS_HIDE1	"[Korisnik #"
#define MSG_STATUS_HIDE2	", zvan" BLUE
#define MSG_STATUS_HIDE3	NORMAL "skriva se od vas u:"

#define MSG_MESSAGE_SENT1	MAGENTA "YSM poruka poslana korisniku zvanom" WHITE
#define MSG_MESSAGE_SENT2	MAGENTA "YSM poruka poslana" WHITE

#define MSG_MESSAGE_SENT3	MAGENTA	"Kriptirana poruka poslana korisniku" WHITE
#define MSG_MESSAGE_SENT4	MAGENTA	"Kriptirana poruka poslana" WHITE

#define MSG_INCOMING_MESSAGE	"[Dolazi poruka]"
#define MSG_INCOMING_URL	"[Dolazi URL]"
#define MSG_INCOMING_AUTHR	"Dolazi zahtjev za autorizaciju od"

#define MSG_INCOMING_PAGER	"[Poruka sa dojavljivaca]"

#define MSG_SLAVES_LIST		"Posjedujete iduce"
#define MSG_SLAVES_ONLINE	"Trenutno ON-LINE korisnici:"

#define MSG_SLAVES_BIRTHDAY	WHITE "RODENDAN" NORMAL

#define MSG_WARN_ADDED		"Korisnik vas je dodao na listu!!"
#define MSG_WARN_AUTHOK		"Korisnik je odobrio vas zahtjev za autorizaciju"
#define MSG_WARN_AUTHDENY	"Korisnik je odbio vas zahtjev za autorizaciju"

/* YSM AFK RELATED MESSAGES */

#define MSG_IDLE_TIME_REACHED	"Neradno vrijeme isteklo. idem AFK..\n"
#define MSG_AFK_MODE_ON		"" BRIGHT_BLUE "sada u AFK" NORMAL " modu (Udaljeni Od Tipkovnice - AFK).\nPristigle poruke bit ce zapisane i auto-odgovor ce bit poslan\nkorisniku koji vas kontaktira.\nmozete napisat 'afk' ponovno za napustit AFK mod. uzivajte u kavi ;)"

#define MSG_AFK_MODE_OFF1	"Dobro dosli natrag! Bili ste odsutni"
#define MSG_AFK_MODE_OFF2	"minuta!.\n"
#define MSG_AFK_MODE_OFF3	"nove poruke su pohranjene u vas ysm\ndirektorij u datoteci"
#define MSG_AFK_MODE_OFF4	"dok ste bili odsutni.\nKoristite 'readafk' za citanje ili brisanje pohranjenih poruka!.\nNapustam" BRIGHT_BLUE " AFK " NORMAL "mod..\n"

#define MSG_AFK_READ_MSG	GREEN "[[ Citam pohranjenje AFK poruke ]]" NORMAL

/* ERROR MESSAGES */

#define MSG_ERR_PASSMATCH	CYAN "<<NE!>>" NORMAL " Lozinke nisu identicne."
#define MSG_ERR_DISCONNECTED	"Odspojen od servera. Provjeri lozinku."
#define MSG_ERR_INVPASSWD	RED "Server kaze: Kriva lozinka." NORMAL
#define MSG_ERR_INVUIN		RED "Server kaze: Pogresan UIN." NORMAL
#define MSG_ERR_TOOMC		RED "Server kaze: Previse korisnika preko istog IP-a." NORMAL
#define MSG_ERR_RATE		RED "Server said: Rate limit exceeded. Try again later." NORMAL
#define MSG_ERR_SAMEUIN		CYAN "!UPOZORENJE!" NORMAL " Netko je vec logiran koristeci ovaj ICQ UIN." 

#define MSG_ERR_CONNECT		"Neuspjesno spajanje!"
#define MSG_ERR_PROXY		"Proxy vraca gresku!::"
#define MSG_ERR_PROXY2		"Proxy zahtjeva autorizaciju.\n"
#define MSG_ERR_PROXY3		"Metoda neuspjesna. Nepoznat razlog.\n"
#define MSG_ERR_REGISTERING	"Neispravan serverov odgovor prilikom registracije. Napustam."

#define MSG_ERR_FEATUREDISABLED	"Opcija onemogucena. Rekompajlirajte YSM sa Threads podrskom."

#define MSG_CONN_PROXYOK	"Spojen na PROXY!"
#define MSG_CONN_SERVEROK	"TCP veza uspostavljena sa ICQ SERVER-om"

#define MSG_AOL_WARNING		"AOL upozorenje! prestanite slati poruke pre-brzo!"

#define MSG_REQ_SENT1		"YSM zahtjev poslan"
#define MSG_REQ_SENT2		"Korisnik zvan"

#define MSG_AUTH_SENT1		"YSM autorizacija poslana"
#define MSG_AUTH_SENT2		"Korisnik zvan"

#define MSG_NEW_OFFLINE		"Poruka primljena kada ste bili" BRIGHT_BLUE " offline"

#define MSG_BUDDY_GROUPCREATED	"YSM grupa kreirana."
#define MSG_BUDDY_SAVING1	BRIGHT_BLUE "Pohranjivanje ["
#define MSG_BUDDY_SAVING2	"]" WHITE " ["
#define MSG_BUDDY_SAVING3	"]" NORMAL	

#define MSG_BUDDY_SAVINGOK	GREEN " [U REDU] KORISNIK POHRANJEN"
#define MSG_BUDDY_SAVINGERROR	RED " [GRESKA]"
#define MSG_BUDDY_SAVINGERROR2	"Dodavanje onemogucenog korisnika?."


#define MSG_BUDDY_SAVINGAUTH	RED " [AUTH]"
#define MSG_BUDDY_SAVINGAUTH2	"Potrebna je autorizacija od korisnika."
#define MSG_BUDDY_SAVINGAUTH3	" [koristite 'req' naredbu]."

#define MSG_BUDDY_BLOCKWARN	"Primam korisnikov status."
#define MSG_BUDDY_BLOCKWARN2	"Unos mozda bude blokiran na nekoliko sekundi."

#define MSG_REG_SUCCESFULL	"Registracija uspjesna. Tvoj novi UIN je "
#define MSG_REG_FAILED		RED "UIN Registration failed. This may be due to too many registration requests from the same IP address. Please try again later." NORMAL

#define MSG_INFO_UPDATED	"Informacija osvjezena!"

#define MSG_SEARCH_FAILED	"Potraga neuspjesna. Tocan UIN/Mail?"

#define MSG_DIRECT_CONNECTING	"Pricekajte. Spajam se na "
#define MSG_DIRECT_ESTABLISHED	"Uspostavljena veza."
#define MSG_DIRECT_ERR1	"Ne mogu uspostavit direktnu vezu iza proxy-a."
#define MSG_DIRECT_ERR2	"Ne mogu uspostavit direktnu vezu.\nUdaljeni IP ili port je nepoznat."
#define MSG_DIRECT_ERR3 "Ne mogu uspostavit direktnu vezu.\nVeza odbijena"
