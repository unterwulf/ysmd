/*	$Id: YSM_Direct.h,v 1.20 2003/06/16 19:39:53 rad2k Exp $	*/
/*
-======================== ysmICQ client ============================-
		Having fun with a boring Protocol
-========================= YSM_Direct.h ============================-

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

#ifndef _YSMDIRECTH_
#define _YSMDIRECTH_

#define MFLAGTYPE_NORM		0x01
#define MFLAGTYPE_ACK		0x02
#define MFLAGTYPE_END		0x04
#define MFLAGTYPE_CRYPTNORM	0x08
#define MFLAGTYPE_CRYPTACK	0x10
#define MFLAGTYPE_UTF8		0x20
#define MFLAGTYPE_UCS2BE	0x40
#define MFLAGTYPE_RTF		0x80

int
YSM_EncryptDCPacket(
	unsigned char	*buf,
	int		buf_len 
	);

int
YSM_DecryptDCPacket(
	unsigned char	*buf,
	unsigned int	buf_len
	);

void YSM_OpenDC( YSM_SLAVE *victim );
void YSM_CloseDC( YSM_SLAVE *victim );
void YSM_CloseTransfer( YSM_SLAVE *victim );

void YSM_DC_Select( void );
int YSM_DC_ReadPacket( int cli_sock, char *buf );
YSM_SLAVE * YSM_DC_Wait4Client( void );

int32_t YSM_DC_CommonResponse( YSM_SLAVE *victim, int32_t sock );

int32_t
YSM_DC_CommonResponseFile( YSM_SLAVE	*victim,
				int32_t		sock,
				u_int8_t	*buf,
				int32_t		rlen );

int32_t
YSM_DC_CommonResponseMisc( YSM_SLAVE	*victim,
				int32_t		sock,
				u_int8_t	*buf,
				int32_t		rlen );

int32_t
YSM_DC_CommonResponseNeg( YSM_SLAVE	*victim,
				int32_t		sock,
				u_int8_t	*buf,
				int32_t		len );


int32_t
YSM_DC_Message( YSM_SLAVE	*victim,
		char		*_msg,
		int		dlen,
		int8_t		type );

int32_t
YSM_DC_MessageACK( YSM_SLAVE	*victim,
		int8_t		type );

int32_t
YSM_DC_IncomingMessageFILE( YSM_SLAVE *victim,
			int8_t	m_type,
			int8_t	m_flags,
			int8_t	*m_data,
			int16_t	m_len,
			int16_t	m_status );

int32_t
YSM_DC_IncomingMessageGREET( YSM_SLAVE *victim,
			int8_t	m_type,
			int8_t	m_flags,
			int8_t	*m_data,
			int16_t	m_len,
			int16_t	m_status );


int32_t
YSM_DC_FileB( YSM_SLAVE	*victim,
		char		*filename,
		char		*reason );

int32_t
YSM_DC_FileDecline( YSM_SLAVE *victim, int8_t *reason );

int32_t YSM_DC_File( YSM_SLAVE	*victim, int8_t *fname, int8_t *desc );

int32_t
YSM_DC_FileInit( YSM_SLAVE *victim, int32_t numfiles, int32_t numbytes );

int32_t
YSM_DC_FileInitAck( YSM_SLAVE *victim, int32_t sock, int8_t *buf, int32_t len );

int32_t YSM_DC_FileStart( YSM_SLAVE *victim );

int32_t
YSM_DC_FileStartAck( YSM_SLAVE *victim, int32_t sock, int8_t *buf, int32_t len );

int32_t
YSM_DC_FileSpeedAck( YSM_SLAVE *victim, int8_t *buf, int32_t len );

int32_t
YSM_DC_FileTransfer( YSM_SLAVE *victim, int8_t *buf, int32_t len );

int32_t YSM_DC_FileReceive( YSM_SLAVE *victim, int8_t *buf, int32_t len );

int YSM_DC_Init( void );

#endif
