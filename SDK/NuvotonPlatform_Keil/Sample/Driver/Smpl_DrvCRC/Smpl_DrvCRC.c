/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                         */
/* Copyright (c) Nuvoton Technology Corp. All rights reserved.                                             */
/*                                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include "ISD9xx.h"
#include "Driver\DrvSYS.h"
#include "Driver\DrvCRC.h"
#include "Driver\DrvUART.h"
#include "Driver\DrvGPIO.h"

#define MAX_PACKET_TEST 510
#define CRC_PKT 32
 
int main(void)
{
	// CRC data should be aligned on word boundary
	__align(4) uint8_t CRCdata[MAX_PACKET_TEST+2];
	STR_UART_T param;
    int32_t  i,j,TestPassed=1;
	uint16_t CalcCRC;
    uint32_t *u32ptr;
    
    /* Unlock the protected registers */	
	UNLOCKREG();

 
	/* HCLK clock source. 0: internal 49MHz;  */
	DrvSYS_SetHCLKSource(0);

	LOCKREG();

	DrvSYS_SetClockDivider(E_SYS_HCLK_DIV, 0); /* HCLK clock frequency = HCLK clock source / (HCLK_N + 1) */

	DrvGPIO_InitFunction(FUNC_UART0);
	
	param.u32BaudRate        = 115200;
	param.u8cDataBits        = DRVUART_DATABITS_8;
	param.u8cStopBits        = DRVUART_STOPBITS_1;
	param.u8cParity          = DRVUART_PARITY_NONE;
	param.u8cRxTriggerLevel  = DRVUART_FIFO_1BYTES;
	param.u8TimeOut        	 = 0;
	DrvUART_Open(UART_PORT0, &param);
	

	
	printf("\n\n");
	printf("+----------------------------------------------------------------------+\n");		
	printf("|                       CRC Driver Sample Code                         |\n");
	printf("|                                                                      |\n");
    printf("+----------------------------------------------------------------------+\n");
	printf("\n");
	printf("CRC Driver version: %x\n", DrvCRC_GetVersion());
	printf("Press any key to start test.\n");
	getchar();
    
 		
	// Some CRC testing

	// Fill data array
	for(i=0; i<MAX_PACKET_TEST; i++)
		  CRCdata[i]= rand();

   
    DrvCRC_Open();
	
	// Test a 64byte CRC
	u32ptr = (uint32_t *)CRCdata;
    DrvCRC_Init( eDRVCRC_LSB, CRC_PKT);
	CalcCRC = DrvCRC_Calc( u32ptr, CRC_PKT);
    printf("\nCalculate CRC of ");
	for(i=0;i<CRC_PKT;i++)
		printf("%02x", CRCdata[i]);
   	printf("\nCRC %d bytes =  %04X\nAdd CRC to packet and recalculate to check, result should be zero\n",i, CalcCRC);
    // Test by adding CRC to packet and recalculating
	CRCdata[i++] = CalcCRC >> 8;
	CRCdata[i++] = CalcCRC & 0xFF;
	u32ptr = (uint32_t *)CRCdata;
    DrvCRC_Init( eDRVCRC_LSB, CRC_PKT+2);
	CalcCRC = DrvCRC_Calc( u32ptr, CRC_PKT+2);
	if(CalcCRC != 0)
		printf("CRC Failed check CRC=%04X\n",CalcCRC);
	else
		printf("CRC Passed check CRC=%04X\n",CalcCRC);
	printf("Calculate CRC of various packets and check results\n");    
	for(j=2; j<=MAX_PACKET_TEST ; j+=2){
        // Do CRC for j bytes
        u32ptr = (uint32_t *)CRCdata;
        DrvCRC_Init( eDRVCRC_LSB, j);
		CalcCRC = DrvCRC_Calc( u32ptr, j);
       
        printf(".");
        if(j%64==0)
			printf("\nCRC %d bytes %x \n",j, CalcCRC);
		 // Now check CRC
        CRCdata[j++] =  CRC->CRC_OUT >> 8;
        CRCdata[j++] =  CRC->CRC_OUT & 0xff;
        u32ptr = (uint32_t *)CRCdata;
		DrvCRC_Init( eDRVCRC_LSB, j);
		CalcCRC = DrvCRC_Calc( u32ptr, j);
       	j-=2;
		if(CalcCRC != 0){
			printf("\nCRC Fail %d bytes %x ",j, CRC->CRC_OUT);
			TestPassed =0;
		}
        CRCdata[j++] = rand();
        CRCdata[j++] = rand();
        j-=2;
	}	
	if(TestPassed)
    printf("\n\nCRC driver sample passed all tests.\n");
	else
    printf("\n\nCRC driver sample failed.\n");

	DrvCRC_Close();

	return 1;
}


