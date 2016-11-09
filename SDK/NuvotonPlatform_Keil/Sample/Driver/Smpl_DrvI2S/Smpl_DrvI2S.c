#define __RECPLAY_DEMO__
/***********************************************************************/
/* (C) Copyright Information Storage Devices, a Nuvoton Company        */
/***********************************************************************/

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
#include "DrvI2C.h"

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
    S_DRVI2S_DATA_T st;

	/* Step 1. Enable and select clock source*/
	UNLOCKREG();
	SYSCLK->PWRCON.OSC49M_EN = 1;
	SYSCLK->PWRCON.OSC10K_EN = 1;
	SYSCLK->PWRCON.XTL32K_EN = 1;
	SYSCLK->CLKSEL0.STCLK_S = 3; /* Use internal HCLK */

	SYSCLK->CLKSEL0.HCLK_S = 0; /* Select HCLK source as 48MHz */
	SYSCLK->CLKDIV.HCLK_N  = 0;	/* Select no division          */
	SYSCLK->CLKSEL0.OSCFSel = 0;	/* 1= 32MHz, 0=48MHz */
	SYSCLK->CLKSEL2.I2S_S 	= 2;  // HCLK

	LOCKREG();

	/* Step 2. GPIO initial and select operation mode for UART*/
	///////
	//UART
	//////
	DrvUART_Init(115200);  //Set UART I/O and UART setting

    printf("+-----------------------------------------------------------+\n");
    printf("|             Development Board Demo Program                |\n");
    printf("|                                                           |\n");
    printf("+-----------------------------------------------------------+\n");

	/* Step 3. GPIO initial and select operation mode for I2C*/
	//////
	// I2C
	//////
    DrvGPIO_InitFunction(FUNC_I2C0); //Set I2C I/O
	DrvI2C_Open(I2C_PORT0, (DrvSYS_GetHCLK() * 1000), 48000);  //clock = 48Kbps
	DrvI2C_EnableInt(I2C_PORT0); //Enable I2C0 interrupt and set corresponding NVIC bit

	/* Step 4. GPIO initial and select operation mode for I2S*/
	///////
	// I2S
	//////
    DrvGPIO_InitFunction(FUNC_I2S0);  //Set I2S I/O
	DrvGPIO_InitFunction(FUNC_MCLK1); //Set MCLK I/O

    st.u32SampleRate 	 = 16000;
    st.u8WordWidth 	 	 = DRVI2S_DATABIT_16;
    st.u8AudioFormat 	 = DRVI2S_STEREO;
	st.u8DataFormat  	 = DRVI2S_FORMAT_I2S;
    st.u8Mode 		 	 = DRVI2S_MODE_SLAVE;
    st.u8RxFIFOThreshold = DRVI2S_FIFO_LEVEL_WORD_0;
    st.u8TxFIFOThreshold = DRVI2S_FIFO_LEVEL_WORD_8-1;
	DrvI2S_Open(&st);

	/* Step 5. Set and enable MCLK*/
	DrvI2S_SetMCLK(12000000);  //MCLK = 12MHz
	DrvI2S_EnableMCLK(1);	   //enable MCLK

	/* Step 6. Configure CODEC registers*/
	WAU88XX_EnterADCTestMode();

	/* Step 7. Enable Rx and interrupt*/
	//Enable Rx threshold level interrupt and install its callback function
	DrvI2S_EnableInt(I2S_RX_FIFO_THRESHOLD, Rx_thresholdCallbackfn);
	u32startFlag = 1;

	// Enable I2S Rx function to receive data
	DrvI2S_EnableRx(TRUE);

	while(1)
	{
		if (u32startFlag)
		{
			/* Step 8. Enable Tx and interrupt*/
			// Enable I2S Tx function to send data when data in the buffer is more than half of buffer size
			if (u32BuffPos >= BUFF_LEN/2)
			{
				DrvI2S_EnableInt(I2S_TX_FIFO_THRESHOLD, Tx_thresholdCallbackfn);
				DrvI2S_EnableTx(TRUE);
				u32startFlag = 0;
			}
		}
	}



}

