/*    $Id: YSM_Charset.c,v 1.32 2005/09/04 01:36:48 rad2k Exp $    */
/*
-======================== ysmICQ client ============================-
        Having fun with a boring Protocol
-======================== YSM_Charset.c ============================-

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
#include "wrappers.h"
#include "charset.h"
#include "direct.h"
#include <errno.h>

#ifdef YSM_USE_CHARCONV

#ifdef USE_FRIBIDI
#include <fribidi/fribidi.h>
#endif

/*
  NOTE: Caller is *always* responsible for freeing *buf_to
*/
static int32_t YSM_Iconv(
    int8_t   *charset_from,
    int8_t   *charset_to,
    int8_t   *buf_from,
    int32_t  *buf_fromlen,
    int8_t  **buf_to,
    size_t    maxlen)
{
    char     *inptr   = NULL, *outptr = NULL, *buffy = NULL;
    size_t    bytes_in, bytes_out;
    iconv_t   ASCII_2_ucs4, input_2_ucs4, ucs4_2_output;
    int8_t    converr_marker = FALSE;

    if (!charset_from || !charset_to || !buf_from || !buf_fromlen ||!buf_to)
        return -1;

    if (strlen(charset_from) <= 1 || strlen(charset_to) <= 1)
        return -1;

    if (!(buffy = ysm_calloc(1, YSM_ICONV_MAXLEN, __FILE__, __LINE__))) 
        return -1;

    /* Do we need to alloc buf_to? is it already in the heap? */
    if (*buf_to == NULL)
    {
        if (!(*buf_to = YSM_CALLOC(1, maxlen)))
        { 
            YSM_FREE(buffy);
            return -1;
        }
    }
    
    ASCII_2_ucs4 = iconv_open("UCS4", "ASCII");
    input_2_ucs4 = iconv_open("UCS4", charset_from);
    ucs4_2_output = iconv_open(charset_to, "UCS4");
    
    if ((ASCII_2_ucs4 == (iconv_t)(-1))
    || (input_2_ucs4 == (iconv_t)(-1))
    || (ucs4_2_output == (iconv_t)(-1)) )
    {
        /* probably the charset passed to iconv isnt supported */
        YSM_FREE(buffy);

        if (ASCII_2_ucs4 != (iconv_t)(-1))
            iconv_close(ASCII_2_ucs4);

        if (input_2_ucs4 != (iconv_t)(-1))
            iconv_close(input_2_ucs4);
        
        if (ucs4_2_output != (iconv_t)(-1))
            iconv_close(ucs4_2_output);

        return -1;
    }

    /*
      First pass: Convert input to UCS-4
      This allows us to treat problems with the input character set
      independently from problems with the output character set.
    */
    inptr     = buf_from;
    outptr    = buffy;
    bytes_in  = *buf_fromlen;

    /* ATT! Must match bytes_in = statement later */
    bytes_out = YSM_ICONV_MAXLEN;

    while (bytes_in > 0)
    {
        size_t result = iconv(input_2_ucs4,
                  &inptr,
                  &bytes_in,
                  &outptr,
                  &bytes_out);
        
        if (result == (size_t)(-1))
        { 
        /* Since UCS4 can represent all characters, an error means that
         * there is probably a character in the input that is not supported
         * by charset_from. This is probably the result of the user specifying
         * an incorrect charset in the config file. All we can do to
         * recover is skip one byte and hope for the best. In the case
         * of single byte character sets and character sets that don't
         * maintain state over several characters this should work, but
         * in other cases, this will probably turn the rest of the text into 
         * undecipherable garbage.  When we do this the first time, insert 
         * a message into the output to make sure that the user knows that 
         * everything past this point could be complete bullshit. 
         */
            printf("ERROR: %d\n", errno);
            if (!converr_marker)
            {
                char* msg="<YSM WARNING: THE REST OF THIS TEXT MAY POSSIBLY "
                          "BE CORRUPTED DUE TO CHARSET CONVERTION ERRORS>";
                size_t len = strlen(msg);

                converr_marker = TRUE;
                iconv(ASCII_2_ucs4, &msg, &len, &outptr, &bytes_out);
            }
               
            ++inptr;
            if (bytes_in>0) --bytes_in;
        }
    }
    
    /* Second pass: Convert UCS-4 to output
     * This allows us to treat problems with the output character set
     * independently from problems with the input character set.
     */

    inptr     = buffy;
    outptr    = *buf_to;
    bytes_in  = (YSM_ICONV_MAXLEN) - bytes_out;
    bytes_out = maxlen - 1; /* leave 1 for later 0-termination */

    while (bytes_in >= 4) {

        size_t result = iconv( ucs4_2_output,
                    &inptr,
                    &bytes_in,
                    &outptr,
                    &bytes_out);
      
        if (result == (size_t)(-1) && bytes_in >= 4) {

            /* An error here usually means that an input character
             * is not representable in the output character set.
             * We recover by replacing the offending character 
             * with '?'. If it is already '?', then we skip it.
             */
              if (inptr[0]!= 0 || inptr[1]!= 0 
            || inptr[2]!= 0 || inptr[3]!= '?') {

                  inptr[0] = inptr[1] = inptr[2] = '\0';
                  inptr[3] = '?';

              } else {
                  inptr += 4;
                  bytes_in -= 4;
              }
        }
    }

    /* make sure output is 0-terminated, just in case someone wants to
     * run string routines such as strlen() on the data. 
     * We have at least 1 byte left for this because we used 
     * maxlen - 1 to initialize bytes_out before 2nd pass.
     * Note that we don't update bytes_out here, because this
     * additional 0-termination is just a precaution and not part
     * of the conversion result. 
     */

    *outptr++ = '\0';

    /* update length */
    *buf_fromlen = (maxlen - 1) - bytes_out;
    
    YSM_FREE(buffy);

    iconv_close(ASCII_2_ucs4);
    iconv_close(input_2_ucs4);
    iconv_close(ucs4_2_output);

    return 1;
}

/* We set a default charset for both LOCAL and TRANS in case the
 * user doesn't specify a charset. This is used if convertion 
 * from/to utf8 is required. (inside YSM_Convert())
 */

void init_charset(void)
{
    /* Did the user specify a CHARSET_LOCAL? */
    if (!g_cfg.charset_local[0]) {
        /* Use CP1252 as the default CHARSET_LOCAL */
        memset( g_cfg.charset_local, 0, MAX_CHARSET + 4 );
        strncpy( g_cfg.charset_local,
            "CP1252",
            sizeof(g_cfg.charset_local) - 1);
    }

    /* Did the user specify a CHARSET_TRANS? */
    if (!g_cfg.charset_trans[0]) {
        /* Use CP1252 as the default CHARSET_TRANS */
        memset( g_cfg.charset_trans, 0, MAX_CHARSET + 4 );
        strncpy( g_cfg.charset_trans,
            "CP1252",
            sizeof(g_cfg.charset_trans) - 1);
    }
}

#endif /* YSM_USE_CHARCONV */

/*
 * m_flags may have MFLAGTYPE_UTF8 to decode from or encode to.
 * it can also be MFLAGTYPE_UCS2BE, but we only support decoding.
 */

int32_t YSM_Charset(
    int8_t     direction,
    int8_t    *buf_from,
    int32_t   *buf_fromlen,
    int8_t   **buf_to,
    u_int8_t   m_flags )
{
#ifndef YSM_USE_CHARCONV
    return 0;
#else
    int8_t  *cfrom = NULL, *cto = NULL;
    int32_t  ret = 0;

    if (direction == CHARSET_INCOMING)
    {
        cfrom = g_cfg.charset_trans;
        cto   = g_cfg.charset_local;
    }
    else if (direction == CHARSET_OUTGOING)
    {
        cto   = g_cfg.charset_trans;
        cfrom = g_cfg.charset_local;
    }
    else
        return -1;

    if (!cfrom || !cto || !buf_from || !buf_fromlen || !buf_to) 
        return -1;

    /* Do we have to decode an incoming UTF-8/UCS-2BE message first? */
    if ((m_flags & MFLAGTYPE_UTF8 || m_flags & MFLAGTYPE_UCS2BE) 
    && direction == CHARSET_INCOMING)
    {
        if (m_flags & MFLAGTYPE_UTF8)
        {
            ret = YSM_Iconv("UTF-8",
                    cto,
                    buf_from,
                    buf_fromlen,
                    buf_to,
                    YSM_ICONV_MAXLEN);
        }
        else
        {    /* UCS-2BE */
            ret = YSM_Iconv("UCS-2BE",
                    cto,
                    buf_from,
                    buf_fromlen,
                    buf_to,
                    YSM_ICONV_MAXLEN);
        }

        if (ret >= 0)
        {
            if (ret)
            {
                /* convertion succeeded */
#ifdef USE_FRIBIDI
            int32_t        size = 0;
            FriBidiChar    *us = NULL, *out_us = NULL;
            FriBidiCharType    base;
            int8_t        *cbuf = NULL, *outstring = NULL;
            
                size = strlen(*buf_to) + 1;    
                us = (FriBidiChar *) ysm_malloc(
                        size * sizeof(FriBidiChar),
                        __FILE__,
                        __LINE__
                        );

                out_us = (FriBidiChar *) ysm_malloc(
                        size * sizeof(FriBidiChar),
                        __FILE__,
                        __LINE__
                        );

                outstring = (int8_t *) ysm_malloc( 
                        size * sizeof(int8_t),
                        __FILE__,
                        __LINE__
                        );

                cbuf = strdup(*buf_to);
                if (cbuf == NULL) {
                    /* if failed continue normally */
                    ysm_free(us, __FILE__, __LINE__);
                    us = NULL;
                    ysm_free(out_us, __FILE__, __LINE__);
                    out_us = NULL;
                    ysm_free(outstring, __FILE__, __LINE__);
                    outstring = NULL;
                } else {
                    base = FRIBIDI_TYPE_N;
                    fribidi_iso8859_8_to_unicode( cbuf,
                                size - 1,
                                us );
                    fribidi_log2vis( us,
                            size - 1,
                            &base,
                            out_us,
                            0,
                            0,
                            0 );

                    fribidi_unicode_to_iso8859_8( out_us,
                                size - 1,
                                outstring );

                    ysm_free(us, __FILE__, __LINE__);
                    us = NULL;
                    ysm_free(out_us, __FILE__, __LINE__);
                    out_us = NULL;
                    ysm_free(cbuf, __FILE__, __LINE__);
                    cbuf = NULL;
                    ysm_free(*buf_to, __FILE__, __LINE__);
                    *buf_to = NULL;

                    *buf_to = outstring;
                }
                
#endif

                buf_from = *buf_to;
            }
            /* else leave things as they are */
        }
        else
        {
            if (*buf_to) {
                ysm_free(*buf_to, __FILE__, __LINE__);
                *buf_to = NULL;
            }
            return ret;
        }

    /* Do we have to encode an outgoing UTF-8 message? */
    }
    else if ((m_flags & MFLAGTYPE_UTF8) && direction == CHARSET_OUTGOING)
    {
        ret = YSM_Iconv(cfrom,
                "UTF-8",
                buf_from,
                buf_fromlen,
                buf_to,
                YSM_ICONV_MAXLEN);

        if (ret < 0)
        {
            if (*buf_to)
                YSM_FREE(*buf_to);
            return ret;
        }
    }
    else
    {
        ret = YSM_Iconv(cfrom,
            cto,
            buf_from,
            buf_fromlen,
            buf_to,
            YSM_ICONV_MAXLEN);

        if (ret < 0)
        {
            if (*buf_to)
                YSM_FREE(*buf_to);
            return ret;
        }
    }

    return 0;
#endif    /* YSM_USE_CHARCONV */
}

const int8_t *base64_table =
"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

int8_t *
YSM_encode64( const int8_t *str )
{

static int8_t    *buf;
const u_int8_t    *src;
int8_t        *dst;
int32_t        bits = 0, data = 0, src_len = 0, dst_len = 0;

    /* make base64 string */
        src_len = strlen(str);
        dst_len = (src_len+2)/3*4;
        buf = ysm_malloc( dst_len+1, __FILE__, __LINE__ );
        src = str; dst = (u_int8_t *)buf;

        while ( dst_len-- ) {
            if ( bits < 6 ) {
            data = (data << 8) | *src;
            bits += 8;
            if ( *src != 0 ) src++;
            }
            *dst++ = base64_table[0x3F & (data >> (bits-6))];
            bits -= 6;
        }

        *dst = '\0';
        /* fix-up tail padding */
        switch ( src_len%3 ) {
            case 1:
                   *--dst = '=';
          case 2:
                    *--dst = '=';
        }

        return buf;
}


/* Talking about useless functions, this function
 * should be a 'speedup' for output convertion.
 * (check YSM_CharsetConvertString below)
 */

int8_t * YSM_CharsetConvertOutputString(int8_t **stringp, int8_t fl_dofree)
{
    if (YSM_CharsetConvertString(stringp, CHARSET_INCOMING, 0, fl_dofree) <= 0)
    {
            /* convertion failed, show original anyway */
    }

    return *stringp;
}    

/* Do all the work of converting a string with YSM_Charset()
 * returns <= 0 on error 
 * returns new length if everything went ok
 */

int32_t YSM_CharsetConvertString(
    int8_t    **stringp,      /* buffer */
    int8_t      direction,    /* incoming/outgoing */
    u_int8_t    flags,        /* buffer flags */
    int8_t      fl_dofree )   /* free original buffer */
{
    int32_t    stringlength = 0;
    int32_t    retval = 0;
    int8_t    *newp = NULL;

    if (stringp == NULL || *stringp == NULL) 
        return -1;

    stringlength = strlen(*stringp) + 1;
    if (stringlength <= 1)
        return -1;

    retval = YSM_Charset(direction, *stringp, &stringlength, &newp, flags);

    /* handle YSM_Charset errors */
    if (retval < 0) return -1;

    /* convertion might have not be required. don't error though */
    if (newp == NULL) return 0;

    /* convertion succeeded, newp points to the new buffer */
    if (newp == *stringp) return -1;

    /* free the original buffer if required */
    if (fl_dofree)
        YSM_FREE(*stringp);

    /* set stringp to point to the new buffer */
    *stringp = newp;

    /* return the new string length */    
    return stringlength;
}
