/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                         */
/* Copyright(c) 2009 Nuvoton Technology Corp. All rights reserved.                                         */
/*                                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
#include <stdio.h>
#include "Driver\DrvUART.h"
#include "Driver\DrvGPIO.h"
#include "Driver\DrvACMP.h"
#include "ISD9xx.h"

/*---------------------------------------------------------------------------------------------------------*/
/* Global variables                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/
static uint32_t uACMPIntCnt = 0;

/*---------------------------------------------------------------------------------------------------------*/
/* Define functions prototype                                                                              */
/*---------------------------------------------------------------------------------------------------------*/


void SysTimerDelay(uint32_t us)
{
    SysTick->LOAD = us * 22; /* Assume the internal 22MHz RC used */
    SysTick->VAL   =  (0x00);
    SysTick->CTRL = (1 << SYSTICK_CLKSOURCE) | (1<<SYSTICK_ENABLE);

    /* Waiting for down-count to zero */
    while((SysTick->CTRL & (1 << 16)) == 0);
}


void DrvACMP_ISR(void)
{
    uACMPIntCnt++;

    if(ACMP->CMPSR.CMPF0)
    {
        if(ACMP->CMPSR.CO0 == 1)
            printf("CP0 > CN0 (%d)\n", uACMPIntCnt);
        else
            printf("CP0 <= CN0 (%d)\n", uACMPIntCnt);
    }

    if(ACMP->CMPSR.CMPF1)
    {
        if(ACMP->CMPSR.CO1 == 1)
            printf("CP1 > CN1 (%d)\n", uACMPIntCnt);
        else
            printf("CP1 <= CN1 (%d)\n", uACMPIntCnt);
    }
}

/*---------------------------------------------------------------------------------------------*/
/* ACMP Test Sample 				                                                           */
/* Test Item					                                                               */
/* It sends the messages to HyperTerminal.											           */
/*---------------------------------------------------------------------------------------------*/

int32_t main()
{
	STR_UART_T sParam;

	UNLOCKREG();
    SYSCLK->PWRCON.XTL32K_EN = 1;
    LOCKREG();
    /* Waiting for 12M Xtal stalble */
    SysTimerDelay(5000);

	/* Set UART Pin */
	DrvGPIO_InitFunction(FUNC_UART0);

	/* UART Setting */
    sParam.u32BaudRate 		= 115200;
    sParam.u8cDataBits 		= DRVUART_DATABITS_8;
    sParam.u8cStopBits 		= DRVUART_STOPBITS_1;
    sParam.u8cParity 		= DRVUART_PARITY_NONE;
    sParam.u8cRxTriggerLevel= DRVUART_FIFO_1BYTES;

    DrvUART_Open(UART_PORT0,&sParam);

    printf("+----------------------------------------------------------------+\n");
    printf("|                     ACMP Sample Code                           |\n");
    printf("+----------------------------------------------------------------+\n");
    printf("  CMP0 PB.1 & CMP1 PB.6 are inputs and used to compare with 1.2v.\n");
    printf("\n");


    /* Set relative GPIO to be input & CMP mode */
    DrvGPIO_InitFunction(FUNC_ACMP0);
    DrvGPIO_InitFunction(FUNC_ACMP1);
    DrvGPIO_Open(GPB, 0, IO_INPUT);
    DrvGPIO_Open(GPB, 1, IO_INPUT);
    DrvGPIO_Open(GPB, 2, IO_INPUT);
    DrvGPIO_Open(GPB, 3, IO_INPUT);
    DrvGPIO_Open(GPB, 4, IO_INPUT);
    DrvGPIO_Open(GPB, 5, IO_INPUT);
    DrvGPIO_Open(GPB, 6, IO_INPUT);
    DrvGPIO_Open(GPB, 7, IO_INPUT);

    /* Enable ACMP clock source */
    SYSCLK->APBCLK.ACMP_EN = 1;

    /* Reset ACMP Block */
    SYS->IPRSTC2.ACMP_RST = 1;
    SYS->IPRSTC2.ACMP_RST = 0;

    /* Configure ACMP0 */
    DrvACMP_Ctrl(CMP0, CMPCR_CN0_VBG, CMPCR_CMPIE_EN, CMPCR_CMPEN_EN);

	ACMP->CMPSEL = 1;

    /* Configure ACMP1 */
    DrvACMP_Ctrl(CMP1, CMPCR_CN1_VBG, CMPCR_CMPIE_EN, CMPCR_CMPEN_EN);

    /* Enable System Interrupt */
    DrvACMP_InstallISR(DrvACMP_ISR);
    NVIC_EnableIRQ(ACMP_IRQn);

    while(1);
}

