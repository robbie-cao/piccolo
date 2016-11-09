#define __RECPLAY_DEMO__
/***********************************************************************/
/* (C) Copyright Information Storage Devices, a Nuvoton Company        */
/***********************************************************************/ 
/* Project:        ISD1500                                             */
/* Author:         GBJ                     Creation Date:  16 Feb  05  */
/* Filename:       vpromt_editor.c         Language:       C           */
/* Rights:         X                       Rights:         X           */
/*                                                                     */
/* Compiler:   MSVC                         Assembler:                 */
/* Version:    4.0                          Version:                   */
/***********************************************************************/
/***********************************************************************/ 
/* External Function Prototypes                                        */ 
/***********************************************************************/
/* Standard ANSI C header files                                        */
/***********************************************************************/
#include <stdio.h>
#include "isd9xx.h"
//#include "SemiHost.h"
//#include "CoOS.h"
//#include "defs.h"
/***********************************************************************/
/* Global Data Declarations                                            */ 
/***********************************************************************/

/***********************************************************************/
/* Home header file                                                    */
/***********************************************************************/ 
//#include "RecPlay_demo.h"             
/***********************************************************************/ 
/***********************************************************************/ 
/* External Declarations                                               */ 
/***********************************************************************/
//#include "spi_cmd.h"
/***********************************************************************/
/* Header files for other modules   		                       */
/***********************************************************************/
#include "DrvPDMA.h"
#include "DrvUART.h"
#include "DrvSYS.h"
#include "DrvGPIO.h"
#include "DrvSPI.h"
#include "DrvI2S.h"
#include "DrvOSC.h"
//#include "i2s.h"
//#include "MemManage.h"
//#include "CompEngine.h"


//#include "../SpiFlash/c2082.h" /* Header file with global prototypes */
//#include "../SpiFlash/Serialize.h" /* Header file with SPI master abstract prototypes */
//#include "dataflash.h"
/***********************************************************************/ 
/* Functions Details:                			               */ 
/***********************************************************************/
/* Executable functions                  		               */
/***********************************************************************/
//#define BUFF_LEN    64
//
//uint32_t PcmBuff[BUFF_LEN] = {0};
//uint32_t u32BuffPos = 0;
//uint32_t u32startFlag;
//S_DRVI2S_DATA_T st;
//uint8_t u8Divider;
//int MclkFreq;



extern uint32_t SystemFrequency;

//#define outpw(port,value)	*((volatile unsigned int *)(port))=value
#define GAIN_UPDATE 0x100

//extern uint32_t g_timer0Ticks;
//extern uint32_t g_timer1Ticks;
//extern uint32_t g_timer2Ticks;
//extern uint32_t g_timer3Ticks;

extern void TimerInit(void);
extern void PwmInit(void);
extern void I2SInit(void);
extern void UART_INT_HANDLE(uint32_t u32IntStatus);
//extern OS_FlagID UARTRxFlag;
//extern OS_FlagID UARTTxFlag;

uint32_t isr_cnt=0;
uint32_t srv_cnt=0;



#define RXBUFSIZE 64
volatile uint8_t comRbuf[RXBUFSIZE];
volatile uint16_t comRbytes = 0;		/* Available receiving bytes */
volatile uint16_t comRhead 	= 0;
volatile uint16_t comRtail 	= 0;
volatile int32_t g_bWait 	= TRUE;

extern uint32_t GetUartCLk(void);
extern 	CapSense(void);

/*---------------------------------------------------------------------------------------------------------*/
/* Define functions prototype                                                                              */
/*---------------------------------------------------------------------------------------------------------*/
void UART_INT_HANDLE(uint32_t u32IntStatus);


/*---------------------------------------------------------------------------------------------------------*/
/* UART Callback function                                                                           	   */
/*---------------------------------------------------------------------------------------------------------*/
//void UART_INT_HANDLE(uint32_t u32IntStatus)
//{
//
// 	uint8_t bInChar[1]={0xFF};
//	
//
//	if(u32IntStatus & RDAIE)
//	{
////WW:		if(DrvUART_GetIntStatus(UART_PORT0,DRVUART_RDAINT))
////WW:			printf("\nInput:(RDAIE)");
//
////WW:		if(DrvUART_kbhit() ==1)
////WW:			printf("\nDrvUART_kbhit");
//
//		printf("\nInput:");
//		
//		/* Get all the input characters */
//		while(UART0->ISR.RDA_IF==1) 
//		{
//			/* Get the character from UART Buffer */
//			DrvUART_Read(UART_PORT0,bInChar,1);
//
//			printf("%c ", bInChar[0]);
//			
//			if(bInChar[0] == '0')	
//			{	
//				g_bWait = FALSE;
//			}
//		
//			/* Check if buffer full */
//			if(comRbytes < RXBUFSIZE)
//			{
//				/* Enqueue the character */
//				comRbuf[comRtail] = bInChar[0];
//				comRtail = (comRtail == (RXBUFSIZE-1)) ? 0 : (comRtail+1);
//				comRbytes++;
//			}			
//		}
//		printf("\nTransmission Test:");
//	}
//	else if(u32IntStatus & THREIE)	 //YL  ???
//	{   
//		   
//        uint16_t tmp;
//        tmp = comRtail;
//		
////WW:		if(DrvUART_GetIntStatus(UART_PORT0,DRVUART_THREINT))
////WW:			printf("\nInput:(THREIE)");
//
//
//		if(comRhead != tmp)
//		{
//			bInChar[0] = comRbuf[comRhead];
//			DrvUART_Write(UART_PORT0,bInChar,1);
//			comRhead = (comRhead == (RXBUFSIZE-1)) ? 0 : (comRhead+1);
//			comRbytes--;
//		}
//	}
//
//}
//
//

/*----------------------------------------------------------------------------
  MAIN function
  *----------------------------------------------------------------------------*/
int main (void)
{
	//SYS->OSCTRIM[0].TRIM = 113;	
	UNLOCKREG();
	SYSCLK->PWRCON.OSC49M_EN = 1;
	SYSCLK->PWRCON.OSC10K_EN = 1;
	SYSCLK->PWRCON.XTL32K_EN = 1;
	SYSCLK->CLKSEL0.STCLK_S = 3; /* Use internal HCLK */
	 		
	SYSCLK->CLKSEL0.HCLK_S = 0; /* Select HCLK source as 48MHz */ 
	SYSCLK->CLKDIV.HCLK_N  = 0;	/* Select no division          */



	SYSCLK->APBCLK.ANA_EN   = 1;               /* Turn on analog peripheral clock */
	SYSCLK->APBCLK.ACMP_EN  = 1;
	//SYSCLK->APBCLK.I2S_EN   = 1;
	//SYSCLK->APBCLK.DPWM_EN  = 1;
	//SYSCLK->APBCLK.I2C0_EN  = 1;

	LOCKREG();
	CapSense();	
    
}

