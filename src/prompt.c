#include "ysm.h"
#include "commands.h"
#include "bytestream.h"
#include "network.h"
#include "icqv7.h"
#include "charset.h"
#include "direct.h"
#include "prompt.h"
#include "wrappers.h"
#include "toolbox.h"
#include "slaves.h"
#include "setup.h"
#include "crypt.h"
#include "output.h"
#include "timers.h"

extern void cmdHELP(uint16_t argc, int8_t **argv);
extern void cmdFILESTATUS(uint16_t argc, int8_t **argv);
extern void cmdCHAT(uint16_t argc, int8_t **argv);

/* only ansi stringz messages for this function.
 * encryption, if neccesary, is done inside.
 * if verbous, print any messages.
 */

void sendMessage(const slave_t *victim, char *data, bool_t verbose)
{
    int32_t      dataLen = 0;
    sl_flags_t   flags = 0;
    keyInstance *crypt_key = NULL;
    char        *dataPtr = data;
    char        *newDataPtr = NULL;

    DEBUG_PRINT("");

    if (victim->caps & CAPFL_UTF8)
        flags |= MFLAGTYPE_UTF8;

    /* Charset Convertion Time! */
    dataLen = strlen(dataPtr);
    YSM_Charset(CHARSET_OUTGOING, dataPtr, &dataLen, &newDataPtr, flags);
    if (newDataPtr != NULL)
    {
        dataPtr = newDataPtr;
    }

    /* encrypt the message if neccesary */
    newDataPtr = dataPtr;
    crypt_key = YSM_EncryptAnyData(victim, &newDataPtr, &dataLen, MAX_MSGDATA_LEN);
    if (newDataPtr != dataPtr)
    {
        if (dataPtr != data)
            YSM_FREE(dataPtr);

        dataPtr = newDataPtr;
    }

    flags |= MFLAGTYPE_NORM;

    sendMessage2Client(victim,
            (victim->caps & CAPFL_SRVRELAY) ? 0x02 : 0x01,
            YSM_MESSAGE_NORMAL,
            dataPtr,
            dataLen,
            0x00,
            flags,
            rand() & 0xffffff7f);

    if (verbose)
    {
        printfOutput(VERBOSE_BASE,
                "OUT MSG %ld %s %s %s\n",
                victim->uin, victim->info.nickName, strStatus(victim->status),
                (crypt_key != NULL ? "CRYPT" : ""));
    }

    /* what do we need to free? */
    if (dataPtr != data)
    {
        YSM_FREE(dataPtr);
    }
}

void YSM_ParseCommand(int8_t *_input, int32_t *argc, int8_t *argv[])
{
    char *aux = NULL, *input = NULL, *tmp = NULL, *last = NULL;
    int8_t fl = FALSE, stop_filtering_spaces = FALSE;
    uint32_t y = 0, z = 0;
    int32_t x = 0;

    input = last = tmp = _input;
    *argc = 0;

    while ((aux = (char *)strchr(input, ' ')) != NULL) {
        if(!fl) argv[*argc] = input;
        input = aux;
        *input = '\0';
        input++;

        if (!strlen(tmp) && !(*tmp)) {
            *tmp = 0x20;
            last = argv[*argc];
            fl = TRUE;
        } else {
            fl = FALSE;
            last = input;
            (*argc)++;
        }

        tmp = input;
    }

    argv[*argc] = last;

    /* Search for empty arguments.
     * The following lines discard those argv arguments which
     * only have spaces in them. We consider them to be empty.
     */

    while (x <= *argc) {

        for (y = 0; y < strlen(argv[x]); y++) {
            if (argv[x][y] != 0x20) goto while_loop_goto_sucks;
            else continue;
        }

        /* empty argv[x] found */
        (*argc)--;

while_loop_goto_sucks:
        x++;
    }

    /* Search for spaced arguments. (only searching cmd + first arg)
     * The following lines remove the beginning spaces from an
     * argument. These situations are usually user mistakes, because
     * its always the USER'S FAULT. ;)
     */

    x = 0;
    while (x <= *argc && x <= 1) {
        for (y = 0, z = 0; y < strlen(argv[x]); y++) {
            if (argv[x][y] != 0x20 || stop_filtering_spaces) {
                argv[x][z] = argv[x][y];
                z++;
                stop_filtering_spaces = TRUE;
            }
            else continue;
        }

        /* finish the new argument with a zero */
        argv[x][z] = 0x00;
        x++;
        stop_filtering_spaces = FALSE;
    }
}

void YSM_DoCommand(char *cmd)
{
    int8_t      *argv[MAX_CMD_ARGS];
    int32_t      argc = 0;

    DEBUG_PRINT("cmd: %s", cmd);

    if (!strlen(cmd)) return;

    memset(argv, 0, sizeof(argv));

    YSM_ParseCommand(cmd, &argc, argv);

    if (!doCommand(argc, argv))
    {
        printfOutput(VERBOSE_BASE, "%s: command not found\n", argv[0]);
    }
}

void YSM_DoChatCommand(int8_t *cmd)
{
    slave_t *victim = NULL;
    uint8_t  flags = 0;

    if (cmd == NULL || cmd[0] == '\0') return;

    /* what command do we have? */
    if (!strcasecmp(cmd, "ch") || !strcasecmp(cmd, "chat"))
    {
        cmdCHAT(0, NULL);
        return;
    }

    /* loop through the slaves list and send the message only to
     * those who have the FL_CHAT marked */
    while (victim = getNextSlave(victim))
    {
        if (victim->flags & FL_CHAT)
        {
            sendMessage(victim, cmd, FALSE);
        }
    }
}

static int32_t parseMessageData(uint8_t *data, uint32_t length)
{
    uint32_t x = 0;

    if (!length)
        return 0;

    for (x = 0; x <= length-1; x++)
    {
        /* lets make all < 32 ascii characters
         * be replaced by a space. but the ones we need :P
         */
        if (data[x] == '\0') continue;    /* NUL */
        if (data[x] == 0x08) continue;    /* backspace */
        if (data[x] == '\t') continue;    /* TAB */
        if (data[x] == '\r') continue;    /* LF */
        if (data[x] == '\n') continue;    /* CR */

        if (data[x] < 32) data[x] = 0x20;
    }

    /* by now we just keep the same length going */
    return length;
}

static void YSM_PreIncoming(msg_t *msg, keyInstance **key)
{
    int8_t *data_conv = NULL, do_conv = 0;

    /* procedures applied to normal messages only */
    if (msg->type == YSM_MESSAGE_NORMAL)
    {
        /* decrypt the incoming message if neccesary.
         * the function will return < 0 if decryption failed.
         * though it might be because we don't have a key with
         * this contact (or we don't have her on our list).
         */

        int msgLen = strlen(getString(msg->data));

#if 0
        if (YSM_DecryptMessage(msg->sender, msg->data, msg->len, key) >= 0)
        {
            /* if decryption went ok, do convertion */
            do_conv = 1;
        }

        if (do_conv)
        {
            /* Charset convertion time */
            YSM_Charset( CHARSET_INCOMING,
                *msg->data,
                msgLen,
                &data_conv,
                msg->flags);

            if (data_conv != NULL)
            {
                /* msgData is freed in the callers function */
                *msg->data = data_conv;
            }
        }
#endif

        /* filter unwanted characters */
        parseMessageData(getString(msg->data), msgLen);
#if 0
        /* set again the data length after the convertion */
        (*msgLen) = strlen(*msgData)+1;
#endif
    }
}

static void YSM_PostIncoming(msg_t *msg)
{
    if (msg->type == YSM_MESSAGE_NORMAL)
    {
        /* do we have to send messages? either CHAT or FORWARD? */

        if (g_state.promptFlags & FL_CHATM)
        {
            if (!(msg->sender->flags & FL_CHAT))
            {
                /* only send the reply to those who don't
                 * belong to my chat session! */

                sendMessage(msg->sender, g_cfg.CHATMessage, FALSE);
            }
        }
    }
}

void displayMessage(msg_t *msg)
{
    char         *aux = NULL, *auxb = NULL;
    keyInstance  *crypt_key = NULL;
    int           x = 0;

    if (msg == NULL || msg->sender == NULL)
        return;
    
    DEBUG_PRINT("from %ld (%s)", msg->sender->uin, msg->sender->info.nickName);

    YSM_PreIncoming(msg, &crypt_key);

    /* are we in CHAT MODE ? we dont print messages which don't belong
     * to our chat session!. */
    if (g_state.promptFlags & FL_CHATM)
    {
        if (!(msg->sender->flags & FL_CHAT))
            goto displaymsg_exit;
    }

    switch (msg->type)
    {
        case YSM_MESSAGE_NORMAL:
            printfOutput(VERBOSE_BASE,
                "IN MSG %ld %s %s\n%s\n",
                msg->sender->uin, msg->sender->info.nickName,
                (crypt_key != NULL ? "CRYPT" : ""),
                getString(msg->data));
            break;

#if 0
        case YSM_MESSAGE_PAGER:
            strtok(msg->data, "IP: ");
            aux = strtok(NULL, "\n");
            auxb = strtok(NULL, "");
            printfOutput( VERBOSE_BASE, "\r%s - %s\n"
                "< Message: %s >\n", MSG_INCOMING_PAGER, aux, auxb);
            break;

        case YSM_MESSAGE_URL:
            auxb = strchr(msg->data, 0xfe);
            if(auxb != NULL) {
                *auxb = '\0';
                aux = strtok(auxb+1, "");
                auxb = &msgData[0];

            } else {
                aux = &msgData[0];
                auxb = "No description Provided";
            }

            printfOutput( VERBOSE_BASE ,"\r%s - from: %s - UIN: %d\n"
                "< url: %s >\n< description: %s >\n"
                "[TIP: you may now use the 'burl' command with a '!'\n"
                "[as parameter to trigger your browser with the last\n"
                "[received URL.\n",
                MSG_INCOMING_URL,
                msg->sender->info.nickName, msg->sender->uin, aux, auxb);

            break;

        case YSM_MESSAGE_CONTACTS:
        {
            int msgLen = strlen(getString(msg->data));

            for (x = 0; x < msgLen; x++)
            {
                if ((unsigned char)msg->data[x] == 0xfe)
                        msg->data[x] = 0x20;
            }

            printfOutput( VERBOSE_BASE,
                "\n\rIncoming CONTACTS from: "
                "%s [ICQ# %d].\n",
                msg->sender->info.nickName, msg->sender->uin);

            strtok(msg->data, " ");    /* amount */
            x = 0;

            aux = strtok(NULL, " ");
            while (aux != NULL && x < atoi(msg->data))
            {
                printfOutput( VERBOSE_BASE,
                    "\r" "UIN %-14.14s\t", aux );

                aux = strtok(NULL, " ");
                if (!aux) printfOutput( VERBOSE_BASE,
                        "Nick Unknown\n");

                else printfOutput( VERBOSE_BASE,
                        "Nick %s\n", aux);

                aux = strtok(NULL, " ");
                x++;
            }
            break;
        }
#endif

        case YSM_MESSAGE_AUTH:
            printfOutput( VERBOSE_BASE, "\r%s UIN #%d "
                "(Slave: %s).\nThe following Message arrived with the"
                " request: %s\n",
                MSG_INCOMING_AUTHR,
                msg->sender->uin, msg->sender->info.nickName,
                getString(msg->data));
            break;

        case YSM_MESSAGE_ADDED:
            printfOutput( VERBOSE_BASE,
                "\r%s ICQ #%d just added You to the list."
                " (Slave: %s).\n", MSG_WARN_ADDED,
                msg->sender->uin, msg->sender->info.nickName);
            break;

        case YSM_MESSAGE_AUTHOK:
            printfOutput(VERBOSE_BASE, "IN AUTH_OK %ld\n", msg->sender->uin);
            break;

        case YSM_MESSAGE_AUTHNOT:
            printfOutput(VERBOSE_BASE, "IN AUTH_DENY %ld\n", msg->sender->uin);
            break;

        default:
            YSM_ERROR(ERROR_CODE, 1);
            break;
    }

displaymsg_exit:
    YSM_PostIncoming(msg);
}
