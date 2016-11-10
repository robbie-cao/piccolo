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
#include "Driver\DrvGPIO.h"


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
uint32_t TotalG722Size;  		// = PCM_LENGTH

//__align(4) int16_t AudioBuffer[2][AudioBufferSize];
uint32_t AudioSampleCount,PDMA1CallBackCount,AudioDataAddr,BufferEmptyAddr,BufferReadyAddr,AudioBufferAddr0,AudioBufferAddr1;
BOOL	bPCMPlaying,bBufferEmpty;
BOOL	bPauseFlag;
uint32_t	u32KeyCount;
uint8_t		u8LastTwoBufferCount;


#include "PlayG722.h"

//#include "Lib\libG722.h"
//G722_CODEC_CTL ectl, dctl;
#include "Lib\LibSiren7.h"
sSiren7_CODEC_CTL sEnDeCtl;
sSiren7_DEC_CTX sS7Dec_Ctx;


/*---------------------------------------------------------------------------------------------------------*/
/* Define Function Prototypes                                                                              */
/*---------------------------------------------------------------------------------------------------------*/
//void InitialUART(void);
void PDMA1_Callback(void);





//====================
// Functions & main
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
			BufferReadyAddr=AudioBufferAddr1;
			BufferEmptyAddr=AudioBufferAddr0;
		}
	else
		{
			BufferReadyAddr=AudioBufferAddr0;
			BufferEmptyAddr=AudioBufferAddr1;
		}


	PDMA1CallBackCount++;

#ifdef INCREMENTAL_PDMA
	if (bBufferEmpty==FALSE)
		PDMA1forDPWM();
	else
	{	
		bPDMA1Done=TRUE;
	}
#else

	if (u8LastTwoBufferCount<=2)
	{
		u8LastTwoBufferCount++;
		if (u8LastTwoBufferCount>=2)
			bPCMPlaying=FALSE;
	}
	else
		bBufferEmpty=TRUE;
#endif
}



void CopySoundData()
{

	//g722DecDecode(&dctl, (signed short *)AudioDataAddr, (signed short *)BufferEmptyAddr);
	LibS7Decode(&sEnDeCtl, &sS7Dec_Ctx, (signed short *)AudioDataAddr, (signed short *)BufferEmptyAddr);  

	//AudioDataAddr=AudioDataAddr+(dctl.number_of_bits_per_frame/8);
	//AudioSampleCount=AudioSampleCount+(dctl.number_of_bits_per_frame/8);	
	AudioDataAddr=AudioDataAddr+(COMPBUFSIZE * 2);
	AudioSampleCount=AudioSampleCount+AUDIOBUFFERSIZE;

	if (AudioSampleCount >= ((TotalG722Size)*8))   		//TotalG722Size unit is byte
		u8LastTwoBufferCount=0;
	
}


/*---------------------------------------------------------------------------------------------------------*/
/* Play sound initialization                                                                               */
/*---------------------------------------------------------------------------------------------------------*/
void PlaySound(uint32_t DataAddr)
{
	InitialDPWM(16000);
	
	AudioDataAddr= DataAddr;
	bPauseFlag=FALSE;
	bPCMPlaying=TRUE;
	u8LastTwoBufferCount=0xFF;
	AudioSampleCount=0;
	PDMA1CallBackCount=0;
	
	BufferEmptyAddr= AudioBufferAddr0;
	CopySoundData();
	BufferReadyAddr= AudioBufferAddr0;
	PDMA1forDPWM();

	BufferEmptyAddr= AudioBufferAddr1;
	bBufferEmpty=TRUE;
}


void CheckPause(void)
{
uint8_t u8PdmaChClk;
	if(DrvGPIO_GetBit(GPB,6)==0)
		u32KeyCount++;
	else
		u32KeyCount=0;

	if (u32KeyCount==180)
	{
		bPauseFlag=	bPauseFlag ^ 0x1;
		u8PdmaChClk= PDMA->GCR.HCLK_EN;
		if(bPauseFlag==TRUE)
			PDMA->GCR.HCLK_EN= u8PdmaChClk&(~BIT1);	   	//Pause, PDMA channel 1 (BIT1) clock disable
		else
			PDMA->GCR.HCLK_EN= u8PdmaChClk|BIT1;		//Resume, PDMA channel 1 (BIT1) clock enable
	}
}

void PlayLoop(void)
{
		if (bBufferEmpty==TRUE)
		{
			CopySoundData();
			bBufferEmpty=FALSE;
		}
}

void PlayG722Open(void)
{
    LibS7Init(&sEnDeCtl,S7BITRATE,S7BANDWIDTH);
    LibS7DeBufReset(sEnDeCtl.frame_size,&sS7Dec_Ctx);

    DrvPDMA_Init();			//PDMA initialization
	UNLOCKREG();
}

void PlayG722Stream(uint16_t AudIndex)
{
	uint32_t  u32AudPointer, u32StartAddr;
	uint32_t *pTemp0,*pTemp1;

__align(4) int16_t AudioBuffer[2][AUDIOBUFFERSIZE];
	AudioBufferAddr0= (uint32_t) &AudioBuffer[0][0];
	AudioBufferAddr1= (uint32_t) &AudioBuffer[1][0];

//----------------------------------------------------------------------------------------
// Based on the header structure of VPE output file to get sound stream address and length
	u32AudPointer = (uint32_t)&AudioDataBegin + 8*(AudIndex+1);
	pTemp0=(uint32_t *)u32AudPointer;
	pTemp1=(uint32_t *)(u32AudPointer+8);
	u32StartAddr= *pTemp0 +	 (uint32_t)&AudioDataBegin;
	TotalG722Size = *pTemp1 - *pTemp0;

	if(TotalG722Size>COMPBUFSIZE*2)
	{ 
		PlaySound(u32StartAddr);

		//DrvGPIO_SetBit(GPB,2);		//For scope measurement, point A

		while (bPCMPlaying == TRUE)
		{
			PlayLoop();	
			CheckPause();				//Check PAUSE/RESUME key

#ifdef INCREMENTAL_PDMA
		if ((PDMA1Done==TRUE) && (bBufferEmpty==FALSE))
			PDMA1forDPWM();
#endif
		}

		//DrvGPIO_ClrBit(GPB,2);		//For scope measurement, it is 5.7ms to the coming point A
	}
}

void PlayG722Close(void)
{
	DrvDPWM_Close();
}




/*---------------------------------------------------------------------------------------------------------*/
/*  Main Function									                                           			   */
/*---------------------------------------------------------------------------------------------------------*/
void PlayG722 (uint16_t AudIndex)
{
	uint32_t  u32AudPointer, u32StartAddr;
	uint32_t *pTemp0,*pTemp1;

__align(4) int16_t AudioBuffer[2][AUDIOBUFFERSIZE];
	AudioBufferAddr0= (uint32_t) &AudioBuffer[0][0];
	AudioBufferAddr1= (uint32_t) &AudioBuffer[1][0];


// Siren7/G722 set bit_rate	and bandwidth first before init
	//ectl.bit_rate = 16000;
    //ectl.bandwidth = 7000;
	//dctl.bit_rate = 16000;
    //dctl.bandwidth = 7000;
    //g722DecInit(&dctl);
    LibS7Init(&sEnDeCtl,S7BITRATE,S7BANDWIDTH);
    LibS7DeBufReset(sEnDeCtl.frame_size,&sS7Dec_Ctx);



    //printf("+-------------------------------------------------------------------------+\n");
    //printf("|       Playing 16K sampling  16Kbps  G722							      |\n");
    //printf("+-------------------------------------------------------------------------+\n");
	


    DrvPDMA_Init();			//PDMA initialization
	UNLOCKREG();
//----------------------------------------------------------------------------------------
// Based on the header structure of VPE output file to get sound stream address and length
	u32AudPointer = (uint32_t)&AudioDataBegin-1 + 8*(AudIndex+1);
	pTemp0=(uint32_t *)u32AudPointer;
	pTemp1=(uint32_t *)(u32AudPointer+8);
	u32StartAddr= *pTemp0 +	 (uint32_t)&AudioDataBegin-1;
	TotalG722Size = *pTemp1 - *pTemp0;

	if(TotalG722Size>COMPBUFSIZE*2)
	{ 
		PlaySound(u32StartAddr);	

		//DrvGPIO_SetBit(GPB,2);		//For scope measurement, point A

		while (bPCMPlaying == TRUE)
		{
			PlayLoop();	
			CheckPause();				//Check PAUSE/RESUME key

#ifdef INCREMENTAL_PDMA
			if ((PDMA1Done==TRUE) && (bBufferEmpty==FALSE))
				PDMA1forDPWM();
#endif

		}

		//DrvGPIO_ClrBit(GPB,2);		//For scope measurement, it is 5.8ms to the coming point A
	}

//	while(bBufferEmpty==FALSE);			 //Waiting last audio buffer played
	
	DrvDPWM_Close();
	DrvPDMA_DisableInt(eDRVPDMA_CHANNEL_1, eDRVPDMA_BLKD); 

	/* Lock protected registers */
	
}



