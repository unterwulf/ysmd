/*	$Id: YSM_FishGUI.c,v 1.11 2004/08/21 22:56:18 rad2k Exp $ */
/*
-======================== ysmICQ client ============================-
		Having fun with a boring Protocol
-======================== YSM_FishGUI.c ============================-


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

#include "YSM.h"
__RCSID("$Id: YSM_FishGUI.c,v 1.11 2004/08/21 22:56:18 rad2k Exp $");

#include "YSM_ToolBox.h"
#include "YSM_Prompt.h"
#include "YSM_FishGUI.h"
#include "YSM_Wrappers.h"
#include "YSM_Slaves.h"
#include "YSM_Lists.h"

#include "YSM_Direct.h"

#define FISHGUI_PROTOVER	(1)

#define im_RAW_CMD		(1)
#define im_GET_CL		(2)
#define im_SET_CL		(2)
#define im_MSG			(4)
#define im_GETCONTACTLST	(6)
#define im_CONTACTSTATUS	(7)
#define im_CHARSETMSG		(8)
#define im_PARTNERVERSION	(9)

/* I didn't need full contactlist anymore */
#ifdef _FISHGUI_FULL_CONTACT_LIST
#define im_CONTACTLST		(5)
#endif

/* must be called on every function */
#define FISH_NOTCONNECTED (YSM_USER.fishgui.socket <= 0)

int32_t 
FishGUI_send_message(int32_t uin, int8_t *data, int32_t len);

int32_t
FishGUI_send_charsetmessage(int32_t uin, int8_t *data, int32_t len, int8_t *charset);

void 
FishGUI_send_contact_status (int32_t uin);

static int32_t	serverversion = 0;


/* FishGUI_recv()
 * blocks on recv, waiting to read $rsize bytes to $outbuf from the FishGUI
 * socket if available.
 * Returns the amount of bytes read from the socket.
 */

int32_t
FishGUI_recv( int8_t *outbuf, size_t rsize )
{
int32_t ret = 0, readb = 0, left = 0;

	if (outbuf == NULL)
		return -1;

	if FISH_NOTCONNECTED
		return -1;

	left = rsize;

	do {
		ret = SOCK_READ(YSM_USER.fishgui.socket, outbuf+readb, left);
		if (ret > 0) {
			readb += ret;
			left -= ret;
		}
			
	} while (ret >= 0 && left);

	return readb;
}

int32_t
myxdr_read_int2(int32_t *state, int32_t *value)
{
int32_t	r = 0;
unsigned char tval;

	if (state == NULL)
		return 0;

	if FISH_NOTCONNECTED
		return 0;

	*value = 0;

	r = 1 == SOCK_READ(YSM_USER.fishgui.socket, &tval, 1);
	*value = *value << 8;
	*value += tval;

	r&= 1 == SOCK_READ(YSM_USER.fishgui.socket, &tval, 1);
	*value = *value << 8;
	*value += tval;

	r&= 1 == SOCK_READ(YSM_USER.fishgui.socket, &tval, 1);
	*value = *value << 8;
	*value += tval;

	r&= 1 == SOCK_READ(YSM_USER.fishgui.socket, &tval, 1);
	*value = *value << 8;
	*value += tval;
	
	return r; 
}

int32_t
myxdr_write_int2(int32_t value)
{
int32_t	r = 0;
int8_t	tval = 0;

	if FISH_NOTCONNECTED
		return 0;

	tval = (value >> (8*3)) & 0xff;
	r = 1==SOCK_WRITE(YSM_USER.fishgui.socket, &tval, 1);
	tval = (value >> (8*2)) & 0xff;
	r = r && 1==SOCK_WRITE(YSM_USER.fishgui.socket, &tval, 1);
	tval = (value >> (8*1)) & 0xff;
	r = r && 1==SOCK_WRITE(YSM_USER.fishgui.socket, &tval, 1);
	tval = (value >> (8*0)) & 0xff;
	r = r && 1==SOCK_WRITE(YSM_USER.fishgui.socket, &tval, 1);
	return r;
}

int32_t
myxdr_write_string(int32_t len, int8_t *string)
{
	if (string == NULL)
		return 0;

	if FISH_NOTCONNECTED
		return 0;

	if (myxdr_write_int2(len)) {
		int8_t *buf = NULL; 
		int32_t r = 0, tlen = 0, rlen = len;

		if (len %4 > 0)
			rlen+=4-(len%4);
		if (len==0)
			rlen=4;
	

		buf = YSM_Malloc(rlen, __FILE__, __LINE__);
		if (buf == NULL) {
			return(0);
		}

		memset(buf, 0, rlen);
		memcpy(buf, string, len);

		tlen = SOCK_WRITE(YSM_USER.fishgui.socket, buf, rlen);

		r = tlen == rlen;
		YSM_Free(buf, __FILE__, __LINE__);
		buf = NULL;
		return(r);
	}

	return(0);
}

int32_t
myxdr_write_ansi_string(int8_t *string)
{
	return (myxdr_write_string(strlen(string), string));
}


int32_t
myxdr_read_string(int8_t **string)
{
static int32_t state = 0;
static int32_t len = 0;

	if FISH_NOTCONNECTED
		return 0;

	if (myxdr_read_int2(&state, &len)) {
		int32_t rlen = len;
		int8_t *buf = NULL;
		buf = YSM_Calloc(1, len+1+4, __FILE__, __LINE__);
		if (buf == NULL) 
			return (0);
		if (len %4 > 0)
			rlen+=4-(len%4);
		if (len==0)
			rlen=4;

		FishGUI_recv(buf, rlen);
		buf[len] = (char)0;
		*string = buf;
		return(1);
	}

	return(0);
}

int32_t
myxdr_write_contactstatus( YSM_SLAVE *contact )
{
int32_t r;
   
	if (contact == NULL)
		return 0;
 
	if FISH_NOTCONNECTED
		return 0;
		
	if (! (r = myxdr_write_int2(contact->Uin)) )
	 return(0);

	if (strlen(contact->info.NickName) != 0)
		r = myxdr_write_ansi_string(contact->info.NickName);
	else
		r = myxdr_write_ansi_string("");

	r = r && myxdr_write_int2(contact->status);
	return r;
}


void
FishGUI_event(int8_t event, int32_t uin, int8_t *nick, 
		int32_t len, int8_t *data, int32_t m_flags)
{
	if FISH_NOTCONNECTED
		return;


	switch (event) {
		case EVENT_PREINCOMINGMESSAGE:
		    while ((len>=1) && (data[len-1]==(char)0))
		           len--;
		    if(serverversion>=0)
		    {
            		/*PRINTF(VERBOSE_BASE, "\nFLAGS=%x\n\n",m_flags);*/
		   	if (m_flags & MFLAGTYPE_UTF8)
				FishGUI_send_charsetmessage(uin, data, len,"utf-8");
			else
				if (m_flags & MFLAGTYPE_UCS2BE)
					/* the encoding is the name of the python's decoder */
					FishGUI_send_charsetmessage(uin, data, len,"utf_16_be");
				else
					FishGUI_send_charsetmessage(uin, data, len,"iso-8859-1");
		    } else
			FishGUI_send_message(uin, data, len);
		    break;
		case EVENT_ONCOMINGUSER:
		case EVENT_OFFGOINGUSER:
		    FishGUI_send_contact_status (uin);
		    break;
	}
}

void
FishGUI_send_contact_status (int32_t uin)
{
	YSM_SLAVE	*node;

	if FISH_NOTCONNECTED
		return;

	node = YSM_FindSlaveinList(NULL,uin);
	if (!node)
		return;

	myxdr_write_int2(im_CONTACTSTATUS);
	myxdr_write_contactstatus( node );
}

int32_t
FishGUI_send_message(int32_t uin, int8_t *data, int32_t len)
{
int32_t r;
	if FISH_NOTCONNECTED
		return(0);
	
	if (len==0)
	    return(1);
    if (!data)
        return(1);

	r = myxdr_write_int2(im_MSG);
	r = r && myxdr_write_int2(uin);
	r = r && myxdr_write_int2(0); // ysm user's uin!
	r = r && myxdr_write_string(len, data);
	return(r);
}

int32_t
FishGUI_send_charsetmessage(int32_t uin, int8_t *data, int32_t len, int8_t *charset)
{
int32_t r;
	assert(charset!=NULL);

	if FISH_NOTCONNECTED
		return(0);
	
	if (len==0)
		return(1);
	if (!data)
		return(1);

	r = myxdr_write_int2(im_CHARSETMSG);
	r = r && myxdr_write_int2(uin);
	r = r && myxdr_write_int2(0); // ysm user's uin!
	r = r && myxdr_write_string(strlen(charset), charset);
	r = r && myxdr_write_string(len, data);
	return(r);
}

void
FishGUI_send_version(void)
{
	if FISH_NOTCONNECTED
		return;

	myxdr_write_int2(im_PARTNERVERSION);
	myxdr_write_int2(FISHGUI_PROTOVER);
}

#ifdef _FISHGUI_FULL_CONTACT_LIST
int32_t
FishGUI_send_contact_list()
{
	YSM_SLAVE	*node = plist_firstSLAVE;
	int32_t      ret=1;

	if FISH_NOTCONNECTED
		return -1;

	myxdr_write_int2(im_CONTACTLST);
	while ((node != NULL) && ret) {
		ret = myxdr_write_int2(1); /* one more item */
		ret = ret && myxdr_write_contactstatus(node);
		node = node->next;
	}
	if (!ret)
	  return(0);
	  
	/* no more items.. */
	myxdr_write_int2(0);
	return(1);
}
#endif

/* FishGUI_init
 *	Connects to the AF_UNIX socket where the FISH GUI is listening.
 *	If anything fails, the global FishGUI socket is set to -1.
 *	Otherwise, the socket is connected.
 */

void
FishGUI_init(void)
{
struct sockaddr_in dest_addr; 

	/* we check if we have to use the FishGUI by its port */
	if (!YSM_USER.fishgui.port)
		return;

	dest_addr.sin_family = AF_INET;    /* host byte order */
    dest_addr.sin_port = htons(YSM_USER.fishgui.port);
    dest_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    memset( &(dest_addr.sin_zero),'\0',
        sizeof(dest_addr.sin_zero)); /* zero the rest of the struct */

	YSM_USER.fishgui.socket = socket(PF_INET, SOCK_STREAM, 0);
	if (connect(YSM_USER.fishgui.socket,
		(struct sockaddr *)&dest_addr, sizeof(struct sockaddr)) < 0) 
	{
		close(YSM_USER.fishgui.socket);
		YSM_USER.fishgui.socket = -1;
	}

	/* do FD initialization */
	FD_Timeout(5, 0);
	FD_Init(FD_FISHGUI);
	if (YSM_USER.fishgui.socket) {
		FD_Add(YSM_USER.fishgui.socket, FD_FISHGUI);

		FishGUI_send_version();
	}
}

/* FishGUI_send() 
 * writes $dsize bytes of $data to the FishGUI socket if available.
 * Returns the amount of bytes written to the socket.
 * Returns -1 on errors.
 */

int32_t
FishGUI_send( int8_t *rawdata, size_t dsize )
{
int8_t c;

	if FISH_NOTCONNECTED
		return -1;

	c = (char) (( dsize >> (3*8) ) & 0xff);
	SOCK_WRITE(YSM_USER.fishgui.socket, &c, 1);
	c = (char) (( dsize >> (2*8) ) & 0xff);
	SOCK_WRITE(YSM_USER.fishgui.socket, &c, 1);
	c = (char) (( dsize >> (1*8) ) & 0xff);
	SOCK_WRITE(YSM_USER.fishgui.socket, &c, 1);
	c = (char) (( dsize ) & 0xff);
	SOCK_WRITE(YSM_USER.fishgui.socket, &c, 1);
	return SOCK_WRITE(YSM_USER.fishgui.socket, rawdata, dsize);
}

void
FishGUI_readcmd(void)
{
int32_t	msgtype = 0, state = 0;

	if FISH_NOTCONNECTED
		return;

	if (myxdr_read_int2(&state, &msgtype)) {
		if (msgtype==im_RAW_CMD) {
			int8_t *buf = NULL;
			if (myxdr_read_string(&buf)) {
				/*printf("COMMAND  %s \n",buf);*/
				YSM_DoCommand(buf);
				YSM_Free(buf, __FILE__, __LINE__);
				buf = NULL;
			}
		}

		if (msgtype == im_GET_CL) {
			/* we must send contact list ! :-) */

		} 

		if (msgtype == im_PARTNERVERSION)
		{
			int32_t	version = 0, ok = 0;
			ok = myxdr_read_int2(&state, &version);
			if (ok)
				serverversion = version;
		}

		if (msgtype == im_MSG) {
			int32_t	fromuin = 0, touin = 0, ok = 0;
			int8_t	*msg = NULL;

			ok = myxdr_read_int2(&state, &fromuin) &&
				myxdr_read_int2(&state, &touin) && 
				myxdr_read_string(&msg);

			if (ok) {
				/* dispatch message */
				YSM_SLAVE	*node;
				node = YSM_FindSlaveinList(NULL,touin);
				
				if (node) {
					YSM_SendMessage(node->Uin,
						msg,
						(int8_t)(node->flags & FL_LOG),
						node, 0);
				} else {
					YSM_SendMessage(touin,
						msg, 0, NULL, 0);
				}
				YSM_Free(msg, __FILE__, __LINE__);
				msg = NULL;
			}
		}

		if (msgtype == im_CHARSETMSG) {
			int32_t	fromuin = 0, touin = 0, ok = 0;
			int8_t  *charset = NULL;
			int8_t	*msg = NULL;

			ok = myxdr_read_int2(&state, &fromuin) &&
				myxdr_read_int2(&state, &touin) && 
				myxdr_read_string(&charset) && 
				myxdr_read_string(&msg);

            //PRINTF(VERBOSE_BASE, "ok is %d\n\n",ok);
            //PRINTF(VERBOSE_BASE, "charset is %s\n\n",charset);
            //PRINTF(VERBOSE_BASE, "touin is %d\n\n",touin);
            //PRINTF(VERBOSE_BASE, "msg is %s\n\n",msg);
            
			if ((ok)) {
				/* dispatch message */
				YSM_SLAVE	*node;
				node = YSM_FindSlaveinList(NULL,touin);
				//PRINTF(VERBOSE_BASE, "node is %d\n\n",node);
					
				if (node) {
					YSM_SendMessage(node->Uin,
						msg,
						(int8_t)(node->flags & FL_LOG),
						node, 0);
				} else {
					YSM_SendMessage(touin,
						msg, 0, NULL, 0);
				}
			}

			if (charset) {
				YSM_Free(charset, __FILE__, __LINE__);
				charset = NULL;
			}

			if (ok) {
				YSM_Free(msg, __FILE__, __LINE__);
				msg = NULL;
			}
		}
        #ifdef _FISHGUI_FULL_CONTACT_LIST
		if (msgtype == im_GETCONTACTLST) {
			FishGUI_send_contact_list();
		}
		#endif
	}
}



void
FishGUI_runcmds(void)
{
	if FISH_NOTCONNECTED
		return;
		
	FD_Add(YSM_USER.fishgui.socket, FD_FISHGUI);
	FD_Select(FD_FISHGUI);
	if (FD_IsSet(YSM_USER.fishgui.socket, FD_FISHGUI)) {
		FishGUI_readcmd();
	}
}




