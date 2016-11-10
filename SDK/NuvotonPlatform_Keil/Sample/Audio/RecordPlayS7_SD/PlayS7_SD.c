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

#include "SDaccess.h"



/*---------------------------------------------------------------------------------------------------------*/
/* Macro, type and constant definitions                                                                    */
/*---------------------------------------------------------------------------------------------------------*/
#include "RecPlaySdG722.h"


//--------------------
// Buffer and global variables for playing 
uint32_t TotalG722Size;  		// Encoded Siren7 data size
uint32_t	u32BufferAddr0, u32BufferAddr1;
uint8_t		u8Buf_CurDataIndex, u8Buf_RemainedCount, u8Buf_StartIndex;
uint32_t	u32AudioDataCount,PDMA1CallBackCounter,u32DataSector,u32BufferEmptyAddr,u32BufferReadyAddr;
BOOL	bPlaying,bBufferEmpty,bPDMA1Done;

__align(4) int16_t AudioBuffer[2][AUDIOBUFFERSIZE];

__align(4) uint32_t SdBuff[128+8];				//512 bytes + 32 bytes (sector size + remained data maximum size)

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

	sPDMA.sSrcAddr.u32Addr 			= u32BufferReadyAddr; 
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

	bPDMA1Done=FALSE;
}


/*---------------------------------------------------------------------------------------------------------*/
/* DPWM Callback                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
void PDMA1_Callback()
{
	if ((PDMA1CallBackCounter&0x1)==0)
		{
			u32BufferReadyAddr=u32BufferAddr1;
			u32BufferEmptyAddr=u32BufferAddr0;
		}
	else
		{
			u32BufferReadyAddr=u32BufferAddr0;
			u32BufferEmptyAddr=u32BufferAddr1;
		}


	PDMA1CallBackCounter++;
	bBufferEmpty=TRUE;
}

/*---------------------------------------------------------------------------------------------------------*/
/* Copy Data from flash to SRAM                                                                            */
/*---------------------------------------------------------------------------------------------------------*/
void RAM2RAM(uint32_t *Addr1,uint32_t *Addr2,uint32_t WordCount)
{
uint32_t	u32LoopCount;
	for(u32LoopCount=0;u32LoopCount<WordCount;u32LoopCount++ )
	{
		*Addr2++ = *Addr1++;
	}
}  

/*---------------------------------------------------------------------------------------------------------*/
/* Copy Data from SD flash to SRAM                                                                        */
/*---------------------------------------------------------------------------------------------------------*/
void CopySdSoundData()
{

   	if(u8Buf_RemainedCount <= 8)
	{
		RAM2RAM(&SdBuff[u8Buf_CurDataIndex], SdBuff, u8Buf_RemainedCount);			//Copy remained data to SdBuff begining
		u32DataSector++;
		disk_read (0, (unsigned char *)&SdBuff[u8Buf_RemainedCount], u32DataSector, 1);		  	//Read one sector data after remained data
		u8Buf_RemainedCount = u8Buf_RemainedCount + 128;
		u8Buf_CurDataIndex = 0;
	}

	//g722DecDecode(&dctl, (uint16_t *)SpiDataBuffer, (signed short *)u32BufferEmptyAddr); 
	LibS7Decode(&sEnDeCtl, &sS7Dec_Ctx, (int16_t *)&SdBuff[u8Buf_CurDataIndex], (signed short *)u32BufferEmptyAddr); 
	
	u8Buf_CurDataIndex= u8Buf_CurDataIndex+(COMPBUFSIZE/4);
	u8Buf_RemainedCount= u8Buf_RemainedCount-(COMPBUFSIZE/4);
	u32AudioDataCount= u32AudioDataCount+COMPBUFSIZE;

	if (u32AudioDataCount >= TotalG722Size) 
		bPlaying=FALSE;
	
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
/* Play function Loop 			                                                                           */
/*---------------------------------------------------------------------------------------------------------*/
void PlayLoop(void)
{
		if (bBufferEmpty==TRUE)
		{
			CopySdSoundData();
			bBufferEmpty=FALSE;
		}
}

/*---------------------------------------------------------------------------------------------------------*/
/* Play sound initialization                                                                               */
/*---------------------------------------------------------------------------------------------------------*/
void PlaySdSound(uint32_t DataSdAddr)
{
	InitialDPWM(16000);

	bPlaying=TRUE;
	u32AudioDataCount=0;
	PDMA1CallBackCounter=0;

	u32DataSector= DataSdAddr/SD_SECTOR_SIZE;
	u8Buf_CurDataIndex=(DataSdAddr%SD_SECTOR_SIZE)/4;		//Data start index for decode
	u8Buf_RemainedCount=128-u8Buf_CurDataIndex;				//Remained buffer data cound in buffer
	disk_read (0, (unsigned char *)SdBuff, u32DataSector, 1);
	
	u32BufferEmptyAddr= u32BufferAddr0;
	CopySdSoundData();
	u32BufferReadyAddr= u32BufferAddr0;
	PDMA1forDPWM();

	u32BufferEmptyAddr= u32BufferAddr1;
	bBufferEmpty=TRUE;
}


uint32_t GetSdAudioSizeStartAddr(uint16_t AudIndex)
{
uint32_t u32AudPointer, u32SectorNum;
uint32_t u32Addr0, u32Addr1, u32BufferIndex;

	u32AudPointer = S7DATA_BASE_ADDR_ON_SD + 8*(AudIndex+1);
	u32SectorNum = u32AudPointer/SD_SECTOR_SIZE;  				//Get the sector number which has the AudIndex header
	u32BufferIndex = (u32AudPointer%SD_SECTOR_SIZE)/4;			//Get the AudIndex header offset, unit word index in buffer	SdBuff
	
	disk_read (0, (unsigned char *)SdBuff, u32SectorNum, 1);
	u32Addr0 = SdBuff[u32BufferIndex];

	if(u32BufferIndex == 126)
	{
		disk_read (0, (unsigned char *)SdBuff, u32SectorNum+1, 1);
		u32Addr1 = SdBuff[0];
	}
	else
		u32Addr1 = SdBuff[u32BufferIndex+2];	//Each AudioIndex has a header of two words
	TotalG722Size = u32Addr1 - u32Addr0;		  	
	return(u32Addr0);
}

/*---------------------------------------------------------------------------------------------------------*/
/*  Main Function									                                           			   */
/*---------------------------------------------------------------------------------------------------------*/
void PlaySdRecordG722(uint32_t u32SectorOffset)
{
	uint32_t  u32StartAddr;


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
	u32StartAddr = (u32SectorOffset+SD_RECORD_START_SEC)*SD_SECTOR_SIZE;
	TotalG722Size = RECORD_SIZE;
	 
	PlaySdSound(u32StartAddr);
	PlayLoop();

				
}




