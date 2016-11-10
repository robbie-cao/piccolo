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

#include "SDaccess.h"

#define BASE_ADDR_ON_SD	0	  	//Should be multiple of 8, it's better on the start of a sector (multiple of 512)
#define SD_SECTOR_SIZE	512	//unit: byte

//#include "PCM.h"
//extern const int16_t i16PCMData[PCM_LENGTH];

//extern uint32_t u32AudioDataBegin, u32AudioDataEnd;
uint32_t TOTAL_PCM_COUNT;  		// = PCM_LENGTH

#define AudioBufferSize     512
__align(4) int16_t AudioBuffer[2][AudioBufferSize];
uint32_t AudioSampleCount,PDMA1CallBackCount,AudioDataAddr,BufferEmptyAddr,BufferReadyAddr;
BOOL	PCMPlaying,BufferEmpty,PDMA1Done;
uint32_t	AudioDataCount,u32DataSector;


/*---------------------------------------------------------------------------------------------------------*/
/* Define Function Prototypes                                                                              */
/*---------------------------------------------------------------------------------------------------------*/
//void InitialUART(void);
void PDMA1_Callback(void);
//void PDMA1forDPWM(void);




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
	sPDMA.sSrcAddr.eAddrDirection 	= eDRVPDMA_DIRECTION_INCREMENTED;
	sPDMA.sDestAddr.eAddrDirection 	= eDRVPDMA_DIRECTION_FIXED;
	//sPDMA.u8WrapBcr				 	= 0x5; 		//Interrupt condition set fro Half buffer & buffer end
    sPDMA.i32ByteCnt = AudioBufferSize * 2;	   	//Full MIC buffer length (byte)
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
	DrvPDMA_EnableInt(eDRVPDMA_CHANNEL_1, eDRVPDMA_BLKD );
	// Install Callback function
	DrvPDMA_InstallCallBack(eDRVPDMA_CHANNEL_1, eDRVPDMA_BLKD, (PFN_DRVPDMA_CALLBACK) PDMA1_Callback );
	DrvPDMA_CHEnablelTransfer(eDRVPDMA_CHANNEL_1);

	PDMA1Done=FALSE;
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
	if (BufferEmpty==FALSE)
    {
        PDMA1forDPWM();
    }
	else
	{
		printf("Late to copy audio data to buffer for PDMA\n");
		PDMA1Done=TRUE;
	}

	BufferEmpty=TRUE;
}


/*---------------------------------------------------------------------------------------------------------*/
/* Copy Data from flash to SRAM                                                                            */
/*---------------------------------------------------------------------------------------------------------*/
void CopyFlash2RAM(uint32_t *Addr1,uint32_t *Addr2,uint32_t WordCount)
{
uint32_t	u32LoopCount;
	for(u32LoopCount=0;u32LoopCount<WordCount;u32LoopCount++ )
	{
		*Addr2++ = *Addr1++;
	}
}

void CopySoundData()
{

	CopyFlash2RAM((uint32_t *)AudioDataAddr,(uint32_t *)BufferEmptyAddr,(AudioBufferSize/2));

	AudioSampleCount=AudioSampleCount+AudioBufferSize;
	AudioDataAddr=AudioDataAddr+(AudioBufferSize*2);
	if (AudioSampleCount > TOTAL_PCM_COUNT)
		PCMPlaying=FALSE;
}

void CopySdSoundData()
{
    u32DataSector = AudioDataAddr / SD_SECTOR_SIZE;
    disk_read (0, (unsigned char *)AudioBuffer, u32DataSector, 1);

    AudioSampleCount = AudioSampleCount + AudioBufferSize;
    AudioDataAddr = AudioDataAddr + (AudioBufferSize*2);
#if 0
    if (AudioSampleCount > TOTAL_PCM_COUNT)
        PCMPlaying=FALSE;
#endif
}


/*---------------------------------------------------------------------------------------------------------*/
/* Play sound initialization                                                                               */
/*---------------------------------------------------------------------------------------------------------*/
void PlaySound(uint32_t DataAddr)
{
	InitialDPWM(8000);
	//TOTAL_PCM_COUNT= ((uint32_t)&u32AudioDataEnd-(uint32_t)&u32AudioDataBegin)/2;
	//TOTAL_PCM_COUNT = PCM_LENGTH;
	AudioDataAddr = DataAddr;
	PCMPlaying=TRUE;
	AudioSampleCount=0;
	PDMA1CallBackCount=0;

	BufferEmptyAddr= (uint32_t) &AudioBuffer[0][0];
	CopySoundData();
	BufferReadyAddr= (uint32_t) &AudioBuffer[0][0];
	PDMA1forDPWM();

	BufferEmptyAddr= (uint32_t) &AudioBuffer[1][0];
	BufferEmpty=TRUE;
}

void PlaySdSound(uint32_t DataSdAddr)
{
    InitialDPWM(44100);

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
//	uint8_t u8Option;
//	int32_t	i32Err;
//	uint32_t	u32temp;




	UNLOCKREG();
	SYSCLK->PWRCON.OSC49M_EN = 1;
	SYSCLK->CLKSEL0.HCLK_S = 0; /* Select HCLK source as 48MHz */
	SYSCLK->CLKDIV.HCLK_N  = 0;	/* Select no division          */
	SYSCLK->CLKSEL0.OSCFSel = 0;	/* 1= 32MHz, 0=48MHz */

	/* Set UART Configuration */
    UartInit();

    printf("+-------------------------------------------------------------------------+\n");
    printf("|       Playing 8K sampling	PCM										      |\n");
    printf("+-------------------------------------------------------------------------+\n");

    printf("\n=== Flash audio data To SPK test ===\n");

    if((DSTATUS)disk_initialize(0)!=0)
    {
        goto Error;
    }

    DrvPDMA_Init();			//PDMA initialization
	UNLOCKREG();
	AudioDataAddr = BASE_ADDR_ON_SD;
	PlaySdSound(AudioDataAddr);

#if 0
 	//printf("===================\n");
	//printf("  [q] Exit\n");
	while (PCMPlaying == TRUE)
	{
		if (BufferEmpty==TRUE)
		{
			CopySoundData();
			BufferEmpty=FALSE;
		}


		if ((PDMA1Done==TRUE) && (BufferEmpty==FALSE))
			PDMA1forDPWM();

	}

#else

    while (PCMPlaying == TRUE)
    {
        if (BufferEmpty == TRUE)
        {
            CopySdSoundData();
            BufferEmpty = FALSE;
        }


        if ((PDMA1Done == TRUE) && (BufferEmpty == FALSE))
        {
            PDMA1forDPWM();
        }

    }
#endif

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



