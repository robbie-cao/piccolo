/*----------------------------------------------------------------------------------------*/
/*                                                                                        */
/* Copyright(c) 2011 Nuvoton Technology Corp. All rights reserved.                        */
/*                                                                                        */
/* Siren7 (G.722) licensed from Polycom Technology                                        */
/*----------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/* Include related headers                                                                                 */
/*---------------------------------------------------------------------------------------------------------*/
#include <stdio.h>
#include "ISD9xx.h"
#include "Driver\DrvPDMA.h"
#include "Driver\DrvDPWM.h"
#include "Driver\DrvUART.h"
#include "Driver\DrvGPIO.h"
#include "Driver\DrvSYS.h"
#include "Lib\LibSiren7.h"

/*---------------------------------------------------------------------------------------------------------*/
/* Macro, type and constant definitions                                                                    */
/*---------------------------------------------------------------------------------------------------------*/
#define AUDIOBUFFERSIZE 320
#define DPWMSAMPLERATE  8000
#define S7BITRATE       12000
#define S7BANDWIDTH     7000
#define COMPBUFSIZE     60   //According to S7BITRATE

/*----------------------------------------------------------------------------------------*/
/* Global variables                                                                       */
/*----------------------------------------------------------------------------------------*/
extern uint32_t u32AudioDataBegin, u32AudioDataEnd;
uint32_t u32TotalPCMcount;  		// = PCM_LENGTH

__align(4) int16_t i16AudioBuffer[2][AUDIOBUFFERSIZE];
uint32_t u32AudioSampleCount,u32PDMA1CallBackCount;
uint32_t u32AudioDataAddr,u32BufferEmptyAddr,u32BufferReadyAddr;
BOOL	boPCMplaying,boBufferEmpty,boPDMA1Done;
static sSiren7_CODEC_CTL sEnDeCtl;
static sSiren7_ENC_CTX sS7Enc_Ctx;
static sSiren7_DEC_CTX sS7Dec_Ctx;

/*----------------------------------------------------------------------------------------*/
/* Define functions prototype                                                             */
/*----------------------------------------------------------------------------------------*/
void Smpl_LibS7PDMA1_Callback(void);

/*----------------------------------------------------------------------------------------*/
/* Function: Smpl_LibS7UartInit		                                      			   	  */
/*                                                                                        */
/* Parameters:                                                                            */
/*               None											                    	  */
/* Returns:                                                                               */
/*               None											                    	  */
/* Description:                                                                           */
/*               Initialize UART as BaudRate:115200; 8Bits; 1StopBit; None parity         */
/*----------------------------------------------------------------------------------------*/
void Smpl_LibS7UartInit(void)
{
	STR_UART_T sParam;
	
	/* Set UART Pin */
	DrvGPIO_InitFunction(FUNC_UART0);
	
	/* UART Setting */
    sParam.u32BaudRate 		= 115200;
    sParam.u8cDataBits 		= DRVUART_DATABITS_8;
    sParam.u8cStopBits 		= DRVUART_STOPBITS_1;
    sParam.u8cParity 		= DRVUART_PARITY_NONE;
    sParam.u8cRxTriggerLevel= DRVUART_FIFO_1BYTES;

    DrvUART_Open(UART_PORT0,&sParam);
}

/*----------------------------------------------------------------------------------------*/
/* Function: Smpl_LibS7InitDPWM		                                      			   	  */
/*                                                                                        */
/* Parameters:                                                                            */
/*               u32SampleRate: Set sample rate to DPWM output                      	  */
/* Returns:                                                                               */
/*               None											                    	  */
/* Description:                                                                           */
/*               Initialize DPWM output                                                   */
/*----------------------------------------------------------------------------------------*/
void Smpl_LibS7InitDPWM(uint32_t u32SampleRate)
{
	DrvDPWM_Open();
	DrvDPWM_SetDPWMClk(E_DRVDPWM_DPWMCLK_HCLKX2);
	DrvDPWM_SetSampleRate(u32SampleRate);
	DrvDPWM_Enable();
}

/*----------------------------------------------------------------------------------------*/
/* Function: Smpl_LibS7PDMA1forDPWM                                      			   	  */
/*                                                                                        */
/* Parameters:                                                                            */
/*               None											                    	  */
/* Returns:                                                                               */
/*               None											                    	  */
/* Description:                                                                           */
/*               Set PDMA1 to move ADC FIFO to MIC buffer with wrapped-around mode        */
/*----------------------------------------------------------------------------------------*/
void Smpl_LibS7PDMA1forDPWM(void)
{
	STR_PDMA_T sPDMA;  

	sPDMA.sSrcAddr.u32Addr 			= u32BufferReadyAddr; 
	sPDMA.sDestAddr.u32Addr 		= (uint32_t)&DPWM->FIFO;
	sPDMA.u8Mode 					= eDRVPDMA_MODE_MEM2APB;;
	sPDMA.u8TransWidth 				= eDRVPDMA_WIDTH_16BITS;
	sPDMA.sSrcAddr.eAddrDirection 	= eDRVPDMA_DIRECTION_INCREMENTED; 
	sPDMA.sDestAddr.eAddrDirection 	= eDRVPDMA_DIRECTION_FIXED;  
    sPDMA.i32ByteCnt = AUDIOBUFFERSIZE * 2;	   	//Full MIC buffer length (byte)
    DrvPDMA_Open(eDRVPDMA_CHANNEL_1, &sPDMA);

	// PDMA Setting 
	DrvPDMA_SetCHForAPBDevice(
    	eDRVPDMA_CHANNEL_1, 
    	eDRVPDMA_DPWM,
    	eDRVPDMA_WRITE_APB    
	);

	// Enable DPWM DMA
	DrvDPWM_EnablePDMA();
	// Enable INT 
	DrvPDMA_EnableInt(eDRVPDMA_CHANNEL_1, eDRVPDMA_BLKD ); 
	// Install Callback function    
	DrvPDMA_InstallCallBack(eDRVPDMA_CHANNEL_1, eDRVPDMA_BLKD, 
	                        (PFN_DRVPDMA_CALLBACK) Smpl_LibS7PDMA1_Callback ); 	
	DrvPDMA_CHEnablelTransfer(eDRVPDMA_CHANNEL_1);

	boPDMA1Done=FALSE;
}

/*----------------------------------------------------------------------------------------*/
/* Function: Smpl_LibS7PDMA1forDPWM                                      			   	  */
/*                                                                                        */
/* Parameters:                                                                            */
/*               None											                    	  */
/* Returns:                                                                               */
/*               None											                    	  */
/* Description:                                                                           */
/*               PDMA1 callback function; switching ping pong FIFO                        */
/*----------------------------------------------------------------------------------------*/
void Smpl_LibS7PDMA1_Callback(void)
{
	if ((u32PDMA1CallBackCount&0x1)==0)
		{
			u32BufferReadyAddr=(uint32_t) &i16AudioBuffer[1][0];
			u32BufferEmptyAddr=(uint32_t) &i16AudioBuffer[0][0];
		}
	else
		{
			u32BufferReadyAddr=(uint32_t) &i16AudioBuffer[0][0];
			u32BufferEmptyAddr=(uint32_t) &i16AudioBuffer[1][0];
		}


	u32PDMA1CallBackCount++;
	if (boBufferEmpty==FALSE)
		Smpl_LibS7PDMA1forDPWM();
	else
	{	
		boPDMA1Done=TRUE;
		printf("Late = %d\n", u32AudioSampleCount);
	}

	boBufferEmpty=TRUE;
}

/*----------------------------------------------------------------------------------------*/
/* Function: Smpl_LibS7CopyMem2Mem                                      			   	  */
/*                                                                                        */
/* Parameters:                                                                            */
/*               u32SrcAddr1: Source address		        	                    	  */
/*               u32DesAddr2: Destination address   			                    	  */
/*               u32WordCount: Copy count Cal in 4 bytes			                  	  */
/* Returns:                                                                               */
/*               None											                    	  */
/* Description:                                                                           */
/*               Copy Data from Source Address to Destination Address                     */
/*----------------------------------------------------------------------------------------*/
void Smpl_LibS7CopyMem2Mem(uint32_t *u32SrcAddr1,uint32_t *u32DesAddr2,uint32_t u32WordCount)
{
    uint32_t u32LoopCount;
	for(u32LoopCount=0;u32LoopCount<u32WordCount;u32LoopCount++ )
	{
		*u32DesAddr2++ = *u32SrcAddr1++;
	}
}  

/*----------------------------------------------------------------------------------------*/
/* Function: Smpl_LibS7CompEngine                                        			   	  */
/*                                                                                        */
/* Parameters:                                                                            */
/*               None											                    	  */
/* Returns:                                                                               */
/*               None											                    	  */
/* Description:                                                                           */
/*               Encode: PCM data to Siren7 data format                                   */
/*               Decode: Siren7 data to PCM data format                                   */
/*----------------------------------------------------------------------------------------*/
void Smpl_LibS7CompEngine(void)
{
	signed short s16Out_words[COMPBUFSIZE];

	Smpl_LibS7CopyMem2Mem((uint32_t *)u32AudioDataAddr,(uint32_t *)u32BufferEmptyAddr,(AUDIOBUFFERSIZE>>1));

	LibS7Encode(&sEnDeCtl, &sS7Enc_Ctx, (signed short *)u32BufferEmptyAddr, s16Out_words);  
    
	LibS7Decode(&sEnDeCtl, &sS7Dec_Ctx, s16Out_words, (signed short *)u32BufferEmptyAddr);  
        
	u32AudioDataAddr += AUDIOBUFFERSIZE*2;
	u32AudioSampleCount += AUDIOBUFFERSIZE;
	
	if (u32AudioSampleCount > u32TotalPCMcount)
		boPCMplaying=FALSE;
}


/*----------------------------------------------------------------------------------------*/
/* Function: Smpl_LibS7PlaySoundInit                                       			   	  */
/*                                                                                        */
/* Parameters:                                                                            */
/*               None   										                    	  */
/* Returns:                                                                               */
/*               None											                    	  */
/* Description:                                                                           */
/*               Initialize DPWM and start to transform Data                              */
/*----------------------------------------------------------------------------------------*/
void Smpl_LibS7PlaySoundInit(void)
{
	Smpl_LibS7InitDPWM(DPWMSAMPLERATE);		//Compatible PCM file format

	u32TotalPCMcount= ((uint32_t)&u32AudioDataEnd-(uint32_t)&u32AudioDataBegin)/2;
	boPCMplaying=TRUE;
	u32AudioSampleCount=0;
	u32PDMA1CallBackCount=0;
	
	u32BufferEmptyAddr= (uint32_t) &i16AudioBuffer[0][0];
	Smpl_LibS7CompEngine();
	u32BufferReadyAddr= (uint32_t) &i16AudioBuffer[0][0];
	Smpl_LibS7PDMA1forDPWM();

	u32BufferEmptyAddr= (uint32_t) &i16AudioBuffer[1][0];
	boBufferEmpty=TRUE;
}


/*----------------------------------------------------------------------------------------*/
/* Function: Smpl_LibS7APIinit                                             			   	  */
/*                                                                                        */
/* Parameters:                                                                            */
/*               None   										                    	  */
/* Returns:                                                                               */
/*               None											                    	  */
/* Description:                                                                           */
/*               Initial API Library         					                    	  */
/*----------------------------------------------------------------------------------------*/
void Smpl_LibS7APIinit(void)
{
    LibS7Init(&sEnDeCtl,S7BITRATE,S7BANDWIDTH);
   
    LibS7EnBufReset(sEnDeCtl.frame_size,&sS7Enc_Ctx);
    LibS7DeBufReset(sEnDeCtl.frame_size,&sS7Dec_Ctx);
}

/*----------------------------------------------------------------------------------------*/
/* Function: main                                                       			   	  */
/*                                                                                        */
/* Parameters:                                                                            */
/*               None   										                    	  */
/* Returns:                                                                               */
/*               None											                    	  */
/* Description:                                                                           */
/*               LibSiren7 demo code, encode pcm data as siren7 format then decode it.    */
/*               And output pcm data to SPK  					                    	  */
/*----------------------------------------------------------------------------------------*/
int32_t main (void)
{
	UNLOCKREG();
	SYSCLK->PWRCON.OSC49M_EN = 1;
	SYSCLK->CLKSEL0.HCLK_S = 0; /* Select HCLK source as 48MHz */ 
	SYSCLK->CLKDIV.HCLK_N  = 0;	/* Select no division          */
	SYSCLK->CLKSEL0.OSCFSel = 0;	/* 1= 32MHz, 0=48MHz */
	LOCKREG();
	
	/* Set UART Configuration */
    Smpl_LibS7UartInit();

    printf("+-------------------------------------------------------------------------+\n");
    printf("|       Playing 8K sampling	PCM										      |\n");
    printf("+-------------------------------------------------------------------------+\n");
    printf("\n=== Flash audio data To SPK test ===\n");

    /* Initial API Library */
    Smpl_LibS7APIinit();

    DrvPDMA_Init();			//PDMA initialization
	u32AudioDataAddr= (uint32_t)&u32AudioDataBegin-1;

	printf("DataAddr = %08X\n", u32AudioDataAddr);
	printf("DataBegin = %08X\n", (uint32_t)&u32AudioDataBegin);
	printf("DataEnd = %08X\n", (uint32_t)&u32AudioDataEnd);

	Smpl_LibS7PlaySoundInit();

	while (boPCMplaying == TRUE)
	{
		if (boBufferEmpty==TRUE)
		{
			Smpl_LibS7CompEngine();
			boBufferEmpty=FALSE;
		}
	
		if ((boPDMA1Done==TRUE) && (boBufferEmpty==FALSE))
			Smpl_LibS7PDMA1forDPWM();

	}

	DrvPDMA_Close();
	DrvDPWM_Close();

	printf("Test Done\n");
	while(1);
	
}

