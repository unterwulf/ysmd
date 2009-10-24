#include "ysm.h"
#include "wrappers.h"
#include "ystring.h"

string_t *initString()
{
    string_t *str = (string_t *) YSM_MALLOC(sizeof(string_t));

    if (str != NULL)
    {
        str->data = NULL;
        str->ptr = NULL;
        str->len = 0;
    }

    return str;
}

char *getString(const string_t *str)
{
    return str->data;
}

void freeString(string_t *str)
{
    if (str->data != NULL)
    {
        YSM_FREE(str->data);
        str->len = 0;
        str->ptr = NULL;
    }
    YSM_FREE(str);
}

void clearString(string_t *str)
{
    if (str->data != NULL)
        str->data[0] = '\0';

    str->ptr = str->data;
}

static int reallocString(string_t *str, size_t newLen)
{
    int8_t *newData;
    size_t offset;

    DEBUG_PRINT("len: %lu, newlen: %lu", str->len, newLen);

    if ((newData = YSM_REALLOC(str->data, newLen + 1)) != NULL)
    {
        if (str->data == NULL)
            newData[0] = '\0';
        offset = str->ptr - str->data;
        if (offset > newLen)
            offset = newLen;
        str->data = newData;
        str->ptr = newData + offset;
        str->len = newLen;
        return TRUE;
    }

    return FALSE;
}

int concatString(string_t *str, const char *str2)
{
    size_t newLen;
    size_t reqLen;
    size_t addLen;

    addLen = strlen(str2) + 1;
    reqLen = str->ptr - str->data + addLen;
    newLen = (str->data == NULL) ? STRING_INIT_LEN : str->len;

    if (reqLen > newLen)
        newLen = reqLen*2;

    if (newLen != str->len)
    {
        if (!reallocString(str,
                    newLen*2 > STRING_INIT_LEN ? newLen*2 : STRING_INIT_LEN))
            return FALSE;
    }

    strncat(str->ptr, str2, addLen - 1);
    str->ptr += addLen;

    return TRUE;
}

int printfString(string_t *str, const char *fmt, ...)
{
    int retVal;
    va_list ap;

    va_start(ap, fmt);
    retVal = vprintfString(str, fmt, ap);
    va_end(ap);

    return retVal;
}

int vprintfString(string_t *str, const char *fmt, va_list ap)
{
    size_t newLen;
    size_t reqLen;
    size_t addLen;

    addLen = vsnprintf(NULL, 0, fmt, ap);
    reqLen = str->ptr - str->data + addLen;
    newLen = (str->data == NULL) ? STRING_INIT_LEN : str->len;

    if (reqLen > newLen)
        newLen = reqLen*2;

    if (newLen != str->len)
    {
        if (!reallocString(str,
                    newLen*2 > STRING_INIT_LEN ? newLen*2 : STRING_INIT_LEN))
            return FALSE;
    }

    vsnprintf(str->ptr, addLen+1, fmt, ap);
    str->ptr += addLen;

    return TRUE;
}
