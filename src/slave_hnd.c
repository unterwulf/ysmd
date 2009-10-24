#include "slaves.h"

static slave_hnd_t *g_slave_hnd_pool = NULL;

void initSlavesHandlePool()
{
    slave_hnd = YSM_CALLOC(sizeof(slave_hnd_t) * INIT_SLAVE_HND_POOL_SIZE);
}

slave_hnd_t linkSlaveHandle(slave_t *slave)
{
    slave_hnd_t hnd;

    do {
        /* find first free handler */
        for (hnd = 0; hnd < getPoolSize(slaveHndlPool); hnd++)
        {
            if (slaveHndlPool.data[hnd] == NULL)
            {
                slaveHndlPool.data[hnd] = slave;
                return hnd;
            }
        }
        /* if there is no free handler to increase pool size and try again */
    } while (resizePool());

    return -1;
}

void unlinkSlaveHandle(slave_hnd_t hnd)
{
    if (hnd >= 0 && hnd < getPoolSize(slaveHndPool))
    {
        slaveHndlPool.data[hnd] = NULL;
    }
}
