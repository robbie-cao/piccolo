#ifndef __UTIL_H__
#define __UTIL_H__

#include "hal_types.h"

#define MAC_ADDR_STR_LEN      15
#define MAC_ADDR_DEMSTR_LEN   18

#define offset_of(s, m)       (size_t)&(((s *)0)->m)

void delayUS(uint16 us);
void delayMS(uint16 ms);

char *bdAddr2Str(uint8 *pAddr);
char *bdPartialAddr2Str(uint8 *pAddr);
char *bdAddr2DemStr(uint8 *pAddr);
char *arrHex2Str(uint8 *pArr, char *str, uint8 size);

uint8 getTxLevelFromDbm(int8 dbm);

#endif /* __UTIL_H__ */

/* vim: set ts=2 sw=2 tw=0 list : */
