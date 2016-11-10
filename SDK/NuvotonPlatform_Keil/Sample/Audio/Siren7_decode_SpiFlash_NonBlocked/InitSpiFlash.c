/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                         */
/* Copyright(c) 2009 Nuvoton Technology Corp. All rights reserved.                                         */
/*                                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
#include <stdio.h>
#include "ISD9xx.h"

#include "Lib\libSPIFlash.h"


//====================================
// SPI-flash Base and initialization
#ifndef SPI0_BASE
#define SPI0_BASE (0x40000000 + 0x30000)
#endif // SPI0_BASE
#define LDO_ON

static void spi_setcs(const struct tagSFLASH_CTX *ctx, int csOn)
{
	// use the SPI controller's CS control
}

const SFLASH_CTX g_SPIFLASH =
{
	SPI0_BASE,
	1, // channel 0, auto
	spi_setcs
};

#define SPIFLASH_CTX(arg) (&g_SPIFLASH)

void SpiFlashInit(void)
{
	SYS->GPA_ALT.GPA0              = 1; // MOSI0
	SYS->GPA_ALT.GPA2              = 1; // SSB0
	SYS->GPA_ALT.GPA1              = 1; // SCLK
	SYS->GPA_ALT.GPA3              = 1; // MISO0

	SYSCLK->APBCLK.SPI0_EN        = 1;
	SYS->IPRSTC2.SPI0_RST         = 1;
	SYS->IPRSTC2.SPI0_RST         = 0;

	// spi flash
	sflash_set(&g_SPIFLASH);
}


void SpiFlashOpen(void)
{
int iRet;

#ifdef LDO_ON
	SYSCLK->APBCLK.ANA_EN=1;
	ANA->LDOPD.PD=0;
#endif

	SpiFlashInit();
	iRet= sflash_getid(&g_SPIFLASH);
	printf("\n Device ID %x \n", iRet);
	//iRet= sflash_canwrite(&g_SPIFLASH);
}

