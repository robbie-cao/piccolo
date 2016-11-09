/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                         */
/* Copyright(c) 2009 Nuvoton Technology Corp. All rights reserved.                                         */
/*                                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include "ISD9xx.h"
#include "Driver\DrvPDMA.h"
#include "Driver\DrvUART.h"
#include "Driver\DrvSPI.h"
#include "Driver\DrvSYS.h"
#include "Driver\DrvGPIO.h"


/*---------------------------------------------------------------------------------------------------------*/
/* Define functions prototype                                                                              */
/*---------------------------------------------------------------------------------------------------------*/
void InitialSystemClock(void);
void InitialUART(void);

#define	TEST_LENGTH			200
#define MEM_TEST_CNT        100
#define SPI_TEST_CNT		10
#define UART_TEST_CNT		5
uint8_t SrcArray[TEST_LENGTH];
uint8_t DestArray[TEST_LENGTH];
int32_t IntCnt;
volatile int32_t IsTestOver;

/*---------------------------------------------------------------------------------------------------------*/
/* Clear buffer funcion                                                                              	   */
/*---------------------------------------------------------------------------------------------------------*/
void ClearBuf(uint32_t u32Addr, uint32_t u32Length,uint8_t u8Pattern)
{
	uint8_t* pu8Ptr;
	uint32_t i;
	
	pu8Ptr = (uint8_t *)u32Addr;
	
	for (i=0; i<u32Length; i++)
	{
		*pu8Ptr++ = u8Pattern;
	}
}

/*---------------------------------------------------------------------------------------------------------*/
/* Bulid Src Pattern function                                                                         	   */
/*---------------------------------------------------------------------------------------------------------*/
void BuildSrcPattern(uint32_t u32Addr, uint32_t u32Length)
{
    uint32_t i=0,j,loop;
    uint8_t* pAddr;
    
    pAddr = (uint8_t *)u32Addr;
    
    do {
        if (u32Length > 256)
	    	loop = 256;
	    else
	    	loop = u32Length;
	    	
	   	u32Length = u32Length - loop;    	

        for(j=0;j<loop;j++)
            *pAddr++ = (uint8_t)(j+i);
            
	    i++;        
	} while ((loop !=0) || (u32Length !=0));         
}

/*---------------------------------------------------------------------------------------------------------*/
/* PDMA0 Callback function                                                                           	   */
/*---------------------------------------------------------------------------------------------------------*/
void PDMA0_Callback()
{
	extern int32_t IntCnt;
	//printf("\tReceive Done %02d!\r",++IntCnt);
		
	if(IntCnt<SPI_TEST_CNT)
	{
		//SPI0->DMA.RX_DMA_GO = 1;    
		DrvSPI_StartPDMA(eDRVSPI_PORT0, eDRVSPI_RX_DMA, TRUE);
		DrvPDMA_CHEnablelTransfer(eDRVPDMA_CHANNEL_0);		
	}
	else
	{
		IsTestOver = TRUE;
	}
}

/*---------------------------------------------------------------------------------------------------------*/
/* PDMA1 Callback function                                                                           	   */
/*---------------------------------------------------------------------------------------------------------*/
void PDMA1_Callback()
{
	extern int32_t IntCnt;
	printf("\tTransfer Done %02d!\r",++IntCnt);
	
	if(IntCnt<SPI_TEST_CNT)
	{
		//SPI0->DMA.TX_DMA_GO = 1;
		DrvSPI_StartPDMA(eDRVSPI_PORT0, eDRVSPI_TX_DMA, TRUE);
 		DrvPDMA_CHEnablelTransfer(eDRVPDMA_CHANNEL_1);		     		
	}
	else
	{
		IsTestOver = TRUE;
	}
}

/*---------------------------------------------------------------------------------------------------------*/
/* PDMA2 Callback function                                                                           	   */
/*---------------------------------------------------------------------------------------------------------*/
void PDMA2_Callback()
{
	extern int32_t IntCnt;
	printf("\tMem Transfer Done %02d!\r",++IntCnt);
	
	
	if(IntCnt<MEM_TEST_CNT)
	{
 		DrvPDMA_CHEnablelTransfer(eDRVPDMA_CHANNEL_2);		     		
	}
	else
	{
		IsTestOver = TRUE;
	}
}

/*---------------------------------------------------------------------------------------------------------*/
/* PDMA3 Callback function                                                                           	   */
/*---------------------------------------------------------------------------------------------------------*/
void PDMA3_Callback()
{
	extern int32_t IntCnt;
	printf("UART Transfer : %02d!\n",++IntCnt);
	
	
	if(IntCnt<UART_TEST_CNT)
	{
 		DrvPDMA_CHEnablelTransfer(eDRVPDMA_CHANNEL_3);		     		
	}
	else
	{
		IsTestOver = TRUE;
	}
}

/*-------------------------------------------------------------------------------*/
/* PDMA Sample Code: UART0 TX PDMA                                               */
/*-------------------------------------------------------------------------------*/
void PDMA_UART(void)
{
	STR_PDMA_T sPDMA;  
    uint32_t  UARTPort;
	char Message[]="This message is transfered by UART through PDMA\r\n";

   	UARTPort = UART0_BA;    	
    
	/* PDMA Init */
    DrvPDMA_Init();

	/* PDMA Setting */
	DrvPDMA_SetCHForAPBDevice(eDRVPDMA_CHANNEL_3,eDRVPDMA_UART0,eDRVPDMA_WRITE_APB);

	/* CH1 TX Setting */
	sPDMA.sSrcAddr.u32Addr 			= (uint32_t)Message;
    sPDMA.sDestAddr.u32Addr 		= UARTPort;   
    sPDMA.u8TransWidth 				= eDRVPDMA_WIDTH_8BITS;
	sPDMA.u8Mode 					= eDRVPDMA_MODE_MEM2APB;
	sPDMA.sSrcAddr.eAddrDirection 	= eDRVPDMA_DIRECTION_INCREMENTED; 
	sPDMA.sDestAddr.eAddrDirection 	= eDRVPDMA_DIRECTION_FIXED;   
	sPDMA.i32ByteCnt                = strlen(Message);
	DrvPDMA_Open(eDRVPDMA_CHANNEL_3,&sPDMA);

	/* Enable INT */
	DrvPDMA_EnableInt(eDRVPDMA_CHANNEL_3, eDRVPDMA_BLKD );
        
	/* Install Callback function */   
 	DrvPDMA_InstallCallBack(eDRVPDMA_CHANNEL_3,eDRVPDMA_BLKD,	(PFN_DRVPDMA_CALLBACK) PDMA3_Callback ); 
	
	/* Enable UART PDMA and Trigger PDMA specified Channel */
	DrvUART_SetPDMA(UART_PORT0,ENABLE);
   	
	IntCnt = 0;
	IsTestOver=FALSE;
  
	DrvPDMA_CHEnablelTransfer(eDRVPDMA_CHANNEL_3);

	/* Trigger PDMA and the S/W Flag will be change in PDMA callback funtion */
	while(IsTestOver==FALSE);

	/* Close PDMA Channel */
	DrvPDMA_Close(); 	
}

/*---------------------------------------------------------------------------------------------------------*/
/* SysTimerDelay                                                                           	   			   */
/*---------------------------------------------------------------------------------------------------------*/
void SysTimerDelay(uint32_t us)
{
    SysTick->LOAD = us * 49; /* Assume the internal 49.152MHz oscillator used */
    SysTick->VAL   =  (0x00);
    SysTick->CTRL = (1 << SYSTICK_CLKSOURCE) | (1<<SYSTICK_ENABLE);

    /* Waiting for down-count to zero */
    while((SysTick->CTRL & (1 << 16)) == 0);
}

/*---------------------------------------------------------------------------------------------------------*/
/* PDMA_MEM                                                                              	   			   */
/*---------------------------------------------------------------------------------------------------------*/
void PDMA_MEM(void)
{
	STR_PDMA_T sPDMA;  
    volatile uint32_t i;
	BuildSrcPattern((uint32_t)SrcArray,TEST_LENGTH);

    ClearBuf((uint32_t)DestArray, TEST_LENGTH,0xFF);
    
	/* PDMA Init */
    DrvPDMA_Init();

	/* PDMA Setting */
    sPDMA.u8TransWidth 				= eDRVPDMA_WIDTH_32BITS;
	sPDMA.i32ByteCnt                = TEST_LENGTH;	
	sPDMA.sSrcAddr.u32Addr 			= (uint32_t)SrcArray;
    sPDMA.sDestAddr.u32Addr 		= (uint32_t)DestArray;   
	sPDMA.u8Mode 					= eDRVPDMA_MODE_MEM2MEM;
	sPDMA.sSrcAddr.eAddrDirection 	= eDRVPDMA_DIRECTION_INCREMENTED; 
	sPDMA.sDestAddr.eAddrDirection 	= eDRVPDMA_DIRECTION_INCREMENTED;   
	DrvPDMA_Open(eDRVPDMA_CHANNEL_2,&sPDMA);

	/* Enable INT */
    DrvPDMA_EnableInt(eDRVPDMA_CHANNEL_2, eDRVPDMA_BLKD );
     
	/* Install Callback Function */   
 	DrvPDMA_InstallCallBack(eDRVPDMA_CHANNEL_2, eDRVPDMA_BLKD,	(PFN_DRVPDMA_CALLBACK) PDMA2_Callback ); 
	
	/* Trigger PDMA specified channel */	
	IntCnt = 0;
	IsTestOver=FALSE;
    DrvPDMA_CHEnablelTransfer(eDRVPDMA_CHANNEL_2);

	/* Trigger PDMA and the S/W Flag will be change in PDMA callback funtion */
	while(IsTestOver==FALSE);

	/* Close PDMA Channel */
	DrvPDMA_Close();	
}

/*-------------------------------------------------------------------------------*/
/* PDMA Sample Code: SPI TX/RX PDMA                                              */
/*-------------------------------------------------------------------------------*/
void PDMA_SPI(void)
{
	STR_PDMA_T sPDMA;  

	BuildSrcPattern((uint32_t)SrcArray,TEST_LENGTH);
    	
    ClearBuf((uint32_t)DestArray, TEST_LENGTH,0xFF);
    
	/* PDMA Init */
    DrvPDMA_Init();

	/* PDMA Setting */
	DrvPDMA_SetCHForAPBDevice(eDRVPDMA_CHANNEL_1,eDRVPDMA_SPI0,eDRVPDMA_WRITE_APB);
	DrvPDMA_SetCHForAPBDevice(eDRVPDMA_CHANNEL_0,eDRVPDMA_SPI0,eDRVPDMA_READ_APB);

	/* CH1 TX Setting */
	sPDMA.sSrcAddr.u32Addr 			= (uint32_t)SrcArray;
    sPDMA.sDestAddr.u32Addr 		= (uint32_t)&(SPI0->TX[0]);   
    sPDMA.u8TransWidth 				= eDRVPDMA_WIDTH_32BITS;
	sPDMA.u8Mode 					= eDRVPDMA_MODE_MEM2APB;
	sPDMA.sSrcAddr.eAddrDirection 	= eDRVPDMA_DIRECTION_INCREMENTED; 
	sPDMA.sDestAddr.eAddrDirection 	= eDRVPDMA_DIRECTION_FIXED;   
	sPDMA.i32ByteCnt                = TEST_LENGTH;
	DrvPDMA_Open(eDRVPDMA_CHANNEL_1,&sPDMA);

 	/* CH0 RX Setting */
	sPDMA.sSrcAddr.u32Addr 			= (uint32_t)&(SPI0->RX[0]);
    sPDMA.sDestAddr.u32Addr 		= (uint32_t)DestArray;   
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

   	
	IntCnt = 0;
	IsTestOver=FALSE;
  
 	DrvPDMA_CHEnablelTransfer(eDRVPDMA_CHANNEL_0);
	DrvPDMA_CHEnablelTransfer(eDRVPDMA_CHANNEL_1);

	/* Trigger PDMA 10 time and the S/W Flag will be change in PDMA callback funtion */
	while(IsTestOver==FALSE);


	/* Close PDMA Channel */
	DrvPDMA_Close(); 	
}

/*---------------------------------------------------------------------------------------------------------*/
/* InitialSystemClock                                                                      	   			   */
/*---------------------------------------------------------------------------------------------------------*/
void InitialSystemClock(void)
{
    /* Unlock the protected registers */	
	UNLOCKREG();

	/* HCLK clock source. 0: internal 49.152MHz oscillator */
	DrvSYS_SetHCLKSource(0);

	LOCKREG();

	/* HCLK clock frequency = HCLK clock source / (HCLK_N + 1) */
	DrvSYS_SetClockDivider(E_SYS_HCLK_DIV, 0); 
}

/*---------------------------------------------------------------------------------------------------------*/
/* InitialUART                                                                          	   			   */
/*---------------------------------------------------------------------------------------------------------*/
void InitialUART(void)
{
	STR_UART_T sParam;

	/* Set UART Pin */
	DrvGPIO_InitFunction(FUNC_UART0);
	
	/* UART Setting */
    sParam.u32BaudRate 		= 115200;
    sParam.u8cDataBits 		= DRVUART_DATABITS_8;
    sParam.u8cStopBits 		= DRVUART_STOPBITS_1;
    sParam.u8cParity 		= DRVUART_PARITY_NONE;
    sParam.u8cRxTriggerLevel= DRVUART_FIFO_1BYTES;

	/* Set UART Configuration */
	DrvUART_Open(UART_PORT0,&sParam);
}

/*---------------------------------------------------------------------------------------------------------*/
/* InitialSPIPortMaster                                                                    	   			   */
/*---------------------------------------------------------------------------------------------------------*/
void InitialSPIPortMaster(void)
{
	/* Configure SPI0 as a master, 32-bit transaction */
	DrvSPI_Open(eDRVSPI_PORT0, eDRVSPI_MASTER, eDRVSPI_TYPE1, 32);
	
	/* Enable the automatic slave select function of SS0. */
	DrvSPI_EnableAutoCS(eDRVSPI_PORT0, eDRVSPI_SS0);

	/* Set the active level of slave select. */
	DrvSPI_SetSlaveSelectActiveLevel(eDRVSPI_PORT0, eDRVSPI_ACTIVE_LOW_FALLING);

	/* SPI clock rate 3MHz */
	DrvSPI_SetClock(eDRVSPI_PORT0, 3000000, 0);

	/* Enable the SPI0 interrupt and install the callback function. */
	//DrvSPI_EnableInt(eDRVSPI_PORT0, SPI0_Callback, 0);
}

/*---------------------------------------------------------------------------------------------------------*/
/* MAIN function                                                                           	   			   */
/*---------------------------------------------------------------------------------------------------------*/

int32_t main()
 {
	InitialSystemClock();
    
	InitialUART();

	InitialSPIPortMaster();

	/* PDMA Sample Code: memory to memory test */
	printf("+------------------------------------------------------------------------+\n");
    printf("|                         PDMA Driver Sample Code                        |\n");
    printf("|                                                                        |\n");
    printf("+------------------------------------------------------------------------+\n");  	                  
	printf("  This sample code will use PDMA to do memory to memory test. \n");
	printf("  Test loopback %d times \n", MEM_TEST_CNT);
    printf("  press any key to continue ...\n");	  
	getchar();

	IsTestOver =FALSE;
	PDMA_MEM();
    printf("\n  PDMA MEM sample code is complete.\n\n");

	/* PDMA Sample Code: UART0 Tx */	
	printf("+------------------------------------------------------------------------+\n");
    printf("|                         PDMA Driver Sample Code                        |\n");
    printf("|                                                                        |\n");
    printf("+------------------------------------------------------------------------+\n");                    
	printf("  This sample code will use PDMA to do UART0 transfer test. \n");
	printf("  Test UART transfer %d times \n", UART_TEST_CNT);
    printf("  press any key to continue ...\n");
	getchar();

	IsTestOver =FALSE;
	PDMA_UART();
	printf("\n  PDMA UART sample code is complete.\n\n");
	

	/* PDMA Sample Code: SPI0 Tx/Rx Loopback */	
	printf("+------------------------------------------------------------------------+\n");
    printf("|                         PDMA Driver Sample Code                        |\n");
    printf("|                                                                        |\n");
    printf("+------------------------------------------------------------------------+\n");                    
	printf("  This sample code will use PDMA to do SPI0 loopback test. \n");
	printf("  Test loopback %d times \n",SPI_TEST_CNT);
	printf("  I/O configuration:\n");
    printf("    GPA0 <--> GPA3\n\n");
    printf("  press any key to continue ...\n");
	getchar();

	IsTestOver =FALSE;
	PDMA_SPI();
	printf("\n  PDMA SPI sample code is complete.\n\n");

	printf("\n  PDMA Demo End.\n\n");
    while(1);

}
