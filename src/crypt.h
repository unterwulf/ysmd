#ifndef _CRYPT_H_
#define _CRYPT_H_

#include "rijndael/rijndael-api-fst.h"

int YSM_EncryptData(char *plain_data, int len, keyInstance *cipherKey);
int YSM_DecryptData(char *enc_data, keyInstance *cipherKey);

keyInstance *YSM_EncryptAnyData(
    slave_t      *contact,
    int8_t      **m_data,
    int32_t      *m_len,
    uint32_t      maxsize);

int32_t YSM_DecryptMessage(
    slave_t      *contact,
    uint8_t     **m_data,
    int32_t      *m_len,
    keyInstance **crypt_key);

int32_t YSM_DecryptFileData(
    slave_t      *contact,
    int8_t      **m_data,
    int32_t      *m_len,
    keyInstance **crypt_key);

bool_t isKeyEmpty(uint8_t *key);

struct YSMCryptH
{
    uint8_t id[2];
    uint8_t d_len[2];
};

#endif
