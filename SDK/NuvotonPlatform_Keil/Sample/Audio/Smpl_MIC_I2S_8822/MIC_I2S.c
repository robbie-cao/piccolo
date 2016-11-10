/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                         */
/* Copyright(c) 2009 Nuvoton Technology Corp. All rights reserved.                                         */
/*                                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
/*   This Sample code use ADC SR 32k mono as 16k stereo data for I2S stereo test                           */
/*---------------------------------------------------------------------------------------------------------*/
#include <stdio.h>
#include "ISD9xx.h"
#include "Driver\DrvUART.h"
#include "Driver\DrvGPIO.h"
#include "Driver\DrvSYS.h"
#include "DrvI2C.h"

#include "NVTTypes.h"


/*---------------------------------------------------------------------------------------------------------*/
/* Define Function Prototypes                                                                              */
/*---------------------------------------------------------------------------------------------------------*/
void InitialUART(void);
void RecordStart(void);
void PlayStart(void);


/*---------------------------------------------------------------------------------------------------------*/
/* Define global variables                                                                                 */
/*---------------------------------------------------------------------------------------------------------*/

extern volatile uint32_t CallBackCounter0;
extern BOOL	bI2sBufferEmpty;


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

/*---------------------------------------------------------------------------------------------------------*/
/* SysTimerDelay                                                                                           */
/*---------------------------------------------------------------------------------------------------------*/
void SysTimerDelay(uint32_t us)
{
    SysTick->LOAD = us * 49; /* Assume the internal 49MHz RC used */
    SysTick->VAL  =  (0x00);
    SysTick->CTRL = (1 << SYSTICK_CLKSOURCE) | (1<<SYSTICK_ENABLE);

    /* Waiting for down-count to zero */
    while((SysTick->CTRL & (1 << 16)) == 0);
}

void InitialI2C(void)
{
	/*  GPIO initial and select operation mode for I2C*/
    DrvGPIO_InitFunction(FUNC_I2C0); //Set I2C I/O
	//DrvI2C_Open(I2C_PORT0, (DrvSYS_GetHCLK() * 1000), 12000);  //clock = 24Kbps
	DrvI2C_Open(I2C_PORT0, (DrvSYS_GetHCLK() * 1000), 48000);  //clock = 48Kbps
	DrvI2C_EnableInt(I2C_PORT0); //Enable I2C0 interrupt and set corresponding NVIC bit
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

	SYSCLK->CLKSEL2.I2S_S 	= 2;  // HCLK

	DrvADC_AnaOpen();

	/* Set UART Configuration */
    //UartInit();
	InitialI2C();
	WAU8822_Setup();

	InitialI2S();

	RecordStart();
	while(CallBackCounter0 == 0);
	//DownSampling();
	//while(CallBackCounter0 == 1);

	StartI2SPdma();
	//DownSampling();

	while(1)
	{
		if(bI2sBufferEmpty==TRUE)
		{
		  	bI2sBufferEmpty=FALSE;
			//DownSampling();
		}
	}

	/* Lock protected registers */
	
}



