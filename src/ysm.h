/*	$Id: YSM.h,v 1.151 2007/01/29 19:15:29 rad2k Exp $	*/
/*
-======================== ysmICQ client ============================-
		Having fun with a boring Protocol
-============================ YSM.h ================================-

YSM (YouSickMe) ICQ Client. An Original Multi-Platform ICQ client.
Copyright (C) 2002 rad2k Argentina.

YSM is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

For Contact information read the AUTHORS file.

*/

#ifndef _YSMH_
#define _YSMH_

#include "YSM_Config.h"
#include "YSM_Compat.h"

#define YSM_INFORMATION		"ysmICQ - Bringing ICQ back to life - "  
#define YSM_INFORMATION2	"2.9.9.1"

#ifdef WIN32
#define YSM_CFGDIRECTORY	"ysm"
#define YSM_PROXYKEY		"Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings"
#define YSM_PROXYVALUE		"ProxyServer"
#else
#define YSM_CFGDIRECTORY	".ysm"
#endif
#define YSM_CFGFILENAME		"ysm-cfg"
#define YSM_INCOMINGDIRECTORY	"incoming"
#define YSM_SOUNDSDIRECTORY	"sounds"
#define YSM_DEFAULTSRV		"login.icq.com"
#define YSM_DEFAULTPORT		5190
#define YSM_KEEPALIVETIME	90
#define YSM_COMMANDSTIME	15

#ifndef WIN32
#define MAX_PATH		256
#endif

#define MAX_SEQS_FILED		50
#define MAX_DATA_LEN		2547
#define MAX_MSGDATA_LEN		MAX_DATA_LEN
#define MAX_PEER_DATA_SIZE	2050
#define MAX_PEER_FILENAMEL	50
#define MAX_CRYPT_PADDING	16
#define MAX_PWD_LEN		12
#define MAX_NICK_LEN		30
#define MAX_UIN_LEN		12
#define MAX_CMD_NAME		15
#define MAX_CMD_ARGS		MAX_DATA_LEN+2
#define	MAX_EXEC_ARGS		50
#define MAX_CMD_LEN		MAX_DATA_LEN
#define MAX_CMD_HIST		15
#define MAX_LINE_LEN		80
#define MAX_LOGSEP_LEN		5
#define MAX_STATUS_LEN		50
#define MAX_CONTACTS_SEND	15
#define MAX_CHARSET		10
#define MAX_SAVE_COUNT		5
#define MAX_SCAN_COUNT		2
#define MAX_TIME_LEN		21	/* dd mmm yyyy hh:mm:ss + NUL */
#define MAX_SLAVELIST_LEN	8192

/* Required for log files parsing watch out for MAX_LOGSEP_LEN  */
#define YSM_LOG_SEPARATOR	"$YSM$"

#define YSM_VERSIONCHECK_HOST	"ysmv7.sourceforge.net"
#define YSM_VERSIONCHECK_FILE	"lastversion.dat"

/* AFK definitions */
#define YSM_AFKFILENAME		"afk-log"
#define YSM_AFK_MESSAGE		"I'm AFK (away from keyboard) right now. \
Your message has been saved. I'll be back shortly! :)"
/* Seconds to wait before re-sending an afk message (skip flooding) */
#define MINIMUM_AFK_WAIT	30

#define YSM_CHAT_MESSAGE	"I'm busy in a conversation. Your message has been logged. I'll get back to you ASAP."

/* Authorization Request definitions */

#define YSM_DEFAULT_AUTHREQ_MESSAGE	"Hi. I require your authorization in order to add you to my contacts list."


#define YSM_BUDDY_GROUPNAME	"YSM"
#define YSM_BUDDY_GROUPID	0x1337

#define YSM_BUDDY_SLAVE		0x0000
#define YSM_BUDDY_SLAVE_VIS	0x0002
#define YSM_BUDDY_SLAVE_INV	0x0003
#define YSM_BUDDY_SLAVE_IGN	0x000e

#define YSM_BUDDY_GROUP		0x0001


/* Debug Level Definitions! */
#define VERBOSE_BASE		0
#define VERBOSE_STCHANGE	1
#define VERBOSE_CONNINFO	2
#define VERBOSE_DCON		20
#define VERBOSE_MOREDATA	21
#define VERBOSE_PACKET		22
#define VERBOSE_SDOWNLOAD	23


#define FINGERPRINT_YSM_CLIENT			0xabffffff
#define FINGERPRINT_YSM_CLIENT_CRYPT		0xdeadbabe
#define FINGERPRINT_TRILLIAN_CLIENT		0x3b75ac09
#define FINGERPRINT_LIB2K_CLIENT		0x3aa773ee
#define FINGERPRINT_MICQ2003A_CLIENT_1		0x3CF02D22
#define FINGERPRINT_MICQ2003A_CLIENT_2		0x3E76502C
#define FINGERPRINT_LICQ_CLIENT			0x7d000000
#define FINGERPRINT_MICQ_CLIENT			0xffffff42
#define FINGERPRINT_MIRANDA_CLIENT		0xffffffff
#define FINGERPRINT_STRICQ_CLIENT		0xffffff8f

/* the following clients are identified by capabilities */
#define FINGERPRINT_SIMICQ_CLIENT		0x04040404
#define FINGERPRINT_M2000_CLIENT		0x06060606
#define FINGERPRINT_M20012_CLIENT		0x07070707
#define FINGERPRINT_M2002_CLIENT		0x08080808
#define FINGERPRINT_MICQLITE_CLIENT		0x09090909 
#define FINGERPRINT_ICQ2GO_CLIENT		0x0a0a0a0a
#define FINGERPRINT_MISC_CLIENT			0x00

#define SLAVES_TAG	"SLAVES"

#define STATUS_ONLINE		0x0000
#define STATUS_OFFLINE		0xffff
#define STATUS_INVISIBLE	0x0100
#define STATUS_NA		0x0005
#define STATUS_NA2		0x0004
#define STATUS_DND		0x0013
#define STATUS_OCCUPIED		0x0011
#define STATUS_FREE_CHAT	0x0020
#define STATUS_AWAY		0x0001
#define STATUS_AFK		0xfed0
#define	STATUS_UNKNOWN		0xdead

#define STATUS_FLWEBAWARE	0x0001
#define STATUS_FLIPSHOW		0x0002
#define STATUS_FLBIRTHDAY	0x0008
#define STATUS_FLDC_AUTH	0x1000
#define STATUS_FLDC_CONT	0x2000

/* Definition of Queries */
#define SLAVE_NAME		0x01
#define SLAVE_UIN		0x02
#define SLAVE_REQID		0x03

#define INFO_MAIN		0x01
#define INFO_HP			0x02
#define INFO_WORK		0x03
#define INFO_ABOUT		0x04

#define UPDATE_SLAVE		0x00
#define UPDATE_NICK		0x01

#define YSM_INFO_NICK		0x00
#define YSM_INFO_EMAIL		0x01

/* Definition of Message Types for Displaying functions */
#define YSM_MESSAGE_NORMAL	0x01
#define YSM_MESSAGE_FILE	0x03
#define YSM_MESSAGE_URL		0x04
#define YSM_MESSAGE_AUTH	0x06
#define YSM_MESSAGE_AUTHNOT	0x07
#define YSM_MESSAGE_AUTHOK	0x08
#define YSM_MESSAGE_ADDED	0x0c
#define YSM_MESSAGE_PAGER	0x0d
#define YSM_MESSAGE_CONTACTS	0x13
#define YSM_MESSAGE_GREET	0x1a
#define YSM_MESSAGE_GETAWAY	0xe8
#define YSM_MESSAGE_GETOCC	0xe9
#define YSM_MESSAGE_GETNA	0xea
#define YSM_MESSAGE_GETDND	0xeb
#define YSM_MESSAGE_GETFFC	0xec


/* Definition of ERROR standards */
#define ERROR_CODE		0
#define ERROR_CODE_M		"There has been a code ERROR. Debug!.\n"
#define ERROR_NETWORK		1
#define ERROR_NETWORK_M		"There has been a Network Disconnection!.\n"
#define ERROR_CRITICAL		2
#define ERROR_CRITICAL_M	"There has been a CRITICAL ERROR. duck!\n"

typedef int32_t	uin_t;

struct COMMAND
{
	int8_t		*cmd_name;
	int8_t		*cmd_alias;
	int8_t		*cmd_help;
	u_int16_t	cmd_groupid;
	u_int16_t	cmd_margs;
	void		(*cmd_func)(int32_t argc, int8_t **argv);

	/* list related */
	struct COMMAND	*next;
	struct COMMAND	*back;
};

struct YSM_TIMING_INFO
{
	time_t	LastMessage;
 	time_t	Signon;
 	time_t	StatusChange;
};

struct YSM_MAIN_INFO 
{
	char	NickName[MAX_NICK_LEN+1];
	char	FirstName[MAX_NICK_LEN+1];
	char	LastName[MAX_NICK_LEN+1];
	char	email[MAX_NICK_LEN+1];
	char	city[MAX_NICK_LEN+1];
	char	state[MAX_NICK_LEN+1];
	char	gender;
	short	age;	
};

struct YSM_NETWORK_CONNECTION
{
	int32_t		rSocket;
	int8_t		auth_host[MAX_PATH];
	int8_t		cookie_host[MAX_PATH];
	u_int16_t	auth_port;
	u_int16_t	cookie_port;
};

struct YSM_DIRECT_CONNECTION_FILETINFO
{
	u_int32_t	size;
	u_int32_t	totsize;
	int8_t		name[MAX_PEER_FILENAMEL];
	u_int16_t	num;
	u_int16_t	totnum;
	u_int16_t	rPort;	
	int32_t		speed;
	int32_t		kbs;
	time_t		statstime;
	int32_t		statsbytes;
	int32_t		rSocket;
	u_int16_t	lPort;
#define DC_TRANSFERTIMEOUT	30	/* seconds */
	FILE		*fd;
};

struct YSM_DIRECT_CONNECTION
{
	u_int16_t				seq_in;
	u_int16_t				seq_out;
	int32_t					rSocket;
	u_int16_t				rPort;
	u_int32_t				rIP_int;
	u_int32_t				rIP_ext;	
	u_int16_t				version;
	int32_t					rCookie;
#define DC_EXPECTDATA	0x01
#define DC_EXPECTNEG	0x02
#define DC_OUTGOINGNEG	0x04
#define DC_INCOMINGNEG	0x08
#define DC_CONNECTED	0x10
#define DC_TRANSFERING	0x20
#define DC_RECEIVING	0x40
#define DC_ACTIVITY	0x80

	u_int8_t				flags;
	struct YSM_DIRECT_CONNECTION_FILETINFO	finfo; 
};

struct YSM_SPECIAL_STATUS
{
	int8_t	birthday;
	int32_t	IgnoreID;
	int32_t	VisibleID;
	int32_t	InvisibleID;
	int32_t	BudID;
	int32_t	grpID;
};

struct YSM_PROXY_INFO
{
	int8_t		username[MAX_PATH];
	int8_t		password[MAX_PATH];
	int8_t		proxy_host[MAX_PATH];
	u_int16_t	proxy_port;
#define YSM_PROXY_HTTPS		0x01
#define YSM_PROXY_AUTH		0x02
#define YSM_PROXY_RESOLVE	0x04
	u_int8_t	proxy_flags;
};

struct	YSM_PROMPTSTATUS
{
#define FL_OVERWRITTEN	0x01
#define FL_REDRAW	0x02
#define FL_BUSYDISPLAY	0x04
#define FL_COMFORTABLEM	0x08
#define FL_AFKM		0x10
#define FL_CHATM	0x20	
#define FL_AUTOAWAY	0x40
	u_int8_t	flags;
};

struct	YSM_SERVERINFORMATION
{
	u_int16_t	seqnum;
	u_int32_t	onlineslaves;

	/* queued slave scans */
	int32_t		scanqueue;

	/* buddy list information */
	int32_t		blentries;
	int32_t		blgroupsidentries;
	int8_t		*blgroupsid;
	int32_t		blusersidentries;
	int8_t		*blusersid;
	u_int32_t	blysmgroupid;
	u_int16_t	blprivacygroupid;

#define FL_LOGGEDIN		0x01
#define FL_DOWNLOADEDSLAVES	0x02
	u_int8_t	flags;
};

struct YSM_FISH_GUI
{
	int32_t		socket;
	fd_set		fish_fds;
	int32_t		max_fish_fd;
	u_int16_t	port;
	u_int8_t	hide_console;
};

struct ENCRYPTION_INFO
{
#define MAX_KEY_LEN	64 /* 512 bits */
	int8_t		strkey[MAX_KEY_LEN+1];
	keyInstance	key_out;
	keyInstance	key_in;
};

struct SLAVE 
{
	int8_t				*Color;
	uin_t				Uin;
	u_int32_t			fprint;
	u_int8_t			caps;
	u_int32_t			ReqID;
	u_int16_t			status;
	u_int16_t			status_flags;
	int8_t				DownloadedFlag;
#define FL_SCANNED	0x01
#define FL_LOG		0x02
#define FL_ALERT	0x04
#define FL_CHAT		0x08
	u_int8_t			flags;
	time_t				LastAFK;
	struct YSM_SPECIAL_STATUS	BudType;
	struct YSM_TIMING_INFO		timing;
	struct YSM_DIRECT_CONNECTION	d_con;
	struct YSM_MAIN_INFO		info;
	struct ENCRYPTION_INFO		crypto;

	/* list related */
	struct SLAVE			*next;	
	struct SLAVE			*back;	
};

struct YSM_MODEL
{ 
        uin_t				Uin;
        u_int16_t			status;
	u_int16_t			status_flags;
        int8_t				password[MAX_PWD_LEN];
	u_int8_t			flags;
	time_t				delta;
	struct YSM_MAIN_INFO		info;
	struct YSM_TIMING_INFO		timing;
	struct YSM_PROXY_INFO		proxy;
	struct YSM_DIRECT_CONNECTION	d_con;
	struct YSM_NETWORK_CONNECTION	network;
	struct YSM_FISH_GUI		fishgui;
};

typedef struct _FileMap {
	/* list related */
	struct _FileMap	*next;	
	struct _FileMap	*back;	

        FILE		*fd;
        int32_t		pos;
        int32_t		size;
        int8_t		*data;
} FileMap, *pFileMap;

typedef struct SLAVE YSM_SLAVE;
typedef struct COMMAND YSM_COMMAND;


/* exports */
extern struct	YSM_MODEL 		YSM_USER;
extern struct	YSM_SERVERINFORMATION	g_sinfo;
extern struct	YSM_PROMPTSTATUS	g_promptstatus;
extern short	YSM_Redraw_Console;
extern short	YSM_SETTING_LOGALL;
extern short	YSM_SETTING_NEWLOGSFIRST;
extern short	YSM_SETTING_VERBOSE;
extern short	YSM_SETTING_VERSION_CHECK;
extern short	YSM_SETTING_BEEP;
extern short	YSM_SETTING_SOUNDS;
extern short	YSM_SETTING_AFKMAXSHOWN;
extern short	YSM_SETTING_AFKMINIMUMWAIT;
extern short	YSM_SETTING_AWAYTIME;
extern short	YSM_SETTING_SPOOF;
extern short	YSM_SETTING_WINALERT;
extern short	YSM_SETTING_ANTISOCIAL;
extern short	YSM_SETTING_UPDATENICKS;
extern short 	YSM_SETTING_DCDISABLE;
extern short 	YSM_SETTING_DCLAN;
extern u_int16_t YSM_SETTING_DCPORT1;
extern u_int16_t YSM_SETTING_DCPORT2;
extern char	YSM_SETTING_HOT_KEY_MAXIMIZE;
extern int32_t	YSM_SETTING_FORWARD;
extern char	YSM_SETTING_CHARSET_TRANS[MAX_CHARSET + 4];
extern char	YSM_SETTING_CHARSET_LOCAL[MAX_CHARSET + 4];
extern int8_t	*YSM_SETTING_COLOR_MESSAGE;
extern int8_t	*YSM_SETTING_COLOR_TEXT;
extern int8_t	*YSM_SETTING_COLOR_STATUSCHANGENAME;
extern int8_t	*YSM_SETTING_COLOR_STATUSCHANGESTATUS;
extern int8_t	*YSM_SETTING_COLOR_TEXT2; 
#ifdef YSM_TRACE_MEMLEAK
extern int unfreed_blocks;
#endif

#endif // _YSMH_
