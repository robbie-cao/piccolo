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
#include "Driver\DrvGPIO.h"

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




#include "PlayG722.h"

void PlayStreamDemo(void)
{
	PlayG722Open();
	PlayG722Stream(LIGHTS_ARE_ON);
	//printf(" Sound 0 Played\n");
	PlayG722Stream(BRIGHTER);
	//printf(" Sound 1 Played\n");
	PlayG722Stream(DIMMER);
	//printf(" Sound 2 Played\n");
	PlayG722Stream(LIGHTS_ARE_OFF);
	//printf(" Sound 3 Played\n");
	PlayG722Stream(TRY_AGAIN);
	//printf(" Sound 4 Played\n");
	PlayG722Close();
}

void PlayDemo(void)
{
	PlayG722(LIGHTS_ARE_ON);
	//printf(" Sound 0 Played\n");
	PlayG722(BRIGHTER);
	//printf(" Sound 1 Played\n");
	PlayG722(DIMMER);
	//printf(" Sound 2 Played\n");
	PlayG722(LIGHTS_ARE_OFF);
	//printf(" Sound 3 Played\n");
	PlayG722(TRY_AGAIN);
	//printf(" Sound 4 Played\n");
}

/*---------------------------------------------------------------------------------------------------------*/
/* MAIN function                                                                           	   			   */
/*---------------------------------------------------------------------------------------------------------*/
int32_t main(void)
{						

	InitialSystemClock();
    
	UartInit();

	//DrvGPIO_Open(GPB,2,IO_OUTPUT);	//For scope measurement
	//DrvGPIO_SetBit(GPB,2);		//For scope measurement

	//DrvTIMER_Init();
	while(1)
		PlayStreamDemo();
		//PlayDemo();


}	


