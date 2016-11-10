/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                         */
/* Copyright(c) 2009 Nuvoton Technology Corp. All rights reserved.                                         */
/*                                                                                                         */
/* Siren7 (G.722) licensed from Polycom Technology                                                         */
/*---------------------------------------------------------------------------------------------------------*/
#include <stdio.h>
#include "ISD9xx.h"
#include "Driver\DrvPDMA.h"
#include "Driver\DrvDPWM.h"
#include "NVTTypes.h"

#include "Lib\libSPIFlash.h"
extern const SFLASH_CTX g_SPIFLASH;
#define S7DATA_BASE_ADDR_ON_SPI	0

/*---------------------------------------------------------------------------------------------------------*/
/* Macro, type and constant definitions                                                                    */
/*---------------------------------------------------------------------------------------------------------*/
#define AUDIOBUFFERSIZE 320
#define DPWMSAMPLERATE  16000
#define S7BITRATE       16000
#define S7BANDWIDTH     7000
#define COMPBUFSIZE     20   //According to S7BITRATE


//--------------------
// Buffer and global variables for playing 
uint32_t TotalG722Size;  		// Encoded Siren7 data size
uint32_t	u32BufferAddr0, u32BufferAddr1;
uint32_t	AudioSampleCount,PDMA1CallBackCount,AudioDataAddr,BufferEmptyAddr,BufferReadyAddr;
BOOL		bPCMPlaying,bBufferEmpty;
uint8_t		u8LastTwoBufferCount;

__align(4) int16_t AudioBuffer[2][AUDIOBUFFERSIZE];
__align(4) uint32_t	SpiDataBuffer[COMPBUFSIZE/2];

//#include "Lib\libG722.h"
//G722_CODEC_CTL dctl;
#include "Lib\LibSiren7.h"
sSiren7_CODEC_CTL sEnDeCtl;
sSiren7_DEC_CTX sS7Dec_Ctx;

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

	sPDMA.sSrcAddr.u32Addr 			= BufferReadyAddr; 
	sPDMA.sDestAddr.u32Addr 		= (uint32_t)&DPWM->FIFO;
	sPDMA.u8Mode 					= eDRVPDMA_MODE_MEM2APB;;
	sPDMA.u8TransWidth 				= eDRVPDMA_WIDTH_16BITS;
	sPDMA.sSrcAddr.eAddrDirection 	= eDRVPDMA_DIRECTION_WRAPAROUND;
	//sPDMA.sSrcAddr.eAddrDirection 	= eDRVPDMA_DIRECTION_INCREMENTED; 
	sPDMA.sDestAddr.eAddrDirection 	= eDRVPDMA_DIRECTION_FIXED;  
	sPDMA.u8WrapBcr				 	= eDRVPDMA_WRA_WRAP_HALF_INT;; 		//Interrupt condition set fro Half buffer & buffer end
    //sPDMA.i32ByteCnt = AUDIOBUFFERSIZE * 2;	   	//Full MIC buffer length (byte)
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

	//bPDMA1Done=FALSE;
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
	
	if (u8LastTwoBufferCount<=2)
	{
		u8LastTwoBufferCount++;
		if (u8LastTwoBufferCount>=2)
			bPCMPlaying=FALSE;
	}
	else
		bBufferEmpty=TRUE;
}

/*---------------------------------------------------------------------------------------------------------*/
/* Play function Close			                                                                           */
/*---------------------------------------------------------------------------------------------------------*/
void PlayClose(void)
{
	//DrvPDMA_Close();
	DrvDPWM_Close();
}

/*---------------------------------------------------------------------------------------------------------*/
/* Copy Data from SPI flash to SRAM                                                                            */
/*---------------------------------------------------------------------------------------------------------*/
void CopySpiSoundData()
{
__align(4) uint32_t	SpiDataBuffer[10];		 //buffer size = dctl.number_of_bits_per_frame/8, 20 for 16Kbps, 10 for 8Kbps

	sflash_read(&g_SPIFLASH, AudioDataAddr, SpiDataBuffer, (COMPBUFSIZE*2));
	//g722DecDecode(&dctl, (uint16_t *)SpiDataBuffer, (signed short *)BufferEmptyAddr); 
	LibS7Decode(&sEnDeCtl, &sS7Dec_Ctx, (uint16_t *)SpiDataBuffer, (signed short *)BufferEmptyAddr); 
	
	AudioDataAddr=AudioDataAddr+(COMPBUFSIZE * 2);
	AudioSampleCount=AudioSampleCount+AUDIOBUFFERSIZE;

	if (AudioSampleCount >= (TotalG722Size)*8)
			u8LastTwoBufferCount=0;

}

/*---------------------------------------------------------------------------------------------------------*/
/* Play function Loop 			                                                                           */
/*---------------------------------------------------------------------------------------------------------*/
void PlayLoop(void)
{
		if (bBufferEmpty==TRUE)
		{
			CopySpiSoundData();
			bBufferEmpty=FALSE;
		}

}

/*---------------------------------------------------------------------------------------------------------*/
/* Play sound initialization                                                                               */
/*---------------------------------------------------------------------------------------------------------*/
void PlaySpiSound(uint32_t DataAddr)
{
	InitialDPWM(16000);
	AudioDataAddr= DataAddr;
	bPCMPlaying=TRUE;
	u8LastTwoBufferCount=0xFF;
	AudioSampleCount=0;
	PDMA1CallBackCount=0;
	
	BufferEmptyAddr= u32BufferAddr0;
	CopySpiSoundData();
	BufferReadyAddr= u32BufferAddr0;
	PDMA1forDPWM();

	BufferEmptyAddr= u32BufferAddr1;
	bBufferEmpty=TRUE;
}



/*---------------------------------------------------------------------------------------------------------*/
/*  Main Function									                                           			   */
/*---------------------------------------------------------------------------------------------------------*/
void PlaySpiG722(uint16_t AudIndex)
{
	uint32_t  u32AudPointer, u32StartAddr;
	uint32_t  Temp0,Temp1;



   	u32BufferAddr0=	(uint32_t) &AudioBuffer[0][0];
	u32BufferAddr1=	(uint32_t) &AudioBuffer[1][0];

	// set bit_rate	and bandwidth first before init
	//dctl.bit_rate = 16000;
    //dctl.bandwidth = 7000;
	//g722DecInit(&dctl);
    LibS7Init(&sEnDeCtl,S7BITRATE,S7BANDWIDTH);
    LibS7DeBufReset(sEnDeCtl.frame_size,&sS7Dec_Ctx);

   
    DrvPDMA_Init();			//PDMA initialization
	//UNLOCKREG();

//----------------------------------------------------------------------------------------
// Based on the header structure of VPE output file to get sound stream address and length
	u32AudPointer = S7DATA_BASE_ADDR_ON_SPI + 8*(AudIndex+1);

	//======================== Important ==========================================================================
	// MSB first and SPI0->CNTRL.BYTE_ENDIAN is 0 in SPI flash library.  Writer have to follow this rule to program.

	sflash_read(&g_SPIFLASH, u32AudPointer, &Temp0, 4);
	//Temp0=  (Temp>>24) + (((Temp>>16)&0xFF)<<8) + (((Temp>>8)&0xFF)<<16) + ((Temp&0xFF)<<24);

	sflash_read(&g_SPIFLASH, (u32AudPointer+8), &Temp1, 4);
	//Temp1=  (Temp>>24) + (((Temp>>16)&0xFF)<<8) + (((Temp>>8)&0xFF)<<16) + ((Temp&0xFF)<<24);

	u32StartAddr= Temp0 + S7DATA_BASE_ADDR_ON_SPI;
	TotalG722Size = Temp1 - Temp0;

	if(TotalG722Size>COMPBUFSIZE*2)
	{ 
		PlaySpiSound(u32StartAddr);
		PlayLoop();
	}
	
}



