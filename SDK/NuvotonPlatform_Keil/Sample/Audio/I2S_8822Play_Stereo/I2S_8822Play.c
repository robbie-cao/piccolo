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


extern uint32_t u32AudioDataBegin, u32AudioDataEnd;



/*---------------------------------------------------------------------------------------------------------*/
/* Define functions prototype                                                                              */
/*---------------------------------------------------------------------------------------------------------*/


uint32_t TotalPcmCount, AudioDataAddr;  		// = PCM_LENGTH

void InitialI2C(void)
{
	/*  GPIO initial and select operation mode for I2C*/
    DrvGPIO_InitFunction(FUNC_I2C0); //Set I2C I/O
	//DrvI2C_Open(I2C_PORT0, (DrvSYS_GetHCLK() * 1000), 12000);  //clock = 24Kbps
	DrvI2C_Open(I2C_PORT0, (DrvSYS_GetHCLK() * 1000), 48000);  //clock = 48Kbps
	DrvI2C_EnableInt(I2C_PORT0); //Enable I2C0 interrupt and set corresponding NVIC bit
}


//================================================
/*Important Note */
// I2S slave has problem on MSB format, only can use I2S format
// I2S slave has PDMA problem on receiving, it needs 16 bit PDMA datawidth
// I2S master is OK on MSB & I2S format, and PDMA using 32bit datawidth
void InitialI2S(void)
{
	S_DRVI2S_DATA_T st;

	/* GPIO initial and select operation mode for I2S*/
    DrvGPIO_InitFunction(FUNC_I2S0);  //Set I2S I/O
	DrvGPIO_InitFunction(FUNC_MCLK1); //Set MCLK I/O

	/* Set I2S Parameter */
    st.u32SampleRate 	 = 16000;
    st.u8WordWidth 	 	 = DRVI2S_DATABIT_16;
    st.u8AudioFormat 	 = DRVI2S_STEREO;  		
	//st.u8DataFormat  	 = DRVI2S_FORMAT_MSB;   
	st.u8DataFormat  	 = DRVI2S_FORMAT_I2S;
    //st.u8Mode 		 	 = DRVI2S_MODE_SLAVE;
	st.u8Mode 		 	 = DRVI2S_MODE_MASTER;
    st.u8RxFIFOThreshold = DRVI2S_FIFO_LEVEL_WORD_4;
    st.u8TxFIFOThreshold = DRVI2S_FIFO_LEVEL_WORD_4;

	DrvI2S_Open(&st);

    /* Set I2S I/O */
//    DrvGPIO_InitFunction(FUNC_I2S);
	SYS->GPA_ALT.GPA4 		=1;	 // 
    SYS->GPA_ALT.GPA5 		=1;	 // 
	SYS->GPA_ALT.GPA6 		=1;	 // 
    SYS->GPA_ALT.GPA7 		=1;	 // 

	/* Disable I2S Tx/Rx function */
	DrvI2S_EnableRx(FALSE);
    DrvI2S_EnableTx(TRUE);

	DrvI2S_SetMCLK(12000000);  //MCLK = 12MHz
	//DrvI2S_SetMCLK(8000000);  //MCLK = 8MHz
	DrvI2S_EnableMCLK(1);	   //enable MCLK
}

/*----------------------------------------------------------------------------
  MAIN function
  *----------------------------------------------------------------------------*/
int main (void)
{


	//  Enable and select clock source
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

	// GPIO initial and select operation mode for UART
	DrvUART_Init(115200);  //Set UART I/O and UART setting

    printf("+-----------------------------------------------------------+\n");
    printf("|             Development Board Demo Program                |\n");
    printf("|                                                           |\n");
    printf("+-----------------------------------------------------------+\n");



	//Configure CODEC registers
	InitialI2C();
	WAU8822_Setup();


	//Initial I2S engine
	InitialI2S();

   	//TotalPcmCount= ((uint32_t)&u32AudioDataEnd-(uint32_t)&u32AudioDataBegin)/2;
	//AudioDataAddr= (uint32_t)&u32AudioDataBegin-1;

	//Set PDMA with I2S
	//PlayI2S();		   //Start I2S test

	while(1)
	{

   	TotalPcmCount= ((uint32_t)&u32AudioDataEnd-(uint32_t)&u32AudioDataBegin)/2;
	AudioDataAddr= (uint32_t)&u32AudioDataBegin-1;

	//Set PDMA with I2S
	PlayI2S();		   //Start I2S test
	  
	}



}

