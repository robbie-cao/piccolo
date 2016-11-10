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
#define BUFF_LEN    64

uint32_t PcmBuff[BUFF_LEN] = {0};
uint32_t u32BuffPos = 0;
uint32_t u32startFlag;
S_DRVI2S_DATA_T st;
uint8_t u8Divider;
int MclkFreq;



extern uint32_t SystemFrequency;

#define outpw(port,value)	*((volatile unsigned int *)(port))=value
#define GAIN_UPDATE 0x100

extern uint32_t g_timer0Ticks;
extern uint32_t g_timer1Ticks;
extern uint32_t g_timer2Ticks;
extern uint32_t g_timer3Ticks;

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
extern 	WAU88XX_EnterADCTestMode(void);

/*---------------------------------------------------------------------------------------------------------*/
/* Define functions prototype                                                                              */
/*---------------------------------------------------------------------------------------------------------*/
void UART_INT_HANDLE(uint32_t u32IntStatus);


/*---------------------------------------------------------------------------------------------------------*/
/* UART Callback function                                                                           	   */
/*---------------------------------------------------------------------------------------------------------*/
void UART_INT_HANDLE(uint32_t u32IntStatus)
{

 	uint8_t bInChar[1]={0xFF};
	

	if(u32IntStatus & RDAIE)
	{
//WW:		if(DrvUART_GetIntStatus(UART_PORT0,DRVUART_RDAINT))
//WW:			printf("\nInput:(RDAIE)");

//WW:		if(DrvUART_kbhit() ==1)
//WW:			printf("\nDrvUART_kbhit");

		printf("\nInput:");
		
		/* Get all the input characters */
		while(UART0->ISR.RDA_IF==1) 
		{
			/* Get the character from UART Buffer */
			DrvUART_Read(UART_PORT0,bInChar,1);

			printf("%c ", bInChar[0]);
			
			if(bInChar[0] == '0')	
			{	
				g_bWait = FALSE;
			}
		
			/* Check if buffer full */
			if(comRbytes < RXBUFSIZE)
			{
				/* Enqueue the character */
				comRbuf[comRtail] = bInChar[0];
				comRtail = (comRtail == (RXBUFSIZE-1)) ? 0 : (comRtail+1);
				comRbytes++;
			}			
		}
		printf("\nTransmission Test:");
	}
	else if(u32IntStatus & THREIE)	 //YL  ???
	{   
		   
        uint16_t tmp;
        tmp = comRtail;
		
//WW:		if(DrvUART_GetIntStatus(UART_PORT0,DRVUART_THREINT))
//WW:			printf("\nInput:(THREIE)");


		if(comRhead != tmp)
		{
			bInChar[0] = comRbuf[comRhead];
			DrvUART_Write(UART_PORT0,bInChar,1);
			comRhead = (comRhead == (RXBUFSIZE-1)) ? 0 : (comRhead+1);
			comRbytes--;
		}
	}

}

void LoopDelay(uint32_t delayCnt)
{
    while(delayCnt--)
        {
            __NOP();
            __NOP();
        }
}

/*---------------------------------------------------------------------------------------------------------*/
/*  I2S Tx Threshold Level Callback Function when Tx FIFO is less than Tx FIFO Threshold Level             */
/*---------------------------------------------------------------------------------------------------------*/
void Tx_thresholdCallbackfn(uint32_t status)
{
	uint32_t u32Len, i;
	uint32_t * pBuff;

	pBuff = &PcmBuff[0];

	/* Read Tx FIFO free size */
	u32Len = 8 - _DRVI2S_READ_TX_FIFO_LEVEL();
	
	if (u32BuffPos >= 8)
	{
		for	(i = 0; i < u32Len; i++)
		{
	   		_DRVI2S_WRITE_TX_FIFO(pBuff[i]);
		}

		for (i = 0; i < BUFF_LEN - u32Len; i++)
		{
			pBuff[i] = pBuff[i + u32Len];	
		}

		u32BuffPos -= u32Len;
	}
	else
	{
		for	(i = 0; i < u32Len; i++)
		{
	   		_DRVI2S_WRITE_TX_FIFO(0x00);	   
		}			
	}
}

/*---------------------------------------------------------------------------------------------------------*/
/*  I2S Rx Threshold Level Callback Function when Rx FIFO is more than Rx FIFO Threshold Level             */
/*---------------------------------------------------------------------------------------------------------*/
void Rx_thresholdCallbackfn(uint32_t status)
{
	uint32_t u32Len, i;
	uint32_t *pBuff;

	if (u32BuffPos < (BUFF_LEN-8))
	{
		pBuff = &PcmBuff[u32BuffPos];
		
		/* Read Rx FIFO Level */
		u32Len = _DRVI2S_READ_RX_FIFO_LEVEL();
	
		for ( i = 0; i < u32Len; i++ )
		{
			pBuff[i] = _DRVI2S_READ_RX_FIFO();
		}
	
		u32BuffPos += u32Len;
	
		if (u32BuffPos >= BUFF_LEN)
		{
			u32BuffPos =	0;
		}						 	
	}
}



/*----------------------------------------------------------------------------
  MAIN function
  *----------------------------------------------------------------------------*/
int main (void)
{
//#ifdef VERILOG
//	uint32_t reg_tmp, i;
//    uint32_t uiTmp;
//	char ch, Menu=0;
//	uint32_t Addr=0;
//#endif
//	uint32_t F1, F2;
//    OS_TID Tid;
//	S_DRVI2S_DATA_T st;	


	UNLOCKREG();
	SYSCLK->PWRCON.OSC49M_EN = 1;
	SYSCLK->PWRCON.OSC10K_EN = 1;
	SYSCLK->PWRCON.XTL32K_EN = 1;
	SYSCLK->CLKSEL0.STCLK_S = 3; /* Use internal HCLK */

//	/* Trim secondary oscillator to 32*1.024 MHz. */
//	SystemFrequency = 32768000; 	
//	F1 = TrimOscillator(SystemFrequency,1);
//	 
//	/* The default CPU clock source is 48*1.024 MHz. */
//	SystemFrequency = 49152000;
//	F2 = TrimOscillator(SystemFrequency,0);
	
	SYSCLK->CLKSEL0.HCLK_S = 0; /* Select HCLK source as 48MHz */ 
	SYSCLK->CLKDIV.HCLK_N  = 0;	/* Select no division          */
	SYSCLK->CLKSEL0.OSCFSel = 0;	/* 1= 32MHz, 0=48MHz */
	//--- Enable PDMA clock---
//	SYSCLK->AHBCLK.PDMA_EN  = 1;	
	SYSCLK->APBCLK.ANA_EN   = 1;               /* Turn on analog peripheral clock */
	SYSCLK->APBCLK.I2S_EN   = 1;
	SYSCLK->APBCLK.DPWM_EN  = 1;	 
	SYSCLK->APBCLK.I2C0_EN  = 1;

	/* Enable ISP function */
//	DrvFMC_EnableISP(1);
    //	DrvFMC_EnableConfigUpdate(1);
    //	DrvFMC_WriteConfig(0xfffffffe, 0x00010000);

	LOCKREG();
	
//	CoInitOS();
		
//	UARTTxFlag = CoCreateFlag(TRUE,0);
//	UARTRxFlag = CoCreateFlag(TRUE,0);

	DrvGPIO_Open(GPB, 0, IO_OUTPUT);

#ifndef SEMIHOST
    /* Init UART0 as 115200 bps, 8 bits, no-parity, 1 stop bit. */
	UartInit(115200);
	DrvUART_EnableInt(UART_PORT0, (DRVUART_RLSNT | DRVUART_RDAINT), UART_INT_HANDLE);
	
#endif

	
    printf("+-----------------------------------------------------------+\n");
    printf("|             Development Board Demo Program                |\n");
    printf("|                                                           |\n");
    printf("+-----------------------------------------------------------+\n");

	
	WAU88XX_EnterADCTestMode();
//    AudioInit ();

	printf("OK\n");

	SYS->IPRSTC2.I2S_RST = 1;
	SYS->IPRSTC2.I2S_RST = 0;
	SYSCLK->APBCLK.I2S_EN = 1;

	//	// GPIOB[1] is MCLK
	SYS->GPB_ALT.GPB1 = 1;
	I2S->CON.I2SEN = 1;
	// Enable MCLK output
	I2S->CON.MCLKEN = 1;
	I2S->CLKDIV.BCLK_DIV = 1;
   	I2S->CLKDIV.MCLK_DIV = 6 ;	
	I2S->CON.WORDWIDTH 	= DRVI2S_DATABIT_16;
	I2S->CON.MONO 		= DRVI2S_STEREO;
	I2S->CON.FORMAT 	= DRVI2S_FORMAT_I2S;
	I2S->CON.SLAVE 		= DRVI2S_MODE_SLAVE;
	I2S->CON.TXTH 		= DRVI2S_FIFO_LEVEL_WORD_0;
	I2S->CON.RXTH 		= DRVI2S_FIFO_LEVEL_WORD_8-1;
	//u32SrcClk = DrvI2S_GetSourceClock();		  //49152000	
	//u32BitRate = sParam->u32SampleRate * (sParam->u8WordWidth + 1) * 16;
	u8Divider = ((49152000/512000) >> 1) - 1;	
	I2S->CLKDIV.BCLK_DIV = u8Divider;	
	//I2S->CON.I2SEN = 1;	

	/* Set I2S I/O */
    //DrvGPIO_InitFunction(FUNC_I2S);
    SYS->GPA_ALT.GPA4     =1; // FS ALT FUNC
    SYS->GPA_ALT.GPA5     =1; // BCLK ALT FUNC
    SYS->GPA_ALT.GPA6     =1; // SDI ALT FUNC
    SYS->GPA_ALT.GPA7     =1; // SDO ALT FUNC
	*(volatile unsigned int*)(0x400A0004)=0xAF02;

//    CoStartOS();


	DrvI2S_EnableInt(I2S_RX_FIFO_THRESHOLD, Rx_thresholdCallbackfn);
	u32startFlag = 1;
	/* Enable I2S Rx function to receive data */
	DrvI2S_EnableRx(TRUE);

	while(1)
	{
		if (u32startFlag)
		{
			/* Enable I2S Tx function to send data when data in the buffer is more than half of buffer size */
			if (u32BuffPos >= BUFF_LEN/2)
			{
				DrvI2S_EnableInt(I2S_TX_FIFO_THRESHOLD, Tx_thresholdCallbackfn);	
				DrvI2S_EnableTx(TRUE);
				u32startFlag = 0;
			}
		}
	}


    
}

