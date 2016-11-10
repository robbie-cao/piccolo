/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                         */
/* Copyright(c) 2009 Nuvoton Technology Corp. All rights reserved.                                         */
/*                                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
#include <stdio.h>
#include "ISD9xx.h"
#include "Driver\DrvPDMA.h"
#include "Driver\DrvDPWM.h"
#include "Driver\DrvFMC.h"


#define BUFFER_SAMPLECOUNT 320	

uint32_t PDMA1Counter;
BOOL	bBufferEmpty;
volatile uint32_t u32AudBufAddr0, u32AudBufAddr1, AudBufEmptyAddr,AudBufReadyAddr;
volatile __align(4) int16_t AudioBuffer[2][BUFFER_SAMPLECOUNT];

/*---------------------------------------------------------------------------------------------------------*/
/* Define Function Prototypes                                                                              */
/*---------------------------------------------------------------------------------------------------------*/
void PDMA1_Callback(void);
//void PDMA1forDPWM(void);




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

	sPDMA.sSrcAddr.u32Addr 			= AudBufReadyAddr; 
	sPDMA.sDestAddr.u32Addr 		= (uint32_t)&DPWM->FIFO;
	sPDMA.u8Mode 					= eDRVPDMA_MODE_MEM2APB;;
	sPDMA.u8TransWidth 				= eDRVPDMA_WIDTH_16BITS;
	//sPDMA.sSrcAddr.eAddrDirection 	= eDRVPDMA_DIRECTION_WRAPAROUND;
	sPDMA.sSrcAddr.eAddrDirection 	= eDRVPDMA_DIRECTION_INCREMENTED; 
	sPDMA.sDestAddr.eAddrDirection 	= eDRVPDMA_DIRECTION_FIXED;  
	//sPDMA.u8WrapBcr				 	= eDRVPDMA_WRA_WRAP_HALF_INT; 		//Interrupt condition set fro Half buffer & buffer end	 //For WARPROUND
    sPDMA.i32ByteCnt = BUFFER_SAMPLECOUNT * 2;	   	//Full MIC buffer length (byte)		//For INCREMENTED
	//sPDMA.i32ByteCnt = BUFFER_SAMPLECOUNT * 4;	   	//Full MIC buffer length (byte)		//For WARPROUND
    DrvPDMA_Open(eDRVPDMA_CHANNEL_1, &sPDMA);

	// PDMA Setting 
    //PDMA_GCR->PDSSR.ADC_RXSEL = eDRVPDMA_CHANNEL_2;

	DrvPDMA_SetCHForAPBDevice(
    	eDRVPDMA_CHANNEL_1, 
    	eDRVPDMA_DPWM,
    	eDRVPDMA_WRITE_APB    
	);


	// Enable INT 
	DrvPDMA_DisableInt(eDRVPDMA_CHANNEL_1, eDRVPDMA_BLKD );
	//DrvPDMA_DisableInt(eDRVPDMA_CHANNEL_1, eDRVPDMA_WAR );
	//DrvPDMA_EnableInt(eDRVPDMA_CHANNEL_1, eDRVPDMA_WAR); 	  		//For WARPROUND
	DrvPDMA_EnableInt(eDRVPDMA_CHANNEL_1, eDRVPDMA_BLKD ); 		//For INCREMENTED

	// Install Callback function    
	//DrvPDMA_InstallCallBack(eDRVPDMA_CHANNEL_1, eDRVPDMA_WAR, (PFN_DRVPDMA_CALLBACK) PDMA1_Callback ); 
	DrvPDMA_InstallCallBack(eDRVPDMA_CHANNEL_1, eDRVPDMA_BLKD, (PFN_DRVPDMA_CALLBACK) PDMA1_Callback ); 

	// Enable DPWM DMA
	DrvDPWM_EnablePDMA();	
		
	DrvPDMA_CHEnablelTransfer(eDRVPDMA_CHANNEL_1);

}


/*---------------------------------------------------------------------------------------------------------*/
/* DPWM Callback                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
void PDMA1_Callback()
{

	if ((PDMA1Counter&0x1)==0)
	{
	 	AudBufEmptyAddr = u32AudBufAddr0;
		AudBufReadyAddr = u32AudBufAddr1;
	}
	else
	{
	   	AudBufEmptyAddr = u32AudBufAddr1;
		AudBufReadyAddr = u32AudBufAddr0;	
	}
	
	PDMA1Counter++;
	//bBufferEmpty=TRUE;
	PDMA1forDPWM();


}



void PlayBufferSet(void)
{
	u32AudBufAddr0= (uint32_t)&AudioBuffer[0][0];
	u32AudBufAddr1= (uint32_t)&AudioBuffer[1][0];
	AudBufEmptyAddr = u32AudBufAddr0;
}


/*---------------------------------------------------------------------------------------------------------*/
/*  Main Function									                                           			   */
/*---------------------------------------------------------------------------------------------------------*/
void PlayStart(void)
{
//	uint8_t u8Option;
//	int32_t	i32Err;
//	uint32_t	u32temp;
	PDMA1Counter = 0;
//	u32AudBufAddr0= (uint32_t)&AudioBuffer[0][0];
//	u32AudBufAddr1= (uint32_t)&AudioBuffer[1][0];
//	AudBufEmptyAddr = u32AudBufAddr0;
	AudBufReadyAddr	= u32AudBufAddr0;
	AudBufEmptyAddr = u32AudBufAddr1;

	InitialDPWM(16000); 
	PDMA1forDPWM();

	
}



