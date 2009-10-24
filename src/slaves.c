#include "ysm.h"
#include "ylist.h"
#include "slaves.h"
#include "wrappers.h"
#include "toolbox.h"
#include "setup.h"
#include "ystring.h"
#include "output.h"
#include "icqv7.h"
#include "handle.h"

typedef struct
{
    COMMON_LIST

    uin_t                  uin;
    sl_fprint_t            fprint;
    sl_caps_t              caps;
    req_id_t               reqId;
    sl_status_t            status;
    sl_status_t            status_flags;
    uint8_t               *statusStr;
    sl_flags_t             flags;
    buddy_special_status_t budType;
    buddy_timing_t         timing;
    direct_connection_t    d_con;
    buddy_main_info_t      info;
    encryption_info_t      crypto;
} slave_t;

static list_t     g_slave_list = { NULL, 0 };
static hnd_pool_t g_slave_hnd_pool = NULL;

int initSlaveList()
{
    g_slave_hnd_pool = initHandlePool(SLAVE_HND_POOL_INIT_SIZE);

    return g_slave_hnd_pool == NULL ? -1 : 0;
}

void freeSlaveList()
{
    freeList(&g_slave_list);
    freeHandlePool(g_slave_hnd_pool);
}

static slave_t *normalizeSlaveHandle(slave_hnd_t *hnd)
{
    slave_t *victim = NULL;

    if (hnd == NULL)
        return NULL;

    if (hnd->hnd == SLAVE_HND_NOT_A_SLAVE)
    {
        return NULL;
    }
    else if (hnd->hnd == SLAVE_HND_FIND)
    {
        querySlaveByUin(hnd->uin, hnd);
    }
    else if (hnd->hnd >= 0)
    {
        victim = (slave_t *)dereferenceHandle(g_slave_hnd_pool, hnd->hnd);
        if (victim && victim->uin == hnd->uin)
        {
            return victim;
        }
        else
        {
            querySlaveByUin(hnd->uin, hnd);
        }
    }
    else
        return NULL;
    
    victim = (slave_t *)dereferenceHandle(g_slave_hnd_pool, hnd->hnd);

    return victim;
}

int getNextSlave(slave_hdl_t *hdl)
{
    slave_t *suc = NULL;

    if (hdl == NULL)
        return -1;

    if (hdl->hdl == SLAVE_HND_START)
        suc = (slave_t *)g_slave_list.start;
    else
    {
        slave = (slave_t *)dereferenceHandle(g_slave_hdl_pool, hdl->hdl);

        if (slave == NULL)
            return -1;

        suc = (slave_t *)slave->suc;
    }

    if (suc == NULL)
    {
        hdl->hdl = SLAVE_HND_END;
        hdl->uin = 0;

        return -2;
    }

    hdl->hdl = suc->hdl;
    hdl->uin = suc->uin;

    return 0;
}

int querySlaveByUin(uin_t uin, slave_hnd_t *hnd)
{
    slave_t *node = NULL;

    if (hnd == NULL)
        return -1;

    for (node = (slave_t *) g_slave_list.start;
         node != NULL;
         node = (slave_t *) node->suc)
    {
        if (node->uin == uin)
        {
            hnd->hnd = node->hnd;
            hnd->uin = node->uin;

            return 0;
        }
    }

    hnd->hnd = SLAVE_HND_NOT_A_SLAVE;

    return -1;
}

int querySlaveByNick(uint8_t *nick, slave_hnd_t *hnd)
{
    slave_t *node = NULL;

    if (hnd == NULL)
        return -1;

    for (node = (slave_t *) g_slave_list.start;
         node != NULL;
         node = (slave_t *) node->suc)
    {
        if (strcmp(node->info, nick) == 0)
        {
            hnd->hnd = node->hnd;
            hnd->uin = node->uin;

            return 0;
        }
    }

    hnd->hnd = SLAVE_HND_NOT_A_SLAVE;

    return -1;
}

static slave_t *insertSlaveNode(slave_t *new)    /* inserts ordered */
{
    if (g_slave_list.start == NULL)
    {
        /* list empty, insert as first */
        unshiftListNode(&g_slave_list, (list_node_t *) new);
    }
    else
    {
        slave_t *n, *nprev;

        nprev = NULL;
        n = (slave_t *) g_slave_list.start;

        /* find position; if equal, add behind */
        while (n != NULL && strcasecmp(new->info.nickName,
            n->info.nickName) >= 0) {
            nprev = n;
            n = (slave_t *) n->suc;
        }

        if (nprev == NULL)
        {
            /* insert first */
            unshiftListNode(&g_slave_list, (list_node_t *) new);
        }
        else
        {
            /* insert last or middle */
            insertListNodeAfter(
                &g_slave_list,
                (list_node_t *) new,
                (list_node_t *) nprev);
        }
    }

    return new;
}

int addSlaveToList(
    uint8_t     *nick,
    uin_t        uin,
    sl_flags_t   flags,
    uint8_t     *c_key,
    uint32_t     budId,
    uint32_t     grpId,
    uint16_t     budType,
    slave_hnd_t *newHnd)
{
    slave_t   *new = NULL, *res = NULL;
    uint32_t   x = 0, keylen = 0;
    int32_t    retval = 0;
    uint8_t    StringUIN[MAX_UIN_LEN+1], goodKey[64];
    hnd_t      hnd;

//    DEBUG_PRINT("%s (#%ld)", nick, uin);

    if (uin < 10000) /* why? */
    {
        DEBUG_PRINT("%s has invalid UIN %ld -- skip it", nick, uin);
        return -1;
    }

    /* First Seek if theres another Slave with the same UIN (Duh!) */
    if ((res = querySlave(SLAVE_UIN, NULL, uin, 0)) != NULL)
    {
        /* User already exists. Since this function is also
         * called for downloaded slaves, a slave might be stored
         * in the config AND in the server. So if its a downloaded
         * slave just update the information from the config one
         * else return NULL, it already exists
         */
        if (!(flags & FL_DOWNLOADED))
            return -1;

        /* Set it as downloaded! */
        res->flags |= FL_DOWNLOADED;

        /* Updating User Special properties */
        switch (budType)
        {
            case YSM_BUDDY_SLAVE_VIS:               /* Buddy Visible */
                res->budType.visibleId = budId;
                break;

            case YSM_BUDDY_SLAVE_INV:               /* Buddy Invisible */
                res->budType.invisibleId = budId;
                break;

            case YSM_BUDDY_SLAVE_IGN:               /* Buddy ignored */
                res->budType.ignoreId = budId;
                break;

            default:                                /* Buddy Normal */
                break;
        }

        /* set its buddy id */
        res->budType.budId = budId;

        /* set its group id (required when deleting from srv) */
        res->budType.grpId = grpId;

        /* update nick if we should */
        if (g_cfg.updateNicks)
        {
            strncpy(res->info.nickName, nick, sizeof(res->info.nickName)-1);
        }

        return -1;
    }

    /* Now with the same nick (Conflict!) */
    /* XXX: alejo: shouldn't we warn the user ? */
    if (querySlave(SLAVE_NICK, nick, 0, 0) != NULL)
    {
        memset(StringUIN, 0, MAX_UIN_LEN+1);
        snprintf(StringUIN, MAX_UIN_LEN+1, "%d", (int)uin);
        nick = &StringUIN[0];
    }

    new = (slave_t *) YSM_CALLOC(1, sizeof(slave_t));
    new->uin = uin;

    /* no need to end in zero, called Calloc before */
    strncpy(new->info.nickName, nick, sizeof(new->info.nickName) - 1);

    new->status = STATUS_OFFLINE;
    new->flags = flags;
    new->d_con.rSocket = -1;
    
    /* set its buddy id */ 
    new->budType.budId = budId;
    /* set its group id (required when deleting from server) */
    new->budType.grpId = grpId;

    /* set the encryption key if any */
    if (c_key != NULL)
    {
        /* no need to end in zero, called Calloc before */
        strncpy(new->crypto.strkey, c_key, sizeof(new->crypto.strkey) - 1);

        /* ACTION:
         *    since the key might be smaller than 64 bytes long
         *    we make sure the final key is at least 64 bytes long by
         *    repeating the key n amount of times as neccesary.
         */

        keylen = strlen(new->crypto.strkey);

        for (x = 0; x < sizeof(goodKey); x++)
            goodKey[x] = new->crypto.strkey[x % keylen];

        /* ACTION:
         *    Try to make both in and out keys.
         *    At this point we can have a valid or invalid key
         *     either because the user added it manually to the
         *     configuration file or due to whatever reason.
         */

        retval = makeKey(&new->crypto.key_out, DIR_ENCRYPT, 256, goodKey);

        if (retval == TRUE)
        {
            /* OUT key instance created successfully.
             * Proceed to create the second key */

            retval = makeKey(&new->crypto.key_in, DIR_DECRYPT, 256, goodKey);
        }

        /* ACTION:
         *    Check if any of the keys failed creating. We don't mind
         *    telling at this point which of them failed. The user
         *    doesn't really care.
         */

        if (TRUE != retval)
        {
            switch (retval)
            {
                case BAD_KEY_DIR:
                    /* bad key direction */
                case BAD_KEY_MAT:
                    /* key material length is incorrect */
                case BAD_KEY_INSTANCE:
                    /* invalid supplied key */
                default:
                    /* unknown */
                    break;
            }

            printfOutput(VERBOSE_BASE,
                "Error setting cipher key for slave: %s.\n"
                "Please check the key meets the requirements by"
                "\nusing the 'help key' command.\n",
                new->info.nickName
                );
        }
    }

    /* Updating User Special properties */
    switch (budType)
    {
        case YSM_BUDDY_SLAVE_VIS:    /* Buddy Visible */
            new->budType.visibleId = budId;
            break;

        case YSM_BUDDY_SLAVE_INV:    /* Buddy Invisible */
            new->budType.invisibleId = budId;
            break;

        case YSM_BUDDY_SLAVE_IGN:    /* Buddy ignored */
            new->budType.ignoreId = budId;
            break;

        default:            /* Buddy Normal */
            break;
    }

    insertSlaveNode(new);
    hnd = linkHandle(g_slave_hnd_pool, new);

    if (hnd != 0)
    {
        deleteSlaveFromList(new->uin);
        return -1;
    }

    new->hnd = hnd;

    if (newHnd != 0)
    {
        newHnd->hnd = hnd;
        newHnd->uin = new->uin;
    }

    return 0;
}

void deleteSlaveFromList(slave_hnd_t *hnd)
{
    slave_t *victim;

    victim = normalizeSlaveHandle(hnd);

    if (victim)
    {
        unlinkHandle(g_slave_hdl_pool, victim->hnd);
        /* Free the slave from the linked list of slaves */
        deleteListNode(&g_slave_list, (list_node_t *)victim);
    }
}

uint32_t getSlavesListLen()
{
    return g_slave_list.length;
}

int updateSlave(slave_update_t type, uint8_t *nick, uin_t uin)
{
    slave_t *slave = NULL;

    if (type == UPDATE_NICK)
    {
        /* Can't rename to an existing name */
        slave = querySlave(SLAVE_NICK, nick, 0, 0);
        if (slave) return -1;
    }

    slave = querySlave(SLAVE_UIN, NULL, uin, 0);
    if (!slave) return -1;

    /* We remove the slave from the config file but not from memory */
    deleteSlaveFromDisk(slave->uin);

    /* We update the information on memory */
    switch (type)
    {
        case UPDATE_NICK:
            strncpy(
                slave->info.nickName,
                nick,
                sizeof(slave->info.nickName) - 1);

            slave->info.nickName[sizeof(slave->info.nickName)-1] = '\0';
            break;

        default:
            break;
    }

    /* We re-add the slave in the config file with the new data */
    addSlaveToDisk(slave);

    return 0;
}

int getSlaveStatus(slave_hnd_t *hnd, sl_status_t *status)
{
    slave_t *victim = NULL;

    if (victim = normalizeSlaveHandle(hnd))
    {
        *status = victim->status;
        return 0;
    }

    return -1;
}

int getSlaveCapabilities(slave_hnd_t *hnd, sl_caps_t *caps);
{
    slave_t *victim = NULL;

    if (victim = normalizeSlaveHandle(hnd))
    {
        *caps = victim->caps;
        return 0;
    }

    return -1;
}

int getSlaveFingerprint(slave_hnd_t *hnd, sl_fprint_t *fprint);
{
    slave_t *victim = NULL;

    if (victim = normalizeSlaveHandle(hnd))
    {
        *fprint = victim->fprint;
        return 0;
    }

    return -1;
}

int getSlaveFlags(slave_hnd_t *hnd, sl_flags_t *status);
{
    slave_t *victim = NULL;

    if (victim = normalizeSlaveHandle(hnd))
    {
        *flags = victim->flags;
        return 0;
    }

    return -1;
}

int getSlaveSpecialStatus(slave_hnd_t *hnd, buddy_special_status_t *bss);
