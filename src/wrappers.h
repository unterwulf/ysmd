/*

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

#ifndef _WRAPPERS_H_
#define _WRAPPERS_H_

#define YSM_MALLOC(x)    ysm_malloc((x), __FILE__, __LINE__)
#define YSM_CALLOC(x, y) ysm_calloc((x), (y), __FILE__, __LINE__)
#define YSM_FREE(x)      ysm_free((x), __FILE__, __LINE__); (x)=NULL

void  * ysm_malloc(size_t size, char *file, int line);
void  * ysm_calloc(size_t nmemb, size_t size, char *file, int line);
void    ysm_free(void *what, char *file, int line);

int     YSM_READ(int32_t sock, void *buf, int read_len, char priority);
size_t  YSM_READ_LN(int32_t sock, int8_t *obuf, size_t maxsize); 
int     YSM_WRITE(int32_t sock, void *data, int32_t data_len);
int32_t YSM_WRITE_DC(slave_t *victim, int32_t sock, void *data, int32_t data_len);
void    ysm_exit(int32_t status, int8_t ask);
void    ysm_reconnect(void);
int     ysm_isInvalidPtr(void *ptr);

#endif /* _WRAPPERS_H_ */
