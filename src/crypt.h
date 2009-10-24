/*    $Id: YSM_Crypt.h,v 1.10 2004/04/02 01:34:16 rad2k Exp $    */
/*
-======================== ysmICQ client ============================-
        Having fun with a boring Protocol
-========================== YSM_Crypt.h ============================-

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

#ifndef _CRYPT_H_
#define _CRYPT_H_

#include "rijndael/rijndael-api-fst.h"

int YSM_EncryptData(char *plain_data, int len, keyInstance *cipherKey);
int YSM_DecryptData(char *enc_data, keyInstance *cipherKey);

keyInstance * YSM_EncryptAnyData(
    slave_t *contact,
    int8_t   **m_data,
    int32_t   *m_len,
    u_int32_t  maxsize);

int32_t YSM_DecryptMessage(
    slave_t     *contact,
    int8_t      **m_data,
    int32_t      *m_len,
    keyInstance **crypt_key);

int32_t YSM_DecryptFileData(
    slave_t    *contact,
    int8_t      **m_data,
    int32_t      *m_len,
    keyInstance **crypt_key);

int32_t YSM_KeyEmpty(char *key);
int32_t YSM_ClearKey(slave_t *slave);

struct YSMCryptH
{
    unsigned char id[2];
    unsigned char d_len[2];
};

#endif
