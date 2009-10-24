#include "ysm.h"
#include "toolbox.h"
#include "wrappers.h"
#include "prompt.h"
#include "output.h"
#include "slaves.h"

void initStranger(slave_t *strg, uin_t uin)
{
    memset(strg, '\0', sizeof(*strg));
    strg->uin = uin;
    strg->status = STATUS_UNKNOWN;
}
