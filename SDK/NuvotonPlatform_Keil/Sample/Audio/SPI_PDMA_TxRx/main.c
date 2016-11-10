/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                         */
/* Copyright(c) 2009 Nuvoton Technology Corp. All rights reserved.                                         */
/*                                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
/* This code can be SPI master or SPI slave															       */
/*---------------------------------------------------------------------------------------------------------*/
#include <stdio.h>
#include "ISD9xx.h"
#include "Driver\DrvUART.h"
#include "Driver\DrvSYS.h"
#include "Driver\DrvSPI.h"
#include "Driver\DrvPDMA.h"
#include "NVTTypes.h"

#define TEST_LENGTH	24			//unit:bytes
#define SPI_MASTER


extern BOOL bPdmaRxDone, bPdmaTxDone, bIsTestOver;
extern uint8_t au8SrcArray[TEST_LENGTH], au8DestArray[TEST_LENGTH];

/*---------------------------------------------------------------------------------------------------------*/
/* Define Function Prototypes                                                                              */
/*---------------------------------------------------------------------------------------------------------*/
void InitialUART(void);

void TouchSenseInit(uint8_t u8AverageCount);
void TouchSense(void);


void InitialSPIPortSlave(void);
int32_t SendSpiCmd(uint8_t u8Command, uint8_t u8MessageNo, uint16_t u16Data);
int32_t ReadSpiCmd(uint32_t *pu32Buf);




//====================
// Functions & main

void UartInit(void)
{
	/* Reset IP */
	SYS->IPRSTC2.UART0_RST = 1;
	SYS->IPRSTC2.UART0_RST = 0;
	/* Enable UART clock */
    SYSCLK->APBCLK.UART0_EN = 1;
    /* Data format */
    UART0->LCR.WLS = 3;
    /* Configure the baud rate */
    M32(&UART0->BAUD) = 0x3F0001A8; /* Internal 48MHz, 115200 bps */

    /* Multi-Function Pin: Enable UART0:Tx Rx */
	SYS->GPA_ALT.GPA8 = 1;
	SYS->GPA_ALT.GPA9 = 1;
}

void LdoOn(void)
{
	SYSCLK->APBCLK.ANA_EN=1;
	ANA->LDOPD.PD=0;
	ANA->LDOSET=3;
}


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
/*  Main Function									                                           			   */
/*---------------------------------------------------------------------------------------------------------*/
int32_t main (void)
{
   
	UNLOCKREG();
	SYSCLK->PWRCON.OSC49M_EN = 1;
	SYSCLK->CLKSEL0.HCLK_S = 0; /* Select HCLK source as 48MHz */ 
	SYSCLK->CLKDIV.HCLK_N  = 0;	/* Select no division          */
	SYSCLK->CLKSEL0.OSCFSel = 0;	/* 1= 32MHz, 0=48MHz */
	SYSCLK->CLKSEL0.STCLK_S = 3; /* Use internal HCLK */



	/* Set UART Configuration */

    //UartInit();
	//DrvADC_AnaOpen();			//Will reset all IP in ANA like current source, VMID, PGA, ALC, ...(ALL ANA_BA IP)

	LdoOn();  					//Enable GPA0~GPA7 power if no external power providing

#ifdef SPI_MASTER
	InitialSPIPortMaster();		//Set SPI as master
#else
	InitialSPIPortSlave();		//Set SPI as slave
#endif


	BuildSrcPattern((uint32_t)au8SrcArray,TEST_LENGTH);
    	
    ClearBuf((uint32_t)au8DestArray, TEST_LENGTH,0xFF);
    
	/* PDMA Init */
    DrvPDMA_Init();

	bPdmaRxDone=FALSE;
	bPdmaTxDone=FALSE;
	PDMA_SPI();
	bIsTestOver=FALSE;			//No function will set it for endless loop in the sample
	
	while(!bIsTestOver)
	{
		if(bPdmaRxDone==TRUE)			  	//Trigger next packet receiving
		{
			bPdmaRxDone=FALSE;
			ClearBuf((uint32_t)au8DestArray, TEST_LENGTH,0xFF);		//Clear buffer for next packet receiving, just for verification checking
			DrvSPI_StartPDMA(eDRVSPI_PORT0, eDRVSPI_RX_DMA, TRUE);
			DrvPDMA_CHEnablelTransfer(eDRVPDMA_CHANNEL_0);
		}
		if(bPdmaTxDone==TRUE)				//Trigger next packet transmitting
		{
			bPdmaTxDone=FALSE;
			DrvSPI_StartPDMA(eDRVSPI_PORT0, eDRVSPI_TX_DMA, TRUE);
			DrvPDMA_CHEnablelTransfer(eDRVPDMA_CHANNEL_1);
		}
	}
	DrvPDMA_Close();
	/* Lock protected registers */
	
}



