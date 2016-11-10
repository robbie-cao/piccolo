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
#include "DrvI2C.h"
#include "ISD9xx.h"
#include "NVTTypes.h"

/*---------------------------------------------------------------------------------------------------------*/
/* Define functions prototype                                                                              */
/*---------------------------------------------------------------------------------------------------------*/
void InitialI2S(void);
void PDMA2_Callback(void);

/*-------------------------------------------------------------------------------*/
/* Global Variable                                                               */
/*-------------------------------------------------------------------------------*/

#define MIC_BUFFERSIZE 0x100	
extern uint32_t u32MicTempAddr0, u32MicTempAddr1, BufferReadyAddr;

//#define I2S_BUFFERSIZE 0x80
//extern int16_t I2sBuffer[2][I2S_BUFFERSIZE];
extern uint32_t u32I2sTempAddr0, u32I2sTempAddr1, I2sBufferEmptyAddr;
volatile uint32_t PDMA2CallBackCount;
BOOL	bI2sBufferEmpty;


/*--------------------------------------------------------------------------------------------------------*/
/* PDMA_I2S                                                                                               */
/*--------------------------------------------------------------------------------------------------------*/
void PDMA2forI2S(void)
{
	STR_PDMA_T sPDMA;  

   
	/* PDMA Init */
    //DrvPDMA_Init();

	/* PDMA Setting */
	PDMA_GCR->PDSSR.I2S_TXSEL = eDRVPDMA_CHANNEL_2;

    
	/* CH3 TX Setting */
	//sPDMA.sSrcAddr.u32Addr 			= BufferReadyAddr;
	sPDMA.sSrcAddr.u32Addr 			= u32MicTempAddr0;
    sPDMA.sDestAddr.u32Addr 		= (uint32_t)&I2S->TXFIFO;   
    sPDMA.u8TransWidth 				= eDRVPDMA_WIDTH_32BITS;
	sPDMA.u8Mode 					= eDRVPDMA_MODE_MEM2APB;
	sPDMA.sSrcAddr.eAddrDirection 	= eDRVPDMA_DIRECTION_WRAPAROUND;
	//sPDMA.sSrcAddr.eAddrDirection 	= eDRVPDMA_DIRECTION_INCREMENTED;  
	sPDMA.sDestAddr.eAddrDirection 	= eDRVPDMA_DIRECTION_FIXED;   
	sPDMA.u8WrapBcr				 	= eDRVPDMA_WRA_WRAP_HALF_INT; 		//Interrupt condition set fro Half buffer & buffer end
	//sPDMA.i32ByteCnt                = MIC_BUFFERSIZE * 2;
	sPDMA.i32ByteCnt                = MIC_BUFFERSIZE * 4;
	DrvPDMA_Open(eDRVPDMA_CHANNEL_2,&sPDMA);


	/* Enable INT */
	//DrvPDMA_EnableInt(eDRVPDMA_CHANNEL_2, eDRVPDMA_BLKD );
	//DrvPDMA_InstallCallBack(eDRVPDMA_CHANNEL_2,eDRVPDMA_BLKD,(PFN_DRVPDMA_CALLBACK) PDMA2_Callback );
	DrvPDMA_EnableInt(eDRVPDMA_CHANNEL_2, eDRVPDMA_WAR );		//Enable INT    
	DrvPDMA_InstallCallBack(eDRVPDMA_CHANNEL_2, eDRVPDMA_WAR, (PFN_DRVPDMA_CALLBACK) PDMA2_Callback );		//Callback registration	 
    DrvI2S_EnableTxDMA (TRUE);		 
    DrvI2S_EnableTx(TRUE);

  	DrvPDMA_CHEnablelTransfer(eDRVPDMA_CHANNEL_2);
 	

	
}

/*---------------------------------------------------------------------------------------------------------*/
/* DPWM Callback                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
void PDMA2_Callback()
{
	PDMA2CallBackCount++;
	bI2sBufferEmpty=TRUE;
	//PDMA3forI2S();
}





/*---------------------------------------------------------------------------------------------------------*/
/* MAIN function                                                                           	   			   */
/*---------------------------------------------------------------------------------------------------------*/

void StartI2SPdma()
{
	bI2sBufferEmpty=FALSE;
	PDMA2CallBackCount=0;
	PDMA2forI2S();
}	


void InitialI2S(void)
{
	S_DRVI2S_DATA_T st;

	/* GPIO initial and select operation mode for I2S*/
    DrvGPIO_InitFunction(FUNC_I2S0);  //Set I2S I/O
	DrvGPIO_InitFunction(FUNC_MCLK1); //Set MCLK I/O

	/* Set I2S Parameter */
    st.u32SampleRate 	 = 16000;
    st.u8WordWidth 	 	 = DRVI2S_DATABIT_16;
    //st.u8AudioFormat 	 = DRVI2S_MONO;
	st.u8AudioFormat 	 = DRVI2S_STEREO;
	st.u8DataFormat  	 = DRVI2S_FORMAT_MSB;   
	//st.u8DataFormat  	 = DRVI2S_FORMAT_I2S;
    //st.u8Mode 		 	 = DRVI2S_MODE_SLAVE;
	st.u8Mode 		 	 = DRVI2S_MODE_MASTER;
    st.u8RxFIFOThreshold = DRVI2S_FIFO_LEVEL_WORD_4;
    st.u8TxFIFOThreshold = DRVI2S_FIFO_LEVEL_WORD_4;

	DrvI2S_Open(&st);

    /* Set I2S I/O */
//    DrvGPIO_InitFunction(FUNC_I2S);
	SYS->GPA_ALT.GPA4 		=1;	 // 
    SYS->GPA_ALT.GPA5 		=1;	 // 
	SYS->GPA_ALT.GPA6 		=1;	 // 
    SYS->GPA_ALT.GPA7 		=1;	 // 

	/* Disable I2S Tx/Rx function */
	DrvI2S_EnableRx(FALSE);
    DrvI2S_EnableTx(TRUE);

	//I2S->CLKDIV.BCLK_DIV=0x2F;

	DrvI2S_SetMCLK(12000000);  //MCLK = 4MHz
	DrvI2S_EnableMCLK(1);	   //enable MCLK
}

