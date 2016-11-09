/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                         */
/* Copyright(c) 2009 Nuvoton Technology Corp. All rights reserved.                                         */
/*                                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
/* Description:                                                                                            */
/*   This is a sample code for reference on using the API LibSPIFlash.lib                                  */
/*   It defines read/write buffer for writting to SPI flash and read back                                  */
/*   First the code get the SPI-flash ID then erase the SPI flash of specified size then check erase 	   */
/*   result. 																							   */
/*   Once erase is success. It will write data from WriteBuffer to SPI flash then read back to ReadBuffer  */
/*   then compare.																						   */
/*---------------------------------------------------------------------------------------------------------*/
#include <stdio.h>
#include "ISD9xx.h"
#include "Driver\DrvPDMA.h"
#include "Driver\DrvDPWM.h"
#include "Driver\DrvADC.h"
#include "Driver\DrvALC.h"
#include "Driver\DrvUART.h"
#include "Driver\DrvGPIO.h"
#include "Driver\DrvSYS.h"
#include "Driver\DrvFMC.h"
#include "Driver\DrvSPI.h"

#include "Lib\LibSPIFlash.h"	  

/*---------------------------------------------------------------------------------------------------------*/
/* Define Function Prototypes                                                                              */
/*---------------------------------------------------------------------------------------------------------*/
void InitialUART(void);
void InitialADC(void);



/*---------------------------------------------------------------------------------------------------------*/
/* Define global variables                                                                                 */
/*---------------------------------------------------------------------------------------------------------*/
#define BUFFER_SAMPLECOUNT 	0x1000		//256 samples
#define RECORD_START_ADDR	0x0	

//#define LDO_ON		



//====================
// Functions & main

void UartInit(void)
{
	/* Reset IP */
	SYS->IPRSTC2.UART0_RST = 1;
	SYS->IPRSTC2.UART0_RST = 0;

	/* Enable UART clock */
    SYSCLK->APBCLK.UART0_EN = 1;
 
    /* Data format */
    UART0->LCR.WLS = 3;

    /* Configure the baud rate */
    M32(&UART0->BAUD) = 0x3F0001A8; /* Internal 48MHz, 115200 bps */


    /* Multi-Function Pin: Enable UART0:Tx Rx */
	SYS->GPA_ALT.GPA8 = 1;
	SYS->GPA_ALT.GPA9 = 1;
 
}

/*---------------------------------------------------------------------------------------------------------*/
/* SysTimerDelay                                                                                           */
/*---------------------------------------------------------------------------------------------------------*/
void SysTimerDelay(uint32_t us)
{
    SysTick->LOAD = us * 49; /* Assume the internal 49MHz RC used */
    SysTick->VAL  =  (0x00);
    SysTick->CTRL = (1 << SYSTICK_CLKSOURCE) | (1<<SYSTICK_ENABLE);

    /* Waiting for down-count to zero */
    while((SysTick->CTRL & (1 << 16)) == 0);
}

void LedOn(void)
{
	DrvGPIO_ClrBit(GPA,15);	
	SysTimerDelay(100000);
	DrvGPIO_SetBit(GPA,15);
}

void LdoOn(void)
{
	SYSCLK->APBCLK.ANA_EN=1;
	ANA->LDOPD.PD=0;
	ANA->LDOSET=3;		//Set LDO output @ 3.3V
}

//================================
// SPI-flash
#ifndef SPI0_BASE
#define SPI0_BASE (0x40000000 + 0x30000)
#endif // SPI0_BASE


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
//Enable SPI shared function pins
	SYS->GPA_ALT.GPA0              = 1; // MOSI0
	SYS->GPA_ALT.GPA2              = 1; // SSB0
	SYS->GPA_ALT.GPA1              = 1; // SCLK
	SYS->GPA_ALT.GPA3              = 1; // MISO0

//Reset the ISD9160 SPI
	SYSCLK->APBCLK.SPI0_EN        = 1;
	SYS->IPRSTC2.SPI0_RST         = 1;
	SYS->IPRSTC2.SPI0_RST         = 0;	

	// spi flash
	LibSPIFlash_Open(&g_SPIFLASH);	 	//Default using PCLK/4, here is 12MHz. Using 24MHz needs care on board loading design
}




/*---------------------------------------------------------------------------------------------------------*/
/*  Main Function									                                           			   */
/*---------------------------------------------------------------------------------------------------------*/
__align(4) 	uint8_t ReadBuffer[BUFFER_SAMPLECOUNT],WriteBuffer[BUFFER_SAMPLECOUNT];

int32_t main (void)
{

//__align(4) 	uint8_t ReadBuffer[BUFFER_SAMPLECOUNT],WriteBuffer[BUFFER_SAMPLECOUNT];  
	uint32_t u32temp, u32ErrorCount, ReadBufferAddr, WriteBufferAddr;
	uint32_t   u32ReadAddr,u32ReadbackData,u32EraseStartAddr,u32EraseLength;
	int		iRet;

 	ReadBufferAddr=(uint32_t) &ReadBuffer;
	WriteBufferAddr=(uint32_t) &WriteBuffer;
	
	   
	UNLOCKREG();
	SYSCLK->PWRCON.OSC49M_EN = 1;
	SYSCLK->CLKSEL0.HCLK_S = 0; /* Select HCLK source as 48MHz */ 
	SYSCLK->CLKDIV.HCLK_N  = 0;	/* Select no division          */
	SYSCLK->CLKSEL0.OSCFSel = 0;	/* 1= 32MHz, 0=48MHz */
	LOCKREG();

#ifdef LDO_ON
	LdoOn();
#endif


/* Set UART Configuration */
    UartInit();

//--------------------------------------------//
//	Initial SPI-flash interface
 
    printf("+-------------------------------------------------------------------------+\n");
    printf("|       SPI-Flash sampling											      |\n");
    printf("+-------------------------------------------------------------------------+\n");
    printf("\n=== SPI Flash W/R test ===\n");

	u32temp=0;
	SpiFlashInit();
	u32temp= LibSPIFlash_GetID(&g_SPIFLASH);
	printf("\n Manufacture ID: %2X \n", u32temp>>16);
	printf("\n Device ID: %4X \n", u32temp&0xFFFF);
	if ((u32temp==0)||(u32temp==0xFFFFFF))
		goto Error;




//#define ERASE_ALL			  	//Erase whole SPI flash
#ifdef ERASE_ALL
	iRet=LibSPIFlash_EraseAll(&g_SPIFLASH);
#else
	u32EraseStartAddr=0;
	u32EraseLength= ((BUFFER_SAMPLECOUNT>>12)+1)<<12;	//Erase whole sector when it occpuies partial of a secotor, programmer needs to care the correct data in this sector
														//u32EraseStartAddr & u32EraseLength should be multiple of 4096. From u32EraseStartAddr to erase  Erase unit is 4096B
	iRet=LibSPIFlash_Erase(&g_SPIFLASH, u32EraseStartAddr, u32EraseLength);	
#endif


	//Check Erase result
	u32ErrorCount=0;
	LibSPIFlash_FastRead(&g_SPIFLASH, RECORD_START_ADDR,  (uint32_t *)ReadBufferAddr, BUFFER_SAMPLECOUNT);
	for (u32temp=0; u32temp<BUFFER_SAMPLECOUNT; u32temp++)
	{
		if(ReadBuffer[u32temp]!=0xFF)				//for ReadBuffer[] uint8_t definition
		//if((ReadBuffer[u32temp]&0xFF)!=0xFF)		//for ReadBuffer[] int8_t definition
			u32ErrorCount++;
	}
	if	(u32ErrorCount==0)
		printf("\n Erase OK \n");
	else
		printf("\n Erase failure Count %x \n", u32ErrorCount);



	//Write ready buffer data, erase before write. 
	for (u32temp=0; u32temp<BUFFER_SAMPLECOUNT; u32temp++)
		WriteBuffer[u32temp]=u32temp;
	LibSPIFlash_Write(&g_SPIFLASH, RECORD_START_ADDR,  (uint32_t *) WriteBufferAddr, BUFFER_SAMPLECOUNT);


	//Read back data to buffer
	LibSPIFlash_FastRead(&g_SPIFLASH, RECORD_START_ADDR,  (uint32_t *)ReadBufferAddr, BUFFER_SAMPLECOUNT);


	//Compare Write/Read Buffer
	u32ErrorCount=0;
	for (u32temp=0; u32temp<BUFFER_SAMPLECOUNT; u32temp++)
	{
		if(WriteBuffer[u32temp]!=ReadBuffer[u32temp])
			u32ErrorCount++;
	}



	if	(u32ErrorCount==0)
		printf("\n Comparison OK \n");
	else
		printf("\n Comparison failure Count %x \n", u32ErrorCount);

	goto Done;

Error:
	printf("\n SPI-Flash access failure \n");
Done:
	LedOn();
	printf("=== Test Done ===\n");

	while(1);

	/* Lock protected registers */
	
}

