/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                         */
/* Copyright(c) 2009 Nuvoton Technology Corp. All rights reserved.                                         */
/*                                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------------------------------*/
/* Include related header files                                                                            */
/*---------------------------------------------------------------------------------------------------------*/
#include <stdio.h>
#include "ISD9xx.h"
#include "Driver\DrvPDMA.h"
#include "Driver\DrvDPWM.h"
#include "Driver\DrvADC.h"
#include "Driver\DrvI2S.h"
#include "Driver\DrvALC.h"
#include "Driver\DrvUART.h"
#include "Driver\DrvGPIO.h"
#include "Driver\DrvSYS.h"

/*--------------------------------*/
/* macro                          */
/*--------------------------------*/
#define abs(x)   (x>=0 ? x : -x)
#define sign(x)  (x>=0 ? '+' : '-')

/*---------------------------------------------------------------------------------------------------------*/
/* Define Function Prototypes                                                                              */
/*---------------------------------------------------------------------------------------------------------*/
void InitialSystemClock(void);
void InitialUART(void);
void InitialADC(void);
void InitialDPWM(uint32_t u32SampleRate);
void InitialI2S(uint32_t u32SampleRate);
void DPWM_Play(void);
void I2S_Play(void);
void PDMA1_Callback(uint32_t status);
void PDMA1_TA_Callback(uint32_t status);
void PDMA0_Callback(void);

void AdcByPDMATest(void);
void AdcSingleModeTest(void);
void AdcSoundQualityTest(void);
void AdcCompMonitorTest(void);
void AdcTemperatureMonitorTest(void);
void VMIDTest(void);
void MICBIASTest(void);
uint8_t SingleEndInput_ChannelSelect(void);
uint8_t DifferentialInput_ChannelSelect(void);
void AdcIntCallback(uint32_t);
void Cmp0IntCallback(uint32_t);
void Cmp1IntCallback(uint32_t);


/*---------------------------------------------------------------------------------------------------------*/
/* Define global variables                                                                                 */
/*---------------------------------------------------------------------------------------------------------*/
uint8_t gu8AdcIntFlag;
uint8_t gu8AdcCmp0IntFlag;
uint8_t gu8AdcCmp1IntFlag;

#define TEST_LENGTH         256
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



/*---------------------------------------------------------------------------------------------------------*/
/* ADC RX Callback                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
void PDMA0_Callback()
{
    static uint8_t PingPong=0;

    c_ctrl.Ch1AudioDataRdy[PingPong] = TRUE;
        
    PingPong ^= 1;
    PDMA->channel[eDRVPDMA_CHANNEL_0].DAR = (uint32_t)&audio_buffer[PingPong+2][0];
    DrvPDMA_CHEnablelTransfer(eDRVPDMA_CHANNEL_0);

}

/*---------------------------------------------------------------------------------------------------------*/
/* PDMA Callback function                                                                                  */
/*---------------------------------------------------------------------------------------------------------*/
void PDMA1_TA_Callback(uint32_t status)
{    
      DrvPDMA_CHSoftwareReset (eDRVPDMA_CHANNEL_1 );
      DrvPDMA_CHEnablelTransfer (eDRVPDMA_CHANNEL_1 );
}

/*---------------------------------------------------------------------------------------------------------*/
/* I2S/DPWM TX Callback                                                                                    */
/*---------------------------------------------------------------------------------------------------------*/
void PDMA1_Callback(uint32_t status)
{
    static uint8_t PingPong, TxSilence;
    volatile uint32_t u32SFR;
     
    if(status == I2S_INITIALIZE){
        // Callback can be called from compressor to initialize
        // the buffers.
        PingPong = 0;
        TxSilence =1;
        // Point PDMA to silence buffer 
        PDMA->channel[eDRVPDMA_CHANNEL_1].SAR = (uint32_t)&zero_buf;
        PDMA->channel[eDRVPDMA_CHANNEL_1].BCR = ZERO_BUF_SAMPLES * sizeof(int16_t);

        DrvPDMA_CHEnablelTransfer (eDRVPDMA_CHANNEL_1 );
    }else{
        if(TxSilence){
            // We are waiting for valid data in the output buffer,
            // check to see if present and start transfer
            if(c_ctrl.Ch1AudioDataRdy[PingPong] == TRUE){
                PDMA->channel[eDRVPDMA_CHANNEL_1].SAR = (uint32_t)&audio_buffer[PingPong+2][0];
                PDMA->channel[eDRVPDMA_CHANNEL_1].BCR = MAX_FRAMESIZE * sizeof(int16_t);
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
                PDMA->channel[eDRVPDMA_CHANNEL_1].SAR = (uint32_t)&zero_buf;    
                PDMA->channel[eDRVPDMA_CHANNEL_1].BCR = ZERO_BUF_SAMPLES * sizeof(int16_t);
            }else{
                // Data ready
                PDMA->channel[eDRVPDMA_CHANNEL_1].SAR = (uint32_t)&audio_buffer[PingPong+2][0];
                PDMA->channel[eDRVPDMA_CHANNEL_1].BCR = MAX_FRAMESIZE * sizeof(int16_t);
                TxSilence =0;
            }           
        }// if(TxSilence) else
        DrvPDMA_CHEnablelTransfer(eDRVPDMA_CHANNEL_1);  
    } // if(status == I2S_INITIALIZE) else
}


/*---------------------------------------------------------------------------------------------------------*/
/* DPWM_Play                                                                                               */
/*---------------------------------------------------------------------------------------------------------*/
void DPWM_Play(void)
{
    STR_PDMA_T sPDMA;  

    // CH3 DPWM TX Setting 
    sPDMA.sSrcAddr.u32Addr          = (uint32_t)&zero_buf;
    sPDMA.sDestAddr.u32Addr         = (uint32_t)&DPWM->FIFO;  
    sPDMA.u8TransWidth              = eDRVPDMA_WIDTH_16BITS;
    sPDMA.u8Mode                    = eDRVPDMA_MODE_MEM2APB;
    sPDMA.sSrcAddr.eAddrDirection   = eDRVPDMA_DIRECTION_INCREMENTED; 
    sPDMA.sDestAddr.eAddrDirection  = eDRVPDMA_DIRECTION_FIXED;   
    sPDMA.i32ByteCnt                = ZERO_BUF_SAMPLES * 4;

    DrvPDMA_Open(eDRVPDMA_CHANNEL_1,&sPDMA);

    // PDMA Setting 
    DrvPDMA_SetCHForAPBDevice(
        eDRVPDMA_CHANNEL_1, 
        eDRVPDMA_DPWM,
        eDRVPDMA_WRITE_APB    
    );

    // Enable INT 
    DrvPDMA_EnableInt(eDRVPDMA_CHANNEL_1, eDRVPDMA_BLKD );
        
    // Install Callback function    
    DrvPDMA_InstallCallBack(eDRVPDMA_CHANNEL_1, eDRVPDMA_BLKD, (PFN_DRVPDMA_CALLBACK) PDMA1_Callback );      
    DrvPDMA_InstallCallBack(eDRVPDMA_CHANNEL_1, eDRVPDMA_TABORT, (PFN_DRVPDMA_CALLBACK) PDMA1_TA_Callback );    

    // Enable DPWM PDMA 
    DrvDPWM_EnablePDMA();
    
    PDMA1_Callback( I2S_INITIALIZE );
}

/*---------------------------------------------------------------------------------------------------------*/
/* I2S_Play                                                                                                */
/*---------------------------------------------------------------------------------------------------------*/
void I2S_Play(void)
{
    STR_PDMA_T sPDMA;  


    // CH1 I2S TX Setting 
    sPDMA.sSrcAddr.u32Addr          = (uint32_t)&zero_buf;
    sPDMA.sDestAddr.u32Addr         = (uint32_t)&(I2S->TXFIFO);  
    sPDMA.u8TransWidth              = eDRVPDMA_WIDTH_16BITS;
    sPDMA.u8Mode                    = eDRVPDMA_MODE_MEM2APB;
    sPDMA.sSrcAddr.eAddrDirection   = eDRVPDMA_DIRECTION_INCREMENTED; 
    sPDMA.sDestAddr.eAddrDirection  = eDRVPDMA_DIRECTION_FIXED;   
    sPDMA.i32ByteCnt                = ZERO_BUF_SAMPLES * 4;

    DrvPDMA_Open(eDRVPDMA_CHANNEL_1, &sPDMA);

    // PDMA Setting 
    DrvPDMA_SetCHForAPBDevice(
        eDRVPDMA_CHANNEL_1, 
        eDRVPDMA_I2S,
        eDRVPDMA_WRITE_APB    
    );  
           
    // Install Callback function    
    DrvPDMA_InstallCallBack(eDRVPDMA_CHANNEL_1, eDRVPDMA_BLKD, (PFN_DRVPDMA_CALLBACK) PDMA1_Callback );      
    DrvPDMA_InstallCallBack(eDRVPDMA_CHANNEL_1, eDRVPDMA_TABORT, (PFN_DRVPDMA_CALLBACK) PDMA1_TA_Callback );    

    // Enable INT 
    DrvPDMA_EnableInt(eDRVPDMA_CHANNEL_1, eDRVPDMA_BLKD );

    // Enable I2S PDMA 
    DrvI2S_EnableTxDMA (TRUE);       
    DrvI2S_EnableTx(TRUE);

    PDMA1_Callback( I2S_INITIALIZE );       
}

/*---------------------------------------------------------------------------------------------------------*/
/* VMIDTest                                                                                                */
/*---------------------------------------------------------------------------------------------------------*/
void VMIDTest()
{
    /* Open Analog block */
    DrvADC_AnaOpen();

    printf("\n\n");
    /* MIC circuit configuration */
    DrvADC_SetVMID(
        eDRVADC_PULLDOWN_VMID_GND,     
        eDRVADC_PDLORES_DISCONNECTED,        
        eDRVADC_PDHIRES_DISCONNECTED);    

    printf("eDRVADC_PULLDOWN_VMID_GND eDRVADC_PDLORES_DISCONNECTED eDRVADC_PDHIRES_DISCONNECTED\n");

    DrvADC_SetVMID(
        eDRVADC_PULLDOWN_VMID_RELEASE,   
        eDRVADC_PDLORES_DISCONNECTED,   
        eDRVADC_PDHIRES_DISCONNECTED);   

    printf("eDRVADC_PULLDOWN_VMID_RELEASE eDRVADC_PDLORES_DISCONNECTED eDRVADC_PDHIRES_DISCONNECTED\n");

    DrvADC_SetVMID(
        eDRVADC_PULLDOWN_VMID_RELEASE,    
        eDRVADC_PDLORES_CONNECTED,       
        eDRVADC_PDHIRES_DISCONNECTED);  

    printf("eDRVADC_PULLDOWN_VMID_RELEASE eDRVADC_PDLORES_CONNECTED eDRVADC_PDHIRES_DISCONNECTED\n");

    DrvADC_SetVMID(
        eDRVADC_PULLDOWN_VMID_RELEASE,  
        eDRVADC_PDLORES_DISCONNECTED,
        eDRVADC_PDHIRES_CONNECTED);    

    printf("eDRVADC_PULLDOWN_VMID_RELEASE eDRVADC_PDLORES_DISCONNECTED eDRVADC_PDHIRES_CONNECTED\n");

    DrvADC_SetVMID(
        eDRVADC_PULLDOWN_VMID_RELEASE,    
        eDRVADC_PDLORES_CONNECTED,        
        eDRVADC_PDHIRES_CONNECTED);    

    printf("eDRVADC_PULLDOWN_VMID_RELEASE eDRVADC_PDLORES_CONNECTED eDRVADC_PDHIRES_CONNECTED\n");

}

/*---------------------------------------------------------------------------------------------------------*/
/* MICBIASTest                                                                                             */
/*---------------------------------------------------------------------------------------------------------*/
void MICBIASTest(void)
{
    printf("\n\n");
    DrvADC_SetMIC(TRUE, eDRVADC_MIC_BIAS_90_VCCA);
    printf("MICBIAS Sel: 90% VCCA \n");

    DrvADC_SetMIC(TRUE, eDRVADC_MIC_BIAS_65_VCCA);
    printf("MICBIAS Sel: 65% VCCA \n");

    DrvADC_SetMIC(TRUE, eDRVADC_MIC_BIAS_75_VCCA);
    printf("MICBIAS Sel: 75% VCCA \n");

    DrvADC_SetMIC(TRUE, eDRVADC_MIC_BIAS_50_VCCA);
    printf("MICBIAS Sel: 50% VCCA \n");

    DrvADC_SetMIC(TRUE, eDRVADC_MIC_BIAS_2p4V);
    printf("MICBIAS Sel: 2.4V \n");

    DrvADC_SetMIC(TRUE, eDRVADC_MIC_BIAS_1p7V);
    printf("MICBIAS Sel: 1.7V \n");

    DrvADC_SetMIC(TRUE, eDRVADC_MIC_BIAS_2p0V);
    printf("MICBIAS Sel: 2.0V \n");

    DrvADC_SetMIC(TRUE, eDRVADC_MIC_BIAS_1p3V);
    printf("MICBIAS Sel: 1.3V \n");
}

/*---------------------------------------------------------------------------------------------------------*/
/* SysTimerDelay                                                                                           */
/*---------------------------------------------------------------------------------------------------------*/
void SysTimerDelay(uint32_t us)
{
    SysTick->LOAD = us * 49; /* Assume the internal 49MHz RC used */
    SysTick->VAL  =  (0x00);
    SysTick->CTRL = (1 << SYSTICK_CLKSOURCE) | (1<<SYSTICK_ENABLE);

    /* Waiting for down-count to zero */
    while((SysTick->CTRL & (1 << 16)) == 0);
}



/*---------------------------------------------------------------------------------------------------------*/
/* InitialSystemClock                                                                                      */
/*---------------------------------------------------------------------------------------------------------*/
void InitialSystemClock(void)
{
    /* Unlock the protected registers */    
    UNLOCKREG();
 
    /* HCLK clock source */
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
    sParam.u32BaudRate      = 115200;
    sParam.u8cDataBits      = DRVUART_DATABITS_8;
    sParam.u8cStopBits      = DRVUART_STOPBITS_1;
    sParam.u8cParity        = DRVUART_PARITY_NONE;
    sParam.u8cRxTriggerLevel= DRVUART_FIFO_1BYTES;

    /* Set UART Configuration */
    DrvUART_Open(UART_PORT0,&sParam);
}

/*---------------------------------------------------------------------------------------------------------*/
/* InitialADC                                                                                              */
/*---------------------------------------------------------------------------------------------------------*/
void InitialADC(void)
{
    S_DRVADC_PARAM sParam;
    uint32_t u32AdcStatus;
    uint32_t OSR;

    // b0,b1,b2,a1,a2,b0,b1,b2,a1,a2,b0,b1,b2,a1,a2
    /*
    uint32_t u32BiqCoeff[15]={0x10000, 0x15b8a, 0x10000, 0x15068, 0x0ef98,
                              0x10000, 0x00000, 0x00000, 0x00000, 0x00000,
                              0x10000, 0x00000, 0x00000, 0x00000, 0x00000};
    */

    /* Open Analog block */
    DrvADC_AnaOpen();

    /* Power control */
    DrvADC_SetPower( 
        eDRVADC_PU_MOD_ON, 
        eDRVADC_PU_IBGEN_ON, 
        eDRVADC_PU_BUFADC_ON, 
        eDRVADC_PU_BUFPGA_ON, 
        eDRVADC_PU_ZCD_OFF);

    /* PGA Setting */   
    DrvADC_PGAMute(eDRVADC_MUTE_PGA);
    DrvADC_PGAUnMute(eDRVADC_MUTE_IPBOOST);
    DrvADC_SetPGA(  
        eDRVADC_REF_SEL_VMID,
        eDRVADC_PU_PGA_ON,
        eDRVADC_PU_BOOST_ON,
        eDRVADC_BOOSTGAIN_0DB);

    DrvADC_SetPGAGaindB(0);   // 0 dB

    /* MIC circuit configuration */
    DrvADC_SetVMID(
        eDRVADC_PULLDOWN_VMID_RELEASE,
        eDRVADC_PDLORES_CONNECTED,
        eDRVADC_PDHIRES_DISCONNECTED);
    DrvADC_SetMIC(TRUE, eDRVADC_MIC_BIAS_90_VCCA);

    /* ALC Setting */   
    //ALC->ALC_CTRL.ALCLVL = 15;
    //ALC->ALC_CTRL.ALCSEL =1;
    //ALC->ALC_CTRL.ALCSEL =0;
    //ALC->ALC_CTRL.ALCDCY = 3;
    //ALC->ALC_CTRL.NGEN =1;
    

    /* Open ADC block */
    sParam.u8AdcDivisor   = 0;
    //sParam.u8SDAdcDivisor = 16;   //OSR64  :16 for 48K
    //sParam.u8SDAdcDivisor = 16;   //OSR128 :48 for 8K, 24 for 16K
    sParam.u8SDAdcDivisor = 16;     //OSR192 :32 for 8K, 16 for 16K
    //sParam.eOSR         = eDRVADC_OSR_128;
    sParam.eOSR       = eDRVADC_OSR_192;
    //sParam.eOSR         = eDRVADC_OSR_64;
    sParam.eInputSrc  = eDRVADC_MIC;
    sParam.eInputMode = eDRVADC_DIFFERENTIAL;
    sParam.u8ADCFifoIntLevel = 7;
    u32AdcStatus=DrvADC_Open(&sParam);
    if(u32AdcStatus == E_SUCCESS) {
        printf("ADC has been successfully opened.\n");
        printf("ADC clock divisor=%d\n",SYSCLK->CLKDIV.ADC_N);
        printf("ADC over sampling clock divisor=%d\n",SDADC->CLK_DIV);
        switch(SDADC->DEC.OSR)
        {
          case eDRVADC_OSR_64:OSR=64;break;
          case eDRVADC_OSR_128:OSR=128;break;
          case eDRVADC_OSR_192:OSR=192;break;
          case eDRVADC_OSR_384:OSR=384;break;
        }
        printf("ADC over sampling ratio=%d\n", OSR);
        printf("Select microphone path as differential input\n");
        printf("ADC Fifo Interrupt Level=%d\n", SDADC->INT.FIFO_IE_LEV);
        printf("Conversion rate: %d samples/second\n", DrvADC_GetConversionRate());
    }
    else {
        printf("ADC Open failed!\n");
    }
    /* Change Decimation and FIFO Setting */
    //DrvADC_SetAdcOverSamplingClockDivisor(u8SDAdcDivisor);
    //DrvADC_SetOverSamplingRatio(eOSR);
    //DrvADC_SetCICGain(u8CICGain);
    //DrvADC_SetFIFOIntLevel(u8ADCFifoIntLevel);

    /* Change BIQ Setting */            
    //SYSCLK->APBCLK.BIQ_EN = 1;
    //SYS->IPRSTC2.BIQ_RST = 1;
    //SYS->IPRSTC2.BIQ_RST = 0;
    //DrvADC_SetBIQ(1023, 1, eDRVADC_BIQ_IN_ADC, u32BiqCoeff);

    /* Interrupt Setting */
    //DrvADC_EnableAdcInt(DRVADC_ADC_CALLBACK Callback, uint32_t u32UserData);

    DrvADC_PGAUnMute(eDRVADC_MUTE_PGA);
}

/*---------------------------------------------------------------------------------------------------------*/
/* InitialDPWM                                                                                             */
/*---------------------------------------------------------------------------------------------------------*/
void InitialDPWM(uint32_t u32SampleRate)
{
    DrvDPWM_Open();
    DrvDPWM_SetDPWMClk(E_DRVDPWM_DPWMCLK_HCLKX2);
    //DrvDPWM_SetDPWMClk(E_DRVDPWM_DPWMCLK_HCLK);
    DrvDPWM_SetSampleRate(u32SampleRate);
    DrvDPWM_Enable();
}

/*---------------------------------------------------------------------------------------------------------*/
/* InitialI2S                                                                                              */
/*---------------------------------------------------------------------------------------------------------*/
void InitialI2S(uint32_t u32SampleRate)
{
    S_DRVI2S_DATA_T st;

    SYSCLK->CLKSEL2.I2S_S=3;

    /* Set I2S Parameter */
    switch(u32SampleRate) 
    {
     case 8000:  st.u32SampleRate    = 8000; break;
     case 16000: st.u32SampleRate    = 16000; break;
     case 48000: st.u32SampleRate    = 48000; break;
     default:    st.u32SampleRate    = 16000; break;
    }
    st.u8WordWidth       = DRVI2S_DATABIT_16;
    st.u8AudioFormat     = DRVI2S_STEREO;       
    st.u8DataFormat      = DRVI2S_FORMAT_I2S;   
    st.u8Mode            = DRVI2S_MODE_MASTER;
    st.u8RxFIFOThreshold = DRVI2S_FIFO_LEVEL_WORD_4;
    st.u8TxFIFOThreshold = DRVI2S_FIFO_LEVEL_WORD_4;
    DrvI2S_Open(&st);
    
    I2S->CON.MCLKEN = 1;
    SYS->GPA_ALT.GPA0    = 2;    // MCLK Output 

    switch(u32SampleRate) 
    {
     case 8000:  I2S->CLKDIV.BCLK_DIV = 95; break;
     case 16000: I2S->CLKDIV.BCLK_DIV = 47; break;
     case 48000: I2S->CLKDIV.BCLK_DIV = 15; break;
     default:    I2S->CLKDIV.BCLK_DIV = 47; break;
    }

    I2S->CLKDIV.MCLK_DIV = 6 ;   // 6 

    /* Set I2S I/O */
    //DrvGPIO_InitFunction(FUNC_I2S0);
    SYS->GPA_ALT.GPA4       =1;  // 
    SYS->GPA_ALT.GPA5       =1;  // 
    SYS->GPA_ALT.GPA6       =1;  // 
    SYS->GPA_ALT.GPA7       =1;  // 

    /* Enable I2S Tx/Rx function */
    DrvI2S_EnableRx(TRUE);
    DrvI2S_EnableTx(TRUE);
     
}

/*---------------------------------------------------------------------------------------------------------*/
/* AdcByPDMATest                                                                                           */
/*---------------------------------------------------------------------------------------------------------*/
void AdcByPDMATest(void)
{
    STR_PDMA_T sPDMA;  
    uint8_t u8Option;
    int32_t i32PGAGaindB = 0; //dB
    int32_t i32PGAGainSet;
	uint8_t ALC_ON;
    
    printf("\n=== MIC To SPK test ===\n");

    InitialADC();
	
    DrvADC_SetPGA(  
        eDRVADC_REF_SEL_VMID,
        eDRVADC_PU_PGA_ON,
        eDRVADC_PU_BOOST_ON,
        eDRVADC_BOOSTGAIN_26DB);
	printf("BOOST Gain = 26dB\n");

	
	printf("ALC on/off (1 on, 0 off):");
	u8Option = getchar();

	if(u8Option == '1')  {
		ALC_ON=1;
		printf("ALC is on.\n");
	}
	else {               
		ALC_ON=0;
		printf("ALC is off. Tune PGA gain by user input.\n");
	}

	if(ALC_ON) {
    // ALC Setting    	
    SYSCLK->APBCLK.BIQALC_EN = 1;
    ALC->ALC_CTRL.ALCLVL = 15;
    ALC->ALC_CTRL.ALCSEL =1;         //ALC enable
//  ALC->ALC_CTRL.ALCSEL =0;        //ALC disable
    ALC->ALC_CTRL.ALCDCY = 3;
    ALC->ALC_CTRL.ALCATK = 2;
    ALC->ALC_CTRL.NGEN =1;
    ALC->ALC_CTRL.NGTH =6;
	}

    //-------------------------------------------------------------------------
    // PDMA Init 
    DrvPDMA_Init();

    // CH2 ADC RX Setting 
    sPDMA.sSrcAddr.u32Addr          = (uint32_t)&SDADC->ADCOUT; 
    sPDMA.sDestAddr.u32Addr         = (uint32_t)&audio_buffer[2][0];
    sPDMA.u8Mode                    = eDRVPDMA_MODE_APB2MEM;
    sPDMA.u8TransWidth              = eDRVPDMA_WIDTH_16BITS;
    sPDMA.sSrcAddr.eAddrDirection   = eDRVPDMA_DIRECTION_FIXED; 
    sPDMA.sDestAddr.eAddrDirection  = eDRVPDMA_DIRECTION_INCREMENTED;   
    sPDMA.i32ByteCnt = MAX_FRAMESIZE * sizeof(int16_t);
    DrvPDMA_Open(eDRVPDMA_CHANNEL_0, &sPDMA);

    // PDMA Setting 
    DrvPDMA_SetCHForAPBDevice(
        eDRVPDMA_CHANNEL_0, 
        eDRVPDMA_ADC,
        eDRVPDMA_READ_APB    
    );


    // Enable INT 
    DrvPDMA_EnableInt(eDRVPDMA_CHANNEL_0, eDRVPDMA_BLKD );
        
    // Install Callback function    
    DrvPDMA_InstallCallBack(eDRVPDMA_CHANNEL_0, eDRVPDMA_BLKD, (PFN_DRVPDMA_CALLBACK) PDMA0_Callback ); 

    // Enable ADC PDMA and Trigger PDMA specified Channel 
    DrvADC_PdmaEnable();
    
    // Enable DPWM and set sampling rate
    InitialDPWM(16000);
    //InitialDPWM(8000);
    DPWM_Play();

    // Start A/D conversion 
    DrvADC_StartConvert();

    // start ADC PDMA transfer
    DrvPDMA_CHEnablelTransfer(eDRVPDMA_CHANNEL_0);
    
    while(1)
    {
		if(ALC_ON) {
			printf("\n Press 'q' to quit\n");
			u8Option=getchar();
			if(u8Option=='q') {
			ALC->ALC_CTRL.ALCSEL = 0;
			SYSCLK->APBCLK.BIQALC_EN = 0;
			break;
			}
		} else {
        printf("\nChange ADC parameter\n");
        printf("  [i] increase PGA gain\n");
        printf("  [d] decrease PGA gain\n");
        printf("  [q] Exit\n");
        u8Option = getchar();

        if(u8Option=='i') {
            i32PGAGaindB += 50;
            i32PGAGainSet=DrvADC_SetPGAGaindB(i32PGAGaindB);
            printf("Current PGA Gain = %c%d.%d dB\n\n\n", sign(i32PGAGainSet), abs(i32PGAGainSet)/100, abs(i32PGAGainSet)%100);
        }
        else if(u8Option=='d') {
            i32PGAGaindB -= 50;
            i32PGAGainSet=DrvADC_SetPGAGaindB(i32PGAGaindB);
            printf("Current PGA Gain = %c%d.%d dB\n\n\n", sign(i32PGAGainSet), abs(i32PGAGainSet)/100, abs(i32PGAGainSet)%100);
        }
        else if(u8Option=='q')
            break;
		}
    }


    DrvPDMA_Close();

    DrvADC_Close();

    DrvDPWM_Close();
            
}

/*---------------------------------------------------------------------------------------------------------*/
/* AdcSoundQualityTest                                                                                     */
/*---------------------------------------------------------------------------------------------------------*/
void AdcSoundQualityTest()
{
    STR_PDMA_T sPDMA;  
    uint8_t u8Option;
    int32_t i32PGAGainSet;
    int32_t i32PGAGaindB = 0; //dB
    
    printf("\n=== ADC Sound Quality test ===\n");

    InitialADC();

    //------------------------------------------------------------------
    // PDMA Init 
    DrvPDMA_Init();

    // CH2 ADC RX Setting 
    sPDMA.sSrcAddr.u32Addr          = (uint32_t)&SDADC->ADCOUT; 
    sPDMA.sDestAddr.u32Addr         = (uint32_t)&audio_buffer[2][0];
    sPDMA.u8Mode                    = eDRVPDMA_MODE_APB2MEM;
    sPDMA.u8TransWidth              = eDRVPDMA_WIDTH_16BITS;
    sPDMA.sSrcAddr.eAddrDirection   = eDRVPDMA_DIRECTION_FIXED; 
    sPDMA.sDestAddr.eAddrDirection  = eDRVPDMA_DIRECTION_INCREMENTED;   
    sPDMA.i32ByteCnt = MAX_FRAMESIZE * sizeof(int16_t);
    DrvPDMA_Open(eDRVPDMA_CHANNEL_0, &sPDMA);

    // PDMA Setting 
    DrvPDMA_SetCHForAPBDevice(
        eDRVPDMA_CHANNEL_0, 
        eDRVPDMA_ADC,
        eDRVPDMA_READ_APB    
    );

    // Enable INT 
    DrvPDMA_EnableInt(eDRVPDMA_CHANNEL_0, eDRVPDMA_BLKD );
        
    // Install Callback function    
    DrvPDMA_InstallCallBack(eDRVPDMA_CHANNEL_0, eDRVPDMA_BLKD, (PFN_DRVPDMA_CALLBACK) PDMA0_Callback ); 

    // Enable ADC PDMA and Trigger PDMA specified Channel 
    DrvADC_PdmaEnable();
    
    // I2S interface
    InitialI2S(16000);
    I2S_Play();

    // Start A/D conversion 
    DrvADC_StartConvert();

    // start ADC PDMA transfer
    DrvPDMA_CHEnablelTransfer(eDRVPDMA_CHANNEL_0);
    
    while(1)
    {
        printf("ADC record and send to I2S interface\n");
        printf("  [i] increase PGA gain\n");
        printf("  [d] decrease PGA gain\n");
        printf("  [q] Exit\n");
        u8Option = getchar();

        
        if(u8Option=='i') {
            i32PGAGaindB += 50;
            i32PGAGainSet=DrvADC_SetPGAGaindB(i32PGAGaindB);
            printf("Current PGA Gain = %c%d.%d dB\n\n\n", sign(i32PGAGainSet), abs(i32PGAGainSet)/100, abs(i32PGAGainSet)%100);
        }
        else if(u8Option=='d') {
            i32PGAGaindB -= 50;
            i32PGAGainSet=DrvADC_SetPGAGaindB(i32PGAGaindB);                                                           
            printf("Current PGA Gain = %c%d.%d dB\n\n\n", sign(i32PGAGainSet), abs(i32PGAGainSet)/100, abs(i32PGAGainSet)%100);
        }
        else 
        
        if(u8Option=='q')
            break;
    }

    DrvPDMA_Close();

    DrvADC_Close();

    DrvI2S_Close();
}

/*---------------------------------------------------------------------------------------------------------*/
/* AdcSingleModeTest                                                                                       */
/*---------------------------------------------------------------------------------------------------------*/
void AdcSingleModeTest()
{
    int8_t  j;
    uint8_t u8Option;
    uint8_t u8InputMode;
    uint32_t u32ConversionData;
    
    printf("\n== Single mode test ===\n");
    InitialADC();

    DrvADC_SetFIFOIntLevel(7);  
    while(1)
    {
        single_menu:
        printf("Select input mode:\n");
        printf("  [1] Single end input\n");
        printf("  [2] Differential input\n");
        printf("  [q] Exit single mode test\n");
        u8Option = getchar();
        if(u8Option=='1')
            u8InputMode = 1; // single-end
        else if(u8Option=='2')
            u8InputMode = 2; // differential 
        else if(u8Option=='q')
            return ;
        else
            goto single_menu;
        
        if(u8InputMode==1)        
            SingleEndInput_ChannelSelect();     // Select the active channel
        else
            DifferentialInput_ChannelSelect();  // Select the active channel 
                
        // Enable ADC Interrupt function
        DrvADC_EnableAdcInt(AdcIntCallback, 0);
    
        // Start A/D conversion 
        DrvADC_StartConvert();
        // Wait ADC interrupt 

        while(gu8AdcIntFlag==0);
        gu8AdcIntFlag = 0;

        for(j=1;j<=8;j++) {
            u32ConversionData = DrvADC_GetConversionData();
            printf("0x%X (%d)\n\n", u32ConversionData, u32ConversionData);
        }

        
    }    
}

/*---------------------------------------------------------------------------------------------------------*/
/* AdcCyleModeTest                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
void AdcCycleModeTest()
{
    int8_t  i,j;
    uint8_t u8Option;
    uint8_t u8InputMode;
    uint32_t u32ConversionData[8];
    
    printf("\n== Cycle mode test ===\n");
    InitialADC();

    DrvADC_SetFIFOIntLevel(7);        
    while(1)
    {
        printf("Select input mode:\n");
        printf("  [1] Single end input\n");
        printf("  [2] Differential input\n");
        printf("  [q] Exit cycle mode test\n");
        u8Option = getchar();
        if(u8Option=='1')
            u8InputMode = 1; // single-end
        else if(u8Option=='2')
            u8InputMode = 2; // differential 
        else
            return ;
        
        if(u8InputMode==1)        
            SingleEndInput_ChannelSelect();     // Select the active channel
        else
            DifferentialInput_ChannelSelect();  // Select the active channel 
        
        for(i=1;i<=30;i++) {
                
            // Enable ADC Interrupt function
            DrvADC_EnableAdcInt(AdcIntCallback, 0);
    
            // Start A/D conversion 
            DrvADC_StartConvert();

            // Wait ADC interrupt 
            while(gu8AdcIntFlag==0);
            gu8AdcIntFlag = 0;

            printf("--\n");
            for(j=0;j<=7;j++) {
            u32ConversionData[j] = DrvADC_GetConversionData();
            }
            for(j=0;j<=7;j++) {
            printf("0x%X (%d)\n", u32ConversionData[j], u32ConversionData[j]);          
            }
            SysTimerDelay(3000000);
        }
    }    
}


/*---------------------------------------------------------------------------------------------------------*/
/* AdcCompMonitorTest                                                                                      */
/*---------------------------------------------------------------------------------------------------------*/
void AdcCompMonitorTest()
{
    uint8_t u8CmpChannelNum, u8CmpMatchCount;
    uint32_t u32ConversionData[8];
    uint8_t i;
    uint32_t u32HiTh,u32LoTh;
    uint8_t Option;

    u32HiTh = 0x7000;
    u32LoTh = 0x9000;

    show_menu:
    printf("\n\n");
    printf("+----------------------------------------------------------------------+\n");       
    printf("|                      ADC compare monitor test                        |\n");       
    printf("+----------------------------------------------------------------------+\n");
    printf("Threshold values\n");
    printf("a) 0x7000\n");
    printf("b) 0x6000\n");
    printf("c) 0x5000\n");
    printf("d) 0x4000\n");
    printf("e) 0x3000\n");
    printf("f) 0x2000\n");
    printf("g) 0x1000\n");
    printf("h) 0x0000\n");
    printf("i) 0xf000\n");
    printf("j) 0xe000\n");
    printf("k) 0xd000\n");
    printf("l) 0xc000\n");
    printf("m) 0xb000\n");
    printf("n) 0xa000\n");
    printf("o) 0x9000\n");
    printf("input choice:");
    Option=getchar();

    if(Option == 'a')
        u32LoTh = 0x7000;
    else if (Option == 'b')
        u32LoTh = 0x6000;
    else if (Option == 'c')
        u32LoTh = 0x5000;
    else if (Option == 'd')
        u32LoTh = 0x4000;
    else if (Option == 'e')
        u32LoTh = 0x3000;
    else if (Option == 'f')
        u32LoTh = 0x2000;
    else if (Option == 'g')
        u32LoTh = 0x1000;
    else if (Option == 'h')
        u32LoTh = 0x0000;
    else if (Option == 'i')
        u32LoTh = 0xf000;
    else if (Option == 'j')
        u32LoTh = 0xe000;
    else if (Option == 'k')
        u32LoTh = 0xd000;
    else if (Option == 'l')
        u32LoTh = 0xc000;
    else if (Option == 'm')
        u32LoTh = 0xb000;
    else if (Option == 'n')
        u32LoTh = 0xa000;
    else if (Option == 'o')
        u32LoTh = 0x9000;
    else {
        printf("Invalid option!\n");
        goto show_menu;
    }
    u32HiTh = u32LoTh;
    printf("Test Lo threshold = %x Hi threshold = %x\n", u32LoTh, u32HiTh);

    InitialADC();
                        
    //use GPB0/1 for test
    //DrvADC_SetAdcChannel(eDRVADC_GPIO, eDRVADC_SINGLE_END_CH1_IN_N);
    //DrvADC_SetAdcChannel(eDRVADC_GPIO, eDRVADC_DIFFERENTIAL_CH01);
    
    DrvADC_SetFIFOIntLevel(7);
    //------------------------------------------------
    DrvADC_EnableAdcInt(AdcIntCallback, 0);
    // start A/D conversion 
    DrvADC_StartConvert();
    while(gu8AdcIntFlag==0);
            gu8AdcIntFlag = 0;
    for(i=1;i<=8;i++)
        DrvADC_GetConversionData();

    DrvADC_EnableAdcInt(AdcIntCallback, 0);
    while(gu8AdcIntFlag==0);
            gu8AdcIntFlag = 0;
    for(i=1;i<=8;i++)
        DrvADC_GetConversionData();

    //------------------------------------------------------------------------
    u8CmpChannelNum = 0;
    gu8AdcCmp0IntFlag = 0;
    u8CmpMatchCount = 8;
    
    // Enable ADC compare 0. Compare condition: conversion result < 0x9000. 
    //DrvADC_Adcmp0Enable(eDRVADC_LESS_THAN, u32HiTh, u8CmpMatchCount);
    //DrvADC_Adcmp0Enable(eDRVADC_LESS_THAN, u32LoTh, u8CmpMatchCount);

    // enable ADC compare 0 interrupt and set the callback function 
    //DrvADC_EnableAdcmp0Int(Cmp0IntCallback, 0);

    //------------------------------------------------------------------------
    gu8AdcCmp1IntFlag = 0;
    u8CmpMatchCount = 8;    
    // Enable ADC compare 1. Compare condition: conversion result >= 0x7000. 
    DrvADC_Adcmp1Enable(eDRVADC_GREATER_OR_EQUAL, u32HiTh,  u8CmpMatchCount);
    //DrvADC_Adcmp1Enable(eDRVADC_GREATER_OR_EQUAL, u32LoTh,  u8CmpMatchCount);

    // enable ADC compare 1 interrupt and set the callback function 
    DrvADC_EnableAdcmp1Int(Cmp1IntCallback, 0);
    

    // Wait ADC compare interrupt 
    while( (gu8AdcCmp1IntFlag==0) )  {
            DrvADC_EnableAdcInt(AdcIntCallback, 0);
            while(gu8AdcIntFlag==0);
            gu8AdcIntFlag = 0;
            //printf("--\n");
            for(i=0;i<=7;i++) {             
                u32ConversionData[i] = DrvADC_GetConversionData();          
            }
            
            for(i=0;i<=7;i++) {
                printf("0x%X (%d)\n", u32ConversionData[i], u32ConversionData[i]);
            }
            SysTimerDelay(300000);
    }

            printf("--\n");
            for(i=0;i<=7;i++) {             
                u32ConversionData[i] = DrvADC_GetConversionData();          
            }
            for(i=0;i<=7;i++) {
                printf("0x%X (%d)\n", u32ConversionData[i], u32ConversionData[i]);
            }   
    //DrvADC_StopConvert();
    DrvADC_DisableAdcmp0Int();
    DrvADC_DisableAdcmp1Int();
    DrvADC_Adcmp0Disable();
    DrvADC_Adcmp1Disable();

    if(gu8AdcCmp0IntFlag==1)
    {
        printf("The conversion result of channel %d is less than 0x%X\n", u8CmpChannelNum, u32LoTh);
    }
    else
    {
        printf("The conversion result of channel %d is greater or equal to 0x%X\n", u8CmpChannelNum, u32HiTh);
    }
    
}

/*---------------------------------------------------------------------------------------------------------*/
/* AdcTemperatureMonitorTest                                                                               */
/*---------------------------------------------------------------------------------------------------------*/

void AdcTemperatureMonitorTest(void)
{
    uint8_t u8Option;
    uint8_t i;
    uint32_t u32ConversionData;
    int32_t i32Temp;
    
    printf("\n=== ADC Temperature mode test ===\n");

    InitialADC();

    // For Temperature setting 
    printf("Change to temperature monitor input\n");
    DrvADC_SetAdcChannel(eDRVADC_TEMP, eDRVADC_DIFFERENTIAL);

    DrvADC_SetPGA(  
        eDRVADC_REF_SEL_VBG,
        eDRVADC_PU_PGA_ON,
        eDRVADC_PU_BOOST_ON,
        eDRVADC_BOOSTGAIN_0DB);

    DrvADC_SetPGAGaindB(525);         

    DrvADC_SetFIFOIntLevel(1);        // interrupt occurs when one sample is converted.

    // Start A/D conversion 
    DrvADC_StartConvert();

    for(i=1;i<=10;i++) {
    // Enable ADC Interrupt function
        DrvADC_EnableAdcInt(AdcIntCallback, 0);
    
        while(gu8AdcIntFlag==0);
            gu8AdcIntFlag = 0;
    
        u32ConversionData = DrvADC_GetConversionData();
    }

    while(1)
    {
        printf("Start to get temperature[y/n]:\n");
        
        u8Option = getchar();
        if(u8Option=='y')
             ;
        else
            return ;
                
        // Enable ADC Interrupt function
        DrvADC_EnableAdcInt(AdcIntCallback, 0);
    
        // Start A/D conversion 
        //DrvADC_StartConvert();

        // Wait ADC interrupt 
        while(gu8AdcIntFlag==0);

        gu8AdcIntFlag = 0;
        
        u32ConversionData = DrvADC_GetConversionData();

        i32Temp = 27+ (u32ConversionData - 0x42EA) / 50;   //refer to TRM for Temperature formula 

        printf("Temperature result: (%d) degree. AD Convernt Data: (%d)\n\n", i32Temp, u32ConversionData);      
    }    
}

/*---------------------------------------------------------------------------------------------------------*/
/* ADC interrupt callback function                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
void AdcIntCallback(uint32_t u32UserData)
{
    gu8AdcIntFlag = 1;
    DrvADC_DisableAdcInt();   // for single mode test
}

/*---------------------------------------------------------------------------------------------------------*/
/* ADC interrupt callback function                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
void Cmp0IntCallback(uint32_t u32UserData)
{
    gu8AdcCmp0IntFlag = 1;
    GPIOA->DOUT = 0x0;
}

/*---------------------------------------------------------------------------------------------------------*/
/* ADC interrupt callback function                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
void Cmp1IntCallback(uint32_t u32UserData)
{
    gu8AdcCmp1IntFlag = 1;
    GPIOA->DOUT = 0x1;
}

/*---------------------------------------------------------------------------------------------------------*/
/* SingleEndInput_ChannelSelect                                                                            */
/*---------------------------------------------------------------------------------------------------------*/
uint8_t SingleEndInput_ChannelSelect()
{
    uint8_t u8Option;
    
    printf("Select ADC channel:\n");
    printf("  [0] Channel 0\n");
    printf("  [1] Channel 1\n");
    printf("  [2] Channel 2\n");
    printf("  [3] Channel 3\n");
    printf("  [4] Channel 4\n");
    printf("  [5] Channel 5\n");
    printf("  [6] Channel 6\n");
    printf("  [7] Channel 7\n");
    printf("  Other keys: exit single mode test\n");
    u8Option = getchar();
   
    if(u8Option=='0')
        DrvADC_SetAdcChannel(eDRVADC_GPIO, eDRVADC_SINGLE_END_CH0_IN_N);
    else if(u8Option=='1')
        DrvADC_SetAdcChannel(eDRVADC_GPIO, eDRVADC_SINGLE_END_CH1_IN_N);
    else if(u8Option=='2')
        DrvADC_SetAdcChannel(eDRVADC_GPIO, eDRVADC_SINGLE_END_CH2_IN_N);
    else if(u8Option=='3')
        DrvADC_SetAdcChannel(eDRVADC_GPIO, eDRVADC_SINGLE_END_CH3_IN_N);
    else if(u8Option=='4')
        DrvADC_SetAdcChannel(eDRVADC_GPIO, eDRVADC_SINGLE_END_CH4_IN_N);
    else if(u8Option=='5')
        DrvADC_SetAdcChannel(eDRVADC_GPIO, eDRVADC_SINGLE_END_CH5_IN_N);
    else if(u8Option=='6')
        DrvADC_SetAdcChannel(eDRVADC_GPIO, eDRVADC_SINGLE_END_CH6_IN_N);
    else if(u8Option=='7')
        DrvADC_SetAdcChannel(eDRVADC_GPIO, eDRVADC_SINGLE_END_CH7_IN_N);
    else
        return 0xFF;

    u8Option = u8Option - '0';
    return u8Option;   // return the the active channel number 
}

/*---------------------------------------------------------------------------------------------------------*/
/* DifferentialInput_ChannelSelect                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
uint8_t DifferentialInput_ChannelSelect()
{
    uint8_t u8Option;
    
    printf("Select ADC channel:\n");
    printf("  [0] Differential input pair 0(CH0 and 1)\n");
    printf("  [1] Differential input pair 1(CH2 and 3)\n");
    printf("  [2] Differential input pair 2(CH4 and 5)\n");
    printf("  [3] Differential input pair 3(CH6 and 7)\n");
    printf("  Other keys: quit\n");
    u8Option = getchar();
    if(u8Option=='0')
    {
        DrvADC_SetAdcChannel(eDRVADC_GPIO, eDRVADC_DIFFERENTIAL_CH01);
    }
    else if(u8Option=='1')
    {
        DrvADC_SetAdcChannel(eDRVADC_GPIO, eDRVADC_DIFFERENTIAL_CH23);
    }
    else if(u8Option=='2')
    {
        DrvADC_SetAdcChannel(eDRVADC_GPIO, eDRVADC_DIFFERENTIAL_CH45);
    }
    else if(u8Option=='3')
    {
        DrvADC_SetAdcChannel(eDRVADC_GPIO, eDRVADC_DIFFERENTIAL_CH67);
    }
    else
        return 0xFF;
    return u8Option;
}

void NMITest(void)
{
    // execute will enter NMI interrupt routine
    SCB->ICSR = SCB->ICSR | 0x80000000;

    //UNLOCKREG();
    //SYSINT->NMISEL.NMISEL=0x0e;
    //LOCKREG();

}




/*---------------------------------------------------------------------------------------------------------*/
/* Main function                                                                                           */
/*---------------------------------------------------------------------------------------------------------*/
int main(void)
{
    uint8_t u8Option;
    
    InitialSystemClock();

    InitialUART();
    
    printf("\n\n");
    printf("----------------------------------------------------------------------------\n");
    printf(" This code tests the ADC function for ISD9160 Chip.\n");
    printf(" ADC Driver version: %x\n", DrvADC_GetVersion());    

    
    while(1)
    {       
        printf("\n\n\n");           
        printf("+----------------------------------------------------------------------+\n");       
        printf("|                            ADC Test code                             |\n");       
        printf("+----------------------------------------------------------------------+\n");       
        printf("|  [1] MIC To SPK Test                                                 |\n");       
        printf("|  [2] ADC Sound Quality Test                                          |\n");       
        printf("|  [3] ADC Compare Monitor Test                                        |\n");
        printf("|  [4] ADC Temperature Monitor Test                                    |\n");
        printf("|  [5] VMID Test                                                       |\n");
        printf("|  [6] MICBIAS Test                                                    |\n");
        printf("|  [7] ADC Single Mode Test                                            |\n");
        printf("|  [8] ADC Cycle Mode Test                                             |\n");
        printf("|  [9] NMI Test                                                        |\n");
        printf("|  [q] Quit                                                            |\n");
        printf("+----------------------------------------------------------------------+\n");
        printf("  Select the test number 1~9 or q:");
        u8Option = getchar();
        

        if(u8Option == '1')
        {
            AdcByPDMATest();    
        }
        else if(u8Option == '2')
        {
            AdcSoundQualityTest();
        }
        else if(u8Option == '3')
        {
            AdcCompMonitorTest();
        }
        else if(u8Option == '4')
        {
            AdcTemperatureMonitorTest();
        }
        else if(u8Option == '5')
        {
            VMIDTest();
        }
        else if(u8Option == '6')
        {
            MICBIASTest();
        }
        else if(u8Option == '7')
        {
            AdcSingleModeTest();    
        }   
        else if(u8Option == '8')
        {
            AdcCycleModeTest(); 
        }   
        else if(u8Option == '9')
        {
            NMITest();  
        }               
        else if( (u8Option == 'q') || (u8Option == 'Q') )
        {                                
            printf("\nADC sample code exit.\n");
            break;
        }
    }
        
    DrvPDMA_Close();
    DrvDPWM_Close();
    DrvADC_Close();
    DrvADC_AnaClose();
    return 0;
}
