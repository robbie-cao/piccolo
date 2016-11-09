/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                         */
/* Copyright(c) 2009 Nuvoton Technology Corp. All rights reserved.                                         */
/*                                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
#include <stdio.h>
#include "ISD9xx.h"
#include "Driver\DrvADC.h"
#include "Driver\DrvDPWM.h"
#include "Driver\DrvPDMA.h"
#include "Driver\DrvUART.h"
#include "Driver\DrvI2S.h"
#include "Driver\DrvSYS.h"
#include "Driver\DrvGPIO.h"


/*---------------------------------------------------------------------------------------------------------*/
/* Define functions prototype                                                                              */
/*---------------------------------------------------------------------------------------------------------*/
void InitialSystemClock(void);
void InitialUART(void);
void InitialI2S(void);
void InitialDPWM(uint32_t);
void DPWM_Play(void);
void PDMA3_Callback(uint32_t status);
void PDMA3_TA_Callback(uint32_t status);
void PDMA2_Callback(void);

/*-------------------------------------------------------------------------------*/
/* Global Variable                                                               */
/*-------------------------------------------------------------------------------*/
#define	TEST_LENGTH			256
int32_t SrcArray[TEST_LENGTH];
int32_t DestArray[TEST_LENGTH];
int32_t IntCnt;
volatile int32_t IsTestOver;
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
#define DPWM_INITIALIZE 0xaa5555aa
#define ADC_INITIALIZE 0xaa555555
#define I2S_INITIALIZE 0xaa55aa55

/*--------------------------------------------------------------------------------------------------------*/
/* ADC/I2S RX Callback                                                                                    */
/*--------------------------------------------------------------------------------------------------------*/
void PDMA2_Callback()
{
	extern int32_t IntCnt;
    static uint8_t PingPong=0;
    
	//if(IntCnt < 10) printf("\tI2S Transfer Done %02d!\r",++IntCnt);
	//else 		 ++IntCnt;

    c_ctrl.Ch1AudioDataRdy[PingPong] = TRUE;
	//u32SFR = (uint32_t)&PDMA0->SAR + eDRVPDMA_CHANNEL_3 * CHANNEL_OFFSET;
    //outpw(u32SFR,(uint32_t)&audio_buffer[PingPong+2][0]);
    
    PingPong ^= 1;
    PDMA->channel[eDRVPDMA_CHANNEL_2].DAR = (uint32_t)&audio_buffer[PingPong+2][0];	
	DrvPDMA_CHEnablelTransfer(eDRVPDMA_CHANNEL_2);

    if(IntCnt==1)
		//PDMA3_Callback(	I2S_INITIALIZE );
		PDMA3_Callback(	ADC_INITIALIZE );

	//if(IntCnt==1)
    //DrvPDMA_CHEnablelTransfer(eDRVPDMA_CHANNEL_3);
	// if(IntCnt > 1000) IsTestOver = TRUE;
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
/* DPWM TX Callback                                                                                       */
/*--------------------------------------------------------------------------------------------------------*/
void PDMA3_Callback(uint32_t status)
{
    static uint8_t PingPong, TxSilence;
    
    if(status == DPWM_INITIALIZE){
        // Callback can be called from compressor to initialize
        // the buffers.
        PingPong = 0;
        TxSilence =1;
        // Point PDMA to silence buffer
        // Set Source Address 	
        PDMA->channel[eDRVPDMA_CHANNEL_3].SAR = (uint32_t)&zero_buf;	
        PDMA->channel[eDRVPDMA_CHANNEL_3].BCR = ZERO_BUF_SAMPLES * sizeof(int16_t);	
        DrvPDMA_CHEnablelTransfer (eDRVPDMA_CHANNEL_3 );
    }else{
        if(TxSilence){
            // We are waiting for valid data in the output buffer,
            // check to see if present and start transfer
            if(c_ctrl.Ch1AudioDataRdy[PingPong] == TRUE){
				PDMA->channel[eDRVPDMA_CHANNEL_3].SAR = (uint32_t)&audio_buffer[PingPong+2][0];	
        		PDMA->channel[eDRVPDMA_CHANNEL_3].BCR = MAX_FRAMESIZE * sizeof(int16_t);	
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
                // Set Source Address 	
                PDMA->channel[eDRVPDMA_CHANNEL_3].SAR = (uint32_t)&zero_buf;	
        		PDMA->channel[eDRVPDMA_CHANNEL_3].BCR = ZERO_BUF_SAMPLES * sizeof(int16_t);	
            }else{
                // Data ready
                PDMA->channel[eDRVPDMA_CHANNEL_3].SAR = (uint32_t)&audio_buffer[PingPong+2][0];	
        		PDMA->channel[eDRVPDMA_CHANNEL_3].BCR = MAX_FRAMESIZE * sizeof(int16_t);
                TxSilence =0;
            }
			
        }// if(TxSilence) else
        DrvPDMA_CHEnablelTransfer(eDRVPDMA_CHANNEL_3);	
    } // if(status == I2S_INITIALIZE) else
}


/*--------------------------------------------------------------------------------------------------------*/
/* DPWM_Play                                                                                              */
/*--------------------------------------------------------------------------------------------------------*/
void DPWM_Play(void)
{
	STR_PDMA_T sPDMA;  
    
	// PDMA Init 
    DrvPDMA_Init();

	// PDMA Setting 
	PDMA_GCR->PDSSR.DPWM_TXSEL = eDRVPDMA_CHANNEL_3;
    //PDMA_GCR->PDSSR.I2S_RXSEL = eDRVPDMA_CHANNEL_2;
	PDMA_GCR->PDSSR.ADC_RXSEL = eDRVPDMA_CHANNEL_2;

	// CH3 DPWM TX Setting 
	sPDMA.sSrcAddr.u32Addr 			= (uint32_t)&zero_buf;
    sPDMA.sDestAddr.u32Addr 		= (uint32_t)&DPWM->FIFO;  
    sPDMA.u8TransWidth 				= eDRVPDMA_WIDTH_16BITS;
	sPDMA.u8Mode 					= eDRVPDMA_MODE_MEM2APB;
	sPDMA.sSrcAddr.eAddrDirection 	= eDRVPDMA_DIRECTION_INCREMENTED; 
	sPDMA.sDestAddr.eAddrDirection 	= eDRVPDMA_DIRECTION_FIXED;   
	sPDMA.i32ByteCnt                = ZERO_BUF_SAMPLES * 4;
	//sPDMA.i32ByteCnt = MAX_FRAMESIZE * sizeof(int16_t);
	DrvPDMA_Open(eDRVPDMA_CHANNEL_3,&sPDMA);

	// CH2 IIS RX Setting 
	//sPDMA.sSrcAddr.u32Addr 			= (uint32_t)&I2S->RXFIFO; 
	// CH2 ADC RX Setting
	sPDMA.sSrcAddr.u32Addr 			= (uint32_t)&SDADC->ADCOUT;
    sPDMA.sDestAddr.u32Addr 		= (uint32_t)&audio_buffer[2][0];
	sPDMA.u8TransWidth 				= eDRVPDMA_WIDTH_16BITS;
	sPDMA.u8Mode 					= eDRVPDMA_MODE_APB2MEM;
	sPDMA.sSrcAddr.eAddrDirection 	= eDRVPDMA_DIRECTION_FIXED; 
	sPDMA.sDestAddr.eAddrDirection 	= eDRVPDMA_DIRECTION_INCREMENTED;   
    sPDMA.i32ByteCnt = MAX_FRAMESIZE * sizeof(int16_t);
    DrvPDMA_Open(eDRVPDMA_CHANNEL_2,&sPDMA);

	// Enable INT 
	DrvPDMA_EnableInt(eDRVPDMA_CHANNEL_3, eDRVPDMA_BLKD );
	DrvPDMA_EnableInt(eDRVPDMA_CHANNEL_2, eDRVPDMA_BLKD );
        
	// Install Callback function    
	DrvPDMA_InstallCallBack(eDRVPDMA_CHANNEL_3, eDRVPDMA_BLKD, (PFN_DRVPDMA_CALLBACK) PDMA3_Callback ); 	 
	DrvPDMA_InstallCallBack(eDRVPDMA_CHANNEL_3, eDRVPDMA_TABORT, (PFN_DRVPDMA_CALLBACK) PDMA3_TA_Callback ); 	
	DrvPDMA_InstallCallBack(eDRVPDMA_CHANNEL_2, eDRVPDMA_BLKD, (PFN_DRVPDMA_CALLBACK) PDMA2_Callback );	

	// Enable I2S PDMA and Trigger PDMA specified Channel 
    //DrvI2S_EnableRxDMA (TRUE);		 
    //DrvI2S_EnableRx(TRUE);

	// Enable ADC PDMA and Trigger PDMA specified Channel 
	DrvADC_PdmaEnable();
	DrvDPWM_EnablePDMA();

	IntCnt = 0;
	IsTestOver=FALSE;
  	
	DrvPDMA_CHEnablelTransfer(eDRVPDMA_CHANNEL_2);
	//DrvPDMA_CHEnablelTransfer(eDRVPDMA_CHANNEL_3);
	PDMA3_Callback(	DPWM_INITIALIZE ); 	
}

/*---------------------------------------------------------------------------------------------------------*/
/* Clear buffer function                                                                              	   */
/*---------------------------------------------------------------------------------------------------------*/
void ClearBuf(int32_t i32Addr, int32_t i32Length,int32_t i32Pattern)
{
	int32_t* pi32Ptr;
	int32_t i;
	
	pi32Ptr = (int32_t *)i32Addr;
	
	for (i=0; i<i32Length; i++)
        {
            *pi32Ptr++ = i32Pattern;
        }
}

/*---------------------------------------------------------------------------------------------------------*/
/* Bulid Src Pattern function                                                                         	   */
/*---------------------------------------------------------------------------------------------------------*/
void BuildSrcPattern(int32_t * i32Addr, int32_t i32Length)
{
    int32_t i=0,j,loop;
    int32_t* pAddr;
    
    pAddr = (int32_t *)i32Addr;
    
    do {
        if (i32Length > 256)
	    	loop = 256;
	    else
	    	loop = i32Length;
	    	
	   	i32Length = i32Length - loop;    	

        for(j=0;j<loop;j++)
            *pAddr++ = (int32_t)(j*40+i);
            
	    i++;        
	} while ((loop !=0) || (i32Length !=0));         
}



/*--------------------------------------------------------------------------------------------------------*/
/* SysTimerDelay                                                                                          */
/*--------------------------------------------------------------------------------------------------------*/
void SysTimerDelay(uint32_t us)
{
    SysTick->LOAD = us * 49; /* Assume the internal 49MHz RC used */
    SysTick->VAL   =  (0x00);
    SysTick->CTRL = (1 << SYSTICK_CLKSOURCE) | (1<<SYSTICK_ENABLE);

    /* Waiting for down-count to zero */
    while((SysTick->CTRL & (1 << 16)) == 0);
}


/*---------------------------------------------------------------------------------------------------------*/
/* MAIN function                                                                           	   			   */
/*---------------------------------------------------------------------------------------------------------*/
int32_t main(void)
{						
	uint32_t u32Clk, Div;
	uint8_t u8Freq, StopTest=0;
	char c;

	InitialSystemClock();
    
	InitialUART();

	InitialI2S();

	InitialDPWM(16000);
	
	// Test DPWM Driver
	//SYSCLK->CLKSEL1.DPWM_S = 0;
	//DrvDPWM_SetFrequency(eDRVDPWM_FREQ_1);

		
	printf("+------------------------------------------------------------------------+\n");
    printf("|                         DPWM Driver Sample Code                        |\n");
    printf("|                                                                        |\n");
    printf("+------------------------------------------------------------------------+\n");                    
	printf("  This sample code will use DPWM to play.                                 \n");
	printf("  DPWM Driver Version: %x\n", DrvDPWM_GetVersion());
	printf("  Sample Rate set to %d Hz\n", DrvDPWM_GetSampleRate());
    
	DPWM_Play();
    while(!StopTest){
        u32Clk = 	DrvSYS_GetHCLK() * 1000;
        u8Freq = 	DPWM->CTRL.Freq;

        switch(u8Freq){
		case 0: Div = 228; break;
		case 1: Div = 156; break;
		case 2: Div = 76 ; break;
		case 3: Div = 52 ; break;
		case 4: Div = 780; break;
		case 5: Div = 524; break;
		case 6: Div = 396; break;
		default: Div = 268; break;
        }
        if(SYSCLK->CLKSEL1.DPWM_S)
            u32Clk *= 2;
	
        printf("  DPWM Clk Frequency %d.%d MHz\n", u32Clk/1000000, u32Clk - ((u32Clk/1000000)*1000000));	
        u32Clk/=Div;
        printf("  Carrier Frequency %d.%d kHz\n", u32Clk/1000, u32Clk - ((u32Clk/1000)*1000));
        printf("  Dither Level %d\n", DPWM->CTRL.Dither);
        printf("  Deatime Level %d\n", DPWM->CTRL.Deadtime);



		printf(" c - change DPWM Clk Freq. f - change carrier freq.\n t - Change Deadtime. d - Change dither \n x - Quit\n\n");		   	
		c=getchar();
	 	switch(c){
		case 'c':
			SYSCLK->CLKSEL1.DPWM_S = !SYSCLK->CLKSEL1.DPWM_S;
            break;
		case 'f':
			DPWM->CTRL.Freq++;
            break;
		case 't':
			DPWM->CTRL.Deadtime++;
            break;
		case 'd':
            DPWM->CTRL.Dither++;
			break;
		case 'x':
            StopTest=TRUE;
            break;
		}

	}

	printf("\n  DPWM sample code is complete.\n\n");
  	
	
	DrvI2S_EnableRx(FALSE);
	DrvI2S_EnableRxDMA (FALSE);		 
	DrvDPWM_DisablePDMA();
	DrvADC_PdmaDisable();
	DrvI2S_Close();
	DrvDPWM_Close();
	DrvADC_Close();
	DrvPDMA_Close();
    while(1) ;

}	

/*---------------------------------------------------------------------------------------------------------*/
/* InitialSystemClock                                                                                      */
/*---------------------------------------------------------------------------------------------------------*/
void InitialSystemClock(void)
{
    /* Unlock the protected registers */	
	UNLOCKREG();

	/* HCLK clock source. */
	DrvSYS_SetHCLKSource(0);

	LOCKREG();

	/* HCLK clock frequency = HCLK clock source / (HCLK_N + 1) */
	DrvSYS_SetClockDivider(E_SYS_HCLK_DIV, 0); 
}


/*---------------------------------------------------------------------------------------------------------*/
/* InitialUART                                                                                             */
/*---------------------------------------------------------------------------------------------------------*/
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

/*---------------------------------------------------------------------------------------------------------*/
/* InitialI2S                                                                                              */
/*---------------------------------------------------------------------------------------------------------*/
void InitialI2S(void)
{
	S_DRVI2S_DATA_T st;

	/* Set I2S Parameter */
    st.u32SampleRate 	 = 8000;
    st.u8WordWidth 	 	 = DRVI2S_DATABIT_16;
    st.u8AudioFormat 	 = DRVI2S_STEREO;  		
	st.u8DataFormat  	 = DRVI2S_FORMAT_I2S;   
    st.u8Mode 		 	 = DRVI2S_MODE_SLAVE;
    st.u8RxFIFOThreshold = DRVI2S_FIFO_LEVEL_WORD_4;
    st.u8TxFIFOThreshold = DRVI2S_FIFO_LEVEL_WORD_4;
	DrvI2S_Open(&st);
	

    /* Set I2S I/O */
    DrvGPIO_InitFunction(FUNC_I2S0);
	//SYS->GPA_ALT.GPA4 		=1;	 // 
    //SYS->GPA_ALT.GPA5 		=1;	 // 
	//SYS->GPA_ALT.GPA6 		=1;	 // 
    //SYS->GPA_ALT.GPA7 		=1;	 // 

	/* Disable I2S Tx/Rx function */
	DrvI2S_EnableRx(FALSE);
    DrvI2S_EnableTx(FALSE);
}

/*---------------------------------------------------------------------------------------------------------*/
/* InitialADC                                                                                              */
/*---------------------------------------------------------------------------------------------------------*/
void InitialADC(void)
{
	S_DRVADC_PARAM sParam;

	/* Open Analog block */
	DrvADC_AnaOpen();

	/* Power control */
	DrvADC_SetPower( 
		eDRVADC_PU_MOD_ON, 
		eDRVADC_PU_IBGEN_ON, 
		eDRVADC_PU_BUFADC_ON, 
		eDRVADC_PU_BUFPGA_ON, 
		eDRVADC_PU_ZCD_ON);

	/* PGA Setting */	
	DrvADC_PGAMute(eDRVADC_MUTE_PGA);
	DrvADC_PGAUnMute(eDRVADC_MUTE_IPBOOST);
	DrvADC_SetPGA(	
	    eDRVADC_REF_SEL_VMID,
	    eDRVADC_PU_PGA_ON,
	    eDRVADC_PU_BOOST_ON,
	    eDRVADC_BOOSTGAIN_0DB);

	DrvADC_SetPGAGaindB(0);	  // 0 dB

	/* MIC circuit configuration */
	DrvADC_SetVMID(
		eDRVADC_PULLDOWN_VMID_RELEASE,
		eDRVADC_PDLORES_CONNECTED,
		eDRVADC_PDHIRES_DISCONNECTED);
	DrvADC_SetMIC(TRUE, eDRVADC_MIC_BIAS_SEL_2);

	/* ALC Setting */

	/* Open ADC block */
	sParam.u8AdcDivisor   = 0;
	sParam.u8SDAdcDivisor = 24;
	sParam.eOSR		  = eDRVADC_OSR_128;
	sParam.eInputSrc  = eDRVADC_MIC;
	sParam.eInputMode = eDRVADC_DIFFERENTIAL;
	sParam.u8ADCFifoIntLevel = 7;
	DrvADC_Open(&sParam);

	DrvADC_PGAUnMute(eDRVADC_MUTE_PGA);
}

/*-------------------------------------------------------------------------------*/
/* InitialDPWM                                                                   */
/*-------------------------------------------------------------------------------*/
void InitialDPWM(uint32_t SampleRate)
{

	DrvDPWM_Open();
	DrvDPWM_SetDPWMClk(E_DRVDPWM_DPWMCLK_HCLKX2);
	DrvDPWM_SetSampleRate(SampleRate);
	DrvDPWM_Enable();
}

