/*	$Id: YSM_Network.h,v 1.41 2005/12/26 23:44:58 rad2k Exp $	*/
/*
-======================== ysmICQ client ============================-
		Having fun with a boring Protocol
-======================== YSM_Network.h ============================-

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

#ifndef _YSMNETWORKH_
#define _YSMNETWORKH_

typedef struct {

	int8_t familyID[2];
	int8_t SubTypeID[2];
	int8_t Flags_a;
	int8_t Flags_b;
	u_int32_t ReqID;

} SNAC_Head;

typedef struct {

	int8_t cmd;
	int8_t channelID;
	int8_t seq[2];
	int8_t dlen[2];

} FLAP_Head;

typedef struct
{
        int8_t type[2];
        int8_t len[2];
} TLV;


int32_t YSM_NetworkInit( void );

void
YSM_UpdatePrivacy(
	int	Setting
	);

int YSM_LookupHN( char *Hostname, u_int32_t *out );

int32_t
YSM_LoginSequence( uin_t uin, int8_t *password );

void
YSM_Init_LoginA(
	uin_t		Uin,
	u_int8_t	*Password
	);

void
YSM_Init_LoginB(
	FLAP_Head	*,
	int8_t	*,
	int32_t
	);

void
YSM_Init_LoginC(
	TLV	thetlv,
	char	*buff
	);

void
YSM_SendCapabilities(
	void
	);

void
YSM_RequestContacts(
	void
	);

void
YSM_RequestOffline(
	void
	);

void
YSM_RequestAutoMessage(
	YSM_SLAVE *victim
	);

void
YSM_AckOffline(
	void
	);

void
YSM_SendCliReady(
	void
	);

void
YSM_RequestICBMRights(
	void
	);

void
YSM_SendICBM(
	void
	);

void
YSM_RequestBuddyRights(
	void
	);

void
YSM_RequestRates(
	void
	);

void
YSM_IncomingMultiUse(
	FLAP_Head	*head,
	SNAC_Head	*thesnac,
	char		*buf
	);

void
YSM_BuddyIncomingList(
	FLAP_Head	*head,
	SNAC_Head	thesnac,
	char		*buf,
	int		buf_len
	);

void
YSM_BuddyIncomingChange(
	FLAP_Head	*head,
	SNAC_Head	thesnac,
	char		*buf
	);
void
YSM_Incoming_ClientAck( 
		FLAP_Head *flap,
		SNAC_Head	*snac,
		int8_t		*buf 
		);
int
YSM_Incoming_SNAC(
	FLAP_Head *,
	char *,
	int
	);

void
YSM_ReceiveMessage(
	FLAP_Head	*,
	SNAC_Head	*,
	char		*data
	);

int32_t
YSM_ReceiveMessageData( YSM_SLAVE	*victim,
			int8_t		*r_uin,
			int16_t		r_status,
			int8_t		m_type,
			u_int8_t	m_flags,
			int16_t		m_len,
			int8_t		*m_data );


void
YSM_ReceiveMessageType1( YSM_SLAVE	*victim,
		int32_t		tsize,
		u_int8_t	*data,
		int8_t		*r_uin,
		int8_t		*r_status,
		int8_t		*m_id );

void
YSM_ReceiveMessageType4( YSM_SLAVE	*victim,
		int32_t		tsize,
		u_int8_t	*data,
		int8_t		*r_uin,
		int8_t		*r_status );

int32_t
YSM_ReceiveMessageType2Common( YSM_SLAVE	*victim,
		int32_t		 tsize,
		u_int8_t	*data,
		int8_t		*r_uin,
		int8_t		*r_status,
		int8_t		*m_id,		
		u_int8_t	m_flags,
		u_int16_t	*pseq,
		int8_t		dc_flag );

void
YSM_ReceiveMessageType2( YSM_SLAVE	*victim,
	int32_t		tsize,
	u_int8_t	*data,
	int8_t		*r_uin,
	int8_t		*r_status,
	int8_t		*msgid );

void
YSM_SendACKType2( int8_t	*r_uin,
		int8_t		*pseq,
		int8_t		m_type,
		int8_t		*m_id );

void
YSM_SendContact(
	YSM_SLAVE	*victim,
	char		*datalist,
	char		*am 
	);


void
YSM_SendUrl( 
	YSM_SLAVE	*victim,
	int8_t		*url,
	int8_t		*desc 
	);

void
YSM_ForwardMessage(
	uin_t		_rUIN,
	char		*_msg
	);


void
YSM_IncomingPersonal(
	FLAP_Head	*head,
	SNAC_Head	*thesnac,
	int8_t		*buf
	);

void
YSM_RequestPersonal(
	void
	);

void
YSM_BuddyChangeStatus(
	FLAP_Head	*flap,
	SNAC_Head	*snac,
	int8_t		*data );

void
YSM_BuddyParseStatus( YSM_SLAVE			*victim,	/* IN */
		FLAP_Head			*flap,
		SNAC_Head			*snac,
		int8_t				*data,
		int32_t				pos,
		struct YSM_DIRECT_CONNECTION	*dcinfo,
		u_int32_t			*fprint,
		u_int16_t			*status,
		u_int16_t			*flags, 
		time_t				*onsince );

void
YSM_BuddyUpdateStatus( YSM_SLAVE		*victim,
		struct YSM_DIRECT_CONNECTION	*dcinfo,	
		u_int16_t			status,
		u_int16_t			flags,
		u_int32_t			fprint,
		time_t				onsince );

		
int32_t
YSM_SendSNAC( int	Family,
	int		Subtype,
	int8_t		FlA,
	int8_t		FlB,
	const char	*data,
	u_int32_t	size,
	int32_t		lseq,
	char		*reqid );

void
YSM_IncomingInfo( 
		char	type,
		char	*buf,
		int		tsize,
		unsigned int	reqid
		);

void
YSM_IncomingMainInfo( int8_t	*buf,
		int32_t		tsize,
		int8_t		*pnick,
		int8_t		*pfirst,
		int8_t		*plast,	
		int8_t		*pemail,
		u_int32_t	reqid,
		uin_t		*puin 
		);

void
YSM_IncomingHPInfo( int8_t	*buf,
		int32_t		tsize
		);

void
YSM_IncomingWorkInfo(
	int8_t	*buf,
	int32_t	tsize 
	);

void
YSM_IncomingAboutInfo( 
	int8_t	*buf,
	int32_t tsize
	);

void
YSM_IncomingSearch(
	char	*buf,
	int	tsize
	);


void
YSM_BuddyAck(
	FLAP_Head	*,
	SNAC_Head	thesnac,
	char		*buf
);

void
YSM_BuddyRequestModify(
	void
	);

void
YSM_BuddyRequestFinished(
	void
	);

int
YSM_BuddyReadSlave(
	char	*buf,
	int	tsize
	);

void
YSM_Incoming_Scan(
	SNAC_Head	*thesnac
	);


int32_t
YSM_SendMessage2Client( YSM_SLAVE	*victim,
		uin_t		r_uin,
		int16_t		m_format,
		int32_t		m_type,
		int8_t		*m_data,
		int32_t		m_len,
		u_int8_t	m_flags,
		u_int8_t	sendflags,
		int32_t		reqid );

int32_t
YSM_BuildMessageHead( uin_t	r_uin,
		int16_t		m_format,
		u_int32_t	m_time,
		u_int32_t	m_id,
		int8_t		**phead );

int32_t
YSM_BuildMessageBodyType1( uin_t	r_uin,
		int8_t		*m_data,
		int32_t		m_len,
		int8_t		**pbody );

int32_t
YSM_BuildMessageBodyType2( YSM_SLAVE	*victim,
		uin_t		r_uin,
		int8_t		*m_data,
		int32_t		m_len,
		int32_t		m_type,
		u_int32_t	m_time,
		u_int32_t	m_id,
		u_int8_t	m_flags,
		u_int8_t	sendflags,
		int8_t		**pbody );

int32_t
YSM_BuildMessageBodyType4( uin_t	r_uin,
		int8_t		*m_data,
		int32_t		m_len,
		int32_t		m_type,
		u_int8_t	m_flags,
		int8_t		**pbody );

void YSM_SendContacts( void );
void YSM_BuddyDelSlave( YSM_SLAVE *poorone );
void YSM_SrvResponse( void );

int32_t YSM_ChangeStatus( u_int16_t status );

int InsertTLV(void *string, int type, void *memplace, int len);

int32_t
YSM_BuddyAddItem( YSM_SLAVE	*item,
		int8_t		*grpname,
		u_int32_t	grpid,
		u_int32_t	bID,
		u_int32_t	type,
		u_int32_t	cmd,
		u_int32_t	authawait,
		u_int32_t	add_update);

int32_t
YSM_Connect( int8_t *host, u_int16_t port, int8_t verbose );

int32_t
YSM_RawConnect(	int8_t *host, u_int16_t port );

int32_t
YSM_ProxyHalfConnect( int8_t	*host,
		u_int16_t	port,
		struct in_addr	*outaddress );

int32_t
YSM_ProxyConnect( int8_t *host, u_int16_t port );

int32_t
YSM_ConnectOld( char	*srv_host,
	u_int32_t	srv_ip,
	u_int16_t	srv_port,
	int32_t		USE_PROXY,
	int8_t		*proxy_method,
	int32_t		v );

void YSM_KeepAlive( void );

void DumpPacket( FLAP_Head *flap, int8_t *data );

void YSM_RemoveContact( YSM_SLAVE *Contact );
void YSM_SendAuthRequest( uin_t UIN, char *Nick, char *Message );
void YSM_SendAuthOK( uin_t UIN, char *Nick );
void YSM_InfoChange( int desired, char *newsetting );
int32_t YSM_RequestInfo( uin_t r_uin, int16_t subtype );
void YSM_SearchUINbyMail( char *ContactMail );

void YSM_BuddyUploadList (YSM_SLAVE *refugee);
void YSM_BuddyInvisible( YSM_SLAVE *buddy, int flag );
void YSM_BuddyVisible( YSM_SLAVE *buddy, int flag );
void YSM_BuddyIgnore( YSM_SLAVE *buddy, int flag );

void YSM_ChangePassword( char *newp );
void YSM_War_Kill( YSM_SLAVE *victim );
void YSM_War_Scan( YSM_SLAVE *victim );
void YSM_SendRTF( YSM_SLAVE *victim );
#endif

