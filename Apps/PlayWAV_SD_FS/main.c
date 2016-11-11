/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                         */
/* Copyright(c) 2009 Nuvoton Technology Corp. All rights reserved.                                         */
/*                                                                                                         */
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
#include "Driver\DrvTimer.h"

#include "disk_io.h"

#define BASE_ADDR_ON_SD	0	  	//Should be multiple of 8, it's better on the start of a sector (multiple of 512)
#define SD_SECTOR_SIZE	512	//unit: byte

#define AUDIOBUFFERSIZE     512
__align(4) int16_t AudioBuffer[2][AUDIOBUFFERSIZE];
uint32_t AudioSampleCount,PDMA1CallBackCount,AudioDataAddr,BufferEmptyAddr,BufferReadyAddr;
BOOL	PCMPlaying,BufferEmpty = TRUE,PDMA1Done;
uint32_t	AudioDataCount,u32DataSector = 0;


/*---------------------------------------------------------------------------------------------------------*/
/* Define Function Prototypes                                                                              */
/*---------------------------------------------------------------------------------------------------------*/
//void InitialUART(void);
void PDMA1_Callback(void);
//void PDMA1forDPWM(void);


/*---------------------------------------------------------*/
/* User Provided RTC Function for FatFs module             */
/*---------------------------------------------------------*/
/* This is a real time clock service to be called from     */
/* FatFs module. Any valid time must be returned even if   */
/* the system does not support an RTC.                     */
/* This function is not required in read-only cfg.         */

unsigned long get_fattime (void)
{
    unsigned long tmr;

    tmr=0x00000;

    return tmr;
}



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
/* InitialDPWM                                                                                             */
/*---------------------------------------------------------------------------------------------------------*/
void InitialDPWM(uint32_t u32SampleRate)
{
	DrvDPWM_Open();
	DrvDPWM_SetDPWMClk(E_DRVDPWM_DPWMCLK_HCLKX2);
	//DrvDPWM_SetDPWMClk(E_DRVDPWM_DPWMCLK_HCLK);
	DrvDPWM_SetSampleRate(u32SampleRate);
	DrvDPWM_Enable();
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



/*---------------------------------------------------------------------------------------------------------*/
/* Set PDMA0 to move ADC FIFO to MIC buffer with wrapped-around mode                                       */
/*---------------------------------------------------------------------------------------------------------*/
void PDMA1forDPWM(void)
{
    STR_PDMA_T sPDMA;

    sPDMA.sSrcAddr.u32Addr 			= BufferReadyAddr;
    sPDMA.sDestAddr.u32Addr 		= (uint32_t)&DPWM->FIFO;
    sPDMA.u8Mode 					= eDRVPDMA_MODE_MEM2APB;;
    sPDMA.u8TransWidth 				= eDRVPDMA_WIDTH_16BITS;
    sPDMA.sSrcAddr.eAddrDirection 	= eDRVPDMA_DIRECTION_WRAPAROUND;
    // FIXME:
    // INCREMENTED or WRAPPAROUND?
    //sPDMA.sSrcAddr.eAddrDirection 	= eDRVPDMA_DIRECTION_INCREMENTED;
    sPDMA.sDestAddr.eAddrDirection 	= eDRVPDMA_DIRECTION_FIXED;
    sPDMA.u8WrapBcr				 	= eDRVPDMA_WRA_WRAP_HALF_INT;; 		//Interrupt condition set fro Half buffer & buffer end
    //sPDMA.i32ByteCnt = AUDIOBUFFERSIZE * 2;	   	//Full MIC buffer length (byte)
    // FIXME:
    // 2 or 4?
    sPDMA.i32ByteCnt = AUDIOBUFFERSIZE * 4;	   	//Full MIC buffer length (byte), Wrap around
    DrvPDMA_Open(eDRVPDMA_CHANNEL_1, &sPDMA);

    // PDMA Setting
    //PDMA_GCR->PDSSR.ADC_RXSEL = eDRVPDMA_CHANNEL_2;
    DrvPDMA_SetCHForAPBDevice(
            eDRVPDMA_CHANNEL_1,
            eDRVPDMA_DPWM,
            eDRVPDMA_WRITE_APB
            );

    // Enable DPWM DMA
    DrvDPWM_EnablePDMA();
    // Enable INT
    DrvPDMA_EnableInt(eDRVPDMA_CHANNEL_1, eDRVPDMA_WAR); 	  		//For WARPROUND
    //DrvPDMA_EnableInt(eDRVPDMA_CHANNEL_1, eDRVPDMA_BLKD );
    // Install Callback function
    DrvPDMA_InstallCallBack(eDRVPDMA_CHANNEL_1, eDRVPDMA_WAR, (PFN_DRVPDMA_CALLBACK) PDMA1_Callback );     //For Wrap
    //DrvPDMA_InstallCallBack(eDRVPDMA_CHANNEL_1, eDRVPDMA_BLKD, (PFN_DRVPDMA_CALLBACK) PDMA1_Callback );
    DrvPDMA_CHEnablelTransfer(eDRVPDMA_CHANNEL_1);
}


/*---------------------------------------------------------------------------------------------------------*/
/* DPWM Callback                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
void PDMA1_Callback()
{
    if ((PDMA1CallBackCount&0x1)==0)
    {
        BufferReadyAddr=(uint32_t) &AudioBuffer[1][0];
        BufferEmptyAddr=(uint32_t) &AudioBuffer[0][0];
    }
    else
    {
        BufferReadyAddr=(uint32_t) &AudioBuffer[0][0];
        BufferEmptyAddr=(uint32_t) &AudioBuffer[1][0];
    }

	PDMA1CallBackCount++;

	BufferEmpty = TRUE;
}

void CopySdSoundData()
{
    //u32DataSector = AudioDataAddr / SD_SECTOR_SIZE;
    disk_read (0, (unsigned char *)BufferEmptyAddr, u32DataSector, 2);
    u32DataSector += 2;

    AudioSampleCount = AudioSampleCount + AUDIOBUFFERSIZE;
    AudioDataAddr = AudioDataAddr + (AUDIOBUFFERSIZE * 2);
}


/*---------------------------------------------------------------------------------------------------------*/
/* Play sound initialization                                                                               */
/*---------------------------------------------------------------------------------------------------------*/
void PlaySdSound(uint32_t DataSdAddr)
{
    InitialDPWM(16000);     // Sample rate to match audio data on SD card

    AudioDataAddr = DataSdAddr;
    PCMPlaying = TRUE;
    AudioDataCount = 0;
    PDMA1CallBackCount = 0;

    BufferEmptyAddr = (uint32_t) &AudioBuffer[0][0];
    CopySdSoundData();
    BufferReadyAddr = (uint32_t) &AudioBuffer[0][0];
    PDMA1forDPWM();

    BufferEmptyAddr = (uint32_t) &AudioBuffer[1][0];
    BufferEmpty = TRUE;
}



/*---------------------------------------------------------------------------------------------------------*/
/*  Main Function									                                           			   */
/*---------------------------------------------------------------------------------------------------------*/
int32_t main (void)
{
	UNLOCKREG();
	SYSCLK->PWRCON.OSC49M_EN = 1;
	SYSCLK->CLKSEL0.HCLK_S = 0; /* Select HCLK source as 48MHz */
	SYSCLK->CLKDIV.HCLK_N  = 0;	/* Select no division          */
	SYSCLK->CLKSEL0.OSCFSel = 0;	/* 1= 32MHz, 0=48MHz */

	/* Set UART Configuration */
    UartInit();

    DrvTIMER_Init();

    printf("+-------------------------------------------------------------------------+\n");
    printf("|       Playing PCM Raw Data										      |\n");
    printf("+-------------------------------------------------------------------------+\n");

    printf("\n=== Test PCM raw data on SD card to SPK ===\n");

    if ((DSTATUS)disk_initialize(0) != 0)
    {
        goto Error;
    }

#if 0
    // Test SD card reading speed
    {
        uint32_t u32Start = 0;
        uint32_t u32End = 0;
        uint32_t i = 0;

        DrvTIMER_Open(TMR0, 1000, PERIODIC_MODE);
        u32Start = DrvTIMER_GetTicks(TMR0);
        for (i = 0; i < 1024; i++) {
            disk_read (0, (unsigned char *)AudioBuffer, i, 1);
        }
        u32End = DrvTIMER_GetTicks(TMR0);
        DrvTIMER_Close(TMR0);
        printf("Read:\n");
        printf("Total Sector: %d\n", i);
        printf("Start Time  : %d\n", u32Start);
        printf("End Time    : %d\n", u32End);
    }
#endif

    DrvPDMA_Init();			//PDMA initialization
	UNLOCKREG();
	AudioDataAddr = BASE_ADDR_ON_SD;
	PlaySdSound(AudioDataAddr);

    while (PCMPlaying == TRUE)
    {
        if (BufferEmpty == TRUE)
        {
            CopySdSoundData();
            BufferEmpty = FALSE;
        }
        //PlaySdSound(AudioDataAddr);
    }

    while(BufferEmpty==FALSE);			 //Waiting last audio buffer played

    DrvPDMA_Close();
    UNLOCKREG();

	printf("Test Done\n");
	while(1);

Error:
    printf(" SD card Initialization fial\n");
    while(1);

	/* Lock protected registers */

}



