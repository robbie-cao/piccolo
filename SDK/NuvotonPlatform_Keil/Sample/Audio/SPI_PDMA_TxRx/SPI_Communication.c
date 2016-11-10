/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                         */
/* Copyright (c) Nuvoton Technology Corp. All rights reserved.                                             */
/*                                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/

#include <stdio.h>
#include "ISD9xx.h"
#include "Driver\DrvSYS.h"
#include "Driver\DrvSPI.h"
#include "Driver\DrvGPIO.h"
#include "Driver\DrvPDMA.h"

#include "NVTTypes.h"

/*---------------------*/
/* Constant Definition */
/*---------------------*/
#define SPI_MASTER
#define TEST_LENGTH	24			//unit:bytes	

/*-----------------*/
/* Global Variable */
/*-----------------*/
BOOL bPdmaRxDone, bPdmaTxDone, bIsTestOver;
uint8_t au8SrcArray[TEST_LENGTH], au8DestArray[TEST_LENGTH];


/*--------------------*/
/* Function Prototype */
/*--------------------*/



/*---------------------------------------------------------------------------------------------------------*/
/* PDMA0 Callback function                                                                           	   */
/*---------------------------------------------------------------------------------------------------------*/
void PDMA0_Callback(void)
{
	bPdmaRxDone=TRUE;			//TEST_LENGTH bytes data received
}

/*---------------------------------------------------------------------------------------------------------*/
/* PDMA1 Callback function                                                                           	   */
/*---------------------------------------------------------------------------------------------------------*/
void PDMA1_Callback(void)
{
	bPdmaTxDone=TRUE;			//TEST_LENGTH bytes data sended
}




/*-------------------------------------------------------------------------------*/
/* PDMA Sample Code: SPI TX/RX PDMA                                              */
/* Have to make sure no other IP use same PDMA channel							 */
/*-------------------------------------------------------------------------------*/
void PDMA_SPI(void)
{
	STR_PDMA_T sPDMA;  


	/* PDMA Setting */
	DrvPDMA_SetCHForAPBDevice(eDRVPDMA_CHANNEL_1,eDRVPDMA_SPI0,eDRVPDMA_WRITE_APB);
	DrvPDMA_SetCHForAPBDevice(eDRVPDMA_CHANNEL_0,eDRVPDMA_SPI0,eDRVPDMA_READ_APB);

	/* CH1 TX Setting */
	sPDMA.sSrcAddr.u32Addr 			= (uint32_t)au8SrcArray;
    sPDMA.sDestAddr.u32Addr 		= (uint32_t)&(SPI0->TX[0]);   
    sPDMA.u8TransWidth 				= eDRVPDMA_WIDTH_32BITS;
	sPDMA.u8Mode 					= eDRVPDMA_MODE_MEM2APB;
	sPDMA.sSrcAddr.eAddrDirection 	= eDRVPDMA_DIRECTION_INCREMENTED; 
	sPDMA.sDestAddr.eAddrDirection 	= eDRVPDMA_DIRECTION_FIXED;   
	sPDMA.i32ByteCnt                = TEST_LENGTH;
	DrvPDMA_Open(eDRVPDMA_CHANNEL_1,&sPDMA);

 	/* CH0 RX Setting */
	sPDMA.sSrcAddr.u32Addr 			= (uint32_t)&(SPI0->RX[0]);
    sPDMA.sDestAddr.u32Addr 		= (uint32_t)au8DestArray;   
	sPDMA.u8TransWidth 				= eDRVPDMA_WIDTH_32BITS;
	sPDMA.u8Mode 					= eDRVPDMA_MODE_APB2MEM;
	sPDMA.sSrcAddr.eAddrDirection 	= eDRVPDMA_DIRECTION_FIXED; 
	sPDMA.sDestAddr.eAddrDirection 	= eDRVPDMA_DIRECTION_INCREMENTED;   
	sPDMA.i32ByteCnt                = TEST_LENGTH;
	DrvPDMA_Open(eDRVPDMA_CHANNEL_0,&sPDMA);

	/* Enable INT */
    DrvPDMA_EnableInt(eDRVPDMA_CHANNEL_0, eDRVPDMA_BLKD );
	DrvPDMA_EnableInt(eDRVPDMA_CHANNEL_1, eDRVPDMA_BLKD );
        
	/* Install Callback function */   
 	DrvPDMA_InstallCallBack(eDRVPDMA_CHANNEL_0,eDRVPDMA_BLKD,	(PFN_DRVPDMA_CALLBACK) PDMA0_Callback ); 
	DrvPDMA_InstallCallBack(eDRVPDMA_CHANNEL_1,eDRVPDMA_BLKD,	(PFN_DRVPDMA_CALLBACK) PDMA1_Callback ); 
	
	/* Enable SPI PDMA and Trigger PDMA specified Channel */
	//SPI0->DMA.TX_DMA_GO = 1;
	//SPI0->DMA.RX_DMA_GO = 1;
	DrvSPI_StartPDMA(eDRVSPI_PORT0, eDRVSPI_TX_DMA, TRUE);
	DrvSPI_StartPDMA(eDRVSPI_PORT0, eDRVSPI_RX_DMA, TRUE);

  
 	DrvPDMA_CHEnablelTransfer(eDRVPDMA_CHANNEL_0);
	DrvPDMA_CHEnablelTransfer(eDRVPDMA_CHANNEL_1);

	/* Trigger PDMA 10 time and the S/W Flag will be change in PDMA callback funtion */
	
}


void InitialSPIPortMaster(void)
{
	/* Enable SPI0 function */
	DrvGPIO_InitFunction(FUNC_SPI0);

	/* Configure SPI0 as a master, 32-bit transaction */
	DrvSPI_Open(eDRVSPI_PORT0, eDRVSPI_MASTER, eDRVSPI_TYPE1, 32);
	//DrvSPI_Open(eDRVSPI_PORT0, eDRVSPI_MASTER, eDRVSPI_TYPE1, 8);
	
	/* Enable the automatic slave select function of SS0. */
	DrvSPI_EnableAutoCS(eDRVSPI_PORT0, eDRVSPI_SS0);
	//DrvSPI_DisableAutoCS(eDRVSPI_PORT0);

	/* Set the active level of slave select. */
	DrvSPI_SetSlaveSelectActiveLevel(eDRVSPI_PORT0, eDRVSPI_ACTIVE_LOW_FALLING);

	/* SPI clock rate 3MHz */
	DrvSPI_SetClock(eDRVSPI_PORT0, 2000000, 0);

	/* Enable the SPI0 interrupt and install the callback function. */
	//DrvSPI_EnableInt(eDRVSPI_PORT0, SPI0_Callback, 0);		
}

void InitialSPIPortSlave(void)
{
	/* Enable SPI0 function */
	//DrvGPIO_InitFunction(FUNC_SPI0);

	/* Configure SPI0 as a master, 32-bit transaction */
	DrvSPI_Open(eDRVSPI_PORT0, eDRVSPI_SLAVE, eDRVSPI_TYPE1, 32);

	
	/* Enable the automatic slave select function of SS0. */
	DrvSPI_EnableAutoCS(eDRVSPI_PORT0, eDRVSPI_SS0);

	/* Set the active level of slave select. */
	DrvSPI_SetSlaveSelectActiveLevel(eDRVSPI_PORT0, eDRVSPI_ACTIVE_LOW_FALLING);

	/* SPI clock rate 3MHz */
	DrvSPI_SetClock(eDRVSPI_PORT0, 2000000, 0);

	/* Enable the SPI0 interrupt and install the callback function. */
	//DrvSPI_EnableInt(eDRVSPI_PORT0, SPI0_Callback, 0);
}
