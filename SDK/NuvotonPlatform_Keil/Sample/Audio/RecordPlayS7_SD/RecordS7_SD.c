/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                         */
/* Copyright(c) 2009 Nuvoton Technology Corp. All rights reserved.                                         */
/*                                                                                                         */
/* Siren7 (G.722) licensed from Polycom Technology                                                         */
/*---------------------------------------------------------------------------------------------------------*/
#include <stdio.h>
#include "ISD9xx.h"
#include "Driver\DrvPDMA.h"
#include "Driver\DrvADC.h"
#include "Driver\DrvALC.h"

#include "SDaccess.h"

/*---------------------------------------------------------------------------------------------------------*/
/* Macro, type and constant definitions                                                                    */
/*---------------------------------------------------------------------------------------------------------*/
#include "RecPlaySdG722.h"


/*---------------------------------------------------------------------------------------------------------*/
/* Define Function Prototypes                                                                              */
/*---------------------------------------------------------------------------------------------------------*/
void InitialADC(void);
void PDMA0_Callback(void);
void PDMA0forMIC(uint32_t u32DestAddr);
void Record2DataFlash(uint32_t RecordAddr, uint32_t TotalPCMCount);




/*---------------------------------------------------------------------------------------------------------*/
/* Define global variables                                                                                 */
/*---------------------------------------------------------------------------------------------------------*/
uint32_t PDMA0CallbackCounter;
int8_t i8RecordDataReady;

//#include "Lib\libG722.h"
//G722_CODEC_CTL ectl;
#include "Lib\LibSiren7.h"
extern sSiren7_CODEC_CTL sEnDeCtl;			  	//defined in PlayS7_SD.c
sSiren7_ENC_CTX sS7Enc_Ctx;

//--------------------
// Buffer and global variables for playing 

extern int16_t AudioBuffer[2][AUDIOBUFFERSIZE];
extern uint32_t SdBuff[128+8];				//512 bytes + 32 bytes (sector size + remained data maximum size)

extern uint32_t TotalG722Size;  		// Encoded Siren7 data size
extern uint32_t	u32BufferAddr0, u32BufferAddr1;
extern uint8_t		u8Buf_CurDataIndex, u8Buf_RemainedCount, u8Buf_StartIndex;
extern uint32_t	AudioDataCount, PDMA1CallBackCounter, u32DataSector, u32BufferEmptyAddr, u32BufferReadyAddr;
extern BOOL	bPlaying,bBufferEmpty,bPDMA1Done;

BOOL bRecording, bBufferReady, bPDMA0Done;
uint32_t u32SdRecordAddr, PDMA0CallBackCounter;



void RecordClose(void)
{
	DrvADC_Close();
}

void RecordLoop(void)
{
	if (i8RecordDataReady>=0)				   // Late to write flash if i8RecordDataReady is larger than 0
	{
		if((PDMA0CallbackCounter & 1)==1)
			u32BufferReadyAddr= u32BufferAddr0;
		else
			u32BufferReadyAddr= u32BufferAddr1;	

		LibS7Encode(&sEnDeCtl, &sS7Enc_Ctx, (signed short *)u32BufferReadyAddr, (int16_t *)&SdBuff[u8Buf_CurDataIndex]);
		u8Buf_CurDataIndex= u8Buf_CurDataIndex+(COMPBUFSIZE/4);
		TotalG722Size = TotalG722Size + COMPBUFSIZE;

   		if(u8Buf_CurDataIndex >= 128)		  	//u8Buf_CurDataIndex only be 0, 2, 4, 6, ..., 136 words
		{
			disk_write (0, (unsigned char *)SdBuff, u32DataSector, 1);		  	//Write one sector data
			u32DataSector++;
			u8Buf_RemainedCount = u8Buf_CurDataIndex - 128;
			RAM2RAM(&SdBuff[u8Buf_CurDataIndex], SdBuff, u8Buf_RemainedCount);			//Copy remained data to SdBuff begining
			u8Buf_CurDataIndex = u8Buf_RemainedCount;
		}

		//u32SdRecordAddr = u32SdRecordAddr +  COMPBUFSIZE;
		i8RecordDataReady--;


		if (TotalG722Size >= RECORD_SIZE)
			bRecording=FALSE;
			
	}

}


/*----------------------------------------------------------------------------------------------------------*/
/*  Functions									                                           			   		*/
/*----------------------------------------------------------------------------------------------------------*/
void Record2SD(uint32_t u32SectorOffset)
{

    //printf("+-------------------------------------------------------------------------+\n");
    //printf("|       Recroding Start												      |\n");
    //printf("+-------------------------------------------------------------------------+\n");


    LibS7Init(&sEnDeCtl,S7BITRATE,S7BANDWIDTH);
    LibS7EnBufReset(sEnDeCtl.frame_size,&sS7Enc_Ctx);


	InitialADC();	  		//ADC initialization
    DrvPDMA_Init();			//PDMA initialization
	UNLOCKREG();

	bRecording=TRUE;
	TotalG722Size=0;
	PDMA0CallbackCounter = 0;
	i8RecordDataReady = -1;

	u8Buf_RemainedCount=  128+8;
	u8Buf_CurDataIndex = 0;
	u32DataSector= u32SectorOffset+SD_RECORD_START_SEC;

	//u32SdRecordAddr = RecordStartAddr+(SD_RECORD_START_SEC*SD_SECTOR_SIZE);
	u32BufferAddr0= (uint32_t)&AudioBuffer[0][0];
	u32BufferAddr1= (uint32_t)&AudioBuffer[1][0];
	//EncBufAddr=	 (uint32_t)&s16Out_words[0];

	PDMA0forMIC(u32BufferAddr0);

 	RecordLoop();

	
}




/*---------------------------------------------------------------------------------------------------------*/
/* ADC RX Callback                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
void PDMA0_Callback()
{
	PDMA0CallbackCounter++;
	i8RecordDataReady++;
}


/*---------------------------------------------------------------------------------------------------------*/
/* InitialADC                                                                                              */
/*---------------------------------------------------------------------------------------------------------*/
extern	LdoOn(void);
void InitialADC(void)
{
	S_DRVADC_PARAM sParam;
	uint32_t u32AdcStatus;
	uint32_t OSR;

	// b0,b1,b2,a1,a2,b0,b1,b2,a1,a2,b0,b1,b2,a1,a2
	/*
	uint32_t u32BiqCoeff[15]={0x10000, 0x15b8a, 0x10000, 0x15068, 0x0ef98,
							  0x10000, 0x00000,	0x00000, 0x00000, 0x00000,
							  0x10000, 0x00000,	0x00000, 0x00000, 0x00000};
	*/

	/* Open Analog block */
	//DrvADC_AnaOpen();
	//LdoOn();

	/* Power control */
	DrvADC_SetPower( 
		eDRVADC_PU_MOD_ON, 
		eDRVADC_PU_IBGEN_ON, 
		eDRVADC_PU_BUFADC_ON, 
		eDRVADC_PU_BUFPGA_ON, 
		//eDRVADC_PU_ZCD_OFF);	   //ALC off
		eDRVADC_PU_ZCD_ON);	   //ALC on

	/* PGA Setting */	
	DrvADC_PGAMute(eDRVADC_MUTE_PGA);
	DrvADC_PGAUnMute(eDRVADC_MUTE_IPBOOST);
	DrvADC_SetPGA(	
	    eDRVADC_REF_SEL_VMID,
	    eDRVADC_PU_PGA_ON,
	    eDRVADC_PU_BOOST_ON,
	    //eDRVADC_BOOSTGAIN_0DB);
		eDRVADC_BOOSTGAIN_26DB);

	DrvADC_SetPGAGaindB(1000);	  

	/* MIC circuit configuration */
	DrvADC_SetVMID(
		eDRVADC_PULLDOWN_VMID_RELEASE,
		eDRVADC_PDLORES_CONNECTED,
		eDRVADC_PDHIRES_DISCONNECTED);
	DrvADC_SetMIC(TRUE, eDRVADC_MIC_BIAS_65_VCCA);

	/* ALC Setting */	
	SYSCLK->APBCLK.BIQALC_EN = 1;
	ALC->ALC_CTRL.ALCLVL = 8;
	ALC->ALC_CTRL.ALCSEL =1;		 //ALC enable
	//ALC->ALC_CTRL.ALCSEL =0;		//ALC disable
	ALC->ALC_CTRL.ALCDCY = 3;
	ALC->ALC_CTRL.ALCATK = 2;
	ALC->ALC_CTRL.NGEN =1;
	ALC->ALC_CTRL.NGTH =7;
 
   	//BIQ->BIQ_CTRL.RSTn = 1;
	//(*((volatile unsigned long*)0x400B0048)) = 0xFF01E360; // target level


	/* Open ADC block */
	sParam.u8AdcDivisor   = 0;
	//sParam.u8SDAdcDivisor = 16;	//OSR64  :16 for 48K
	//sParam.u8SDAdcDivisor = 16;	//OSR128 :48 for 8K, 24 for 16K
	sParam.u8SDAdcDivisor = 16;		//OSR192 :32 for 8K, 16 for 16K
	//sParam.eOSR		  = eDRVADC_OSR_128;
	sParam.eOSR		  = eDRVADC_OSR_192;
	//sParam.eOSR		  = eDRVADC_OSR_64;
	sParam.eInputSrc  = eDRVADC_MIC;
	sParam.eInputMode = eDRVADC_DIFFERENTIAL;
	sParam.u8ADCFifoIntLevel = 7;
	u32AdcStatus=DrvADC_Open(&sParam);
	if(u32AdcStatus == E_SUCCESS) {
		printf("ADC has been successfully opened.\n");
		printf("ADC clock divisor=%d\n",SYSCLK->CLKDIV.ADC_N);
		printf("ADC over sampling clock divisor=%d\n",SDADC->CLK_DIV);
		switch(SDADC->DEC.OSR)
		{
		  case eDRVADC_OSR_64:OSR=64;break;
		  case eDRVADC_OSR_128:OSR=128;break;
		  case eDRVADC_OSR_192:OSR=192;break;
		  case eDRVADC_OSR_384:OSR=384;break;
		}
		printf("ADC over sampling ratio=%d\n", OSR);
		printf("Select microphone path as differential input\n");
		printf("ADC Fifo Interrupt Level=%d\n", SDADC->INT.FIFO_IE_LEV);
		printf("Conversion rate: %d samples/second\n", DrvADC_GetConversionRate());
	}
	else {
		printf("ADC Open failed!\n");
	}
	/* Change Decimation and FIFO Setting */
	//DrvADC_SetAdcOverSamplingClockDivisor(u8SDAdcDivisor);
	//DrvADC_SetOverSamplingRatio(eOSR);
	//DrvADC_SetCICGain(u8CICGain);
	//DrvADC_SetFIFOIntLevel(u8ADCFifoIntLevel);

	DrvADC_PGAUnMute(eDRVADC_MUTE_PGA);
}



/*---------------------------------------------------------------------------------------------------------*/
/* Set PDMA0 to move ADC FIFO to MIC buffer with wrapped-around mode                                       */
/*---------------------------------------------------------------------------------------------------------*/
void PDMA0forMIC(uint32_t u32DestAddr)
{
	STR_PDMA_T sPDMA;  

	sPDMA.sSrcAddr.u32Addr 			= (uint32_t)&SDADC->ADCOUT; 
    sPDMA.sDestAddr.u32Addr 		= u32DestAddr;
	sPDMA.u8Mode 					= eDRVPDMA_MODE_APB2MEM;
	sPDMA.u8TransWidth 				= eDRVPDMA_WIDTH_16BITS;
	sPDMA.sSrcAddr.eAddrDirection 	= eDRVPDMA_DIRECTION_FIXED; 
	sPDMA.sDestAddr.eAddrDirection 	= eDRVPDMA_DIRECTION_WRAPAROUND;  
	sPDMA.u8WrapBcr				 	= 0x5; 		//Interrupt condition set fro Half buffer & buffer end
    sPDMA.i32ByteCnt = AUDIOBUFFERSIZE * 4;	   	//Full MIC buffer length (byte)
    DrvPDMA_Open(eDRVPDMA_CHANNEL_0, &sPDMA);

	// PDMA Setting 
    //PDMA_GCR->PDSSR.ADC_RXSEL = eDRVPDMA_CHANNEL_2;
	DrvPDMA_SetCHForAPBDevice(
    	eDRVPDMA_CHANNEL_0, 
    	eDRVPDMA_ADC,
    	eDRVPDMA_READ_APB    
	);

	DrvPDMA_EnableInt(eDRVPDMA_CHANNEL_0, eDRVPDMA_WAR );		//Enable INT    
	DrvPDMA_InstallCallBack(eDRVPDMA_CHANNEL_0, eDRVPDMA_WAR, (PFN_DRVPDMA_CALLBACK) PDMA0_Callback );		//Callback registration
	DrvADC_PdmaEnable();		// Enable ADC PDMA and Trigger PDMA specified Channel 
	DrvADC_StartConvert();		 // Start A/D conversion 
	DrvPDMA_CHEnablelTransfer(eDRVPDMA_CHANNEL_0);		// start ADC PDMA transfer
}
