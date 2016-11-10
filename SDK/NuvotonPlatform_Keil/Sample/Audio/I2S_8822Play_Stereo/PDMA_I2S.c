/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                         */
/* Copyright(c) 2009 Nuvoton Technology Corp. All rights reserved.                                         */
/*                                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
#include <stdio.h>
#include "Driver\DrvPDMA.h"
#include "Driver\DrvUART.h"
#include "Driver\DrvSYS.h"
#include "Driver\DrvGPIO.h"
#include "Driver\DrvI2S.h"
#include "ISD9xx.h"
#include "NVTTypes.h"

/*---------------------------------------------------------------------------------------------------------*/
/* Define functions prototype                                                                              */
/*---------------------------------------------------------------------------------------------------------*/
void InitialSystemClock(void);
void InitialUART(void);
void InitialI2S(void);
void PDMA3_Callback();

/*-------------------------------------------------------------------------------*/
/* Global Variable                                                               */
/*-------------------------------------------------------------------------------*/
#define	UART_TEST_LENGTH			256
uint8_t SrcArray[UART_TEST_LENGTH];
uint8_t DestArray[UART_TEST_LENGTH];


extern uint32_t TotalPcmCount;  		// = PCM_LENGTH
extern uint32_t AudioDataAddr;		//PCM data address
#define AudioBufferSize 80
__align(4) int16_t AudioBuffer[2][AudioBufferSize];
uint32_t AudioSampleCount,PDMA3CallBackCount,BufferEmptyAddr,BufferReadyAddr;
BOOL	PCMPlaying,BufferEmpty,PDMA3Done;


/*--------------------------------------------------------------------------------------------------------*/
/* PDMA_I2S                                                                                               */
/*--------------------------------------------------------------------------------------------------------*/
/*Important Note */
// I2S slave has problem on MSB format, only can use I2S format
// I2S slave has PDMA problem on receiving, it needs 16 bit PDMA datawidth
// I2S master is OK on MSB & I2S format, and PDMA using 32bit datawidth
void PDMA3forI2S(void)
{
	STR_PDMA_T sPDMA;  
    uint32_t  I2SPort;
    volatile uint32_t i;

    
	/* PDMA Init */
    DrvPDMA_Init();

	/* PDMA Setting */
	PDMA_GCR->PDSSR.I2S_TXSEL = eDRVPDMA_CHANNEL_3;

    
	/* CH3 TX Setting */
	sPDMA.sSrcAddr.u32Addr 			= BufferReadyAddr;
    sPDMA.sDestAddr.u32Addr 		= (uint32_t)&I2S->TXFIFO;   
    sPDMA.u8TransWidth 				= eDRVPDMA_WIDTH_32BITS;
	sPDMA.u8Mode 					= eDRVPDMA_MODE_MEM2APB;
	sPDMA.sSrcAddr.eAddrDirection 	= eDRVPDMA_DIRECTION_INCREMENTED; 
	sPDMA.sDestAddr.eAddrDirection 	= eDRVPDMA_DIRECTION_FIXED;   
	sPDMA.i32ByteCnt                = AudioBufferSize * 2;
	//sPDMA.i32ByteCnt = MAX_FRAMESIZE * sizeof(int16_t);
	DrvPDMA_Open(eDRVPDMA_CHANNEL_3,&sPDMA);


	/* Enable INT */
	DrvPDMA_EnableInt(eDRVPDMA_CHANNEL_3, eDRVPDMA_BLKD );
	DrvPDMA_InstallCallBack(eDRVPDMA_CHANNEL_3,eDRVPDMA_BLKD,
                            (PFN_DRVPDMA_CALLBACK) PDMA3_Callback ); 
    DrvI2S_EnableTxDMA (TRUE);		 
    DrvI2S_EnableTx(TRUE);

  	DrvPDMA_CHEnablelTransfer(eDRVPDMA_CHANNEL_3);
 	
	PDMA3Done=FALSE;
	
}

/*---------------------------------------------------------------------------------------------------------*/
/* DPWM Callback                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
void PDMA3_Callback()
{
	if ((PDMA3CallBackCount&0x1)==0)
		{
			BufferReadyAddr=(uint32_t) &AudioBuffer[1][0];
			BufferEmptyAddr=(uint32_t) &AudioBuffer[0][0];
		}
	else
		{
			BufferReadyAddr=(uint32_t) &AudioBuffer[0][0];
			BufferEmptyAddr=(uint32_t) &AudioBuffer[1][0];
		}


	PDMA3CallBackCount++;
	if (BufferEmpty==FALSE)
		PDMA3forI2S();
	else
	{	
		printf("Late to copy audio data to buffer for PDMA\n");
		PDMA3Done=TRUE;
	}

	BufferEmpty=TRUE;
}


/*---------------------------------------------------------------------------------------------------------*/
/* Bulid Src Pattern function                                                                         	   */
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
	if (AudioSampleCount > TotalPcmCount)
		PCMPlaying=FALSE;
}

/*---------------------------------------------------------------------------------------------------------*/
/* Play sound initialization                                                                               */
/*---------------------------------------------------------------------------------------------------------*/
void PlayI2sSound()
{

	//TOTAL_PCM_COUNT= ((uint32_t)&u32AudioDataEnd-(uint32_t)&u32AudioDataBegin)/2;

	PCMPlaying=TRUE;
	AudioSampleCount=0;
	PDMA3CallBackCount=0;
	
	BufferEmptyAddr= (uint32_t) &AudioBuffer[0][0];
	CopySoundData();
	BufferReadyAddr= (uint32_t) &AudioBuffer[0][0];
	PDMA3forI2S();

	BufferEmptyAddr= (uint32_t) &AudioBuffer[1][0];
	BufferEmpty=TRUE;
}




/*--------------------------------------------------------------------------------------------------------*/
/* SysTimerDelay                                                                                          */
/*--------------------------------------------------------------------------------------------------------*/
void SysTimerDelay(uint32_t us)
{
    SysTick->LOAD = (us * 49152)/1000; /* Assume the internal 48MHz RC used */
    SysTick->VAL   =  (0x00);
    SysTick->CTRL = (1 << SYSTICK_CLKSOURCE) | (1<<SYSTICK_ENABLE);

    /* Waiting for down-count to zero */
    while((SysTick->CTRL & (1 << 16)) == 0);
}




/*---------------------------------------------------------------------------------------------------------*/
/* MAIN function                                                                           	   			   */
/*---------------------------------------------------------------------------------------------------------*/

int32_t PlayI2S()
{

	//InitialI2S();
		
	//printf("+------------------------------------------------------------------------+\n");
    //printf("|                 IS2 8822 Play Sample Code                              |\n");
    //printf("|                ISD9160/Master => 8822/Slave                            |\n");
    //printf("+------------------------------------------------------------------------+\n");                    
	//printf("  This sample code will use PDMA to do I2S test. \n");


	PlayI2sSound();
	while (PCMPlaying == TRUE)
	{
		if (BufferEmpty==TRUE)
		{
			CopySoundData();
			BufferEmpty=FALSE;
		}
	
					
		if ((PDMA3Done==TRUE) && (BufferEmpty==FALSE))
			PDMA3forI2S();

	}

	//printf("\n  PDMA I2S sample code is complete.\n\n");

	/* Close PDMA Channel */
    DrvI2S_EnableTx(FALSE);
	DrvI2S_EnableTxDMA (FALSE);		 
	DrvPDMA_Close();
 
  	
   // while(1);

}	


