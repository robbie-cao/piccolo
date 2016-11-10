/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                         */
/* Copyright(c) 2009 Nuvoton Technology Corp. All rights reserved.                                         */
/*                                                                                                         */
/* Siren7 (G.722) licensed from Polycom Technology                                                         */
/*---------------------------------------------------------------------------------------------------------*/
#include <stdio.h>
#include "ISD9xx.h"
#include "Driver\DrvADC.h"
#include "Driver\DrvUART.h"
#include "Driver\DrvSYS.h"
#include "Driver/DrvTimer.h"
#include "NVTTypes.h"

#define TOTAL_VP_COUNT	5

extern void SpiFlashOpen(void);
extern void PlayLoop(void);
extern void PlayClose(void);

extern BOOL bPCMPlaying;

/*---------------------------------------------------------------------------------------------------------*/
/* Define functions prototype                                                                              */
/*---------------------------------------------------------------------------------------------------------*/
void InitialSystemClock(void);
void InitialUART(void);

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

/*---------------------------------------------------------------------------------------------------------*/
/* InitialSystemClock                                                                                      */
/*---------------------------------------------------------------------------------------------------------*/
void InitialSystemClock(void)
{
    /* Unlock the protected registers */	
	UNLOCKREG();

	/* HCLK clock source. */
	DrvSYS_SetHCLKSource(0);

	LOCKREG();

	/* HCLK clock frequency = HCLK clock source / (HCLK_N + 1) */
	DrvSYS_SetClockDivider(E_SYS_HCLK_DIV, 0); 
}



/*---------------------------------------------------------------------------------------------------------*/
/* InitialUART                                                                                             */
/*---------------------------------------------------------------------------------------------------------*/

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

void Delay(uint16_t	 u16Sec)
{
	uint32_t u32Start = 0;
	uint32_t u32End = 0;
	
	DrvTIMER_Open(TMR0,1000,PERIODIC_MODE);
	u32Start = DrvTIMER_GetTicks(TMR0);
	while(	u32End - u32Start < u16Sec*1000)
	{
		u32End = DrvTIMER_GetTicks(TMR0);
	}
	DrvTIMER_Close(TMR0);
}




#include "PlaySpiG722.h"

void PlayMessage(uint8_t u8MessageNo)
{
	PlaySpiG722(u8MessageNo);
				  
}

/*---------------------------------------------------------------------------------------------------------*/
/* MAIN function                                                                           	   			   */
/*---------------------------------------------------------------------------------------------------------*/
int32_t main(void)
{						
uint8_t	u8MessNo;
	InitialSystemClock();
    
	UartInit();
	SpiFlashOpen();
	bPCMPlaying=FALSE;
	u8MessNo=0;

	//DrvTIMER_Init();
	while(1)
	{
		if(bPCMPlaying==TRUE)					//Playing message handling
			PlayLoop();
		else
		{
			PlayClose();
			if(u8MessNo==TOTAL_VP_COUNT)
				u8MessNo=0;
			PlayMessage(u8MessNo);
			u8MessNo++;
		}

	}
}	


