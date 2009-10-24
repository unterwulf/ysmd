#ifndef _DUMP_H_
#define _DUMP_H_

#include "ystring.h"

void hexDump(string_t *str, const uint8_t *buf, uint32_t len);
void hexDumpOutput(const uint8_t *buf, uint32_t len);

void dumpPacket(
    string_t *str,
    const flap_head_bit_t *flap,
    const uint8_t *data);

#endif /* _DUMP_H_ */
