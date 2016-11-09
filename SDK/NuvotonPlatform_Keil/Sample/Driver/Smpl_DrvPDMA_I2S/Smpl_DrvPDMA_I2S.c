/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                         */
/* Copyright(c) 2009 Nuvoton Technology Corp. All rights reserved.                                         */
/*                                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
#include <stdio.h>
#include "Driver\DrvPDMA.h"
#include "Driver\DrvUART.h"
#include "Driver\DrvSYS.h"
#include "Driver\DrvGPIO.h"
#include "Driver\DrvI2S.h"
#include "ISD9xx.h"

/*---------------------------------------------------------------------------------------------------------*/
/* Define functions prototype                                                                              */
/*---------------------------------------------------------------------------------------------------------*/
void InitialSystemClock(void);
void InitialUART(void);
void InitialI2S(void);

/*-------------------------------------------------------------------------------*/
/* Global Variable                                                               */
/*-------------------------------------------------------------------------------*/
#define	UART_TEST_LENGTH			256
uint8_t SrcArray[UART_TEST_LENGTH];
uint8_t DestArray[UART_TEST_LENGTH];
int32_t IntCnt;
volatile int32_t IsTestOver;

/*-------------------------------------------------------------------------------*/
/* PDMA Sample Code: I2S                                                         */
/*-------------------------------------------------------------------------------*/
typedef struct
{
    int8_t  Ch0Mode;             // Mode determines whether record or play or loop
    int8_t  Ch0AudioDataRdy[2];  // DataRdy is set by source cleared by destination.
    int8_t  Ch0CompDataRdy[2];   // DataRdy is set by source cleared by destination.
    int8_t  Ch0Stop;             // Stop Ch0
    int16_t Ch0Bandwidth;        // Bandwidth setting for Ch0
    int32_t Ch0Bitrate;          // Bitrate setting for Ch0
    int32_t Ch0StartAddr;        // Start Addr for Ch0 play
    int8_t  Ch1Mode;             // Mode determines whether record or play or loop
    int8_t  Ch1AudioDataRdy[2];  // DataRdy is set by source cleared by destination.
    int8_t  Ch1CompDataRdy[2];   // DataRdy is set by source cleared by destination.
    int16_t Ch1Bandwidth;        // Bandwidth setting for Ch1
    int32_t Ch1Bitrate;          // Bitrate setting for Ch1
    int32_t Ch1StartAddr;        // Start Addr for Ch1 play
    
}COMPRESSOR_CONTROL;

COMPRESSOR_CONTROL c_ctrl;

#define MAX_FRAMESIZE 160
__align(4) int16_t audio_buffer[4][MAX_FRAMESIZE];
#define ZERO_BUF_SAMPLES 4
__align(4)  int16_t zero_buf[ZERO_BUF_SAMPLES]={0,0,0,0};
#define I2S_INITIALIZE 0xaa55aa55

/*---------------------------------------------------------------------------------------------------------*/
/* Clear buffer function                                                                              	   */
/*---------------------------------------------------------------------------------------------------------*/
void ClearBuf(uint32_t u32Addr, uint32_t u32Length,uint8_t u8Pattern)
{
	uint8_t* pu8Ptr;
	uint32_t i;
	
	pu8Ptr = (uint8_t *)u32Addr;
	
	for (i=0; i<u32Length; i++)
	{
		*pu8Ptr++ = u8Pattern;
	}
}

/*---------------------------------------------------------------------------------------------------------*/
/* Bulid Src Pattern function                                                                         	   */
/*---------------------------------------------------------------------------------------------------------*/
void BuildSrcPattern(uint32_t u32Addr, uint32_t u32Length)
{
    uint32_t i=0,j,loop;
    uint8_t* pAddr;
    
    pAddr = (uint8_t *)u32Addr;
    
    do {
        if (u32Length > 256)
	    	loop = 256;
	    else
	    	loop = u32Length;
	    	
	   	u32Length = u32Length - loop;    	

        for(j=0;j<loop;j++)
            *pAddr++ = (uint8_t)(j+i);
            
	    i++;        
	} while ((loop !=0) || (u32Length !=0));         
}

/*---------------------------------------------------------------------------------------------------------*/
/* PDMA Callback function                                                                           	   */
/*---------------------------------------------------------------------------------------------------------*/
void PDMA3_TA_Callback(uint32_t status)
{    
	  DrvPDMA_CHSoftwareReset (eDRVPDMA_CHANNEL_3 );
	  DrvPDMA_CHEnablelTransfer (eDRVPDMA_CHANNEL_3 );
}

/*--------------------------------------------------------------------------------------------------------*/
/* I2S TX Callback                                                                                        */
/*--------------------------------------------------------------------------------------------------------*/
void PDMA3_Callback(uint32_t status)
{
    static uint8_t PingPong, TxSilence;
    volatile uint32_t u32SFR;
 
    if(status == I2S_INITIALIZE){
        // Callback can be called from compressor to initialize
        // the buffers.
        PingPong = 0;
        TxSilence =1;
        // Point PDMA to silence buffer
        /* Set Source Address */	
        u32SFR = (uint32_t)&PDMA0->SAR + eDRVPDMA_CHANNEL_3 * CHANNEL_OFFSET;
        outpw(u32SFR,(uint32_t)&zero_buf);	
        u32SFR = (uint32_t)&PDMA0->BCR + eDRVPDMA_CHANNEL_3 * CHANNEL_OFFSET;
        /* Set Byte Count Register */
        outpw(u32SFR,ZERO_BUF_SAMPLES * sizeof(int16_t));
        DrvPDMA_CHEnablelTransfer (eDRVPDMA_CHANNEL_3 );
    }else{
        if(TxSilence){
            // We are waiting for valid data in the output buffer,
            // check to see if present and start transfer
            if(c_ctrl.Ch1AudioDataRdy[PingPong] == TRUE){
                u32SFR = (uint32_t)&PDMA0->SAR + eDRVPDMA_CHANNEL_3 * CHANNEL_OFFSET;
                outpw(u32SFR,(uint32_t)&audio_buffer[PingPong+2][0]);	
                u32SFR = (uint32_t)&PDMA0->BCR + eDRVPDMA_CHANNEL_3 * CHANNEL_OFFSET;
                /* Set Byte Count Register */
                outpw(u32SFR,MAX_FRAMESIZE * sizeof(int16_t));
                TxSilence =0;
            }
        }else{
            // Getting here means we are finished with the current buffer
            // of audio data. Can tell compressor it can process the next
            // one.
            c_ctrl.Ch1AudioDataRdy[PingPong] = FALSE;
            PingPong ^= 1;
            if(c_ctrl.Ch1AudioDataRdy[PingPong] == FALSE){
                TxSilence =1;
                // Point PDMA to silence buffer
                /* Set Source Address */	
                u32SFR = (uint32_t)&PDMA0->SAR + eDRVPDMA_CHANNEL_3 * CHANNEL_OFFSET;
                outpw(u32SFR,(uint32_t)&zero_buf);	
                u32SFR = (uint32_t)&PDMA0->BCR + eDRVPDMA_CHANNEL_3 * CHANNEL_OFFSET;
                /* Set Byte Count Register */
                outpw(u32SFR,ZERO_BUF_SAMPLES * sizeof(int16_t));
            }else{
                // Data ready
                u32SFR = (uint32_t)&PDMA0->SAR + eDRVPDMA_CHANNEL_3 * CHANNEL_OFFSET;
                outpw(u32SFR,(uint32_t)&audio_buffer[PingPong+2][0]);	
                u32SFR = (uint32_t)&PDMA0->BCR + eDRVPDMA_CHANNEL_3 * CHANNEL_OFFSET;
                /* Set Byte Count Register */
                outpw(u32SFR,MAX_FRAMESIZE * sizeof(int16_t));
                TxSilence =0;
            }
			
        }// if(TxSilence) else
        DrvPDMA_CHEnablelTransfer(eDRVPDMA_CHANNEL_3);	
    } // if(status == I2S_INITIALIZE) else
}


/*--------------------------------------------------------------------------------------------------------*/
/* I2S RX Callback                                                                                        */
/*--------------------------------------------------------------------------------------------------------*/
void PDMA2_Callback()
{
	extern int32_t IntCnt;
    static uint8_t PingPong=0;
    volatile uint32_t u32SFR;
    
	if(IntCnt < 10) printf("\tI2S Transfer Done %02d!\r",++IntCnt);
	else 		 ++IntCnt;

    c_ctrl.Ch1AudioDataRdy[PingPong] = TRUE;
	//u32SFR = (uint32_t)&PDMA0->SAR + eDRVPDMA_CHANNEL_3 * CHANNEL_OFFSET;
    //outpw(u32SFR,(uint32_t)&audio_buffer[PingPong+2][0]);
    
    PingPong ^= 1;
    u32SFR = (uint32_t)&PDMA0->DAR + eDRVPDMA_CHANNEL_2 * CHANNEL_OFFSET;
    outpw(u32SFR,(uint32_t)&audio_buffer[PingPong+2][0]);
    DrvPDMA_CHEnablelTransfer(eDRVPDMA_CHANNEL_2);
    if(IntCnt==1)
		PDMA3_Callback(	I2S_INITIALIZE );

	//if(IntCnt==1)
		//DrvPDMA_CHEnablelTransfer(eDRVPDMA_CHANNEL_3);
	// if(IntCnt > 1000) IsTestOver = TRUE;
}

/*--------------------------------------------------------------------------------------------------------*/
/* PDMA_I2S                                                                                               */
/*--------------------------------------------------------------------------------------------------------*/
void PDMA_I2S(void)
{
	STR_PDMA_T sPDMA;  
    uint32_t  I2SPort;
    volatile uint32_t i;
	BuildSrcPattern((uint32_t)SrcArray, UART_TEST_LENGTH);

   	I2SPort = I2S_BA;    	
    i=UART_TEST_LENGTH;
    ClearBuf((uint32_t)DestArray, UART_TEST_LENGTH, 0xFF);
    
	/* PDMA Init */
    DrvPDMA_Init();

	/* PDMA Setting */
	PDMA_GCR->PDSSR.I2S_TXSEL = eDRVPDMA_CHANNEL_3;
    PDMA_GCR->PDSSR.I2S_RXSEL = eDRVPDMA_CHANNEL_2;
    
	/* CH3 TX Setting */
	sPDMA.sSrcAddr.u32Addr 			= (uint32_t)&zero_buf;
    sPDMA.sDestAddr.u32Addr 		= I2SPort  + 0x10;   
    sPDMA.u8TransWidth 				= eDRVPDMA_WIDTH_16BITS;
	sPDMA.u8Mode 					= eDRVPDMA_MODE_MEM2APB;
	sPDMA.sSrcAddr.eAddrDirection 	= eDRVPDMA_DIRECTION_INCREMENTED; 
	sPDMA.sDestAddr.eAddrDirection 	= eDRVPDMA_DIRECTION_FIXED;   
	sPDMA.i32ByteCnt                = ZERO_BUF_SAMPLES * 4;
	//sPDMA.i32ByteCnt = MAX_FRAMESIZE * sizeof(int16_t);
	DrvPDMA_Open(eDRVPDMA_CHANNEL_3,&sPDMA);

 	/* CH2 RX Setting */
	sPDMA.sSrcAddr.u32Addr 			= I2SPort  + 0x14; 
    sPDMA.sDestAddr.u32Addr 		= (uint32_t)&audio_buffer[2][0];
	sPDMA.u8Mode 					= eDRVPDMA_MODE_APB2MEM;
	sPDMA.sSrcAddr.eAddrDirection 	= eDRVPDMA_DIRECTION_FIXED; 
	sPDMA.sDestAddr.eAddrDirection 	= eDRVPDMA_DIRECTION_INCREMENTED;   
    sPDMA.i32ByteCnt = MAX_FRAMESIZE * sizeof(int16_t);
    DrvPDMA_Open(eDRVPDMA_CHANNEL_2,&sPDMA);

	/* Enable INT */
    DrvPDMA_EnableInt(eDRVPDMA_CHANNEL_2, eDRVPDMA_BLKD );
	DrvPDMA_EnableInt(eDRVPDMA_CHANNEL_3, eDRVPDMA_BLKD );
        
	/* Install Callback function */   
 	DrvPDMA_InstallCallBack(eDRVPDMA_CHANNEL_2,eDRVPDMA_BLKD,
                            (PFN_DRVPDMA_CALLBACK) PDMA2_Callback ); 
	DrvPDMA_InstallCallBack(eDRVPDMA_CHANNEL_3,eDRVPDMA_BLKD,
                            (PFN_DRVPDMA_CALLBACK) PDMA3_Callback ); 
	 
	DrvPDMA_InstallCallBack(eDRVPDMA_CHANNEL_3 ,eDRVPDMA_TABORT ,(PFN_DRVPDMA_CALLBACK) PDMA3_TA_Callback ); 	
	/* Enable UART PDMA and Trigger PDMA specified Channel */
    DrvI2S_EnableTxDMA (TRUE);		 
    DrvI2S_EnableRxDMA (TRUE);
	DrvI2S_EnableRx(TRUE);
    DrvI2S_EnableTx(TRUE);

	IntCnt = 0;
	IsTestOver=FALSE;
 
 	//DrvPDMA_CHEnablelTransfer(eDRVPDMA_CHANNEL_3);
	DrvPDMA_CHEnablelTransfer(eDRVPDMA_CHANNEL_2);
	PDMA3_Callback(	I2S_INITIALIZE );
 	
	/* Trigger PDMA 10 time and the S/W Flag will be change in PDMA callback funtion */
	while(IsTestOver==FALSE);

	/* Close PDMA Channel */
	DrvI2S_EnableRx(FALSE);
    DrvI2S_EnableTx(FALSE);
	DrvI2S_EnableTxDMA (FALSE);		 
    DrvI2S_EnableRxDMA (FALSE);
	DrvPDMA_Close();
	return;  
	
}

/*--------------------------------------------------------------------------------------------------------*/
/* SysTimerDelay                                                                                          */
/*--------------------------------------------------------------------------------------------------------*/
void SysTimerDelay(uint32_t us)
{
    SysTick->LOAD = (us * 49152)/1000; /* Assume the internal 48MHz RC used */
    SysTick->VAL   =  (0x00);
    SysTick->CTRL = (1 << SYSTICK_CLKSOURCE) | (1<<SYSTICK_ENABLE);

    /* Waiting for down-count to zero */
    while((SysTick->CTRL & (1 << 16)) == 0);
}


/*---------------------------------------------------------------------------------------------------------*/
/* MAIN function                                                                           	   			   */
/*---------------------------------------------------------------------------------------------------------*/

int32_t main()
{
	InitialSystemClock();
    
	InitialUART();

	InitialI2S();
		
	printf("+------------------------------------------------------------------------+\n");
    printf("|                         PDMA Driver Sample Code                        |\n");
    printf("|                                                                        |\n");
    printf("+------------------------------------------------------------------------+\n");                    
	printf("  This sample code will use PDMA to do I2S test. \n");
	printf("  Test PDMA for I2S 10 times \n");
    printf("  press any key to continue ...\n");

	getchar();
	PDMA_I2S();
	printf("\n  PDMA I2S sample code is complete.\n\n");
  	
    while(1);

}	


void InitialSystemClock(void)
{
    /* Unlock the protected registers */	
	UNLOCKREG();

	/* Enable the 12MHz oscillator oscillation */
//	DrvSYS_SetOscCtrl(E_SYS_XTL12M, 1);
 
     /* Waiting for 12M Xtal stalble */
    SysTimerDelay(5000);
 
	/* HCLK clock source. 0: external 12MHz; 4:internal 22MHz RC oscillator */
	DrvSYS_SetHCLKSource(0);

	LOCKREG();

	/* HCLK clock frequency = HCLK clock source / (HCLK_N + 1) */
	DrvSYS_SetClockDivider(E_SYS_HCLK_DIV, 0); 
}


void InitialUART(void)
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

	/* Set UART Configuration */
	DrvUART_Open(UART_PORT0,&sParam);
}

void InitialI2S(void)
{
	S_DRVI2S_DATA_T st;

	/* Set I2S Parameter */
    st.u32SampleRate 	 = 16000;
    st.u8WordWidth 	 	 = DRVI2S_DATABIT_16;
    st.u8AudioFormat 	 = DRVI2S_STEREO;  		
	st.u8DataFormat  	 = DRVI2S_FORMAT_I2S;   
    st.u8Mode 		 	 = DRVI2S_MODE_SLAVE;
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
    DrvI2S_EnableTx(FALSE);
}






















































































































