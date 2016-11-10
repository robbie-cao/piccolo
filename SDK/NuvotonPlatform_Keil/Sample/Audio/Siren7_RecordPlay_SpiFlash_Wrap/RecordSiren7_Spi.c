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

#include "Lib\libSPIFlash.h"
extern const SFLASH_CTX g_SPIFLASH;

/*---------------------------------------------------------------------------------------------------------*/
/* Define Function Prototypes                                                                              */
/*---------------------------------------------------------------------------------------------------------*/
void InitialADC(void);
void PDMA0_Callback(void);
void PDMA0forMIC(uint32_t u32DestAddr);
void Record2DataFlash(uint32_t RecordAddr, uint32_t TotalPCMCount);

/*---------------------------------------------------------------------------------------------------------*/
/* Macro, type and constant definitions                                                                    */
/*---------------------------------------------------------------------------------------------------------*/
#define AUDIOBUFFERSIZE 320
#define DPWMSAMPLERATE  16000
#define S7BITRATE       16000
#define S7BANDWIDTH     7000
#define COMPBUFSIZE     20   //According to S7BITRATE

#define	SECTORSIZE	0x1000				//4KB/sector


/*---------------------------------------------------------------------------------------------------------*/
/* Define global variables                                                                                 */
/*---------------------------------------------------------------------------------------------------------*/
uint32_t CallbackCounter;
int8_t RecordDataReady;

//#include "Lib\libG722.h"
//G722_CODEC_CTL ectl;
#include "Lib\LibSiren7.h"
sSiren7_CODEC_CTL sEnDeCtl;
sSiren7_ENC_CTX sS7Enc_Ctx;




/*----------------------------------------------------------------------------------------------------------*/
/*  Functions									                                           			   		*/
/*----------------------------------------------------------------------------------------------------------*/
void Record2SPIFlash(uint32_t RecordStartAddr, uint32_t TotalPCMCount)
{
	
	uint32_t u32TempAddr0,u32TempAddr1;
	uint32_t DataReadyAddr,FlashRecordAddr;


__align(4)	int16_t MicBuffer[2][AUDIOBUFFERSIZE];
signed short s16Out_words[COMPBUFSIZE];

    //printf("+-------------------------------------------------------------------------+\n");
    //printf("|       Recroding Start												      |\n");
    //printf("+-------------------------------------------------------------------------+\n");

	sflash_erase(&g_SPIFLASH, RecordStartAddr, (TotalPCMCount/8)+4095);	//Erase unit is 4096B

	// set bit_rate	and bandwidth first before init
	//ectl.bit_rate = 16000;
    //ectl.bandwidth = 700;
	//g722EncInit(&ectl);
    LibS7Init(&sEnDeCtl,S7BITRATE,S7BANDWIDTH);
    LibS7EnBufReset(sEnDeCtl.frame_size,&sS7Enc_Ctx);



	InitialADC();	  		//ADC initialization
    DrvPDMA_Init();			//PDMA initialization
	UNLOCKREG();

	CallbackCounter = 0;
	RecordDataReady = -1;
	FlashRecordAddr = RecordStartAddr;
	u32TempAddr0= (uint32_t)&MicBuffer[0][0];
	u32TempAddr1= (uint32_t)&MicBuffer[1][0];
	//EncBufAddr=	 (uint32_t)&s16Out_words[0];

	PDMA0forMIC(u32TempAddr0);

 
	while (CallbackCounter <= (TotalPCMCount/AUDIOBUFFERSIZE))
	{
		if (RecordDataReady>=0)				   // Late to write flash if RecordDataReady is larger than 0
		{
			if((CallbackCounter & 1)==1)
				DataReadyAddr= u32TempAddr0;
			else
				DataReadyAddr= u32TempAddr1;

			//g722EncEncode(&ectl, (signed short *)DataReadyAddr, s16Out_words);
			LibS7Encode(&sEnDeCtl, &sS7Enc_Ctx, (signed short *)DataReadyAddr, s16Out_words);

			//Write ready buffer data 
			sflash_write(&g_SPIFLASH, FlashRecordAddr, (uint32_t *) s16Out_words, (COMPBUFSIZE*2));

			FlashRecordAddr = FlashRecordAddr +  (COMPBUFSIZE * 2);
			RecordDataReady--;
			
		}		//end of if(RecordDataReady==0)
	}

	DrvPDMA_Close();
 	DrvADC_Close();

	UNLOCKREG();



	/* Lock protected registers */
	
}




/*---------------------------------------------------------------------------------------------------------*/
/* ADC RX Callback                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
void PDMA0_Callback()
{
	CallbackCounter++;
	RecordDataReady++;
}


/*---------------------------------------------------------------------------------------------------------*/
/* InitialADC                                                                                              */
/*---------------------------------------------------------------------------------------------------------*/
extern	LdoOn(void);
void InitialADC(void)
{
	S_DRVADC_PARAM sParam;
	uint32_t u32AdcStatus;
	uint32_t OSR, TempCount;

	// b0,b1,b2,a1,a2,b0,b1,b2,a1,a2,b0,b1,b2,a1,a2
	/*
	uint32_t u32BiqCoeff[15]={0x10000, 0x15b8a, 0x10000, 0x15068, 0x0ef98,
							  0x10000, 0x00000,	0x00000, 0x00000, 0x00000,
							  0x10000, 0x00000,	0x00000, 0x00000, 0x00000};
	*/

	/* Open Analog block */
	//DrvADC_AnaOpen();			//Will reset all IP in ANA like current source, VMID, PGA, ALC, ...(ALL ANA_BA IP)
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
 

        BIQ->BIQ_CTRL.RSTn = 0;
        //SysTimerDelay(20);
        for(TempCount=0;TempCount<32;TempCount++);
        BIQ->BIQ_CTRL.RSTn = 1;
        //SysTimerDelay(20);
        for(TempCount=0;TempCount<32;TempCount++);
                
        BIQ->BIQ_CTRL.EN=1;



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
