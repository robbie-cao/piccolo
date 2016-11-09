/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                         */
/* Copyright(c) 2011 Nuvoton Technology Corp. All rights reserved.                                         */
/*                                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
#include <stdio.h>
#include "Driver\DrvGPIO.h"
#include "Driver\DrvUART.h"
#include "Driver\DrvSYS.h"

/*---------------------------------------------------------------------------------------------------------*/
/* MAIN function                                                                                          */
/*---------------------------------------------------------------------------------------------------------*/

int main (void)
{
	STR_UART_T param;
    int32_t i32Err;

	/* Step 1. Enable and Select clock source*/
	UNLOCKREG();
    SYSCLK->PWRCON.OSC49M_EN = 1;
    SYSCLK->PWRCON.OSC10K_EN = 1;
    SYSCLK->PWRCON.XTL32K_EN = 1;
    SYSCLK->CLKSEL0.STCLK_S = 3; //Use internal HCLK

//	SYSCLK->CLKSEL0.HCLK_S = 0; /* Select HCLK source as 48MHz */
	SYSCLK->CLKSEL0.HCLK_S = 1; /* Select HCLK source as 32KHz */
	SYSCLK->CLKDIV.HCLK_N  = 0;	/* Select no division          */
	SYSCLK->CLKSEL0.OSCFSel = 0;	/* 1= 32MHz, 0=48MHz */
	LOCKREG();

	/* Step 2. Select UART Operation mode */
	DrvGPIO_InitFunction(FUNC_UART0);
 //   param.u32BaudRate        = 115200;
    param.u32BaudRate        = 2400;
    param.u8cDataBits        = DRVUART_DATABITS_8;
    param.u8cStopBits        = DRVUART_STOPBITS_1;
    param.u8cParity          = DRVUART_PARITY_NONE;
    param.u8cRxTriggerLevel  = DRVUART_FIFO_1BYTES;
    param.u8TimeOut        	 = 0;
    DrvUART_Open(UART_PORT0, &param);

	printf("\n\n");
    printf("+-----------------------------------------------------------------------+\n");
    printf("|                      GPIO Driver Sample Code                          |\n");
    printf("+-----------------------------------------------------------------------+\n");

/*---------------------------------------------------------------------------------------------------------*/
/* Basic Setting                                                                                           */
/*---------------------------------------------------------------------------------------------------------*/
    /*-----------------------------------------------------------------------------------------------------*/
    /* Configure Bit0 in GPIOA to Output pin and Bit1 in GPIOA to Input pin then close it                  */
    /*-----------------------------------------------------------------------------------------------------*/
	printf("  >> Please connect GPA0 and GPA1 first <<\n");

	/* Step 3. Select GPIO Operation mode */
	DrvGPIO_Open(GPA,0, IO_OUTPUT);
	DrvGPIO_Open(GPA,1, IO_INPUT);

	DrvGPIO_ClrBit(GPA,0);

    i32Err = 0;
    printf("  GPIO Input(GPA[1])/Output(GPA[0]) test ................................ ");

	if(DrvGPIO_GetBit(GPA,1)!=0)
	{
		i32Err = 1;
	}

	DrvGPIO_SetBit(GPA,0);

	if(DrvGPIO_GetBit(GPA,1)!=1)
	{
		i32Err = 1;
	}

    DrvGPIO_Close(GPA,0);
	DrvGPIO_Close(GPA,1);


    if(i32Err)
    {
	    printf("[FAIL]\n");
        printf("\n  (Please make sure GPA0 and GPA1 are connected)\n");
    }
    else
    {
        printf("[OK]\n");
    }
}




