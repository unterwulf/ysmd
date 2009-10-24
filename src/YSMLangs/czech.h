/*	$Id: czech.h,v 1.2 2005/08/16 00:28:08 rad2k Exp $	*//* * YSM Language Specific header file * *	CZECH LANGUAGE (iso-8859-2 bez diakritiky) * * by Albert (256340948) * *//* YSM STARTING MESSAGES */#define MSG_STARTING_ONE	"..." WHITE " zkusenost" NORMAL " nikdy nezapomenes.."#define MSG_STARTING_TWO	"[ SPOUSTIM YSM ICQ klienta ][ " BLUE "UIN" NORMAL " POUZIVAM :"#define MSG_FOUND_SECPASS	"Nalezeno zabezpeceni heslem. Miluju paranoidni uzivatele ;)."#define MSG_FOUND_SECPASS2	"Napiste znovu [overeni]: "#define MSG_READ_SLAVES		"YSM mnozstvi OTROKU v bazi" BRIGHT_BLUE " ["#define MSG_ASK_KEYMAP1		"Prednastavene zkratky jsou: "#define MSG_ASK_KEYMAP2		"Chcete prenastavit svou mapu klaves? [Y/N]:"#define MSG_NETWORK_INFO	"[Sitova informace: TCP lokalni port:"#define MSG_NETWORK_INFO2	"Novy server. Prisly informace."#define MSG_NETWORK_INFO3	"Pripojuji se na 2. BOS server.."#define MSG_LOGIN_OK		BLUE "Prihlaseni OK!" NORMAL " << Vitejte do site ICQ, sire >>"#define MSG_ASK_DOWNLOAD1	"Byly nalezeny dostupne kontakty ulozene na ICQ serveru."#define MSG_ASK_DOWNLOAD2	"Prejete si je stahnout? [Y/N]: "#define MSG_DOWNLOADED_SLAVES	"stazeni OTROCI ted patri do vaseho kralovstvi."/* YSM SLAVES INTERACTION MESSAGES */#define MSG_STATUS_CHANGE1	"[OTROK #"#define MSG_STATUS_CHANGE2	", receny" BLUE#define MSG_STATUS_CHANGE3	NORMAL "zmenil naladu na:"#define MSG_STATUS_HIDE1	"[OTROK #"#define MSG_STATUS_HIDE2	", receny" BLUE#define MSG_STATUS_HIDE3	NORMAL "se vam schoval v:"#define MSG_MESSAGE_SENT1	MAGENTA "YSM zprava poslana OTROKOVI zvanemu" WHITE#define MSG_MESSAGE_SENT2	MAGENTA "YSM zprava poslana" WHITE#define MSG_MESSAGE_SENT3	MAGENTA	"Sifrovana zprava poslana OTROKOVI zvanemu" WHITE#define MSG_MESSAGE_SENT4	MAGENTA	"Sifrovana zprava poslana" WHITE#define MSG_INCOMING_MESSAGE	"[Prichozi zprava]"#define MSG_INCOMING_URL	"[Prichozi URL]"#define MSG_INCOMING_AUTHR	"Prichozi autorizacni pozadavek od"#define MSG_INCOMING_PAGER	"[Pager zprava]"#define MSG_SLAVES_LIST		"Vlastnite nasledujici OTROKY:"#define MSG_SLAVES_ONLINE	"Tohle jsou vasi PRAVE pracujici OTROCI:"#define MSG_SLAVES_BIRTHDAY	WHITE "Narozeniny" NORMAL#define MSG_WARN_ADDED		"Stante se paranoidnim vladcem! Nejaky ICQ uzivatel si vas pridal do sveho adresare! "#define MSG_WARN_AUTHOK		"Hotovo! Byli jste autorizovani udelat z nej/ni sveho ICQ OTROKA"#define MSG_WARN_AUTHDENY	"K***a! Ten bastard -ODMITL- vasi autentizacni zadost.\nVyhlaste mu valku!"/* YSM AFK RELATED MESSAGES */#define MSG_IDLE_TIME_REACHED	"Cas ubehl, prechazim do spaciho modu (afk)..\n"#define MSG_AFK_MODE_ON		"Jste " BRIGHT_BLUE "prave ve spacim/tvrde pracujicim (AFK)" NORMAL " modu (Away From Keyboard).\nPrichozi zpravy budou zapsany a automaticka odpoved bude zaslana komukoli,\nkdo vam neco posle.\nMuzete napsat 'afk' znova, cimz opustite spaci (AFK) mod.\nUzijte si kafe ;)"#define MSG_AFK_MODE_OFF1	"Vitejte zpatky! Byl jste pryc"#define MSG_AFK_MODE_OFF2	"minut!.\n"#define MSG_AFK_MODE_OFF3	"nove zpravy byly ulozeny ve vasem ysm adresari v souboru"#define MSG_AFK_MODE_OFF4	"zatimco jste byl pryc.\nPouzijte 'readafk' k precteni nebo vycisteni ulozenych zprav!\nOpoustim" BRIGHT_BLUE " AFK " NORMAL "mod..\n"#define MSG_AFK_READ_MSG	GREEN "[[ Cteni ulozenych zprav ]]" NORMAL/* ERROR MESSAGES */#define MSG_ERR_PASSMATCH	CYAN "<<Nic nebude!>>" NORMAL " Heslo NESOUHLASI!."#define MSG_ERR_DISCONNECTED	"Odpojeno serverem. Prekontrolujte si heslo."#define MSG_ERR_INVPASSWD	RED "Server rekl: SPATNE HESLO." NORMAL#define MSG_ERR_INVUIN		RED "Server rekl: NEPLATNE UIN." NORMAL#define MSG_ERR_TOOMC		RED "Server rekl: Prilis mnoho klientu ze stejne IP." NORMAL#define MSG_ERR_RATE		RED "Server rekl: Rate limit prekrocen. Zkuste pozdeji." NORMAL#define MSG_ERR_SAMEUIN		CYAN "!PARANOIA!" NORMAL " Nekdo jiny se prihlasil pod stejnym ICQ UIN." #define MSG_ERR_CONNECT		"Spojeni neuspelo!"#define MSG_ERR_PROXY		"Proxy vratil CHYBU!::"#define MSG_ERR_PROXY2		"Proxy pozaduje autorizaci.\n"#define MSG_ERR_PROXY3		"Metoda selhala. Duvod? Neznamy.\n"#define MSG_ERR_REGISTERING	"Neplatna odpoved serveru pri registrovani. Opoustim..."#define MSG_ERR_FEATUREDISABLED	"Ficurka zakazana. Rekompilujte YSM s podporou vlaken :)"#define MSG_CONN_PROXYOK	"Pripojeno k PROXY!"#define MSG_CONN_SERVEROK	"TCP spojeni s ICQ serverem ustaveno "#define MSG_AOL_WARNING		"AOL warowani! Prestante posilat zpravy tak rychle!"#define MSG_REQ_SENT1		"YSM pozadavek zaslan"#define MSG_REQ_SENT2		"OTROKU zvanemu"#define MSG_AUTH_SENT1		"YSM autorizace zaslana"#define MSG_AUTH_SENT2		"OTROKU zvanemu"#define MSG_NEW_OFFLINE		"Dosla zprava, kdyz jste byl" BRIGHT_BLUE " nepripojen"#define MSG_BUDDY_GROUPCREATED	"YSM skupina zalozena."#define MSG_BUDDY_SAVING1	BRIGHT_BLUE "Ukladam ["#define MSG_BUDDY_SAVING2	"]" WHITE " ["#define MSG_BUDDY_SAVING3	"]" NORMAL	#define MSG_BUDDY_SAVINGOK	GREEN " [OK] OTROK ulozen"#define MSG_BUDDY_SAVINGERROR	RED " [CHYBA]"#define MSG_BUDDY_SAVINGERROR2	"Mozna pridavate zruseny ucet?"#define MSG_BUDDY_SAVINGAUTH	RED " [AUTH]"#define MSG_BUDDY_SAVINGAUTH2	"Potrebujete autorizaci od OTROKA"#define MSG_BUDDY_SAVINGAUTH3	" [pouzijte prikaz 'req']."#define MSG_BUDDY_BLOCKWARN	"Ziskavam informace o stavu OTROKA."#define MSG_BUDDY_BLOCKWARN2	"Vas vstup se muze na par sekund zablokovat."#define MSG_REG_SUCCESFULL	"Registrace uspesna. Vase nove UIN je "#define MSG_REG_FAILED		RED "UIN Registration failed. This may be due to too many registration requests from the same IP address. Please try again later." NORMAL#define MSG_INFO_UPDATED	"Informace doplneny!"#define MSG_SEARCH_FAILED	"Hledani selhalo. Zkousite platne UIN/email?"#define MSG_DIRECT_CONNECTING	"Poseckejte. Pripojuji se k "#define MSG_DIRECT_ESTABLISHED	"Spojeni vytvoreno."#define MSG_DIRECT_ERR1	"Nemohu vytvorit prime spojeni zpoza proxy."#define MSG_DIRECT_ERR2	"Nemohu vytvorit prime spojeni.\nBud vzdalena IP nebo naslouvhajici port jsou nezname."#define MSG_DIRECT_ERR3 " Nemohu vytvorit prime spojeni.\nSpojeni odmitnuto"