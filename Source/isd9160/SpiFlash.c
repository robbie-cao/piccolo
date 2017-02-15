/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                         */
/* Copyright(c) 2009 Nuvoton Technology Corp. All rights reserved.                                         */
/*                                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
#include <stdio.h>

#include "ISD9xx.h"

#include "Lib\libSPIFlash.h"

#include "SpiFlash.h"
#include "Conf.h"
#include "Log.h"


static void SpiFlash_SetCS(const struct tagSFLASH_CTX *ctx, int csOn)
{
    // Use the SPI controller's CS control
}

const SFLASH_CTX g_SPIFLASH =
{
    SPI0_BASE,
    1,                              // Channel 0, Auto
    SpiFlash_SetCS
};

#define SPIFLASH_CTX(arg) (&g_SPIFLASH)

void SpiFlash_Init(void)
{
    SYS->GPA_ALT.GPA0        = 1;   // MOSI0
    SYS->GPA_ALT.GPA2        = 1;   // SSB0
    SYS->GPA_ALT.GPA1        = 1;   // SCLK
    SYS->GPA_ALT.GPA3        = 1;   // MISO0

    SYSCLK->APBCLK.SPI0_EN   = 1;
    SYS->IPRSTC2.SPI0_RST    = 1;
    SYS->IPRSTC2.SPI0_RST    = 0;

    sflash_set(&g_SPIFLASH);
}

void SpiFlash_LdoOn(void)
{
    UNLOCKREG();
    SYSCLK->APBCLK.ANA_EN = 1;
    ANA->LDOPD.PD = 0;
}

void SpiFlash_Open(void)
{
    int iRet;

#ifdef LDO_ON
    SpiFlash_LdoOn();
#endif

    SpiFlash_Init();
    LibSPIFlash_ReleasePD(&g_SPIFLASH);
    iRet = sflash_getid(&g_SPIFLASH);
    LOG("Device ID: %x\r\n", iRet);
}

void SpiFlash_PowerDown(void)
{
    LibSPIFlash_PowerDown(&g_SPIFLASH);
}

/* vim: set ts=4 sw=4 tw=0 list : */
