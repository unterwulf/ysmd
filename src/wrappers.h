#ifndef _WRAPPERS_H_
#define _WRAPPERS_H_

#include "slaves.h"
#include "bytestream.h"

#define YSM_MALLOC(x)     ysm_malloc((x), __FILE__, __LINE__)
#define YSM_CALLOC(x, y)  ysm_calloc((x), (y), __FILE__, __LINE__)
#define YSM_REALLOC(x, y) ysm_realloc((x), (y), __FILE__, __LINE__)
#define YSM_FREE(x)       ysm_free((x), __FILE__, __LINE__); (x)=NULL

void   *ysm_malloc(size_t size, char *file, int line);
void   *ysm_calloc(size_t nmemb, size_t size, char *file, int line);
void   *ysm_realloc(void *mem, size_t size, char *file, int line);
void    ysm_free(void *what, char *file, int line);

int     YSM_READ(int32_t sock, void *buf, int read_len, char priority);
size_t  YSM_READ_LN(int32_t sock, int8_t *obuf, size_t maxsize);
int     bsAppendReadLoop(bsd_t bsd, int32_t sock, size_t count, uint8_t priority)
int     writeBs(int32_t sock, bsd_t bsd);
int     YSM_WRITE(int32_t sock, void *data, int32_t data_len);
int32_t YSM_WRITE_DC(slave_t *victim, int32_t sock, void *data, int32_t data_len);
void    ysm_exit(int32_t status, int8_t ask);

#endif /* _WRAPPERS_H_ */
