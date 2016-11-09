/*-----------------------------------------------------------------------------------*/
/*                                                                                   */
/* Copyright(c) 2011 Nuvoton Technology Corp. All rights reserved.                   */
/*                                                                                   */
/*-----------------------------------------------------------------------------------*/
#include <stdio.h>
#include "DrvSPI.h"
#include "DrvGPIO.h"
#include "DrvSYS.h"
#include "DrvUART.h"
#include "DrvSDCARD.h"
#include "diskio.h"

/*-----------------------------------------------------------------------------------*/
/* Global variables                                                                  */
/*-----------------------------------------------------------------------------------*/
unsigned char ucWrBuff[512],ucRdBuff[512];

/*-----------------------------------------------------------------------------------*/
/* Define functions prototype                                                        */
/*-----------------------------------------------------------------------------------*/
void Delay(uint32_t delayCnt)
{
    while(delayCnt--)
    {
        __NOP();
        __NOP();
    }
}


/*----------------------------------------------*/
/* Dump a block of byte array                   */
/*----------------------------------------------*/
/* buff: Pointer to the byte array to be dumped */
/* addr: Heading address value                  */
/*  cnt: Number of bytes to be dumped           */
void put_dump (const unsigned char* buff, unsigned long addr, int cnt)
{
	int i;

	printf("%08lX ", addr);

	for (i = 0; i < cnt; i++)
		printf(" %02X", buff[i]);

	putchar(' ');
	for (i = 0; i < cnt; i++)
		putchar((char)((buff[i] >= ' ' && buff[i] <= '~') ? buff[i] : '.'));

	putchar('\n');
}

/*-----------------------------------------------------------------------------------*/
/*  MAIN function                                                                    */
/*-----------------------------------------------------------------------------------*/
int32_t main(void)
{
	int iCnt;
	unsigned char *ucBuff;
	unsigned long ulOfs;

    STR_UART_T sParam;

	UNLOCKREG();
	SYSCLK->PWRCON.OSC49M_EN = 1;
	SYSCLK->CLKSEL0.HCLK_S = 0; /* Select HCLK source as 48MHz */
	SYSCLK->CLKDIV.HCLK_N  = 0;	/* Select no division          */
	SYSCLK->CLKSEL0.OSCFSel = 0;	/* 1= 32MHz, 0=48MHz */
	LOCKREG();

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

    Delay(1000);

    printf("+----------------------------------------------------------------+\n");
    printf("|                     SD Card Sample Code                        |\n");
    printf("+----------------------------------------------------------------+\n");
    printf("| Write integer 0~511 to sector 0                                |\n");
    printf("| Read back and check the data                                   |\n");
    printf("+----------------------------------------------------------------+\n");
    printf("\n");

	printf("rc=%d\n", (DSTATUS)disk_initialize(0));

    for(iCnt=0; iCnt<512; iCnt++)
	    ucWrBuff[iCnt] = iCnt;
	printf("rc=%u\n", disk_write(0, ucWrBuff, 0, 1));

////Read back
    printf("rc=%u\n", disk_read(0, ucRdBuff, 0, 1));

//    for(iCnt=0; iCnt<512; iCnt++)
//    {
//        if(ucRdBuff[iCnt] != ucWrBuff[iCnt])
//            printf("Error in Addr[%d]:%02X\n",iCnt,ucRdBuff[iCnt]);
//    }

	for (ucBuff=(unsigned char*)ucRdBuff, ulOfs = 0; ulOfs < 0x200; ucBuff+=16, ulOfs+=16)
    	put_dump(ucBuff, ulOfs, 16);

}
