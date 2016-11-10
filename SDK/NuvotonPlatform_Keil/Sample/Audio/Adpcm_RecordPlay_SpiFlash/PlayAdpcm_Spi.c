/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                         */
/* Copyright(c) 2009 Nuvoton Technology Corp. All rights reserved.                                         */
/*                                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
#include <stdio.h>
#include "ISD9xx.h"
#include "Driver\DrvPDMA.h"
#include "Driver\DrvDPWM.h"
#include "Driver\DrvGPIO.h"


#include "Lib\libSPIFlash.h"
extern const SFLASH_CTX g_SPIFLASH;

/*---------------------------------------------------------------------------------------------------------*/
/* Macro, type and constant definitions                                                                    */
/*---------------------------------------------------------------------------------------------------------*/
#define AUDIOBUFFERSIZE 320
#define DPWMSAMPLERATE  16000
#define COMPBUFSIZE     (AUDIOBUFFERSIZE/2)+4    //unit: byte


//--------------------
// Buffer and global variables for playing 
uint32_t	u32BufferAddr0, u32BufferAddr1;
uint32_t	AudioSampleCount,PDMA1CallBackCount,AudioDataAddr,BufferEmptyAddr,BufferReadyAddr;
BOOL		PCMPlaying,BufferEmpty,PDMA1Done;

#include "Lib\LibAdpcmCodec.h"
sLibAdpcmState	sAdpcmState;


/*---------------------------------------------------------------------------------------------------------*/
/* Define Function Prototypes                                                                              */
/*---------------------------------------------------------------------------------------------------------*/
void PDMA1_Callback(void);
//void PDMA1forDPWM(void);



/*---------------------------------------------------------------------------------------------------------*/
/* Functions			                                  											 	   */
/*---------------------------------------------------------------------------------------------------------*/
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
/* Set PDMA0 to move ADC FIFO to MIC buffer with wrapped-around mode                                       */
/*---------------------------------------------------------------------------------------------------------*/
void PDMA1forDPWM(void)
{
	STR_PDMA_T sPDMA;  

	sPDMA.sSrcAddr.u32Addr 			= u32BufferAddr0; 
	sPDMA.sDestAddr.u32Addr 		= (uint32_t)&DPWM->FIFO;
	sPDMA.u8Mode 					= eDRVPDMA_MODE_MEM2APB;;
	sPDMA.u8TransWidth 				= eDRVPDMA_WIDTH_16BITS;
	sPDMA.sSrcAddr.eAddrDirection 	= eDRVPDMA_DIRECTION_WRAPAROUND; 
	sPDMA.sDestAddr.eAddrDirection 	= eDRVPDMA_DIRECTION_FIXED;  
	sPDMA.u8WrapBcr				 	= eDRVPDMA_WRA_WRAP_HALF_INT; 		//Interrupt condition set fro Half buffer & buffer end
    sPDMA.i32ByteCnt = AUDIOBUFFERSIZE * 4;	   	//Full MIC buffer length (byte)
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
	DrvPDMA_EnableInt(eDRVPDMA_CHANNEL_1, eDRVPDMA_WAR ); 
	// Install Callback function    
	DrvPDMA_InstallCallBack(eDRVPDMA_CHANNEL_1, eDRVPDMA_WAR, (PFN_DRVPDMA_CALLBACK) PDMA1_Callback ); 	
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
			BufferReadyAddr=u32BufferAddr1;
			BufferEmptyAddr=u32BufferAddr0;
		}
	else
		{
			BufferReadyAddr=u32BufferAddr0;
			BufferEmptyAddr=u32BufferAddr1;
		}


	PDMA1CallBackCount++;

	BufferEmpty=TRUE;
}

extern unsigned char s8Out_bytes[COMPBUFSIZE];

/*---------------------------------------------------------------------------------------------------------*/
/* Copy Data from SPI flash to SRAM                                                                            */
/*---------------------------------------------------------------------------------------------------------*/
void CopySoundData(uint32_t TotalPCMCount)
{

//__align(4) unsigned char s8Out_bytes[COMPBUFSIZE];

	sflash_read(&g_SPIFLASH, AudioDataAddr,  (unsigned long *)s8Out_bytes, COMPBUFSIZE);		 //Read SPI-flash data to empty buffer
    LibAdpcm_DecodeBlock(s8Out_bytes, (PINT16)BufferEmptyAddr, AUDIOBUFFERSIZE, &sAdpcmState);

	AudioSampleCount=AudioSampleCount+AUDIOBUFFERSIZE;
	AudioDataAddr=AudioDataAddr+ COMPBUFSIZE;
	if (AudioSampleCount >TotalPCMCount)
		PCMPlaying=FALSE;
}


/*---------------------------------------------------------------------------------------------------------*/
/* Play sound initialization                                                                               */
/*---------------------------------------------------------------------------------------------------------*/
void PlaySound(uint32_t DataAddr, uint32_t TotalPCMCount)
{
	InitialDPWM(DPWMSAMPLERATE);
	AudioDataAddr= DataAddr;
	PCMPlaying=TRUE;
	AudioSampleCount=0;
	PDMA1CallBackCount=0;
	
	BufferEmptyAddr= u32BufferAddr0;
	CopySoundData(TotalPCMCount);
	BufferReadyAddr= u32BufferAddr0;
	PDMA1forDPWM();

	BufferEmptyAddr= u32BufferAddr1;
	BufferEmpty=TRUE;
}

__align(4) int16_t AudioBuffer[2][AUDIOBUFFERSIZE];

/*---------------------------------------------------------------------------------------------------------*/
/*  Main Function									                                           			   */
/*---------------------------------------------------------------------------------------------------------*/
void PlaySPIFlash(uint32_t PlayStartAddr, uint32_t TotalPCMCount)
{
//	uint8_t u8Option;
//	int32_t	i32Err;
//	uint32_t	u32temp;


//__align(4) int16_t AudioBuffer[2][AUDIOBUFFERSIZE];
   	u32BufferAddr0=	(uint32_t) &AudioBuffer[0][0];
	u32BufferAddr1=	(uint32_t) &AudioBuffer[1][0];

	LibAdpcmInit(&sAdpcmState, 1, DPWMSAMPLERATE, 4);
   
    DrvPDMA_Init();			//PDMA initialization
	//UNLOCKREG();
	 
	PlaySound(PlayStartAddr, TotalPCMCount);


	while (PCMPlaying == TRUE)
	{
		if (BufferEmpty==TRUE)
		{
			CopySoundData(TotalPCMCount);
			BufferEmpty=FALSE;
		}
	
					
		if ((PDMA1Done==TRUE) && (BufferEmpty==FALSE))
			PDMA1forDPWM();

	}

	//while(BufferEmpty==FALSE);			 //Waiting last audio buffer played

	DrvPDMA_Close();
	DrvDPWM_Close();
	UNLOCKREG();

	printf("Play Done\n");
	
	/* Lock protected registers */
	
}



