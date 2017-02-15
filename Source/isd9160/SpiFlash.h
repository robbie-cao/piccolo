#ifndef __SPI_FLASH_H__
#define __SPI_FLASH_H__

#include <stdio.h>
#include "ISD9xx.h"

#include "Lib\libSPIFlash.h"


// SPI-flash Base and initialization
#ifndef SPI0_BASE
#define SPI0_BASE (0x40000000 + 0x30000)
#endif // SPI0_BASE
#define LDO_ON

void SpiFlash_Init(void);
void SpiFlash_LdoOn(void);
void SpiFlash_Open(void);
void SpiFlash_PowerDown(void);

extern const SFLASH_CTX g_SPIFLASH;

#endif /* __SPI_FLASH_H__ */

/* vim: set ts=4 sw=4 tw=0 list : */
