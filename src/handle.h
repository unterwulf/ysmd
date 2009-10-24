#ifndef _HANDLE_H_
#define _HANDLE_H_

typedef int16_t   hnd_t;
typedef hnd_t    *hnd_pool_t;

hnd_pool_t  initHandlePool(uint16_t init_size);
hnd_t       linkHandle(hnd_pool_t pool, void *ptr);
void       *dereferenceHandle(hnd_pool_t pool, hnd_t hnd);
void        unlinkHandle(hnd_pool_t pool, hnd_t hnd);
void        freeHandlePool(hnd_pool_t pool);

#endif /* _HANDLE_H_ */
