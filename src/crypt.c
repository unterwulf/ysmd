/*    $Id: YSM_Crypt.c,v 1.23 2004/04/29 01:52:30 rad2k Exp $    */
/*
-======================== ysmICQ client ============================-
        Having fun with a boring Protocol
-========================== YSM_Crypt.c ============================-

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

#include "ysm.h"
#include "toolbox.h"
#include "wrappers.h"
#include "slaves.h"
#include "crypt.h"

int YSM_EncryptData(char *plain_data, int len, keyInstance *cipherKey)
{
struct YSMCryptH    C_head;
cipherInstance        cipher;

    /* This will identify the incoming message as a crypted YSM message */
    C_head.id[0] = 0xde;
    C_head.id[1] = 0xaf;
    Word_2_Chars(&C_head.d_len[0], len);

    memcpy( &plain_data[0], &C_head, sizeof(C_head) );

    if (cipherKey == NULL) return -1;

    if (TRUE != cipherInit( &cipher, MODE_CBC, NULL )) {
        PRINTF(VERBOSE_BASE,"Cypher error: initializing!\n");
        return 0;
    }

    if (padEncrypt( &cipher,
        cipherKey,
        &plain_data[sizeof C_head],
        len,
        &plain_data[sizeof C_head]) <= 0) return 0;

    return 1;
}

int
YSM_DecryptData( char *enc_data, keyInstance *cipherKey )
{
struct    YSMCryptH    C_head;
cipherInstance        cipher;

    if (enc_data == NULL)
        return -1;

    memcpy( &C_head, &enc_data[0], sizeof(C_head) );

    if (C_head.id[0] != 0xde || C_head.id[1] != 0xaf)
        return 0;

    /* note the check for cipherKey must be done here, after
     * we have detected the message IS encrypted. don't move
     * this check.
     */

    if (cipherKey == NULL)
        return -1;

    if (TRUE != cipherInit( &cipher, MODE_CBC, NULL )) {
        PRINTF(VERBOSE_BASE,"cipherInit failed.\n");
        return -1;
    }

    if (padDecrypt( &cipher,
         cipherKey,
         &enc_data[sizeof C_head],
         Chars_2_Word(&C_head.d_len[0])+16
         - Chars_2_Word(&C_head.d_len[0])%16,
         &enc_data[sizeof C_head] ) <= 0) return -1;

    enc_data[sizeof C_head+Chars_2_Word(&C_head.d_len[0])] = 0;

    return Chars_2_Word(&C_head.d_len[0]);
}

/* Note: the cdata pointer must be released outside this function.
 * This function is used for both messages and files.
 * $maxsize is different depending from where the call is made. */

keyInstance *
YSM_EncryptAnyData( slave_t    *contact,
        int8_t        **m_data,
        int32_t        *m_len,
        u_int32_t    maxsize )
{
int8_t        *cdata = NULL, *m_datap = NULL;
keyInstance    *keyout = NULL;
int32_t        err_code = 0;

    if (contact == NULL) return NULL;

    m_datap    = *m_data;
    keyout    = &contact->crypto.key_out;

    if (YSM_KeyEmpty( contact->crypto.strkey )
        || contact->fprint != FINGERPRINT_YSM_CLIENT_CRYPT)
        return NULL;

    /* We need space for the crypt header. If we lack of space
     * then cut off the required bytes */
    if ((*m_len + sizeof(struct YSMCryptH)
        + MAX_CRYPT_PADDING) >= maxsize) {
        (*m_len) =
        maxsize - sizeof(struct YSMCryptH) - MAX_CRYPT_PADDING;
    }

    cdata = ysm_calloc( 1,
        *m_len + sizeof(struct YSMCryptH) + MAX_CRYPT_PADDING,
        __FILE__,
        __LINE__ );

    memcpy(&cdata[sizeof(struct YSMCryptH)], m_datap, *m_len);

    /* no need to free the data pointer, check outside this func. */
    (*m_data) = cdata;

    err_code = YSM_EncryptData( cdata, *m_len, keyout );
    switch(err_code)
    {
        case 0x1:
            break;
        case -1:
        case 0x0:
            keyout = NULL;
            break;
    }

    (*m_len) += sizeof(struct YSMCryptH)
            + MAX_CRYPT_PADDING
            - *m_len % MAX_CRYPT_PADDING;

    return (keyout);
}

/* YSM_DecryptMessage.
 *    returns >= 0 if OK
 *    returns < 0 if FAILED
 */

int32_t
YSM_DecryptMessage( slave_t    *contact,
        int8_t        **m_data,
        int32_t        *m_len,
        keyInstance     **crypt_key)    /* out */
{
int32_t        err_code = 0;

    /* contact CAN be NULL, if we don't have the caller in our list.
     * We do not want to decrypt the incoming data,
     * but we still need to check whether the message is encrypted
     * or not. Hence we MUST NOT fail instantly, if contact is NULL.
     */

    if (contact == NULL) {
        (*crypt_key) = NULL;

    } else {

        /* crypt_key is set to NULL when we want an error to show up
           * either we received an encrypted message from a stranger or
          * our key with the slave is not yet set
         */

        if (YSM_KeyEmpty( contact->crypto.strkey )
            || contact->fprint != FINGERPRINT_YSM_CLIENT_CRYPT ) {
            (*crypt_key) = NULL;

        } else {
            (*crypt_key) = &contact->crypto.key_in;
            /* change the key's direction */
            (*crypt_key)->direction = 1;
        }
    }

    /* We now check whether to decrypt the data or not.
     * We rely on 2 requirements:
     * a. having crypt_key NOT set to NULL
     * b. the data needs to start with our '\xde\xaf' magic bytes.
     * (This is done inside 'YSM_DecryptData'.)
     */

    /* Skip the head if the data was encrypted */
    err_code = YSM_DecryptData( *m_data, *crypt_key );

    switch (err_code)
    {
        case -1:
            PRINTF( VERBOSE_BASE,
        "\nUnable to decrypt the incoming encrypted message.\n"
        "Either you don't have a key set for this slave, or the key "
        "\nused to encrypt the message differs from the one already "
        "set.\nPlease type 'help key' in order to learn about the "
        "'key' command.\n"
            );
        case 0x0:
            /* message was not encrypted */
            (*crypt_key) = NULL;
            break;

            /* err_code contains the new length */
        default:
            if (err_code <= *m_len) {
                /* IMPORTANT CHECK. else an attacker could
                 * specify a huge size and screw us up
                 */
                (*m_len) = err_code;
            }

            (*m_data) += sizeof(struct YSMCryptH);
            break;
    }

    return err_code;
}

/* Note: this function is a mirror from YSM_DecryptMessage() */

/* YSM_DecryptFileData.
 *    returns >= 0 if OK
 *    returns < 0 if FAILED
 */

int32_t
YSM_DecryptFileData( slave_t    *contact,
        int8_t        **m_data,
        int32_t        *m_len,
        keyInstance    **crypt_key )    /* out */
{
int32_t        err_code = 0;

    if (contact == NULL) return -1;

    /* crypt_key is set to NULL when we want an error to show up */
    /* either we received an encrypted file from a stranger or */
    /* our key with the slave is not yet set */
    if ( YSM_KeyEmpty( contact->crypto.strkey )
        || contact->fprint != FINGERPRINT_YSM_CLIENT_CRYPT ) {
        (*crypt_key) = NULL;

    } else {
        (*crypt_key) = &contact->crypto.key_in;
        /* change the key's direction */
        (*crypt_key)->direction = 1;
    }


    /* Check if decrypt is neccesary */
    /* The slave maybe has us on his list and takes us as    */
    /* an encryption YSM client, but we don't have him,    */
    /* so we check always if the incoming file is encrypted or not. */

    /* Skip the head if the data was encrypted */
    err_code = YSM_DecryptData( *m_data, *crypt_key );

    switch (err_code)
    {
        case -1:
            PRINTF( VERBOSE_BASE,
            "Unable to decrypt incoming file data.\n"
            "This shouldn't really happend AT ALL.\n"
            "mail the programming team please.\n");
        case 0x0:
            /* transfer was not encrypted */
            (*crypt_key) = NULL;
            break;

        default:
            if (err_code <= *m_len) {
                /* IMPORTANT CHECK. else an attacker could
                 * specify a huge size and screw us up
                 */
                (*m_len) = err_code;
            }
            (*m_data) += sizeof( struct YSMCryptH );
            break;
    }

    return err_code;
}

int32_t YSM_KeyEmpty(char *key)
{
    u_int32_t x = 0;

    if (key == NULL) return -1;

    for (x = 0; x < MAX_KEY_LEN; x++)
        if (key[x] != 0x00) return 0;

    return 1;
}

int32_t YSM_ClearKey(slave_t *slave)
{
    if (slave == NULL) return -1;

    /* clear the key by memsetting it to zeroes */
    memset(slave->crypto.strkey, 0, sizeof(slave->crypto.strkey));
    /* update the configuration file for this slave */
    YSM_UpdateSlave(UPDATE_SLAVE, NULL, slave->uin);

    return 0;
}
