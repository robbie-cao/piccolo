/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                         */
/* Copyright(c) 2009 Nuvoton Technology Corp. All rights reserved.                                         */
/*                                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
#include <stdio.h>
#include "ISD9xx.h"
#include "Driver\DrvPDMA.h"
#include "Driver\DrvADC.h"
#include "Driver\DrvALC.h"




/*---------------------------------------------------------------------------------------------------------*/
/* Define Function Prototypes                                                                              */
/*---------------------------------------------------------------------------------------------------------*/
void InitialADC(void);
void PDMA0_Callback(void);
void PDMA0forMIC(uint32_t u32DestAddr);
void RecordStart(void);


/*---------------------------------------------------------------------------------------------------------*/
/* Define global variables                                                                                 */
/*---------------------------------------------------------------------------------------------------------*/
#define MIC_BUFFERSIZE 0x100	
volatile uint32_t CallBackCounter0;
uint32_t u32MicTempAddr0, u32MicTempAddr1, BufferReadyAddr;
__align(4) int16_t MicBuffer[2][MIC_BUFFERSIZE];



uint32_t u32I2sTempAddr0, u32I2sTempAddr1, I2sBufferEmptyAddr;

/*---------------------------------------------------------------------------------------------------------*/
/*  Main Function									                                           			   */
/*---------------------------------------------------------------------------------------------------------*/
void RecordStart(void)
{


	InitialADC();	  		//ADC initialization
    DrvPDMA_Init();			//PDMA initialization


	CallBackCounter0 = 0;

	u32MicTempAddr0= (uint32_t)&MicBuffer[0][0];
	u32MicTempAddr1= (uint32_t)&MicBuffer[1][0];

//	u32I2sTempAddr0= (uint32_t)&I2sBuffer[0][0];
//	u32I2sTempAddr1= (uint32_t)&I2sBuffer[1][0];

	PDMA0forMIC(u32MicTempAddr0);

 
	/* Lock protected registers */
}


void DownSampling(void)
{
int16_t *pi16Source, *pi16Dest;
uint16_t u16Looptemp;

			pi16Source=(int16_t *)BufferReadyAddr;
			pi16Dest=(int16_t *)I2sBufferEmptyAddr;

			for(u16Looptemp=0; u16Looptemp<(MIC_BUFFERSIZE/2); u16Looptemp++)
			{
				*pi16Dest++ = *pi16Source++;

				pi16Source++;	 	//Skip one sampling data
			}

}


/*---------------------------------------------------------------------------------------------------------*/
/* ADC RX Callback                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
void PDMA0_Callback()
{
	if ((CallBackCounter0&0x1)==0)
	{
	 	BufferReadyAddr = u32MicTempAddr0;
		I2sBufferEmptyAddr = u32I2sTempAddr0;
	}
	else
	{
	   	BufferReadyAddr = u32MicTempAddr1;
		I2sBufferEmptyAddr = u32I2sTempAddr1;
	}
	CallBackCounter0++;
}


/*---------------------------------------------------------------------------------------------------------*/
/* InitialADC                                                                                              */
/*---------------------------------------------------------------------------------------------------------*/
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
	//*/


	/* Open Analog block */
	//DrvADC_AnaOpen();

	/* Power control */
	DrvADC_SetPower( 
		eDRVADC_PU_MOD_ON, 
		eDRVADC_PU_IBGEN_ON, 
		eDRVADC_PU_BUFADC_ON, 
		eDRVADC_PU_BUFPGA_ON, 
		eDRVADC_PU_ZCD_OFF);	   //ALC off
		//eDRVADC_PU_ZCD_ON);	   //ALC on

	/* PGA Setting */	
	DrvADC_PGAMute(eDRVADC_MUTE_PGA);
	DrvADC_PGAUnMute(eDRVADC_MUTE_IPBOOST);
	DrvADC_SetPGA(	
	    eDRVADC_REF_SEL_VMID,
	    eDRVADC_PU_PGA_ON,
	    eDRVADC_PU_BOOST_ON,
	    //eDRVADC_BOOSTGAIN_0DB);
		eDRVADC_BOOSTGAIN_26DB);

	DrvADC_SetPGAGaindB(400);		//-200: -2dB, 2000: 20dB  

	/* MIC circuit configuration */
	DrvADC_SetVMID(
		eDRVADC_PULLDOWN_VMID_RELEASE,
		eDRVADC_PDLORES_CONNECTED,
		eDRVADC_PDHIRES_DISCONNECTED);
	DrvADC_SetMIC(TRUE, eDRVADC_MIC_BIAS_75_VCCA);
	//DrvADC_SetMIC(TRUE, 0x7);

	/* ALC Setting */	
	SYSCLK->APBCLK.BIQALC_EN = 1;
	ALC->ALC_CTRL.ALCLVL = 2;
	//ALC->ALC_CTRL.ALCSEL =1;		 //ALC enable
	ALC->ALC_CTRL.ALCSEL =0;		//ALC disable
	ALC->ALC_CTRL.ALCDCY = 3;
	ALC->ALC_CTRL.ALCATK = 2;
	ALC->ALC_CTRL.NGEN =1;
	ALC->ALC_CTRL.NGTH =6;
 
   	
	/* Change BIQ Setting */			
	BiqSetting(0);

	/* Open ADC block */
	sParam.u8AdcDivisor   = 0;
	//sParam.u8SDAdcDivisor = 16;	//OSR64  :16 for 48K
	sParam.u8SDAdcDivisor = 8;	//OSR128 :48 for 8K, 24 for 16K
	//sParam.u8SDAdcDivisor = 32;		//OSR192 :32 for 8K, 16 for 16K, 8 for 32K
	//sParam.eOSR		  = eDRVADC_OSR_128;
	sParam.eOSR		  = eDRVADC_OSR_192;
	//sParam.eOSR		  = eDRVADC_OSR_64;
	sParam.eInputSrc  = eDRVADC_MIC;
	sParam.eInputMode = eDRVADC_DIFFERENTIAL;
	sParam.u8ADCFifoIntLevel = 7;
	u32AdcStatus=DrvADC_Open(&sParam);
	if(u32AdcStatus == E_SUCCESS) {
		//printf("ADC has been successfully opened.\n");
		//printf("ADC clock divisor=%d\n",SYSCLK->CLKDIV.ADC_N);
		//printf("ADC over sampling clock divisor=%d\n",SDADC->CLK_DIV);
		switch(SDADC->DEC.OSR)
		{
		  case eDRVADC_OSR_64:OSR=64;break;
		  case eDRVADC_OSR_128:OSR=128;break;
		  case eDRVADC_OSR_192:OSR=192;break;
		  case eDRVADC_OSR_384:OSR=384;break;
		}
		//printf("ADC over sampling ratio=%d\n", OSR);
		//printf("Select microphone path as differential input\n");
		//printf("ADC Fifo Interrupt Level=%d\n", SDADC->INT.FIFO_IE_LEV);
		//printf("Conversion rate: %d samples/second\n", DrvADC_GetConversionRate());
	}
	else {
		//printf("ADC Open failed!\n");
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
	sPDMA.u8WrapBcr				 	= eDRVPDMA_WRA_WRAP_HALF_INT; 		//Interrupt condition set fro Half buffer & buffer end
    sPDMA.i32ByteCnt = MIC_BUFFERSIZE * 4;	   	//Full MIC buffer length (byte)
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
