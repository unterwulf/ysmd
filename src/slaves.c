#include "ysm.h"
#include "ylist.h"
#include "slaves.h"
#include "wrappers.h"
#include "toolbox.h"
#include "setup.h"
#include "ystring.h"
#include "output.h"
#include "icqv7.h"

static pthread_mutex_t  g_slave_list_mutex = PTHREAD_MUTEX_INITIALIZER;
static list_t           g_slave_list = { NULL, 0 };

int initSlaveList()
{
}

void freeSlaveList()
{
    freeList(&g_slave_list);
}

void lockSlaveList()
{
    pthread_mutex_lock(&g_slave_list_mutex);
}

void unlockSlaveList()
{
    pthread_mutex_unlock(&g_slave_list_mutex);
}

int updateSlave(slave_update_t type, char *nick, uin_t uin)
{
    slave_t *slave = NULL;

    lockSlaveList();

    if (type == UPDATE_NICK)
    {
        /* Can't rename to an existing name */
        slave = getSlaveByNick(nick);
        if (slave) return -1;
    }

    slave = getSlaveByUin(uin);
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
    unlockSlaveList();

    return 0;
}

slave_t *getNextSlave(const slave_t *slave)
{
    if (slave == NULL)
        return (slave_t *)g_slave_list.start;
    else
        return (slave_t *)slave->suc;
}

slave_t *getSlaveByUin(uin_t uin)
{
    slave_t *node = NULL;

    for (node = (slave_t *) g_slave_list.start;
         node != NULL;
         node = (slave_t *) node->suc)
    {
        if (node->uin == uin)
        {
            return node;
        }
    }

    return NULL;
}

slave_t *getSlaveByNick(const char *nick)
{
    slave_t *node = NULL;

    for (node = (slave_t *) g_slave_list.start;
         node != NULL;
         node = (slave_t *) node->suc)
    {
        if (strcmp(node->info.nickName, nick) == 0)
        {
            return node;
        }
    }

    return NULL;
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

slave_t *addSlaveToList(
    char        *nick,
    uin_t        uin,
    sl_flags_t   flags,
    uint8_t     *c_key,
    uint32_t     budId,
    uint32_t     grpId,
    uint16_t     budType)
{
    slave_t   *new = NULL, *res = NULL;
    uint32_t   x = 0, keylen = 0;
    int32_t    retval = 0;
    uint8_t    StringUIN[MAX_UIN_LEN+1], goodKey[64];

//    DEBUG_PRINT("%s (#%ld)", nick, uin);

    if (uin < 10000) /* why? */
    {
        DEBUG_PRINT("%s has invalid UIN %ld -- skip it", nick, uin);
        return NULL;
    }

    /* First Seek if theres another Slave with the same UIN (Duh!) */
    if ((res = getSlaveByUin(uin)) != NULL)
    {
        /* User already exists. Since this function is also
         * called for downloaded slaves, a slave might be stored
         * in the config AND in the server. So if its a downloaded
         * slave just update the information from the config one
         * else return NULL, it already exists
         */
        if (!(flags & FL_DOWNLOADED))
            return NULL;

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

        return NULL;
    }

    /* Now with the same nick (Conflict!) */
    /* XXX: alejo: shouldn't we warn the user ? */
    if (getSlaveByNick(nick) != NULL)
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

    return insertSlaveNode(new);
}

void deleteSlaveFromList(slave_t *victim)
{
    if (victim)
    {
        /* Free the slave from the linked list of slaves */
        deleteListNode(&g_slave_list, (list_node_t *)victim);
    }
}

long getSlavesListLen()
{
    return g_slave_list.length;
}
